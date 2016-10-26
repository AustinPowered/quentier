/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NotebookModelTestHelper.h"
#include "../../models/NotebookModel.h"
#include "modeltest.h"
#include "Macros.h"
#include <quentier/utility/SysInfo.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/UidGenerator.h>
#include <quentier/exception/IQuentierException.h>

namespace quentier {

NotebookModelTestHelper::NotebookModelTestHelper(LocalStorageManagerThreadWorker * pLocalStorageManagerThreadWorker,
                                                 QObject * parent) :
    QObject(parent),
    m_pLocalStorageManagerThreadWorker(pLocalStorageManagerThreadWorker)
{
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookModelTestHelper,onAddNotebookFailed,Notebook,QNLocalizedString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookModelTestHelper,onUpdateNotebookFailed,Notebook,QNLocalizedString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,findNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookModelTestHelper,onFindNotebookFailed,Notebook,QNLocalizedString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,listNotebooksFailed,
                                                                LocalStorageManager::ListObjectsOptions,size_t,size_t,
                                                                LocalStorageManager::ListNotebooksOrder::type,
                                                                LocalStorageManager::OrderDirection::type,
                                                                QString,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookModelTestHelper,onListNotebooksFailed,LocalStorageManager::ListObjectsOptions,
                                  size_t,size_t,LocalStorageManager::ListNotebooksOrder::type,
                                  LocalStorageManager::OrderDirection::type,QString,QNLocalizedString,QUuid));
    QObject::connect(pLocalStorageManagerThreadWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookFailed,Notebook,QNLocalizedString,QUuid),
                     this, QNSLOT(NotebookModelTestHelper,onExpungeNotebookFailed,Notebook,QNLocalizedString,QUuid));
}

void NotebookModelTestHelper::test()
{
    QNDEBUG(QStringLiteral("NotebookModelTestHelper::test"));

    try {
        Notebook first;
        first.setName(QStringLiteral("First"));
        first.setLocal(true);
        first.setDirty(true);

        Notebook second;
        second.setName(QStringLiteral("Second"));
        second.setLocal(true);
        second.setDirty(false);
        second.setStack(QStringLiteral("Stack 1"));

        Notebook third;
        third.setName(QStringLiteral("Third"));
        third.setLocal(false);
        third.setGuid(UidGenerator::Generate());
        third.setDirty(true);
        third.setStack(QStringLiteral("Stack 1"));

        Notebook fourth;
        fourth.setName(QStringLiteral("Fourth"));
        fourth.setLocal(false);
        fourth.setGuid(UidGenerator::Generate());
        fourth.setDirty(false);
        fourth.setPublished(true);
        fourth.setStack(QStringLiteral("Stack 1"));

        Notebook fifth;
        fifth.setName(QStringLiteral("Fifth"));
        fifth.setLocal(false);
        fifth.setGuid(UidGenerator::Generate());
        fifth.setLinkedNotebookGuid(fifth.guid());
        fifth.setDirty(false);
        fifth.setStack(QStringLiteral("Stack 1"));

        Notebook sixth;
        sixth.setName(QStringLiteral("Sixth"));
        sixth.setLocal(false);
        sixth.setGuid(UidGenerator::Generate());
        sixth.setDirty(false);

        Notebook seventh;
        seventh.setName(QStringLiteral("Seventh"));
        seventh.setLocal(false);
        seventh.setGuid(UidGenerator::Generate());
        seventh.setDirty(false);
        seventh.setPublished(true);

        Notebook eighth;
        eighth.setName(QStringLiteral("Eighth"));
        eighth.setLocal(false);
        eighth.setGuid(UidGenerator::Generate());
        eighth.setLinkedNotebookGuid(eighth.guid());
        eighth.setDirty(false);

        Notebook nineth;
        nineth.setName(QStringLiteral("Nineth"));
        nineth.setLocal(true);
        nineth.setDirty(false);
        nineth.setStack(QStringLiteral("Stack 2"));

        Notebook tenth;
        tenth.setName(QStringLiteral("Tenth"));
        tenth.setLocal(false);
        tenth.setGuid(UidGenerator::Generate());
        tenth.setDirty(false);
        tenth.setStack(QStringLiteral("Stack 2"));

#define ADD_NOTEBOOK(notebook) \
        m_pLocalStorageManagerThreadWorker->onAddNotebookRequest(notebook, QUuid())

        // NOTE: exploiting the direct connection used in the current test environment:
        // after the following lines the local storage would be filled with the test objects
        ADD_NOTEBOOK(first);
        ADD_NOTEBOOK(second);
        ADD_NOTEBOOK(third);
        ADD_NOTEBOOK(fourth);
        ADD_NOTEBOOK(fifth);
        ADD_NOTEBOOK(sixth);
        ADD_NOTEBOOK(seventh);
        ADD_NOTEBOOK(eighth);
        ADD_NOTEBOOK(nineth);
        ADD_NOTEBOOK(tenth);

#undef ADD_NOTEBOOK

        NotebookCache cache(5);
        Account account(QStringLiteral("Default user"), Account::Type::Local);

        NotebookModel * model = new NotebookModel(account, *m_pLocalStorageManagerThreadWorker, cache, this);
        ModelTest t1(model);
        Q_UNUSED(t1)

        // Should not be able to change the dirty flag manually
        QModelIndex secondIndex = model->indexForLocalUid(second.localUid());
        if (!secondIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid notebook model item index for local uid"));
        }

        QModelIndex secondParentIndex = model->parent(secondIndex);
        secondIndex = model->index(secondIndex.row(), NotebookModel::Columns::Dirty, secondParentIndex);
        if (!secondIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid notebook model item index for dirty column"));
        }

        bool res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (res) {
            FAIL(QStringLiteral("Was able to change the dirty flag in the notebook model manually which is not intended"));
        }

        QVariant data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the notebook model while expected to get the state of dirty flag"));
        }

        if (data.toBool()) {
            FAIL(QStringLiteral("The dirty state appears to have changed after setData in notebook model even though the method returned false"));
        }

        // Should be able to make the non-synchronizable (local) item synchronizable (non-local)
        secondIndex = model->index(secondIndex.row(), NotebookModel::Columns::Synchronizable, secondParentIndex);
        if (!secondIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid notebook model item index for synchronizable column"));
        }

        res = model->setData(secondIndex, QVariant(true), Qt::EditRole);
        if (!res) {
            FAIL(QStringLiteral("Can't change the synchronizable flag from false to true for notebook model item"));
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the notebook model while expected to get the state of synchronizable flag"));
        }

        if (!data.toBool()) {
            FAIL(QStringLiteral("The synchronizable state appears to have not changed after setData in notebook model even though the method returned true"));
        }

        // Verify the dirty flag has changed as a result of making the item synchronizable
        secondIndex = model->index(secondIndex.row(), NotebookModel::Columns::Dirty, secondParentIndex);
        if (!secondIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid notebook model item index for dirty column"));
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the notebook model while expected to get the state of dirty flag"));
        }

        if (!data.toBool()) {
            FAIL(QStringLiteral("The dirty state hasn't changed after making the notebook model item synchronizable while it was expected to have changed"));
        }

        // Should not be able to make the synchronizable (non-local) item non-synchronizable (local)
        secondIndex = model->index(secondIndex.row(), NotebookModel::Columns::Synchronizable, secondParentIndex);
        if (!secondIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid notebook item model index for synchronizable column"));
        }

        res = model->setData(secondIndex, QVariant(false), Qt::EditRole);
        if (res) {
            FAIL(QStringLiteral("Was able to change the synchronizable flag in notebook model from true to false which is not intended"));
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the notebook model while expected to get the state of synchronizable flag"));
        }

        if (!data.toBool()) {
            FAIL(QStringLiteral("The synchronizable state appears to have changed after setData in notebook model even though the method returned false"));
        }

        // Should be able to change name
        secondIndex = model->index(secondIndex.row(), NotebookModel::Columns::Name, secondParentIndex);
        if (!secondIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid notebook model item index for name column"));
        }

        QString newName = QStringLiteral("Second (name modified)");
        res = model->setData(secondIndex, QVariant(newName), Qt::EditRole);
        if (!res) {
            FAIL(QStringLiteral("Can't change the name of the notebook model item"));
        }

        data = model->data(secondIndex, Qt::EditRole);
        if (data.isNull()) {
            FAIL(QStringLiteral("Null data was returned by the notebook model while expected to get the name of the tag item"));
        }

        if (data.toString() != newName) {
            FAIL(QStringLiteral("The name of the notebook item returned by the model does not match the name just set to this item: received ")
                 << data.toString() << QStringLiteral(", expected ") << newName);
        }

        // Should not be able to remove the row with a synchronizable (non-local) notebook
        res = model->removeRow(secondIndex.row(), secondParentIndex);
        if (res) {
            FAIL(QStringLiteral("Was able to remove the row with a synchronizable notebook which is not intended"));
        }

        QModelIndex secondIndexAfterFailedRemoval = model->indexForLocalUid(second.localUid());
        if (!secondIndexAfterFailedRemoval.isValid()) {
            FAIL(QStringLiteral("Can't get the valid notebook model item index after the failed row removal attempt"));
        }

        if (secondIndexAfterFailedRemoval.row() != secondIndex.row()) {
            FAIL(QStringLiteral("Notebook model returned item index with a different row after the failed row removal attempt"));
        }

        // Should be able to remove the row with a non-synchronizable (local) notebook
        QModelIndex firstIndex = model->indexForLocalUid(first.localUid());
        if (!firstIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid notebook model item index for local uid"));
        }

        QModelIndex firstParentIndex = model->parent(firstIndex);
        res = model->removeRow(firstIndex.row(), firstParentIndex);
        if (!res) {
            FAIL(QStringLiteral("Can't remove the row with a non-synchronizable notebook item from the model"));
        }

        QModelIndex firstIndexAfterRemoval = model->indexForLocalUid(first.localUid());
        if (firstIndexAfterRemoval.isValid()) {
            FAIL(QStringLiteral("Was able to get the valid model index for the removed notebook item by local uid which is not intended"));
        }

        // Should be able to move the non-stacked item to the existing stack
        QModelIndex sixthIndex = model->indexForLocalUid(sixth.localUid());
        if (!sixthIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid notebook model item index for local uid"));
        }

        QModelIndex sixthIndexMoved = model->moveToStack(sixthIndex, fifth.stack());
        if (!sixthIndexMoved.isValid()) {
            FAIL(QStringLiteral("Can't get the valid notebook model item index from the method intended to move the item to the stack"));
        }

        const NotebookModelItem * sixthItem = model->itemForIndex(sixthIndexMoved);
        if (!sixthItem) {
            FAIL(QStringLiteral("Can't get the notebook model item moved to the stack from its model index"));
        }

        if (sixthItem->type() != NotebookModelItem::Type::Notebook) {
            FAIL(QStringLiteral("Notebook model item has wrong type after being moved to the stack: ") << *sixthItem);
        }

        const NotebookItem * sixthNotebookItem = sixthItem->notebookItem();
        if (!sixthNotebookItem) {
            FAIL(QStringLiteral("Notebook model item has null pointer to the notebook item even though it's of notebook type"));
        }

        if (sixthNotebookItem->stack() != fifth.stack()) {
            FAIL(QStringLiteral("Notebook item's stack is not equal to the one expected as the notebook model item was moved to this stack: ")
                 << fifth.stack() << QStringLiteral("; notebook item: ") << *sixthNotebookItem);
        }

        const NotebookModelItem * sixthParentItem = sixthItem->parent();
        if (!sixthParentItem) {
            FAIL(QStringLiteral("The notebook model item has null parent after it has been moved to the existing stack"));
        }

        if (sixthParentItem->type() != NotebookModelItem::Type::Stack) {
            FAIL(QStringLiteral("The notebook model item has parent of non-stack type after it has been moved to the existing stack"));
        }

        const NotebookStackItem * sixthParentStackItem = sixthParentItem->notebookStackItem();
        if (!sixthParentStackItem) {
            FAIL(QStringLiteral("The notebook model item has parent of stack type but with null pointer to the stack item "
                                "after the model item has been moved to the existing stack"));
        }

        if (sixthParentStackItem->name() != fifth.stack()) {
            FAIL(QStringLiteral("The notebook model item has stack parent which name doesn't correspond to the expected one "
                                "after that item has been moved to the existing stack"));
        }

        // Should be able to move the non-stacked item to the new stack
        const QString newStack = QStringLiteral("My brand new stack");

        QModelIndex seventhIndex = model->indexForLocalUid(seventh.localUid());
        if (!seventhIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid notebook model item index from the method intended to move the item to the stack"));
        }

        QModelIndex seventhIndexMoved = model->moveToStack(seventhIndex, newStack);
        if (!seventhIndexMoved.isValid()) {
            FAIL(QStringLiteral("Can't get the valid notebook model item index for local uid"));
        }

        const NotebookModelItem * seventhItem = model->itemForIndex(seventhIndexMoved);
        if (!seventhItem) {
            FAIL(QStringLiteral("Can't get the notebook model item moved to the stack from its model index"));
        }

        if (seventhItem->type() != NotebookModelItem::Type::Notebook) {
            FAIL(QStringLiteral("Notebook model item has wrong type after being moved to the stack: ") << *seventhItem);
        }

        const NotebookItem * seventhNotebookItem = seventhItem->notebookItem();
        if (!seventhNotebookItem) {
            FAIL(QStringLiteral("Notebook model item has null pointer to the notebook item even though it's of notebook type"));
        }

        if (seventhNotebookItem->stack() != newStack) {
            FAIL(QStringLiteral("Notebook item's stack is not equal to the one expected as the notebook model item was moved to the new stack: ")
                 << newStack << QStringLiteral("; notebook item: ") << *seventhNotebookItem);
        }

        const NotebookModelItem * seventhParentItem = seventhItem->parent();
        if (!seventhParentItem) {
            FAIL(QStringLiteral("The notebook model item has null pointer after it has been moved to the new stack"));
        }

        if (seventhParentItem->type() != NotebookModelItem::Type::Stack) {
            FAIL(QStringLiteral("The notebook model item has parent of non-stack type after it has been moved to the existing stack"));
        }

        const NotebookStackItem * seventhParentStackItem = seventhParentItem->notebookStackItem();
        if (!seventhParentStackItem) {
            FAIL(QStringLiteral("The notebook model item has parent of stack type but with null pointer to the stack item "
                                "after the model item has been moved to the existing stack"));
        }

        if (seventhParentStackItem->name() != newStack) {
            FAIL(QStringLiteral("The notebook model item has stack parent which name doesn't correspond to the expected one "
                                "after that item has been moved to the existing stack"));
        }

        // Should be able to move items from one stack to the other one
        QModelIndex fourthIndex = model->indexForLocalUid(fourth.localUid());
        if (!fourthIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid notebook model item index for local uid"));
        }

        const NotebookModelItem * fourthItem = model->itemForIndex(fourthIndex);
        if (!fourthItem) {
            FAIL(QStringLiteral("Can't get the notebook model item for its index in turn retrieved from the local uid"));
        }

        const NotebookModelItem * fourthParentItem = fourthItem->parent();
        if (!fourthParentItem) {
            FAIL(QStringLiteral("Detected notebook model item having null parent"));
        }

        if (fourthParentItem->type() != NotebookModelItem::Type::Stack) {
            FAIL(QStringLiteral("Detected notebook model item which unexpectedly doesn't have a parent of stack type"));
        }

        const NotebookStackItem * fourthParentStackItem = fourthParentItem->notebookStackItem();
        if (!fourthParentStackItem) {
            FAIL(QStringLiteral("Detected notebook model item which parent of stack type has null pointer to the stack item"));
        }

        QModelIndex newFourthItemIndex = model->moveToStack(fourthIndex, newStack);
        if (!newFourthItemIndex.isValid()) {
            FAIL(QStringLiteral("Can't get the valid notebook model item index after the attempt to move the item to another stack"));
        }

        const NotebookModelItem * fourthItemFromNewIndex = model->itemForIndex(newFourthItemIndex);
        if (!fourthItemFromNewIndex) {
            FAIL(QStringLiteral("Can't get the notebook model item after moving it to another stack"));
        }

        if (fourthItem != fourthItemFromNewIndex) {
            FAIL(QStringLiteral("The memory address of the notebook model item appears to have changes as a result of moving the item into another stack"));
        }

        const NotebookModelItem * fourthItemNewParent = fourthItemFromNewIndex->parent();
        if (!fourthItemNewParent) {
            FAIL(QStringLiteral("Notebook model item's parent has become null as a result of moving the item to another stack"));
        }

        if (fourthItemNewParent->type() != NotebookModelItem::Type::Stack) {
            FAIL(QStringLiteral("Notebook model item's parent has non-stack type after moving the item to another stack"));
        }

        const NotebookStackItem * fourthItemNewStackItem = fourthItemNewParent->notebookStackItem();
        if (!fourthItemNewStackItem) {
            FAIL(QStringLiteral("Notebook model item's parent is of stack type but has null pointer to the stack item after "
                                "moving the original item to another stack"));
        }

        if (fourthItemNewStackItem->name() != newStack) {
            FAIL(QStringLiteral("Notebook model item's parent stack item has unexpected name after the attempt to move the item to another stack"));
        }

        int row = fourthParentItem->rowForChild(fourthItem);
        if (row >= 0) {
            FAIL(QStringLiteral("Notebook model item has been moved to another stack but it can still be found within the old parent item's children"));
        }

        row = fourthItemNewParent->rowForChild(fourthItemFromNewIndex);
        if (row < 0) {
            FAIL(QStringLiteral("Notebook model item has been moved to another stack but its row within its new parent cannot be found"));
        }

        // Should be able to remove the notebook item from the stack
        QModelIndex fourthIndexRemovedFromStack = model->removeFromStack(newFourthItemIndex);
        if (!fourthIndexRemovedFromStack.isValid()) {
            FAIL(QStringLiteral("Can't get the valid notebook model item index after the attempt to remove the item from the stack"));
        }

        const NotebookModelItem * fourthItemRemovedFromStack = model->itemForIndex(fourthIndexRemovedFromStack);
        if (!fourthItemRemovedFromStack) {
            FAIL(QStringLiteral("Can't get the pointer to the notebook model item from the index after the item's removal from the stack"));
        }

        if (fourthItemRemovedFromStack != fourthItemFromNewIndex) {
            FAIL(QStringLiteral("The memory address of the notebook model item appears to have changed as a result of removing the item from the stack"));
        }

        const NotebookModelItem * fourthItemNoStackParent = fourthItemRemovedFromStack->parent();
        if (!fourthItemNoStackParent) {
            FAIL(QStringLiteral("Notebook model item has null parent after being removed from the stack"));
        }

        if (fourthItemNoStackParent == fourthItemNewParent) {
            FAIL(QStringLiteral("The notebook model item's parent hasn't changed as a result of removing the item from the stack"));
        }

        if (fourthItemRemovedFromStack->type() != NotebookModelItem::Type::Notebook) {
            FAIL(QStringLiteral("The notebook model item's type has unexpectedly changed after the item's removal from the stack"));
        }

        const NotebookItem * fourthItemRemovedFromStackNotebookItem = fourthItemRemovedFromStack->notebookItem();
        if (!fourthItemRemovedFromStackNotebookItem) {
            FAIL(QStringLiteral("The notebook model item's notebook item is unexpectedly null after the item's removal from the stack"));
        }

        if (!fourthItemRemovedFromStackNotebookItem->stack().isEmpty()) {
            FAIL(QStringLiteral("The notebook item's stack is still not empty after the model item has been removed from the stack"));
        }

        // Check the sorting for notebook and stack items: by default should sort by name in ascending order
        res = checkSorting(*model, fourthItemNoStackParent);
        if (!res) {
            FAIL(QStringLiteral("Sorting check failed for the notebook model for ascending order"));
        }

        // Change the sort order and check the sorting again
        model->sort(NotebookModel::Columns::Name, Qt::DescendingOrder);

        res = checkSorting(*model, fourthItemNoStackParent);
        if (!res) {
            FAIL(QStringLiteral("Sorting check failed for the notebook model for descending order"));
        }

        emit success();
        return;
    }
    CATCH_EXCEPTION()

    emit failure();
}

void NotebookModelTestHelper::onAddNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NotebookModelTestHelper::onAddNotebookFailed: notebook = ") << notebook
            << QStringLiteral("\nError description = ") << errorDescription << QStringLiteral(", request id = ") << requestId);

    emit failure();
}

void NotebookModelTestHelper::onUpdateNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NotebookModelTestHelper::onUpdateNotebookFailed: notebook = ") << notebook
            << QStringLiteral("\nError description = ") << errorDescription << QStringLiteral(", request id = ") << requestId);

    emit failure();
}

void NotebookModelTestHelper::onFindNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NotebookModelTestHelper::onFindNotebookFailed: notebook = ") << notebook << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    emit failure();
}

void NotebookModelTestHelper::onListNotebooksFailed(LocalStorageManager::ListObjectsOptions flag, size_t limit, size_t offset,
                                                    LocalStorageManager::ListNotebooksOrder::type order,
                                                    LocalStorageManager::OrderDirection::type orderDirection,
                                                    QString linkedNotebookGuid, QNLocalizedString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NotebookModelTestHelper::onListNotebooksFailed: flag = ") << flag << QStringLiteral(", limit = ")
            << limit << QStringLiteral(", offset = ") << offset << QStringLiteral(", order = ") << order << QStringLiteral(", direction = ")
            << orderDirection << QStringLiteral(", linked notebook guid = ") << (linkedNotebookGuid.isNull() ? QStringLiteral("<null>") : linkedNotebookGuid)
            << QStringLiteral(", error description = ") << errorDescription << QStringLiteral(", request id = ") << requestId);

    emit failure();
}

void NotebookModelTestHelper::onExpungeNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NotebookModelTestHelper::onExpungeNotebookFailed: notebook = ") << notebook << QStringLiteral("\nError description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    emit failure();
}

bool NotebookModelTestHelper::checkSorting(const NotebookModel & model, const NotebookModelItem * rootItem) const
{
    if (!rootItem) {
        QNWARNING(QStringLiteral("Found null pointer to notebook model item when checking the sorting"));
        return false;
    }

    QList<const NotebookModelItem*> children = rootItem->children();
    if (children.isEmpty()) {
        return true;
    }

    QList<const NotebookModelItem*> sortedChildren = children;

    if (model.sortOrder() == Qt::AscendingOrder) {
        std::sort(sortedChildren.begin(), sortedChildren.end(), LessByName());
    }
    else {
        std::sort(sortedChildren.begin(), sortedChildren.end(), GreaterByName());
    }

    bool res = (children == sortedChildren);
    if (!res) {
        return false;
    }

    for(auto it = children.begin(), end = children.end(); it != end; ++it)
    {
        const NotebookModelItem * child = *it;
        res = checkSorting(model, child);
        if (!res) {
            return false;
        }
    }

    return true;
}

bool NotebookModelTestHelper::LessByName::operator()(const NotebookModelItem * lhs, const NotebookModelItem * rhs) const
{
    const QString & leftName = ((lhs->type() == NotebookModelItem::Type::Stack) ? lhs->notebookStackItem()->name() : lhs->notebookItem()->name());
    const QString & rightName = ((rhs->type() == NotebookModelItem::Type::Stack) ? rhs->notebookStackItem()->name() : rhs->notebookItem()->name());
    return (leftName.localeAwareCompare(rightName) <= 0);
}

bool NotebookModelTestHelper::GreaterByName::operator()(const NotebookModelItem * lhs, const NotebookModelItem * rhs) const
{
    const QString & leftName = ((lhs->type() == NotebookModelItem::Type::Stack) ? lhs->notebookStackItem()->name() : lhs->notebookItem()->name());
    const QString & rightName = ((rhs->type() == NotebookModelItem::Type::Stack) ? rhs->notebookStackItem()->name() : rhs->notebookItem()->name());
    return (leftName.localeAwareCompare(rightName) > 0);
}

} // namespace quentier
