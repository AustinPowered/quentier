#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_DECRYPT_UNDO_COMMAND_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_DECRYPT_UNDO_COMMAND_H

#include "EncryptDecryptUndoCommandInfo.h"
#include "INoteEditorUndoCommand.h"
#include "../NoteEditorPage.h"
#include <qute_note/note_editor/DecryptedTextManager.h>
#include <QSharedPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)

class DecryptUndoCommand: public INoteEditorUndoCommand
{
    typedef NoteEditorPage::Callback Callback;
public:
    DecryptUndoCommand(const EncryptDecryptUndoCommandInfo & info, const QSharedPointer<DecryptedTextManager> & decryptedTextManager,
                       NoteEditorPrivate & noteEditorPrivate, const Callback & callback, QUndoCommand * parent = Q_NULLPTR);
    DecryptUndoCommand(const EncryptDecryptUndoCommandInfo & info, const QSharedPointer<DecryptedTextManager> & decryptedTextManager,
                       NoteEditorPrivate & noteEditorPrivate, const Callback & callback, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~DecryptUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    EncryptDecryptUndoCommandInfo           m_info;
    QSharedPointer<DecryptedTextManager>    m_decryptedTextManager;
    Callback                                m_callback;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_DECRYPT_UNDO_COMMAND_H
