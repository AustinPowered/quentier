/*
 * Copyright 2017 Dmitry Ivanov
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

#ifndef QUENTIER_DELEGATES_DELETED_NOTE_ITEM_DELEGATE_H
#define QUENTIER_DELEGATES_DELETED_NOTE_ITEM_DELEGATE_H

#include "AbstractStyledItemDelegate.h"

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NoteModelItem)

class DeletedNoteItemDelegate: public AbstractStyledItemDelegate
{
    Q_OBJECT
public:
    explicit DeletedNoteItemDelegate(QObject * parent = Q_NULLPTR);

    /**
     * @brief returns null pointer as DeletedNoteItemDelegate doesn't allow editing
     */
    virtual QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option,
                                   const QModelIndex & index) const Q_DECL_OVERRIDE;

    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option,
                       const QModelIndex & index) const Q_DECL_OVERRIDE;

    /**
     * @brief does nothing
     */
    virtual void setEditorData(QWidget * editor, const QModelIndex & index) const Q_DECL_OVERRIDE;

    /**
     * @brief does nothing
     */
    virtual void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const Q_DECL_OVERRIDE;

    virtual QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const Q_DECL_OVERRIDE;

    /**
     * @brief does nothing
     */
    virtual void updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option, const QModelIndex & index) const Q_DECL_OVERRIDE;

private:
    void doPaint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    void drawDeletedNoteTitleOrPreviewText(QPainter * painter, const QStyleOptionViewItem & option,
                                           const NoteModelItem & item) const;
    void drawDeletionDateTime(QPainter * painter, const QStyleOptionViewItem & option,
                              const NoteModelItem & item) const;

    QSize doSizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;

private:
    QString     m_deletionDateTimeReplacementText;
};

} // namespace quentier

#endif // QUENTIER_DELEGATES_DELETED_NOTE_ITEM_DELEGATE_H
