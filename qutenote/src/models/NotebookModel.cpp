#include "NotebookModel.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

// Limit for the queries to the local storage
#define NOTEBOOK_LIST_LIMIT (40)

#define NOTEBOOK_CACHE_LIMIT (5)

#define NUM_NOTEBOOK_MODEL_COLUMNS (6)

NotebookModel::NotebookModel(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker,
                             QObject * parent) :
    QAbstractItemModel(parent),
    m_data(),
    m_fakeRootItem(Q_NULLPTR),
    m_defaultNotebookItem(Q_NULLPTR),
    m_modelItemsByLocalUid(),
    m_modelItemsByStack(),
    m_stackItems(),
    m_cache(),
    m_listNotebooksOffset(0),
    m_listNotebooksRequestId(),
    m_addNotebookRequestIds(),
    m_updateNotebookRequestIds(),
    m_expungeNotebookRequestIds(),
    m_findNotebookToRestoreFailedUpdateRequestIds(),
    m_findNotebookToPerformUpdateRequestIds(),
    m_sortedColumn(Columns::Name),
    m_sortOrder(Qt::AscendingOrder),
    m_lastNewNotebookNameCounter(0)
{
    createConnections(localStorageManagerThreadWorker);
    requestNotebooksList();
}

NotebookModel::~NotebookModel()
{}

const NotebookModelItem * NotebookModel::itemForIndex(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return m_fakeRootItem;
    }

    const NotebookModelItem * item = reinterpret_cast<const NotebookModelItem*>(index.internalPointer());
    if (item) {
        return item;
    }

    return m_fakeRootItem;
}

QModelIndex NotebookModel::indexForItem(const NotebookModelItem * item) const
{
    if (!item) {
        return QModelIndex();
    }

    if (item == m_fakeRootItem) {
        return QModelIndex();
    }

    const NotebookModelItem * parentItem = Q_NULLPTR;

    if ((item->type() == NotebookModelItem::Type::Notebook) && item->notebookItem())
    {
        QString stack = item->notebookItem()->stack();
        if (stack.isEmpty()) {
            // That means the actual parent is fake root item
            return QModelIndex();
        }

        auto it = m_modelItemsByStack.find(stack);
        if (Q_UNLIKELY(it == m_modelItemsByStack.end())) {
            QNDEBUG("Notebook " << item->notebookItem()->name() << " (local uid " << item->notebookItem()->localUid()
                    << ") has stack " << stack << " but the NotebookModelItem corresponding to it cannot be found");
            return QModelIndex();
        }

        parentItem = &(it.value());
    }
    else if ((item->type() == NotebookModelItem::Type::Stack) && item->notebookStackItem())
    {
        parentItem = m_fakeRootItem;
    }

    if (!parentItem) {
        return QModelIndex();
    }

    int row = parentItem->rowForChild(item);
    if (Q_UNLIKELY(row < 0)) {
        QString error = QT_TR_NOOP("Internal error: can't get the row of the child item in parent in TagModel");
        QNWARNING(error << ", child item: " << *item << "\nParent item: " << *parentItem);
        return QModelIndex();
    }

    // NOTE: as long as we stick to using the model index's internal pointer only inside the model, it's fine
    return createIndex(row, Columns::Name, const_cast<NotebookModelItem*>(item));
}

QModelIndex NotebookModel::indexForLocalUid(const QString & localUid) const
{
    auto it = m_modelItemsByLocalUid.find(localUid);
    if (it == m_modelItemsByLocalUid.end()) {
        return QModelIndex();
    }

    const NotebookModelItem * item = &(it.value());
    return indexForItem(item);
}

Qt::ItemFlags NotebookModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags indexFlags = QAbstractItemModel::flags(index);
    if (!index.isValid()) {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;

    if ((index.column() == Columns::Dirty) ||
        (index.column() == Columns::FromLinkedNotebook))
    {
        return indexFlags;
    }

    if (index.column() == Columns::Synchronizable)
    {
        const NotebookModelItem * item = itemForIndex(index);
        if (Q_UNLIKELY(!item)) {
            QNWARNING("Can't find notebook model item for a given index: row = " << index.row()
                      << ", column = " << index.column());
            return indexFlags;
        }

        if (item->type() == NotebookModelItem::Type::Stack)
        {
            const NotebookStackItem * stackItem = item->notebookStackItem();
            if (Q_UNLIKELY(!stackItem)) {
                QNWARNING("Internal inconsistency detected: notebook model item has stack type "
                          "but the pointer to the stack item is null");
                return indexFlags;
            }

            QList<const NotebookModelItem*> children = item->children();
            for(auto it = children.begin(), end = children.end(); it != end; ++it)
            {
                const NotebookModelItem * childItem = *it;
                if (Q_UNLIKELY(!childItem)) {
                    QNWARNING("Detected null pointer to notebook model item within the children of another notebook model item");
                    continue;
                }

                if (Q_UNLIKELY(childItem->type() == NotebookModelItem::Type::Stack)) {
                    QNWARNING("Detected nested notebook stack items which is unexpected and most probably incorrect");
                    continue;
                }

                const NotebookItem * notebookItem = childItem->notebookItem();
                if (Q_UNLIKELY(!notebookItem)) {
                    QNWARNING("Detected null pointer to notebook item in notebook model item having a type of notebook (not stack)");
                    continue;
                }

                if (notebookItem->isSynchronizable()) {
                    return indexFlags;
                }
            }
        }
        else
        {
            const NotebookItem * notebookItem = item->notebookItem();
            if (Q_UNLIKELY(!notebookItem)) {
                QNWARNING("Detected null pointer to notebook item within the notebook model");
                return indexFlags;
            }

            if (notebookItem->isSynchronizable()) {
                return indexFlags;
            }
        }
    }

    indexFlags |= Qt::ItemIsEditable;

    return indexFlags;
}

QVariant NotebookModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    int columnIndex = index.column();
    if ((columnIndex < 0) || (columnIndex >= NUM_NOTEBOOK_MODEL_COLUMNS)) {
        return QVariant();
    }

    const NotebookModelItem * item = itemForIndex(index);
    if (!item) {
        return QVariant();
    }

    if (item == m_fakeRootItem) {
        return QVariant();
    }

    Columns::type column;
    switch(columnIndex)
    {
    case Columns::Name:
        column = Columns::Name;
        break;
    case Columns::Synchronizable:
        column = Columns::Synchronizable;
        break;
    case Columns::Dirty:
        column = Columns::Dirty;
        break;
    case Columns::Default:
        column = Columns::Default;
        break;
    case Columns::Published:
        column = Columns::Published;
        break;
    case Columns::FromLinkedNotebook:
        column = Columns::FromLinkedNotebook;
        break;
    default:
        return QVariant();
    }

    switch(role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::ToolTipRole:
        return dataText(*item, column);
    case Qt::AccessibleTextRole:
    case Qt::AccessibleDescriptionRole:
        return dataAccessibleText(*item, column);
    default:
        return QVariant();
    }
}

QVariant NotebookModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation != Qt::Horizontal) {
        return QVariant();
    }

    switch(section)
    {
    case Columns::Name:
        return QVariant(tr("Name"));
    case Columns::Synchronizable:
        return QVariant(tr("Synchronizable"));
    case Columns::Dirty:
        return QVariant(tr("Dirty"));
    case Columns::Default:
        return QVariant(tr("Default"));
    case Columns::Published:
        return QVariant(tr("Published"));
    case Columns::FromLinkedNotebook:
        return QVariant(tr("From linked notebook"));
    default:
        return QVariant();
    }
}

int NotebookModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != Columns::Name)) {
        return 0;
    }

    const NotebookModelItem * parentItem = itemForIndex(parent);
    return (parentItem ? parentItem->numChildren() : 0);
}

int NotebookModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid() && (parent.column() != Columns::Name)) {
        return 0;
    }

    return NUM_NOTEBOOK_MODEL_COLUMNS;
}

QModelIndex NotebookModel::index(int row, int column, const QModelIndex & parent) const
{
    if (!m_fakeRootItem || (row < 0) || (column < 0) || (column >= NUM_NOTEBOOK_MODEL_COLUMNS) ||
        (parent.isValid() && (parent.column() != Columns::Name)))
    {
        return QModelIndex();
    }

    const NotebookModelItem * parentItem = itemForIndex(parent);
    if (!parentItem) {
        return QModelIndex();
    }

    const NotebookModelItem * item = parentItem->childAtRow(row);
    if (!item) {
        return QModelIndex();
    }

    // NOTE: as long as we stick to using the model index's internal pointer only inside the model, it's fine
    return createIndex(row, column, const_cast<NotebookModelItem*>(item));
}

QModelIndex NotebookModel::parent(const QModelIndex & index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    const NotebookModelItem * childItem = itemForIndex(index);
    if (!childItem) {
        return QModelIndex();
    }

    const NotebookModelItem * parentItem = childItem->parent();
    if (!parentItem) {
        return QModelIndex();
    }

    if (parentItem == m_fakeRootItem) {
        return QModelIndex();
    }

    const NotebookModelItem * grandParentItem = parentItem->parent();
    if (!grandParentItem) {
        return QModelIndex();
    }

    int row = grandParentItem->rowForChild(parentItem);
    if (Q_UNLIKELY(row < 0)) {
        QNWARNING("Internal inconsistency detected in NotebookModel: parent of the item can't find the item within its children: item = "
                  << *parentItem << "\nParent item: " << *grandParentItem);
        return QModelIndex();
    }

    // NOTE: as long as we stick to using the model index's internal pointer only inside the model, it's fine
    return createIndex(row, Columns::Name, const_cast<NotebookModelItem*>(parentItem));
}

bool NotebookModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant & value, int role)
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

bool NotebookModel::setData(const QModelIndex & modelIndex, const QVariant & value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }

    if (!modelIndex.isValid()) {
        return false;
    }

    if (modelIndex.column() == Columns::Dirty) {
        QNWARNING("The \"dirty\" flag can't be set manually in NotebookModel");
        return false;
    }

    if (modelIndex.column() == Columns::Published) {
        QNWARNING("The \"published\" flag can't be set manually in NotebookModel");
        return false;
    }

    if (modelIndex.column() == Columns::FromLinkedNotebook) {
        QNWARNING("The \"from linked notebook\" flag can't be set manually in NotebookModel");
        return false;
    }

    const NotebookModelItem * item = itemForIndex(modelIndex);
    if (!item) {
        return false;
    }

    if (item == m_fakeRootItem) {
        return false;
    }

    bool isNotebookItem = (item->type() == NotebookModelItem::Type::Notebook);

    if (Q_UNLIKELY(isNotebookItem && !item->notebookItem())) {
        QNWARNING("Internal inconsistency detected in NotebookModel: model item of notebook type "
                  "has a null pointer to notebook item");
        return false;
    }

    if (Q_UNLIKELY(!isNotebookItem && !item->notebookStackItem())) {
        QNWARNING("Internal inconsistency detected in NotebookModel: model item of stack type "
                  "has a null pointer to stack item");
        return false;
    }

    if (isNotebookItem)
    {
        const NotebookItem * notebookItem = item->notebookItem();

        if (!canUpdateNotebookItem(*notebookItem)) {
            QString error = QT_TR_NOOP("Can't update notebook \"" + notebookItem->name() +
                                       "\": restrictions on notebook update apply");
            QNINFO(error << ", notebook item: " << *notebookItem);
            emit notifyError(error);
            return false;
        }

        NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();
        auto notebookItemIt = localUidIndex.find(notebookItem->localUid());
        if (Q_UNLIKELY(notebookItemIt == localUidIndex.end())) {
            QString error = QT_TR_NOOP("Can't update notebook: can't find the notebook being updated in the model");
            QNWARNING(error << ", notebook item: " << *notebookItem);
            emit notifyError(error);
            return false;
        }

        NotebookItem notebookItemCopy = *notebookItem;
        bool dirty = notebookItemCopy.isDirty();

        switch(modelIndex.column())
        {
        case Columns::Name:
            {
                QString newName = value.toString();
                dirty |= (newName != notebookItemCopy.name());
                notebookItemCopy.setName(newName);
                break;
            }
        case Columns::Synchronizable:
            {
                if (notebookItemCopy.isSynchronizable() && value.toBool()) {
                    QString error = QT_TR_NOOP("Can't make already synchronizable notebook not synchronizable");
                    QNINFO(error << ", already synchronizable notebook item: " << notebookItemCopy);
                    emit notifyError(error);
                    return false;
                }

                dirty |= (value.toBool() != notebookItemCopy.isSynchronizable());
                notebookItemCopy.setSynchronizable(value.toBool());
                break;
            }
        case Columns::Default:
            {
                if (notebookItemCopy.isDefault() == value.toBool()) {
                    QNDEBUG("The default state of the notebook hasn't changed, nothing to do");
                    return true;
                }

                if (notebookItemCopy.isDefault() && !value.toBool()) {
                    QString error = QT_TR_NOOP("In order to stop notebook being the default one please choose another default notebook");
                    QNDEBUG(error);
                    emit notifyError(error);
                    return false;
                }

                notebookItemCopy.setDefault(true);
                dirty = true;

                if (m_defaultNotebookItem)
                {
                    auto previousDefaultItemIt = localUidIndex.find(m_defaultNotebookItem->localUid());
                    if (previousDefaultItemIt != localUidIndex.end())
                    {
                        NotebookItem previousDefaultItemCopy(*previousDefaultItemIt);
                        previousDefaultItemCopy.setDefault(false);
                        localUidIndex.replace(previousDefaultItemIt, previousDefaultItemCopy);
                        updateNotebookInLocalStorage(previousDefaultItemCopy);
                    }
                }

                m_defaultNotebookItem = &(*notebookItemIt);
                break;
            }
        default:
            return false;
        }

        localUidIndex.replace(notebookItemIt, notebookItemCopy);
        updateNotebookInLocalStorage(notebookItemCopy);
    }
    else
    {
        if (modelIndex.column() != Columns::Name) {
            QNWARNING("Detected attempt to change something rather than name for the notebook stack item, ignoring it");
            return false;
        }

        const NotebookStackItem * notebookStackItem = item->notebookStackItem();
        QString newStack = value.toString();
        QString previousStack = notebookStackItem->name();
        if (newStack == previousStack) {
            QNDEBUG("Notebook stack hasn't changed, nothing to do");
            return true;
        }

#define CHECK_ITEM(childItem) \
            if (Q_UNLIKELY(!childItem)) { \
                QNWARNING("Detected null pointer to notebook model item within the children of another item: item = " << *item); \
                continue; \
            } \
            \
            if (Q_UNLIKELY(childItem->type() == NotebookModelItem::Type::Stack)) { \
                QNWARNING("Internal inconsistency detected: found notebook stack item being a child of another notebook stack item: " \
                          "item = " << *item); \
                continue; \
            } \
            \
            const NotebookItem * notebookItem = childItem->notebookItem(); \
            if (Q_UNLIKELY(!notebookItem)) { \
                QNWARNING("Detected null pointer to notebook item within the children of notebook stack item: item = " << *item); \
                continue; \
            }

        QList<const NotebookModelItem*> children = item->children();
        for(auto it = children.begin(), end = children.end(); it != end; ++it)
        {
            const NotebookModelItem * childItem = *it;
            CHECK_ITEM(childItem)

            if (!canUpdateNotebookItem(*notebookItem)) {
                QString error = QT_TR_NOOP("Can't update notebook stack \"" + item->notebookStackItem()->name() +
                                           "\": restrictions on at least one of stacked notebooks' update apply: notebook \"" +
                                           notebookItem->name() + "\" is restricted from updating");
                QNINFO(error << ", notebook item for which the restrictions apply: " << *notebookItem);
                emit notifyError(error);
                return false;
            }
        }

        // Change the stack item
        auto stackModelItemIt = m_modelItemsByStack.find(previousStack);
        if (stackModelItemIt == m_modelItemsByStack.end()) {
            QString error = QT_TR_NOOP("Can't update notebook stack item: can't find the item for the previous stack value");
            QNWARNING(error << ", previous stack: \"" << previousStack << "\", new stack: \"" << newStack << "\"");
            emit notifyError(error);
            return false;
        }

        NotebookModelItem stackModelItemCopy(stackModelItemIt.value());
        if (Q_UNLIKELY(stackModelItemCopy.type() != NotebookModelItem::Type::Stack)) {
            QNWARNING("Internal inconsistency detected: non-stack item is kept within the hash of model items supposed to be stack items: "
                      "item: " << stackModelItemCopy);
            return false;
        }

        const NotebookStackItem * stackItem = stackModelItemCopy.notebookStackItem();
        if (Q_UNLIKELY(!stackItem)) {
            QNWARNING("Detected null pointer to notebook stack item in notebook model item wrapping it; item: " << stackModelItemCopy);
            return false;
        }

        NotebookStackItem newStackItem(*stackItem);
        newStackItem.setName(newStack);

        auto stackItemIt = m_stackItems.find(previousStack);
        if (stackItemIt != m_stackItems.end()) {
            Q_UNUSED(m_stackItems.erase(stackItemIt))
        }

        stackItemIt = m_stackItems.insert(newStack, newStackItem);
        stackModelItemCopy.setNotebookStackItem(&(*stackItemIt));

        Q_UNUSED(m_modelItemsByStack.erase(stackModelItemIt));
        stackModelItemIt = m_modelItemsByStack.insert(newStack, stackModelItemCopy);

        // Change all the child items
        NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

        // Refresh the list of children; shouldn't be necessary but just n case
        children = stackModelItemIt->children();

        for(auto it = children.begin(), end = children.end(); it != end; ++it)
        {
            const NotebookModelItem * childItem = *it;
            CHECK_ITEM(childItem)

            auto notebookItemIt = localUidIndex.find(notebookItem->localUid());
            if (notebookItemIt == localUidIndex.end()) {
                QNWARNING("Internal inconsistency detected: can't find the iterator for the notebook item for which the stack it being changed: "
                          "non-found notebook item: " << *notebookItem);
                continue;
            }

            NotebookItem notebookItemCopy(*notebookItem);
            notebookItemCopy.setStack(newStack);
            localUidIndex.replace(notebookItemIt, notebookItemCopy);
            updateNotebookInLocalStorage(notebookItemCopy);
        }

#undef CHECK_ITEM

    }

    return true;
}

bool NotebookModel::insertRows(int row, int count, const QModelIndex & parent)
{
    QNDEBUG("NotebookModel::insertRows: row = " << row << ", count = " << count
            << ", parent: is valid = " << (parent.isValid() ? "true" : "false")
            << ", row = " << parent.row() << ", column = " << parent.column());

    if (!m_fakeRootItem) {
        m_fakeRootItem = new NotebookModelItem;
    }

    const NotebookModelItem * parentItem = (parent.isValid()
                                            ? itemForIndex(parent)
                                            : m_fakeRootItem);
    if (!parentItem) {
        QNDEBUG("No model item for given model index");
        return false;
    }

    if (parentItem->type() == NotebookModelItem::Type::Notebook) {
        QNDEBUG("Can't insert notebook under another notebook, only under the notebook stack");
        return false;
    }

    const NotebookStackItem * stackItem = parentItem->notebookStackItem();
    if (Q_UNLIKELY(!stackItem)) {
        QNDEBUG("Detected null pointer to notebook stack item within the notebook model item of stack type: model item = "
                << *parentItem);
        return false;
    }

    NotebookDataByLocalUid & localUidIndex = m_data.get<ByLocalUid>();

    std::vector<NotebookDataByLocalUid::iterator> addedItems;
    addedItems.reserve(static_cast<size_t>(std::max(count, 0)));

    beginInsertRows(parent, row, row + count - 1);
    for(int i = 0; i < count; ++i)
    {
        // Adding notebook item
        NotebookItem item;
        item.setLocalUid(UidGenerator::Generate());
        Q_UNUSED(m_notebookItemsNotYetInLocalStorageUids.insert(item.localUid()))

        item.setName(nameForNewNotebook());
        item.setDirty(true);
        item.setStack(stackItem->name());

        auto insertionResult = localUidIndex.insert(item);
        addedItems.push_back(insertionResult.first);

        // Adding wrapping notebook model item
        NotebookModelItem modelItem(NotebookModelItem::Type::Notebook, &(*(addedItems.back())));
        auto modelItemInsertionResult = m_modelItemsByLocalUid.insert(item.localUid(), modelItem);
        modelItemInsertionResult.value().setParent(parentItem);
    }
    endInsertRows();

    for(auto it = addedItems.begin(), end = addedItems.end(); it != end; ++it) {
        const NotebookItem & item = *(*it);
        updateNotebookInLocalStorage(item);
    }

    // TODO: when sorting is implemented, don't forget to re-arrange the rows of inserted items as well

    return true;
}

bool NotebookModel::removeRows(int row, int count, const QModelIndex & parent)
{
    if (!m_fakeRootItem) {
        return false;
    }

    const NotebookModelItem * parentItem = (parent.isValid()
                                            ? itemForIndex(parent)
                                            : m_fakeRootItem);
    if (!parentItem) {
        return false;
    }

    if (Q_UNLIKELY((parentItem != m_fakeRootItem) && (parentItem->type() != NotebookModelItem::Type::Stack))) {
        QNDEBUG("Can't remove row(s) from parent item not being a stack item: " << *parentItem);
        return false;
    }

    for(int i = 0; i < count; ++i)
    {
        const NotebookModelItem * childItem = parentItem->childAtRow(row + i);
        if (!childItem) {
            QNWARNING("Detected null pointer to child notebook model item on attempt to remove row " << row + i
                      << " from parent item: " << *parentItem);
            continue;
        }

        bool isNotebookItem = (childItem->type() == NotebookModelItem::Type::Notebook);
        if (Q_UNLIKELY(isNotebookItem && !childItem->notebookItem())) {
            QNWARNING("Detected null pointer to notebook item in notebook model item being removed: row in parent = " << row + i
                      << ", parent item: " << *parentItem << "\nChild item: " << *childItem);
            continue;
        }

        if (Q_UNLIKELY(!isNotebookItem && !childItem->notebookStackItem())) {
            QNWARNING("Detected null pointer to notebook stack item in notebook model item being removed: row in parent = " << row + i
                      << ", parent item: " << *parentItem << "\nChild item: " << *childItem);
            continue;
        }

        QList<const NotebookItem*> notebookItems;
        if (isNotebookItem)
        {
            const NotebookItem * notebookItem = childItem->notebookItem();

#define CHECK_NOTEBOOK_ITEM(notebookItem) \
            if (notebookItem->isSynchronizable()) { \
                QString error = QT_TR_NOOP("One of notebooks being removed along with the stack containing it is synchronizable, it can't be removed"); \
                QNINFO(error << ", notebook: " << *notebookItem); \
                emit notifyError(error); \
                return false; \
            } \
            \
            if (notebookItem->isLinkedNotebook()) { \
                QString error = QT_TR_NOOP("One of notebooks being removed along with the stack containing it is the linked notebook from another account, " \
                                           "it can't be removed"); \
                QNINFO(error << ", notebook: " << *notebookItem); \
                emit notifyError(error); \
                return false; \
            }

            CHECK_NOTEBOOK_ITEM(notebookItem)
            notebookItems.push_back(notebookItem);
        }
        else
        {
            QList<const NotebookModelItem*> notebookModelItemsWithinStack = childItem->children();
            for(int j = 0, size = notebookModelItemsWithinStack.size(); j < size; ++j)
            {
                const NotebookModelItem * notebookModelItem = notebookModelItemsWithinStack[j];
                if (Q_UNLIKELY(!notebookModelItem)) {
                    QNWARNING("Detected null pointer to notebook model item within the children of the stack item being removed: " << *childItem);
                    continue;
                }

                if (Q_UNLIKELY(notebookModelItem->type() != NotebookModelItem::Type::Notebook)) {
                    QNWARNING("Detected notebook model item within the stack item which is not a notebook by type; stack item: " << *childItem
                              << "\nIts child with wrong type: " << *notebookModelItem);
                    continue;
                }

                const NotebookItem * notebookItem = notebookModelItem->notebookItem();
                if (Q_UNLIKELY(!notebookItem)) {
                    QNWARNING("Detected null pointer to notebook item in notebook model item being one of those removed "
                              "along with the stack item containing them; stack item: " << *childItem);
                    continue;
                }

                CHECK_NOTEBOOK_ITEM(notebookItem)
                notebookItems.push_back(notebookItem);
                Q_UNUSED(childItem->takeChild(j))
            }
        }

        for(int j = 0, size = notebookItems.size(); j < size; ++j)
        {
            // TODO: remove all the notebook items from both the local data members and then from the local storage
        }
    }

    return true;
}

void NotebookModel::onAddNotebookComplete(Notebook notebook, QUuid requestId)
{
    QNDEBUG("NotebookModel::onAddNotebookComplete: notebook = " << notebook << "\nRequest id = " << requestId);

    // TODO: implement
}

void NotebookModel::onAddNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NotebookModel::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
}

void NotebookModel::onUpdateNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NotebookModel::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
}

void NotebookModel::onFindNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NotebookModel::onListNotebooksComplete(LocalStorageManager::ListObjectsOptions flag,
                                            size_t limit, size_t offset,
                                            LocalStorageManager::ListNotebooksOrder::type order,
                                            LocalStorageManager::OrderDirection::type orderDirection,
                                            QString linkedNotebookGuid, QList<Notebook> foundNotebooks, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(linkedNotebookGuid)
    Q_UNUSED(foundNotebooks)
    Q_UNUSED(requestId)
}

void NotebookModel::onListNotebooksFailed(LocalStorageManager::ListObjectsOptions flag,
                                          size_t limit, size_t offset,
                                          LocalStorageManager::ListNotebooksOrder::type order,
                                          LocalStorageManager::OrderDirection::type orderDirection,
                                          QString linkedNotebookGuid, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(flag)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    Q_UNUSED(order)
    Q_UNUSED(orderDirection)
    Q_UNUSED(linkedNotebookGuid)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NotebookModel::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
}

void NotebookModel::onExpungeNotebookFailed(Notebook notebook, QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NotebookModel::createConnections(LocalStorageManagerThreadWorker & localStorageManagerThreadWorker)
{
    QNDEBUG("NotebookModel::createConnections");

    // Local signals to localStorageManagerThreadWorker's slots
    QObject::connect(this, QNSIGNAL(NotebookModel,addNotebook,Notebook,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookModel,updateNotebook,Notebook,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookModel,findNotebook,Notebook,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onFindNotebookRequest,Notebook,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookModel,listNotebooks,LocalStorageManager::ListObjectsOptions,
                                    size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                    LocalStorageManager::OrderDirection::type,QString,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onListNotebooksRequest,
                                                              LocalStorageManager::ListObjectsOptions,
                                                              size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                                              LocalStorageManager::OrderDirection::type,QString,QUuid));
    QObject::connect(this, QNSIGNAL(NotebookModel,expungeNotebook,Notebook,QUuid),
                     &localStorageManagerThreadWorker, QNSLOT(LocalStorageManagerThreadWorker,onExpungeNotebookRequest,Notebook,QUuid));

    // localStorageManagerThreadWorker's signals to local slots
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookModel,onAddNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(NotebookModel,onAddNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookModel,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(NotebookModel,onUpdateNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookModel,onFindNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(NotebookModel,onFindNotebookFailed,Notebook,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listNotebooksComplete,
                                                                LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                                LocalStorageManager::ListNotebooksOrder::type,
                                                                LocalStorageManager::OrderDirection::type,QString,
                                                                QList<Notebook>,QUuid),
                     this, QNSLOT(NotebookModel,onListNotebooksComplete,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                  LocalStorageManager::OrderDirection::type,QString,QList<Notebook>,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listNotebooksFailed,
                                                                LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                                LocalStorageManager::ListNotebooksOrder::type,
                                                                LocalStorageManager::OrderDirection::type,
                                                                QString,QString,QUuid),
                     this, QNSLOT(NotebookModel,onListNotebooksFailed,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                  LocalStorageManager::OrderDirection::type,QString,QString,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NotebookModel,onExpungeNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookFailed,Notebook,QString,QUuid),
                     this, QNSLOT(NotebookModel,onExpungeNotebookFailed,Notebook,QString,QUuid));
}

void NotebookModel::requestNotebooksList()
{
    QNDEBUG("NotebookModel::requestNotebooksList");

    // TODO: implement
}

QVariant NotebookModel::dataText(const NotebookModelItem & item, const Columns::type column) const
{
    // TODO: implement
    Q_UNUSED(item)
    Q_UNUSED(column)
    return QVariant();
}

QVariant NotebookModel::dataAccessibleText(const NotebookModelItem & item, const Columns::type column) const
{
    // TODO: implement
    Q_UNUSED(item)
    Q_UNUSED(column)
    return QVariant();
}

bool NotebookModel::canUpdateNotebookItem(const NotebookItem & item) const
{
    // TODO: implement
    Q_UNUSED(item)
    return true;
}

void NotebookModel::updateNotebookInLocalStorage(const NotebookItem & item)
{
    // TODO: implement
    Q_UNUSED(item)
}

QString NotebookModel::nameForNewNotebook() const
{
    // TODO: implement
    return QString();
}

bool NotebookModel::LessByName::operator()(const NotebookItem & lhs, const NotebookItem & rhs) const
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) <= 0);
}

#define ITEM_PTR_LESS(lhs, rhs) \
    if (!lhs) { \
        return true; \
    } \
    else if (!rhs) { \
        return false; \
    } \
    else { \
        return this->operator()(*lhs, *rhs); \
    }

bool NotebookModel::LessByName::operator()(const NotebookItem * lhs, const NotebookItem * rhs) const
{
    ITEM_PTR_LESS(lhs, rhs)
}

#define MODEL_ITEM_NAME(item, itemName) \
    if ((item.type() == NotebookModelItem::Type::Notebook) && item.notebookItem()) { \
        itemName = item.notebookItem()->nameUpper(); \
    } \
    else if ((item.type() == NotebookModelItem::Type::Stack) && item.notebookStackItem()) { \
        itemName = item.notebookStackItem()->name().toUpper(); \
    }

bool NotebookModel::LessByName::operator()(const NotebookModelItem & lhs, const NotebookModelItem & rhs) const
{
    QString lhsName;
    MODEL_ITEM_NAME(lhs, lhsName)

    QString rhsName;
    MODEL_ITEM_NAME(rhs, rhsName)

    return (lhsName.localeAwareCompare(rhsName) <= 0);
}

bool NotebookModel::LessByName::operator()(const NotebookModelItem * lhs, const NotebookModelItem * rhs) const
{
    ITEM_PTR_LESS(lhs, rhs)
}

bool NotebookModel::LessByName::operator()(const NotebookStackItem & lhs, const NotebookStackItem & rhs) const
{
    return (lhs.name().localeAwareCompare(rhs.name()) <= 0);
}

bool NotebookModel::LessByName::operator()(const NotebookStackItem * lhs, const NotebookStackItem * rhs) const
{
    ITEM_PTR_LESS(lhs, rhs)
}

bool NotebookModel::GreaterByName::operator()(const NotebookItem & lhs, const NotebookItem & rhs) const
{
    return (lhs.nameUpper().localeAwareCompare(rhs.nameUpper()) > 0);
}

#define ITEM_PTR_GREATER(lhs, rhs) \
    if (!lhs) { \
        return false; \
    } \
    else if (!rhs) { \
        return true; \
    } \
    else { \
        return this->operator()(*lhs, *rhs); \
    }

bool NotebookModel::GreaterByName::operator()(const NotebookItem * lhs, const NotebookItem * rhs) const
{
    ITEM_PTR_GREATER(lhs, rhs)
}

bool NotebookModel::GreaterByName::operator()(const NotebookStackItem & lhs, const NotebookStackItem & rhs) const
{
    return (lhs.name().localeAwareCompare(rhs.name()) > 0);
}

bool NotebookModel::GreaterByName::operator()(const NotebookStackItem * lhs, const NotebookStackItem * rhs) const
{
    ITEM_PTR_GREATER(lhs, rhs)
}

bool NotebookModel::GreaterByName::operator()(const NotebookModelItem & lhs, const NotebookModelItem & rhs) const
{
    QString lhsName;
    MODEL_ITEM_NAME(lhs, lhsName)

    QString rhsName;
    MODEL_ITEM_NAME(rhs, rhsName)

    return (lhsName.localeAwareCompare(rhsName) > 0);
}

bool NotebookModel::GreaterByName::operator()(const NotebookModelItem * lhs, const NotebookModelItem * rhs) const
{
    ITEM_PTR_GREATER(lhs, rhs)
}

} // namespace qute_note
