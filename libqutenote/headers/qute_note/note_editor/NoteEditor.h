#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_H

#include <qute_note/types/Note.h>
#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/utility/Linkage.h>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QUndoStack)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(INoteEditorBackend)

class QUTE_NOTE_EXPORT NoteEditor: public QWidget
{
    Q_OBJECT
public:
    explicit NoteEditor(QWidget * parent = Q_NULLPTR, Qt::WindowFlags flags = 0);
    virtual ~NoteEditor();

    void setBackend(INoteEditorBackend * backend);

    const QUndoStack * undoStack() const;
    void setUndoStack(QUndoStack * pUndoStack);

    void setNoteAndNotebook(const Note & note, const Notebook & notebook);

Q_SIGNALS:
    void contentChanged();
    void notifyError(QString error);

    void convertedToNote(Note note);
    void cantConvertToNote(QString error);

    void noteEditorHtmlUpdated(QString html);

    // Signals to notify anyone interested of the formatting at the current cursor position
    void textBoldState(bool state);
    void textItalicState(bool state);
    void textUnderlineState(bool state);
    void textStrikethroughState(bool state);
    void textAlignLeftState(bool state);
    void textAlignCenterState(bool state);
    void textAlignRightState(bool state);
    void textInsideOrderedListState(bool state);
    void textInsideUnorderedListState(bool state);
    void textInsideTableState(bool state);

    void textFontFamilyChanged(QString fontFamily);
    void textFontSizeChanged(int fontSize);

    void insertTableDialogRequested();

public Q_SLOTS:
    void convertToNote();
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void pasteUnformatted();
    void selectAll();

    void fontMenu();
    void textBold();
    void textItalic();
    void textUnderline();
    void textStrikethrough();
    void textHighlight();

    void alignLeft();
    void alignCenter();
    void alignRight();

    void insertToDoCheckbox();

    void setSpellcheck(const bool enabled);

    void setFont(const QFont & font);
    void setFontHeight(const int height);
    void setFontColor(const QColor & color);
    void setBackgroundColor(const QColor & color);

    void insertHorizontalLine();

    void increaseFontSize();
    void decreaseFontSize();

    void increaseIndentation();
    void decreaseIndentation();

    void insertBulletedList();
    void insertNumberedList();

    void insertTableDialog();
    void insertFixedWidthTable(const int rows, const int columns, const int widthInPixels);
    void insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth);

    void addAttachmentDialog();
    void saveAttachmentDialog(const QString & resourceHash);
    void saveAttachmentUnderCursor();
    void openAttachment(const QString & resourceHash);
    void openAttachmentUnderCursor();
    void copyAttachment(const QString & resourceHash);
    void copyAttachmentUnderCursor();

    void encryptSelectedTextDialog();
    void decryptEncryptedTextUnderCursor();

    void editHyperlinkDialog();
    void copyHyperlink();
    void removeHyperlink();

    void onNoteLoadCancelled();

private:
    INoteEditorBackend * m_backend;
};

} // namespace qute_note

#endif //__LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_H
