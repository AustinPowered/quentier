#ifndef __QUTE_NOTE__CLIENT__TYPES__NOTEBOOK_H
#define __QUTE_NOTE__CLIENT__TYPES__NOTEBOOK_H

#include "IDataElementWithShortcut.h"
#include <QEverCloud.h>
#include <QSharedDataPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(ISharedNotebook)
QT_FORWARD_DECLARE_CLASS(SharedNotebookAdapter)
QT_FORWARD_DECLARE_CLASS(SharedNotebookWrapper)
QT_FORWARD_DECLARE_CLASS(IUser)
QT_FORWARD_DECLARE_CLASS(UserAdapter)
QT_FORWARD_DECLARE_CLASS(NotebookData)

class QUTE_NOTE_EXPORT Notebook: public IDataElementWithShortcut
{
public:
    QN_DECLARE_LOCAL_GUID
    QN_DECLARE_DIRTY
    QN_DECLARE_LOCAL
    QN_DECLARE_SHORTCUT

public:
    Notebook();
    Notebook(const Notebook & other);
    Notebook(Notebook && other);
    Notebook & operator=(const Notebook & other);
    Notebook & operator=(Notebook && other);

    Notebook(const qevercloud::Notebook & other);
    Notebook(qevercloud::Notebook && other);
    Notebook & operator=(const qevercloud::Notebook & other);
    Notebook & operator=(qevercloud::Notebook && other);

    virtual ~Notebook();

    bool operator==(const Notebook & other) const;
    bool operator!=(const Notebook & other) const;

    operator const qevercloud::Notebook&() const;
    operator qevercloud::Notebook&();

    virtual void clear() Q_DECL_OVERRIDE;

    virtual bool hasGuid() const Q_DECL_OVERRIDE;
    virtual const QString & guid() const Q_DECL_OVERRIDE;
    virtual void setGuid(const QString & guid) Q_DECL_OVERRIDE;

    virtual bool hasUpdateSequenceNumber() const Q_DECL_OVERRIDE;
    virtual qint32 updateSequenceNumber() const Q_DECL_OVERRIDE;
    virtual void setUpdateSequenceNumber(const qint32 usn) Q_DECL_OVERRIDE;

    virtual bool checkParameters(QString & errorDescription) const Q_DECL_OVERRIDE;

    bool hasName() const;
    const QString & name() const;
    void setName(const QString & name);

    bool isDefaultNotebook() const;
    void setDefaultNotebook(const bool defaultNotebook);

    bool hasLinkedNotebookGuid() const;
    const QString & linkedNotebookGuid() const;
    void setLinkedNotebookGuid(const QString & linkedNotebookGuid);

    bool hasCreationTimestamp() const;
    qint64 creationTimestamp() const;
    void setCreationTimestamp(const qint64 timestamp);

    bool hasModificationTimestamp() const;
    qint64 modificationTimestamp() const;
    void setModificationTimestamp(const qint64 timestamp);

    bool hasPublishingUri() const;
    const QString & publishingUri() const;
    void setPublishingUri(const QString & uri);

    bool hasPublishingOrder() const;
    qint8 publishingOrder() const;
    void setPublishingOrder(const qint8 order);

    bool hasPublishingAscending() const;
    bool isPublishingAscending() const;
    void setPublishingAscending(const bool ascending);

    bool hasPublishingPublicDescription() const;
    const QString & publishingPublicDescription() const;
    void setPublishingPublicDescription(const QString & publishingPublicDescription);

    bool hasPublished() const;
    bool isPublished() const;
    void setPublished(const bool published);

    bool hasStack() const;
    const QString & stack() const;
    void setStack(const QString & stack);

    bool hasSharedNotebooks();
    QList<SharedNotebookAdapter> sharedNotebooks() const;
    void setSharedNotebooks(QList<qevercloud::SharedNotebook> sharedNotebooks);
    void setSharedNotebooks(QList<SharedNotebookAdapter> && notebooks);
    void setSharedNotebooks(QList<SharedNotebookWrapper> && notebooks);
    void addSharedNotebook(const ISharedNotebook & sharedNotebook);
    void removeSharedNotebook(const ISharedNotebook & sharedNotebook);

    bool hasBusinessNotebookDescription() const;
    const QString & businessNotebookDescription() const;
    void setBusinessNotebookDescription(const QString & businessNotebookDescription);

    bool hasBusinessNotebookPrivilegeLevel() const;
    qint8 businessNotebookPrivilegeLevel() const;
    void setBusinessNotebookPrivilegeLevel(const qint8 privilegeLevel);

    bool hasBusinessNotebookRecommended() const;
    bool isBusinessNotebookRecommended() const;
    void setBusinessNotebookRecommended(const bool recommended);

    bool hasContact() const;
    const UserAdapter contact() const;
    void setContact(const IUser & contact);

    bool isLastUsed() const;
    void setLastUsed(const bool lastUsed);

    // Restrictions
    bool canReadNotes() const;
    void setCanReadNotes(const bool canReadNotes);

    bool canCreateNotes() const;
    void setCanCreateNotes(const bool canCreateNotes);

    bool canUpdateNotes() const;
    void setCanUpdateNotes(const bool canUpdateNotes);

    bool canExpungeNotes() const;
    void setCanExpungeNotes(const bool canExpungeNotes);

    bool canShareNotes() const;
    void setCanShareNotes(const bool canShareNotes);

    bool canEmailNotes() const;
    void setCanEmailNotes(const bool canEmailNotes);

    bool canSendMessageToRecipients() const;
    void setCanSendMessageToRecipients(const bool canSendMessageToRecipients);

    bool canUpdateNotebook() const;
    void setCanUpdateNotebook(const bool canUpdateNotebook);

    bool canExpungeNotebook() const;
    void setCanExpungeNotebook(const bool canExpungeNotebook);

    bool canSetDefaultNotebook() const;
    void setCanSetDefaultNotebook(const bool canSetDefaultNotebook);

    bool canSetNotebookStack() const;
    void setCanSetNotebookStack(const bool canSetNotebookStack);

    bool canPublishToPublic() const;
    void setCanPublishToPublic(const bool canPublishToPublic);

    bool canPublishToBusinessLibrary() const;
    void setCanPublishToBusinessLibrary(const bool canPublishToBusinessLibrary);

    bool canCreateTags() const;
    void setCanCreateTags(const bool canCreateTags);

    bool canUpdateTags() const;
    void setCanUpdateTags(const bool canUpdateTags);

    bool canExpungeTags() const;
    void setCanExpungeTags(const bool canExpungeTags);

    bool canSetParentTag() const;
    void setCanSetParentTag(const bool canSetParentTag);

    bool canCreateSharedNotebooks() const;
    void setCanCreateSharedNotebooks(const bool canCreateSharedNotebooks);

    bool hasUpdateWhichSharedNotebookRestrictions() const;
    qint8 updateWhichSharedNotebookRestrictions() const;
    void setUpdateWhichSharedNotebookRestrictions(const qint8 which);

    bool hasExpungeWhichSharedNotebookRestrictions() const;
    qint8 expungeWhichSharedNotebookRestrictions() const;
    void setExpungeWhichSharedNotebookRestrictions(const qint8 which);

    bool hasRestrictions() const;
    const qevercloud::NotebookRestrictions & restrictions() const;

private:
    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

    QSharedDataPointer<NotebookData> d;
};

} // namespace qute_note

Q_DECLARE_METATYPE(qute_note::Notebook)

#endif // __QUTE_NOTE__CLIENT__TYPES__NOTEBOOK_H