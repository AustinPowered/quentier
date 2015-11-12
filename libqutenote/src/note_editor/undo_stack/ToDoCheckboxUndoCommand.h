#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__TO_DO_CHECKBOX_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__TO_DO_CHECKBOX_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"

namespace qute_note {

class ToDoCheckboxUndoCommand: public INoteEditorUndoCommand
{
public:
    ToDoCheckboxUndoCommand(NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent = Q_NULLPTR);
    ToDoCheckboxUndoCommand(NoteEditorPrivate & noteEditorPrivate, const QString & text,
                            QUndoCommand * parent = Q_NULLPTR);
    virtual ~ToDoCheckboxUndoCommand();

    virtual void redo() Q_DECL_OVERRIDE;
    virtual void undo() Q_DECL_OVERRIDE;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__TO_DO_CHECKBOX_UNDO_COMMAND_H