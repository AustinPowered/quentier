#include "NoteEditor_p.h"
#include "NoteEditorPage.h"
#include "NoteEditorPluginFactory.h"
#include "ResourceFileStorageLocationGetter.h"
#include <client/types/Note.h>
#include <client/types/Notebook.h>
#include <client/types/ResourceWrapper.h>
#include <client/enml/ENMLConverter.h>
#include <client/Utility.h>
#include <client/types/ResourceAdapter.h>
#include <logging/QuteNoteLogger.h>
#include <tools/FileIOThreadWorker.h>
#include <tools/QuteNoteCheckPtr.h>
#include <QWebFrame>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QByteArray>
#include <QImage>
#include <QDropEvent>
#include <QMimeType>
#include <QMimeData>
#include <QMimeDatabase>
#include <QThread>
#include <QXmlStreamReader>

namespace qute_note {

NoteEditorPrivate::NoteEditorPrivate(NoteEditor & noteEditor) :
    QObject(&noteEditor),
    m_jQuery(),
    m_resizableColumnsPlugin(),
    m_onFixedWidthTableResize(),
    m_getSelectionHtml(),
    m_replaceSelectionWithHtml(),
    m_pNote(nullptr),
    m_pNotebook(nullptr),
    m_modified(false),
    m_watchingForContentChange(false),
    m_contentChangedSinceWatchingStart(false),
    m_pageToNoteContentPostponeTimerId(0),
    m_encryptionManager(new EncryptionManager),
    m_decryptedTextCache(new QHash<QString, QPair<QString, bool> >()),
    m_enmlConverter(),
    m_pluginFactory(nullptr),
    m_pagePrefix("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">"
                 "<html><head>"
                 "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">"
                 "<title></title></head>"),
    m_htmlCachedMemory(),
    m_errorCachedMemory(),
    m_pIOThread(new QThread(this)),
    m_pFileIOThreadWorker(nullptr),
    m_resourceLocalFileInfoCache(),
    m_resourceLocalFileStorageFolder(),
    q_ptr(&noteEditor)
{
    Q_Q(NoteEditor);

    NoteEditorPage * page = new NoteEditorPage(*q);
    page->settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);
    page->settings()->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, true);
    page->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    page->setContentEditable(true);

    m_pluginFactory = new NoteEditorPluginFactory(page);
    page->setPluginFactory(m_pluginFactory);

    q->setPage(page);

    // Setting initial "blank" page, it is of great importance in order to make image insertion work
    q->setHtml(m_pagePrefix + "<body></body></html>");

    q->setAcceptDrops(true);

    __initNoteEditorResources();

    QFile file(":/javascript/jquery/jquery-2.1.3.min.js");
    file.open(QIODevice::ReadOnly);
    m_jQuery = file.readAll();
    file.close();

    file.setFileName(":/javascript/colResizable/colResizable-1.5.min.js");
    file.open(QIODevice::ReadOnly);
    m_resizableColumnsPlugin = file.readAll();
    file.close();

    file.setFileName(":/javascript/scripts/onFixedWidthTableResize.js");
    file.open(QIODevice::ReadOnly);
    m_onFixedWidthTableResize = file.readAll();
    file.close();

    file.setFileName(":/javascript/scripts/getSelectionHtml.js");
    file.open(QIODevice::ReadOnly);
    m_getSelectionHtml = file.readAll();
    file.close();

    file.setFileName(":/javascript/scripts/replaceSelectionWithHtml.js");
    file.open(QIODevice::ReadOnly);
    m_replaceSelectionWithHtml = file.readAll();
    file.close();

    QObject::connect(page, SIGNAL(contentsChanged()), q, SIGNAL(contentChanged()));
    QObject::connect(page, SIGNAL(contentsChanged()), this, SLOT(onContentChanged()));
    QObject::connect(q, SIGNAL(loadFinished(bool)), this, SLOT(onNoteLoadFinished(bool)));
    QObject::connect(this, SIGNAL(notifyError(QString)), q, SIGNAL(notifyError(QString)));

    m_pIOThread->start(QThread::LowPriority);
    m_pFileIOThreadWorker = new FileIOThreadWorker;
    m_pFileIOThreadWorker->moveToThread(m_pIOThread);
    QObject::connect(m_pIOThread, SIGNAL(finished()), m_pFileIOThreadWorker, SLOT(deleteLater()));

    m_resourceLocalFileStorageFolder = getResourceFileStorageLocation(q);
    if (m_resourceLocalFileStorageFolder.isEmpty()) {
        QNWARNING("Can't get resource file storage folder");
        // TODO: throw appropriate exception
    }

    QNTRACE("Resource local file storage folder: " << m_resourceLocalFileStorageFolder);

    // TODO: temporary thing for debugging/development, remove later
    onNoteLoadFinished(true);
}

NoteEditorPrivate::~NoteEditorPrivate()
{
    delete m_pNote;
    delete m_pNotebook;
}

void NoteEditorPrivate::onNoteLoadFinished(bool ok)
{
    if (!ok) {
        QNWARNING("Note page was not loaded successfully");
        return;
    }

    Q_Q(NoteEditor);
    QWebFrame * frame = q->page()->mainFrame();
    if (!frame) {
        return;
    }

    Q_UNUSED(frame->evaluateJavaScript(m_jQuery));
    Q_UNUSED(frame->evaluateJavaScript(m_resizableColumnsPlugin));
    Q_UNUSED(frame->evaluateJavaScript(m_onFixedWidthTableResize));
    QNTRACE("Evaluated all JavaScript helper functions");
}

void NoteEditorPrivate::onContentChanged()
{
    QNTRACE("NoteEditorPrivate::onContentChanged");
    m_modified = true;

    if (Q_LIKELY(m_watchingForContentChange)) {
        m_contentChangedSinceWatchingStart = true;
        return;
    }

    m_pageToNoteContentPostponeTimerId = startTimer(SEC_TO_MSEC(5));
    m_watchingForContentChange = true;
    m_contentChangedSinceWatchingStart = false;
    QNTRACE("Started timer to postpone note editor page's content to ENML conversion");
}

void NoteEditorPrivate::timerEvent(QTimerEvent * event)
{
    QNDEBUG("NoteEditorPrivate::timerEvent: " << (event ? QString::number(event->timerId()) : "<null>"));

    if (!event) {
        return;
    }

    if (event->timerId() == m_pageToNoteContentPostponeTimerId)
    {
        if (m_contentChangedSinceWatchingStart)
        {
            QNTRACE("Note editor page's content has been changed lately, "
                    "the editing is most likely in progress now, postponing "
                    "the conversion to ENML");
            m_contentChangedSinceWatchingStart = false;
            return;
        }

        QNTRACE("Looks like the note editing has stopped for a while, "
                "will convert the note editor page's content to ENML");
        bool res = htmlToNoteContent(m_errorCachedMemory);
        if (!res) {
            emit notifyError(m_errorCachedMemory);
        }

        killTimer(m_pageToNoteContentPostponeTimerId);
        m_pageToNoteContentPostponeTimerId = 0;

        m_watchingForContentChange = false;
        m_contentChangedSinceWatchingStart = false;
    }
}

void NoteEditorPrivate::clearContent()
{
    QNDEBUG("NoteEditorPrivate::clearContent");

    if (m_pageToNoteContentPostponeTimerId != 0) {
        killTimer(m_pageToNoteContentPostponeTimerId);
        m_pageToNoteContentPostponeTimerId = 0;
    }

    m_watchingForContentChange = false;
    m_contentChangedSinceWatchingStart = false;

    m_modified = false;

    Q_Q(NoteEditor);
    q->setHtml(m_pagePrefix + "<body></body></html>");
}

void NoteEditorPrivate::updateColResizableTableBindings()
{
    QNDEBUG("NoteEditorPrivate::updateColResizableTableBindings");

    Q_Q(NoteEditor);
    bool readOnly = !q->page()->isContentEditable();

    QString colResizable = "$(\"table\").colResizable({";
    if (readOnly) {
        colResizable += "disable:true});";
    }
    else {
        colResizable += "liveDrag:true, "
                        "gripInnerHtml:\"<div class=\\'grip\\'></div>\", "
                        "draggingClass:\"dragging\", "
                        "postbackSafe:true, "
                        "onResize:onFixedWidthTableResized"
                        "});";
    }

    QNTRACE("colResizable js code: " << colResizable);
    q->page()->mainFrame()->evaluateJavaScript(colResizable);
}

bool NoteEditorPrivate::htmlToNoteContent(QString & errorDescription)
{
    QNDEBUG("NoteEditorPrivate::htmlToNoteContent");

    if (!m_pNote) {
        errorDescription = QT_TR_NOOP("No note was set to note editor");
        return false;
    }

    if (m_pNote->hasActive() && !m_pNote->active()) {
        errorDescription = QT_TR_NOOP("Current note is marked as read-only, the changes won't be saved");
        QNWARNING(errorDescription << ", note: local guid = " << m_pNote->localGuid()
                  << ", guid = " << (m_pNote->hasGuid() ? m_pNote->guid() : "<null>")
                  << ", title = " << (m_pNote->hasTitle() ? m_pNote->title() : "<null>"));
        emit notifyError(errorDescription);
        return false;
    }

    if (m_pNotebook && m_pNotebook->hasRestrictions())
    {
        const qevercloud::NotebookRestrictions & restrictions = m_pNotebook->restrictions();
        if (restrictions.noUpdateNotes.isSet() && restrictions.noUpdateNotes.ref()) {
            errorDescription = QT_TR_NOOP("The notebook the current note belongs to doesn't allow notes modification, "
                                          "the changes won't be saved");
            QNWARNING(errorDescription << ", note: local guid = " << m_pNote->localGuid()
                      << ", guid = " << (m_pNote->hasGuid() ? m_pNote->guid() : "<null>")
                      << ", title = " << (m_pNote->hasTitle() ? m_pNote->title() : "<null>")
                      << ", notebook: local guid = " << m_pNotebook->localGuid() << ", guid = "
                      << (m_pNotebook->hasGuid() ? m_pNotebook->guid() : "<null>") << ", name = "
                      << (m_pNotebook->hasName() ? m_pNotebook->name() : "<null>"));
            emit notifyError(errorDescription);
            return false;
        }
    }

    Q_Q(const NoteEditor);
    m_htmlCachedMemory = q->page()->mainFrame()->toHtml();
    bool res = m_enmlConverter.htmlToNoteContent(m_htmlCachedMemory, *m_pNote, m_errorCachedMemory);
    if (!res) {
        errorDescription = QT_TR_NOOP("Can't convert note editor page's content to ENML: ") + errorDescription;
        QNWARNING(errorDescription)
        emit notifyError(errorDescription);
        return false;
    }

    return true;
}

void NoteEditorPrivate::checkResourceLocalFilesAndProvideSrcForImgResources(const QString & noteContentHtml)
{
    QNDEBUG("NoteEditorPrivate::checkResourceLocalFilesAndProvideSrcForImgResources");

    if (!m_pNote) {
        QNTRACE("No note is set for the editor");
        return;
    }

    QList<ResourceAdapter> resourceAdapters = m_pNote->resourceAdapters();
    if (resourceAdapters.isEmpty()) {
        QNTRACE("Note has no resources, nothing to do");
        return;
    }

    auto resourceAdaptersConstBegin = resourceAdapters.constBegin();
    auto resourceAdaptersConstEnd = resourceAdapters.constEnd();

    size_t numPendingResourceWritesToLocalFiles = 0;

    QXmlStreamReader reader(noteContentHtml);
    while(!reader.atEnd())
    {
        Q_UNUSED(reader.readNext());

        if (!reader.isStartDocument()) {
            continue;
        }

        QStringRef name = reader.name();
        if (name != "img") {
            continue;
        }

        QXmlStreamAttributes attributes = reader.attributes();
        if (!attributes.hasAttribute("en-tag")) {
            continue;
        }

        if (attributes.value("en-tag") != "en-media") {
            continue;
        }

        if (!attributes.hasAttribute("hash")) {
            continue;
        }

        QStringRef hash = attributes.value("hash");
        for(auto it = resourceAdaptersConstBegin; it != resourceAdaptersConstEnd; ++it)
        {
            const ResourceAdapter & resourceAdapter = *it;
            if (!resourceAdapter.hasDataHash()) {
                QNINFO("Detected resource without data hash: " << resourceAdapter);
                continue;
            }

            QString dataHash = QString::fromLocal8Bit(resourceAdapter.dataHash().constData(),
                                                      resourceAdapter.dataHash().size());
            if (dataHash != hash) {
                continue;
            }

            QNTRACE("Found current note's resource corresponding to the data hash "
                    << hash << ": " << resourceAdapter);

            const QString resourceLocalGuid = resourceAdapter.localGuid();

            // FIXME: consider moving this whole stuff into a separate thread,
            // then the separate thread purely for IO purposes would not be necessary
            if (!m_resourceLocalFileInfoCache.contains(resourceLocalGuid))
            {
                bool shouldWriteResourceToLocalFile = false;

                QString resourceLocalFilePath = m_resourceLocalFileStorageFolder + "/" + resourceLocalGuid;
                QFileInfo resourceLocalFileInfo(resourceLocalFilePath);
                if (resourceLocalFileInfo.exists())
                {
                    // Need to ensure the resource stored in the local file is actual
                    // i.e. its corresponding file with the hash has the same hash as the current resource object
                    QFileInfo resourceHashLocalFileInfo(resourceLocalFilePath + ".hash");
                    if (resourceHashLocalFileInfo.exists())
                    {
                        QFile resourceHashLocalFile(resourceLocalFilePath + ".hash");

                        // FIXME: synchronous IO; bad for GUI thread, even for small file
                        QByteArray resourceHashLocalFileData = resourceHashLocalFile.readAll();
                        if (resourceHashLocalFileData != resourceAdapter.dataHash())
                        {
                            QNTRACE("Found non-actual version of the resource stored in the local file: "
                                    "current data hash = " << hash << ", hash from the local file: "
                                    << resourceHashLocalFileData);
                            shouldWriteResourceToLocalFile = true;
                        }
                    }
                    else
                    {
                        QNTRACE("Local file with resource's data hash does not exist, "
                                "will write the resource to local file along with the supplementary .hash file");
                        shouldWriteResourceToLocalFile = true;
                    }
                }
                else
                {
                    QNTRACE("Haven't found the existing local file corresponding to the resource, "
                            "will write the current version to local file asynchronously");
                    shouldWriteResourceToLocalFile = true;
                }

                if (shouldWriteResourceToLocalFile)
                {
                    // TODO: schedule the async job to write the resource to local file, set up signal-slot connections
                    ++numPendingResourceWritesToLocalFiles;
                }
                else
                {
                    ResourceLocalFileInfo & info = m_resourceLocalFileInfoCache[resourceLocalFilePath];
                    info.m_resourceHash = hash.toString();
                    info.m_resourceLocalFilePath = resourceLocalFilePath;
                    QNTRACE("Cached resource with local guid " << resourceLocalGuid
                            << " stored stored in local file " << resourceLocalFilePath
                            << " to the cache for faster access");
                }
            }
        }
    }

    if (numPendingResourceWritesToLocalFiles != 0) {
        QNTRACE("Scheduled writing of " << numPendingResourceWritesToLocalFiles
                << " to local files, will wait until they are written "
                "and add the src attributes to img resources when the files are ready");
        return;
    }

    QNTRACE("All current note's resources are written to local files and are actual. "
            "Will set filepaths to these local files to src attributes of img resource tags");
    provideScrForImgResourcesFromCache();
}

void NoteEditorPrivate::provideScrForImgResourcesFromCache()
{
    QNDEBUG("NoteEditorPrivate::provideScrForImgResourcesFromCache");
    // TODO: implement
}

QVariant NoteEditorPrivate::execJavascriptCommandWithResult(const QString & command)
{
    Q_Q(NoteEditor);
    QWebFrame * frame = q->page()->mainFrame();
    QString javascript = QString("document.execCommand(\"%1\", false, null)").arg(command);
    QVariant result = frame->evaluateJavaScript(javascript);
    QNTRACE("Executed javascript command: " << javascript << ", result = " << result.toString());
    return result;
}

void NoteEditorPrivate::execJavascriptCommand(const QString & command)
{
    Q_UNUSED(execJavascriptCommandWithResult(command));
}

QVariant NoteEditorPrivate::execJavascriptCommandWithResult(const QString & command, const QString & args)
{
    Q_Q(NoteEditor);
    QWebFrame * frame = q->page()->mainFrame();
    QString javascript = QString("document.execCommand('%1', false, '%2')").arg(command).arg(args);
    QVariant result = frame->evaluateJavaScript(javascript);
    QNTRACE("Executed javascript command: " << javascript << ", result = " << result.toString());
    return result;
}

void NoteEditorPrivate::execJavascriptCommand(const QString & command, const QString & args)
{
    Q_UNUSED(execJavascriptCommandWithResult(command, args));
}

void NoteEditorPrivate::setNoteAndNotebook(const Note & note, const Notebook & notebook)
{
    QNDEBUG("NoteEditorPrivate::setNoteAndNotebook: note: local guid = " << note.localGuid()
            << ", guid = " << (note.hasGuid() ? note.guid() : "<null>") << ", title: "
            << (note.hasTitle() ? note.title() : "<null>") << "; notebook: local guid = "
            << notebook.localGuid() << ", guid = " << (notebook.hasGuid() ? notebook.guid() : "<null>")
            << ", name = " << (notebook.hasName() ? notebook.name() : "<null>"));

    // FIXME: if the editor wasn't empty, need to clean it carefully, ensure there'd be no dangling JavaScript functions for some gone elements

    if (!m_pNote) {
        m_pNote = new Note(note);
    }
    else {
        *m_pNote = note;
    }

    if (!m_pNotebook) {
        m_pNotebook = new Notebook(notebook);
    }
    else {
        *m_pNotebook = notebook;
    }

    if (!m_pNote->hasContent()) {
        QNDEBUG("Note without content was inserted into NoteEditor");
        clearContent();
        return;
    }

    m_htmlCachedMemory.resize(0);
    bool res = m_enmlConverter.noteContentToHtml(*m_pNote, m_htmlCachedMemory, m_errorCachedMemory,
                                                 m_decryptedTextCache, m_pluginFactory);
    if (!res)
    {
        QNWARNING("Can't convert note's content to HTML: " << m_errorCachedMemory);
        emit notifyError(m_errorCachedMemory);
        clearContent();
        return;
    }

    int bodyTagIndex = m_htmlCachedMemory.indexOf("<body>");
    if (bodyTagIndex < 0) {
        m_errorCachedMemory = QT_TR_NOOP("can't find <body> tag in the result of note to HTML conversion");
        QNWARNING(m_errorCachedMemory << ", note content: " << m_pNote->content()
                  << ", html: " << m_htmlCachedMemory);
        emit notifyError(m_errorCachedMemory);
        clearContent();
        return;
    }

    m_htmlCachedMemory.remove(0, bodyTagIndex);
    m_htmlCachedMemory.prepend(m_pagePrefix);
    int bodyClosingTagIndex = m_htmlCachedMemory.indexOf("</body>");
    if (bodyClosingTagIndex < 0) {
        m_errorCachedMemory = QT_TR_NOOP("Can't find </body> tag in the result of note to HTML conversion");
        QNWARNING(m_errorCachedMemory << ", note content: " << m_pNote->content()
                  << ", html: " << m_htmlCachedMemory);
        emit notifyError(m_errorCachedMemory);
        clearContent();
        return;
    }

    m_htmlCachedMemory.remove(bodyClosingTagIndex, 7);
    m_htmlCachedMemory.insert(bodyClosingTagIndex, "</html>");

    m_htmlCachedMemory.replace("<br></br>", "</br>");   // Webkit-specific fix

    Q_Q(NoteEditor);

    bool readOnly = false;
    if (m_pNote->hasActive() && !m_pNote->active()) {
        QNDEBUG("Current note is not active, setting it to read-only state");
        q->page()->setContentEditable(false);
        readOnly = true;
    }
    else if (m_pNotebook->hasRestrictions())
    {
        const qevercloud::NotebookRestrictions & restrictions = m_pNotebook->restrictions();
        if (restrictions.noUpdateNotes.isSet() && restrictions.noUpdateNotes.ref())
        {
            QNDEBUG("Notebook restrictions forbid the note modification, setting note's content to read-only state");
            q->page()->setContentEditable(false);
            readOnly = true;
        }
    }

    if (!readOnly) {
        QNDEBUG("Nothing prevents user to modify the note, allowing it in the editor");
        q->page()->setContentEditable(true);
    }

    q->setHtml(m_htmlCachedMemory);
    updateColResizableTableBindings();

    checkResourceLocalFilesAndProvideSrcForImgResources(m_htmlCachedMemory);
    QNTRACE("Done setting the current note and notebook");
}

const Note * NoteEditorPrivate::getNote()
{
    QNDEBUG("NoteEditorPrivate::getNote");

    if (!m_pNote) {
        QNTRACE("No note was set to the editor");
        return nullptr;
    }

    if (m_modified)
    {
        QNTRACE("Note editor's content was modified, converting into note");

        bool res = htmlToNoteContent(m_errorCachedMemory);
        if (!res) {
            return nullptr;
        }

        m_modified = false;
    }

    return m_pNote;
}

const Notebook * NoteEditorPrivate::getNotebook() const
{
    return m_pNotebook;
}

bool NoteEditorPrivate::isModified() const
{
    return m_modified;
}

const NoteEditorPluginFactory & NoteEditorPrivate::pluginFactory() const
{
    return *m_pluginFactory;
}

NoteEditorPluginFactory & NoteEditorPrivate::pluginFactory()
{
    return *m_pluginFactory;
}

void NoteEditorPrivate::onDropEvent(QDropEvent *pEvent)
{
    QNDEBUG("NoteEditorPrivate::onDropEvent");

    if (!pEvent) {
        QNINFO("Null drop event was detected");
        return;
    }

    const QMimeData * pMimeData = pEvent->mimeData();
    if (!pMimeData) {
        QNINFO("Null mime data from drop event was detected");
        return;
    }

    QList<QUrl> urls = pMimeData->urls();
    typedef QList<QUrl>::iterator Iter;
    Iter urlsEnd = urls.end();
    for(Iter it = urls.begin(); it != urlsEnd; ++it)
    {
        QString url = it->toString();
        if (url.toLower().startsWith("file://")) {
            url.remove(0,6);
            dropFile(url);
        }
    }
}

template <typename T>
QString NoteEditorPrivate::composeHtmlTable(const T width, const T singleColumnWidth,
                                            const int rows, const int columns,
                                            const bool relative)
{
    // Table header
    QString htmlTable = "<div><table style=\"border-collapse: collapse; margin-left: 0px; table-layout: fixed; width: ";
    htmlTable += QString::number(width);
    if (relative) {
        htmlTable += "%";
    }
    else {
        htmlTable += "px";
    }
    htmlTable += ";\" ><tbody>";

    for(int i = 0; i < rows; ++i)
    {
        // Row header
        htmlTable += "<tr>";

        for(int j = 0; j < columns; ++j)
        {
            // Column header
            htmlTable += "<td style=\"border: 1px solid rgb(219, 219, 219); padding: 10 px; margin: 0px; width: ";
            htmlTable += QString::number(singleColumnWidth);
            if (relative) {
                htmlTable += "%";
            }
            else {
                htmlTable += "px";
            }
            htmlTable += ";\">";

            // Blank line to preserve the size
            htmlTable += "<div><br></div>";

            // End column
            htmlTable += "</td>";
        }

        // End row
        htmlTable += "</tr>";
    }

    // End table
    htmlTable += "</tbody></table></div>";
    return htmlTable;
}

void NoteEditorPrivate::attachResourceToNote(const QByteArray & data, const QByteArray & dataHash,
                                             const QMimeType & mimeType, const QString & filename)
{
    QNDEBUG("NoteEditorPrivate::attachResourceToNote: hash = " << dataHash
            << ", mime type = " << mimeType.name());

    if (!m_pNote) {
        QNINFO("Can't attach resource to note editor: no actual note was selected");
        return;
    }

    ResourceWrapper resource;
    resource.setDataBody(data);
    resource.setDataHash(dataHash);
    resource.setDataSize(data.size());
    resource.setMime(mimeType.name());
    resource.setDirty(true);

    if (!filename.isEmpty()) {
        qevercloud::ResourceAttributes attributes;
        attributes.fileName = filename;
        resource.setResourceAttributes(attributes);
    }

    m_pNote->addResource(resource);
}

void NoteEditorPrivate::insertToDoCheckbox()
{
    QString html = ENMLConverter::getToDoCheckboxHtml(/* checked = */ false);
    execJavascriptCommand("insertHtml", html);
}

void NoteEditorPrivate::setFont(const QFont & font)
{
    QString fontName = font.family();
    execJavascriptCommand("fontName", fontName);
}

void NoteEditorPrivate::setFontHeight(const int height)
{
    if (height > 0) {
        execJavascriptCommand("fontSize", QString::number(height));
    }
    else {
        QString error = QT_TR_NOOP("Detected incorrect font size: " + QString::number(height));
        QNINFO(error);
        emit notifyError(error);
    }
}

void NoteEditorPrivate::setFontColor(const QColor & color)
{
    if (color.isValid()) {
        execJavascriptCommand("foreColor", color.name());
    }
    else {
        QString error = QT_TR_NOOP("Detected invalid font color: " + color.name());
        QNINFO(error);
        emit notifyError(error);
    }
}

void NoteEditorPrivate::setBackgroundColor(const QColor & color)
{
    if (color.isValid()) {
        execJavascriptCommand("hiliteColor", color.name());
    }
    else {
        QString error = QT_TR_NOOP("Detected invalid background color: " + color.name());
        QNINFO(error);
        emit notifyError(error);
    }
}

void NoteEditorPrivate::insertHorizontalLine()
{
    execJavascriptCommand("insertHorizontalRule");
}

void NoteEditorPrivate::changeIndentation(const bool increase)
{
    execJavascriptCommand((increase ? "indent" : "outdent"));
}

void NoteEditorPrivate::insertBulletedList()
{
    execJavascriptCommand("insertUnorderedList");
}

void NoteEditorPrivate::insertNumberedList()
{
    execJavascriptCommand("insertOrderedList");
}

#define CHECK_NUM_COLUMNS() \
    if (columns <= 0) { \
        QString error = QT_TR_NOOP("Detected attempt to insert table with bad number of columns: ") + QString::number(columns); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

#define CHECK_NUM_ROWS() \
    if (rows <= 0) { \
        QString error = QT_TR_NOOP("Detected attempt to insert table with bad number of rows: ") + QString::number(rows); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

void NoteEditorPrivate::insertFixedWidthTable(const int rows, const int columns, const int widthInPixels)
{
    CHECK_NUM_COLUMNS();
    CHECK_NUM_ROWS();

    Q_Q(NoteEditor);
    int pageWidth = q->geometry().width();
    if (widthInPixels > 2 * pageWidth) {
        QString error = QT_TR_NOOP("Can't insert table, width is too large (more than twice the page width): ") +
                        QString::number(widthInPixels);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }
    else if (widthInPixels <= 0) {
        QString error = QT_TR_NOOP("Can't insert table, bad width: ") + QString::number(widthInPixels);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    int singleColumnWidth = widthInPixels / columns;
    if (singleColumnWidth == 0) {
        QString error = QT_TR_NOOP("Can't insert table, bad width for specified number of columns "
                                   "(single column width is zero): width = ") +
                        QString::number(widthInPixels) + QT_TR_NOOP(", number of columns = ") +
                        QString::number(columns);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString htmlTable = composeHtmlTable(widthInPixels, singleColumnWidth, rows, columns,
                                         /* relative = */ false);
    execJavascriptCommand("insertHTML", htmlTable);
    updateColResizableTableBindings();
}

void NoteEditorPrivate::insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth)
{
    CHECK_NUM_COLUMNS();
    CHECK_NUM_ROWS();

    if (relativeWidth <= 0.01) {
        QString error = QT_TR_NOOP("Can't insert table, relative width is too small: ") +
                        QString::number(relativeWidth) + QString("%");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }
    else if (relativeWidth > 100.0 +1.0e-9) {
        QString error = QT_TR_NOOP("Can't insert table, relative width is too large: ") +
                        QString::number(relativeWidth) + QString("%");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    double singleColumnWidth = relativeWidth / columns;
    QString htmlTable = composeHtmlTable(relativeWidth, singleColumnWidth, rows, columns,
                                         /* relative = */ true);
    execJavascriptCommand("insertHTML", htmlTable);
    updateColResizableTableBindings();
}

void NoteEditorPrivate::encryptSelectedText(const QString & passphrase,
                                            const QString & hint)
{
    QNDEBUG("NoteEditorPrivate::encryptSelectedText");

    Q_Q(NoteEditor);
    if (!q->page()->hasSelection()) {
        QString error = QT_TR_NOOP("Note editor page has no selected text, nothing to encrypt");
        QNINFO(error);
        emit notifyError(error);
        return;
    }

    // NOTE: use JavaScript to get the selected text here, not the convenience method of NoteEditorPage
    // in order to ensure the method returning the selected html and the method replacing the selected html
    // agree about the selected html
    QString selectedHtml = execJavascriptCommandWithResult("getSelectionHtml").toString();
    if (selectedHtml.isEmpty()) {
        QString error = QT_TR_NOOP("Selected html is empty, nothing to encrypt");
        QNINFO(error);
        emit notifyError(error);
        return;
    }

    QString error;
    QString encryptedText;
    QString cipher = "AES";
    size_t keyLength = 128;
    bool res = m_encryptionManager->encrypt(selectedHtml, passphrase, cipher, keyLength,
                                            encryptedText, error);
    if (!res) {
        error.prepend(QT_TR_NOOP("Can't encrypt selected text: "));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString encryptedTextHtmlObject = "<object type=\"application/octet-stream\" en-tag=\"en-crypt\" >"
                                      "<param name=\"cipher\" value=\"";
    encryptedTextHtmlObject += cipher;
    encryptedTextHtmlObject += "\" />";
    encryptedTextHtmlObject += "<param name=\"length\" value=\"";
    encryptedTextHtmlObject += "\" />";
    encryptedTextHtmlObject += "<param name=\"encryptedText\" value=\"";
    encryptedTextHtmlObject += encryptedText;
    encryptedTextHtmlObject += "\" />";

    if (!hint.isEmpty()) {
        encryptedTextHtmlObject = "<param name=\"hint\" value=\"";
        encryptedTextHtmlObject += hint;    // FIXME: ensure there're no unescaped quotes and/or double-quotes here
        encryptedTextHtmlObject += "\" />";
    }

    encryptedTextHtmlObject += "<object/>";

    execJavascriptCommand("replaceSelectionWithHtml", encryptedTextHtmlObject);
    // TODO: ensure the contentChanged signal would be emitted automatically (guess it should)
}

void NoteEditorPrivate::dropFile(QString & filepath)
{
    QNDEBUG("NoteEditorPrivate::dropFile: " << filepath);

    QFileInfo fileInfo(filepath);
    if (!fileInfo.isFile()) {
        QNINFO("Detected attempt to drop something else rather than file: " << filepath);
        return;
    }

    if (!fileInfo.isReadable()) {
        QNINFO("Detected attempt to drop file which is not readable: " << filepath);
        return;
    }

    QMimeDatabase mimeDatabase;
    QMimeType mimeType = mimeDatabase.mimeTypeForFile(fileInfo);

    if (!mimeType.isValid()) {
        QNINFO("Detected invalid mime type for file " << filepath);
        return;
    }

    QByteArray data = QFile(filepath).readAll();
    QByteArray dataHash = QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
    attachResourceToNote(data, dataHash, mimeType, fileInfo.fileName());
    // TODO: schedule the async copying of the file to the folder with other resources stored as local files
    // TODO: re-convert note's ENML to html and "refresh" the web page
}

} // namespace qute_note

void __initNoteEditorResources()
{
    Q_INIT_RESOURCE(checkbox_icons);
    Q_INIT_RESOURCE(generic_resource_icons);
    Q_INIT_RESOURCE(jquery);
    Q_INIT_RESOURCE(colResizable);
    Q_INIT_RESOURCE(scripts);

    QNDEBUG("Initialized NoteEditor's resources");
}