#ifndef __QUTE_NOTE__MODELS__NOTE_MODEL_H
#define __QUTE_NOTE__MODELS__NOTE_MODEL_H

#include "NoteModelItem.h"
#include <qute_note/types/Note.h>
#include <qute_note/types/Notebook.h>
#include <qute_note/local_storage/LocalStorageManagerThreadWorker.h>
#include <qute_note/utility/LRUCache.hpp>
#include <QAbstractItemModel>
#include <QUuid>
#include <QSet>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/bimap.hpp>
#endif

namespace qute_note {

class NoteModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit NoteModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                       QObject * parent = Q_NULLPTR);
    virtual ~NoteModel();

    struct Columns
    {
        enum type {
            CreationTimestamp = 0,
            ModificationTimestamp,
            Title,
            PreviewText,
            ThumbnailImageFilePath,
            NotebookName,
            TagNameList,
            Size,
            Synchronizable,
            Dirty
        };
    };

    int sortingColumn() const { return m_sortedColumn; }
    Qt::SortOrder sortOrder() const { return m_sortOrder; }

    QModelIndex indexForLocalUid(const QString & localUid) const;

public:
    // QAbstractItemModel interface
    virtual Qt::ItemFlags flags(const QModelIndex & index) const Q_DECL_OVERRIDE;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
    virtual QModelIndex parent(const QModelIndex & index) const Q_DECL_OVERRIDE;

    virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant & value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    virtual bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex()) Q_DECL_OVERRIDE;
    virtual bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) Q_DECL_OVERRIDE;

    virtual void sort(int column, Qt::SortOrder order) Q_DECL_OVERRIDE;

Q_SIGNALS:
    void notifyError(QString errorDescription);

// private signals
    void addNote(Note note, Notebook notebook, QUuid requestId);
    void updateNote(Note note, Notebook notebook, QUuid requestId);
    void findNote(Note note, bool withResourceBinaryData, QUuid requestId);
    void listNotes(LocalStorageManager::ListObjectsOptions flag,
                   bool withResourceBinaryData, size_t limit, size_t offset,
                   LocalStorageManager::ListNotesOrder::type order,
                   LocalStorageManager::OrderDirection::type orderDirection,
                   QUuid requestId);
    void deleteNote(Note note, QUuid requestId);
    void expungeNote(Note note, QUuid requestId);
    void findNotebook(Notebook notebook, QUuid requestId);

private Q_SLOTS:
    // Slots for response to events from local storage
    void onAddNoteComplete(Note note, QUuid requestId);
    void onAddNoteFailed(Note note, QString errorDescription, QUuid requestId);
    void onUpdateNoteComplete(Note note, QUuid requestId);
    void onUpdateNoteFailed(Note note, QString errorDescription, QUuid requestId);
    void onFindNoteComplete(Note note, QUuid requestId);
    void onFindNoteFailed(Note note, QString errorDescription, QUuid requestId);
    void onListNotesComplete(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                             size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                             LocalStorageManager::OrderDirection::type orderDirection,
                             QList<Note> foundNotes, QUuid requestId);
    void onListNotesFailed(LocalStorageManager::ListObjectsOptions flag, bool withResourceBinaryData,
                           size_t limit, size_t offset, LocalStorageManager::ListNotesOrder::type order,
                           LocalStorageManager::OrderDirection::type orderDirection,
                           QString errorDescription, QUuid requestId);
    void onDeleteNoteComplete(Note note, QUuid requestId);
    void onDeleteNoteFailed(Note note, QString errorDescription, QUuid requestId);
    void onExpungeNoteComplete(Note note, QUuid requestId);
    void onExpungeNoteFailed(Note note, QString errorDescription, QUuid requestId);

    void onFindNotebookComplete(Notebook notebook, QUuid requestId);
    void onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId);

private:
    void createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker);
    void requestNoteList();

    QVariant dataText(const int row, const Columns::type column) const;
    QVariant dataAccessibleText(const int row, const Columns::type column) const;

    void removeItemByLocalUid(const QString & localUid);

    void updateItemRowWithRespectToSorting(const NoteModelItem & item);
    void updatePersistentModelIndices();

    void updateNoteInLocalStorage(const NoteModelItem & item);

    bool canUpdateNoteItem(const NoteModelItem & item) const;
    bool canCreateNoteItem(const QString & notebookLocalUid) const;

    void onNoteAddedOrUpdated(const Note & note);

private:
    struct ByLocalUid{};
    struct ByIndex{};

    typedef boost::multi_index_container<
        NoteModelItem,
        boost::multi_index::indexed_by<
            boost::multi_index::random_access<
                boost::multi_index::tag<ByIndex>
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<ByLocalUid>,
                boost::multi_index::const_mem_fun<NoteModelItem,const QString&,&NoteModelItem::localUid>
            >
        >
    > NoteData;

    typedef NoteData::index<ByLocalUid>::type NoteDataByLocalUid;
    typedef NoteData::index<ByIndex>::type NoteDataByIndex;

    typedef LRUCache<QString, Note>     Cache;

    class NoteComparator
    {
    public:
        NoteComparator(const Columns::type column,
                       const Qt::SortOrder sortOrder) :
            m_sortedColumn(column),
            m_sortOrder(sortOrder)
        {}

        bool operator()(const NoteModelItem & lhs, const NoteModelItem & rhs) const;

    private:
        Columns::type   m_sortedColumn;
        Qt::SortOrder   m_sortOrder;
    };

private:
    NoteData                m_data;
    size_t                  m_listNotesOffset;
    QUuid                   m_listNotesRequestId;
    QSet<QUuid>             m_noteItemsNotYetInLocalStorageUids;

    Cache                   m_cache;

    QSet<QUuid>             m_addNoteRequestIds;
    QSet<QUuid>             m_updateNoteRequestIds;
    QSet<QUuid>             m_deleteNoteRequestIds;
    QSet<QUuid>             m_expungeNoteRequestIds;

    QSet<QUuid>             m_findNoteToRestoreFailedUpdateRequestIds;
    QSet<QUuid>             m_findNoteToPerformUpdateRequestIds;

    Columns::type           m_sortedColumn;
    Qt::SortOrder           m_sortOrder;

    struct Restrictions
    {
        Restrictions() :
            m_canCreateNotes(false),
            m_canUpdateNotes(false)
        {}

        bool    m_canCreateNotes;
        bool    m_canUpdateNotes;
    };

    QHash<QString, Restrictions>    m_noteRestrictionsByNotebookLocalUid;

    typedef boost::bimap<QString, QUuid> NotebookLocalUidWithFindNotebookRequestIdBimap;
    NotebookLocalUidWithFindNotebookRequestIdBimap  m_findNotebookRequestForNotebookLocalUid;
};

} // namespace qute_note

#endif // __QUTE_NOTE__MODELS__NOTE_MODEL_H