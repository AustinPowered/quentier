#ifndef QUENTIER_TESTS_MODEL_TEST_TAG_MODEL_TEST_HELPER_H
#define QUENTIER_TESTS_MODEL_TEST_TAG_MODEL_TEST_HELPER_H

#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(TagModel)
QT_FORWARD_DECLARE_CLASS(TagModelItem)

class TagModelTestHelper: public QObject
{
    Q_OBJECT
public:
    explicit TagModelTestHelper(LocalStorageManagerThreadWorker * pLocalStorageManagerThreadWorker,
                                QObject * parent = Q_NULLPTR);

Q_SIGNALS:
    void failure();
    void success();

public Q_SLOTS:
    void test();

private Q_SLOTS:
    void onAddTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId);
    void onUpdateTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId);
    void onFindTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId);
    void onListTagsFailed(LocalStorageManager::ListObjectsOptions flag,
                          size_t limit, size_t offset,
                          LocalStorageManager::ListTagsOrder::type order,
                          LocalStorageManager::OrderDirection::type orderDirection,
                          QString linkedNotebookGuid,
                          QNLocalizedString errorDescription, QUuid requestId);
    void onExpungeTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId);

private:
    bool checkSorting(const TagModel & model, const TagModelItem * rootItem) const;

    struct LessByName
    {
        bool operator()(const TagModelItem * lhs, const TagModelItem * rhs) const;
    };

    struct GreaterByName
    {
        bool operator()(const TagModelItem * lhs, const TagModelItem * rhs) const;
    };

private:
    LocalStorageManagerThreadWorker *   m_pLocalStorageManagerThreadWorker;
};

} // namespace quentier

#endif // QUENTIER_TESTS_MODEL_TEST_TAG_MODEL_TEST_HELPER_H
