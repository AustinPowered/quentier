#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_NOTE_EDITOR_PAGE_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_NOTE_EDITOR_PAGE_H

#include "JavaScriptInOrderExecutor.h"
#include <qute_note/utility/Qt4Helper.h>

#ifndef USE_QT_WEB_ENGINE
#include <QWebPage>
#else
#include <QWebEnginePage>
#endif

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditor)
QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)

class NoteEditorPage:
#ifndef USE_QT_WEB_ENGINE
        public QWebPage
#else
        public QWebEnginePage
#endif
{
    Q_OBJECT
public:
    typedef JavaScriptInOrderExecutor::Callback Callback;

public:
    explicit NoteEditorPage(NoteEditorPrivate & parent);
    virtual ~NoteEditorPage();

    bool javaScriptQueueEmpty() const;

    void setInactive();
    void setActive();

    /**
     * @brief stopJavaScriptAutoExecution method can be used to prevent the actual execution of JavaScript code immediately
     * on calling executeJavaScript; instead the code would be put on the queue for subsequent execution and the signal
     * javaScriptLoaded would only be emitted when the whole queue is executed
     */
    void stopJavaScriptAutoExecution();

    /**
     * @brief startJavaScriptAutoExecution is the counterpart of stopJavaScriptAutoExecution: when called on stopped
     * JavaScript queue it starts the execution of the code in the queue until it is empty; if the auto execution
     * was not stopped or the queue of JavaScript code is empty, calling this method has no effect
     */
    void startJavaScriptAutoExecution();

Q_SIGNALS:
    void javaScriptLoaded();
    void noteLoadCancelled();

public Q_SLOTS:
    bool shouldInterruptJavaScript();

    void executeJavaScript(const QString & script, Callback callback = 0, const bool clearPreviousQueue = false);

private Q_SLOTS:
    void onJavaScriptQueueEmpty();

private:
#ifndef USE_QT_WEB_ENGINE
    virtual void javaScriptAlert(QWebFrame * pFrame, const QString & message) Q_DECL_OVERRIDE;
    virtual bool javaScriptConfirm(QWebFrame * pFrame, const QString & message) Q_DECL_OVERRIDE;
    virtual void javaScriptConsoleMessage(const QString & message, int lineNumber, const QString & sourceID) Q_DECL_OVERRIDE;
#else
    virtual void javaScriptAlert(const QUrl & securityOrigin, const QString & msg) Q_DECL_OVERRIDE;
    virtual bool javaScriptConfirm(const QUrl & securityOrigin, const QString & msg) Q_DECL_OVERRIDE;
    virtual void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString & message,
                                          int lineNumber, const QString & sourceID) Q_DECL_OVERRIDE;
#endif

private:
    NoteEditorPrivate *         m_parent;
    JavaScriptInOrderExecutor * m_pJavaScriptInOrderExecutor;
    bool                        m_javaScriptAutoExecution;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_NOTE_EDITOR_PAGE_H
