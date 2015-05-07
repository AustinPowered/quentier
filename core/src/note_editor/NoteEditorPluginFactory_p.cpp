#include "NoteEditorPluginFactory_p.h"
#include "GenericResourceDisplayWidget.h"
#include <logging/QuteNoteLogger.h>
#include <client/types/Note.h>
#include <client/types/ResourceAdapter.h>
#include <QFileIconProvider>
#include <QDir>

namespace qute_note {

NoteEditorPluginFactoryPrivate::NoteEditorPluginFactoryPrivate(NoteEditorPluginFactory & factory,
                                                               QObject * parent) :
    QObject(parent),
    m_plugins(),
    m_lastFreePluginId(1),
    m_pCurrentNote(nullptr),
    m_fallbackResourceIcon(QIcon::fromTheme("unknown")),
    m_mimeDatabase(),
    m_resourceIconCache(),
    m_fileSuffixesCache(),
    q_ptr(&factory)
{}

NoteEditorPluginFactoryPrivate::~NoteEditorPluginFactoryPrivate()
{
    auto pluginsEnd = m_plugins.end();
    for(auto it = m_plugins.begin(); it != pluginsEnd; ++it)
    {
        INoteEditorPlugin * plugin = it.value();
        delete plugin;
    }
}

NoteEditorPluginFactoryPrivate::PluginIdentifier NoteEditorPluginFactoryPrivate::addPlugin(INoteEditorPlugin * plugin,
                                                                                           QString & errorDescription,
                                                                                           const bool forceOverrideTypeKeys)
{
    if (!plugin) {
        errorDescription = QT_TR_NOOP("Detected attempt to install null note editor plugin");
        QNWARNING(errorDescription);
        return 0;
    }

    auto pluginsEnd = m_plugins.end();
    for(auto it = m_plugins.begin(); it != pluginsEnd; ++it)
    {
        const INoteEditorPlugin * currentPlugin = it.value();
        if (plugin == currentPlugin) {
            errorDescription = QT_TR_NOOP("Detected attempt to install the same plugin instance more than once");
            QNWARNING(errorDescription);
            return 0;
        }
    }

    const QStringList & mimeTypes = plugin->mimeTypes();
    if (mimeTypes.isEmpty()) {
        errorDescription = QT_TR_NOOP("Can't install note editor plugin without specified mime types");
        QNWARNING(errorDescription);
        return 0;
    }

    if (!forceOverrideTypeKeys)
    {
        const int numMimeTypes = mimeTypes.size();

        for(auto it = m_plugins.begin(); it != pluginsEnd; ++it)
        {
            const INoteEditorPlugin * currentPlugin = it.value();
            const QStringList & currentPluginMimeTypes = currentPlugin->mimeTypes();

            for(int i = 0; i < numMimeTypes; ++i)
            {
                const QString & mimeType = mimeTypes[i];
                if (currentPluginMimeTypes.contains(mimeType)) {
                    errorDescription = QT_TR_NOOP("Can't install note editor plugin: found "
                                                  "conflicting mime type " + mimeType +
                                                  " from plugin " + currentPlugin->name());
                    QNINFO(errorDescription);
                    return 0;
                }
            }
        }
    }

    plugin->setParent(nullptr);
    m_plugins[m_lastFreePluginId] = plugin;
    return m_lastFreePluginId++;
}

bool NoteEditorPluginFactoryPrivate::removePlugin(const NoteEditorPluginFactoryPrivate::PluginIdentifier id,
                                                  QString & errorDescription)
{
    auto it = m_plugins.find(id);
    if (it == m_plugins.end()) {
        errorDescription = QT_TR_NOOP("Can't uninstall note editor plugin: plugin with id " +
                                      QString::number(id) + " was not found");
        QNDEBUG(errorDescription);
        return false;
    }

    INoteEditorPlugin * plugin = it.value();
    delete plugin;

    Q_UNUSED(m_plugins.erase(it));

    Q_Q(NoteEditorPluginFactory);
    q->refreshPlugins();

    return true;
}

bool NoteEditorPluginFactoryPrivate::hasPlugin(const NoteEditorPluginFactoryPrivate::PluginIdentifier id) const
{
    auto it = m_plugins.find(id);
    return (it != m_plugins.end());
}

void NoteEditorPluginFactoryPrivate::setNote(const Note & note)
{
    QNDEBUG("NoteEditorPluginFactoryPrivate::setNote: change current note to " << (note.hasTitle() ? note.title() : note.ToQString()));
    m_pCurrentNote = &note;
}

void NoteEditorPluginFactoryPrivate::setFallbackResourceIcon(const QIcon & icon)
{
    m_fallbackResourceIcon = icon;
}

QObject * NoteEditorPluginFactoryPrivate::create(const QString & mimeType, const QUrl & url,
                                                 const QStringList & argumentNames,
                                                 const QStringList & argumentValues) const
{
    QNDEBUG("NoteEditorPluginFactoryPrivate::create: mimeType = " << mimeType
            << ", url = " << url.toString() << ", argument names: " << argumentNames.join(", ")
            << ", argument values: " << argumentValues.join(", "));

    if (!m_pCurrentNote) {
        QNFATAL("Can't create note editor plugin for resource display: no note specified");
        return nullptr;
    }

    QList<ResourceAdapter> resourceAdapters = m_pCurrentNote->resourceAdapters();
    if (resourceAdapters.isEmpty()) {
        QNFATAL("Can't create note editor plugin for resource display: note has no resources");
        return nullptr;
    }

    int resourceHashIndex = argumentNames.indexOf("hash");
    if (resourceHashIndex < 0) {
        QNFATAL("Can't find resource hash index within argument names when trying to "
                "create note editor plugin for resource display");
        return nullptr;
    }

    QString resourceHash = argumentValues.at(resourceHashIndex);
    const ResourceAdapter * pCurrentResource = nullptr;
    const int numResources = resourceAdapters.size();
    for(int i = 0; i < numResources; ++i)
    {
        const ResourceAdapter & resource = resourceAdapters[i];
        if (!resource.hasDataHash()) {
            continue;
        }

        if (resource.dataHash() == resourceHash) {
            pCurrentResource = &resource;
            break;
        }
    }

    if (!pCurrentResource) {
        QNFATAL("Can't find resource in note by data hash: " << resourceHash
                << ", note: " << *m_pCurrentNote);
        return nullptr;
    }

    if (!m_plugins.isEmpty())
    {
        // Need to loop through installed plugins considering the last installed plugins first
        // Sadly, Qt doesn't support proper reverse iterators for its own containers without STL compatibility so will emulate them
        auto pluginsBegin = m_plugins.begin();
        auto pluginsLast = m_plugins.end();
        --pluginsLast;
        for(auto it = pluginsLast; it != pluginsBegin; --it)
        {
            const INoteEditorPlugin * plugin = it.value();

            const QStringList mimeTypes = plugin->mimeTypes();
            if (mimeTypes.contains(mimeType))
            {
                QNTRACE("Will use plugin " << plugin->name());
                INoteEditorPlugin * newPlugin = plugin->clone();
                QString errorDescription;
                bool res = newPlugin->initialize(mimeType, url, argumentNames, argumentValues,
                                                 *pCurrentResource, errorDescription);
                if (!res) {
                    QNINFO("Can't initialize note editor plugin " << plugin->name() << ": " << errorDescription);
                    delete newPlugin;
                    continue;
                }

                return newPlugin;
            }
        }
    }

    QNTRACE("Haven't found any installed plugin supporting mime type " << mimeType
            << ", will use generic resource display plugin for that");

    QString resourceDisplayName;
    if (pCurrentResource->hasResourceAttributes())
    {
        const qevercloud::ResourceAttributes & attributes = pCurrentResource->resourceAttributes();
        if (attributes.fileName.isSet()) {
            resourceDisplayName = attributes.fileName;
        }
        else if (attributes.sourceURL.isSet()) {
            resourceDisplayName = attributes.sourceURL;
        }
    }

    QString resourceDataSize;
    if (pCurrentResource->hasDataBody()) {
        const QByteArray & data = pCurrentResource->dataBody();
        int bytes = data.size();
        resourceDataSize = humanReadableSize(bytes);
    }
    else if (pCurrentResource->hasAlternateDataBody()) {
        const QByteArray & data = pCurrentResource->alternateDataBody();
        int bytes = data.size();
        resourceDataSize = humanReadableSize(bytes);
    }

    auto cachedIconIt = m_resourceIconCache.find(mimeType);
    if (cachedIconIt == m_resourceIconCache.end()) {
        QIcon resourceIcon = getIconForMimeType(mimeType);
        cachedIconIt = m_resourceIconCache.insert(mimeType, resourceIcon);
    }

    QStringList fileSuffixes;
    auto fileSuffixesIt = m_fileSuffixesCache.find(mimeType);
    if (fileSuffixesIt == m_fileSuffixesCache.end()) {
        fileSuffixes = getFileSuffixesForMimeType(mimeType);
        m_fileSuffixesCache[mimeType] = fileSuffixes;
    }

    // FIXME: need to convert the mime type to human readable form before passing it on to the generic resource display widget
    return new GenericResourceDisplayWidget(cachedIconIt.value(), resourceDisplayName,
                                            resourceDataSize, fileSuffixes, mimeType, *pCurrentResource);
}

QList<QWebPluginFactory::Plugin> NoteEditorPluginFactoryPrivate::plugins() const
{
    QList<QWebPluginFactory::Plugin> plugins;

    auto pluginsEnd = m_plugins.end();
    for(auto it = m_plugins.begin(); it != pluginsEnd; ++it)
    {
        const INoteEditorPlugin & currentPlugin = *(it.value());

        plugins.push_back(QWebPluginFactory::Plugin());
        auto & plugin = plugins.back();

        plugin.name = currentPlugin.name();
        plugin.description = currentPlugin.description();

        auto & mimeTypes = plugin.mimeTypes;
        const QStringList & currentPluginMimeTypes = currentPlugin.mimeTypes();
        const auto & currentPluginFileExtensions = currentPlugin.fileExtensions();

        const int numMimeTypes = currentPluginMimeTypes.size();
        for(int i = 0; i < numMimeTypes; ++i)
        {
            mimeTypes.push_back(QWebPluginFactory::MimeType());
            auto & mimeType = mimeTypes.back();

            mimeType.name = currentPluginMimeTypes[i];
            auto fileExtIter = currentPluginFileExtensions.find(mimeType.name);
            if (fileExtIter != currentPluginFileExtensions.end()) {
                mimeType.fileExtensions = fileExtIter.value();
            }
        }
    }

    return plugins;
}

QIcon NoteEditorPluginFactoryPrivate::getIconForMimeType(const QString & mimeTypeName) const
{
    QNDEBUG("NoteEditorPluginFactoryPrivate::getIconForMimeType: mime type name = " << mimeTypeName);

    QMimeType mimeType = m_mimeDatabase.mimeTypeForName(mimeTypeName);
    if (!mimeType.isValid()) {
        QNTRACE("Couldn't find valid mime type object for name/alias " << mimeTypeName
                << ", will use \"unknown\" icon");
        return m_fallbackResourceIcon;
    }

    QString iconName = mimeType.iconName();
    if (QIcon::hasThemeIcon(iconName)) {
        QNTRACE("Found icon from theme, name = " << iconName);
        return QIcon::fromTheme(iconName, m_fallbackResourceIcon);
    }

    iconName = mimeType.genericIconName();
    if (QIcon::hasThemeIcon(iconName)) {
        QNTRACE("Found generic icon from theme, name = " << iconName);
        return QIcon::fromTheme(iconName, m_fallbackResourceIcon);
    }

    QStringList suffixes;
    auto fileSuffixesIt = m_fileSuffixesCache.find(mimeTypeName);
    if (fileSuffixesIt == m_fileSuffixesCache.end()) {
        suffixes = getFileSuffixesForMimeType(mimeTypeName);
        m_fileSuffixesCache[mimeTypeName] = suffixes;
    }
    else {
        suffixes = fileSuffixesIt.value();
    }

    const int numSuffixes = suffixes.size();
    if (numSuffixes == 0) {
        QNDEBUG("Can't find any file suffix for mime type " << mimeTypeName
                << ", will use \"unknown\" icon");
        return m_fallbackResourceIcon;
    }

    bool hasNonEmptySuffix = false;
    for(int i = 0; i < numSuffixes; ++i)
    {
        const QString & suffix = suffixes[i];
        if (suffix.isEmpty()) {
            QNTRACE("Found empty file suffix within suffixes, skipping it");
            continue;
        }

        hasNonEmptySuffix = true;
        break;
    }

    if (!hasNonEmptySuffix) {
        QNDEBUG("All file suffixes for mime type " << mimeTypeName << " are empty, will use \"unknown\" icon");
        return m_fallbackResourceIcon;
    }

    QString fakeFilesStoragePath = applicationPersistentStoragePath();
    fakeFilesStoragePath.append("/fake_files");

    QDir fakeFilesDir(fakeFilesStoragePath);
    if (!fakeFilesDir.exists())
    {
        QNDEBUG("Fake files storage path doesn't exist yet, will attempt to create it");
        if (!fakeFilesDir.mkpath(fakeFilesStoragePath)) {
            QNWARNING("Can't create fake files storage path folder");
            return m_fallbackResourceIcon;
        }
    }

    QString filename("fake_file");
    QFileInfo fileInfo;
    for(int i = 0; i < numSuffixes; ++i)
    {
        const QString & suffix = suffixes[i];
        if (suffix.isEmpty()) {
            continue;
        }

        fileInfo.setFile(fakeFilesDir, filename + "." + suffix);
        if (fileInfo.exists() && !fileInfo.isFile())
        {
            if (!fakeFilesDir.rmpath(fakeFilesStoragePath + "/" + filename + "." + suffix)) {
                QNWARNING("Can't remove directory " << fileInfo.absolutePath()
                          << " which should not be here in the first place...");
                continue;
            }
        }

        if (!fileInfo.exists())
        {
            QFile fakeFile(fakeFilesStoragePath + "/" + filename + "." + suffix);
            if (!fakeFile.open(QIODevice::ReadWrite)) {
                QNWARNING("Can't open file " << fakeFilesStoragePath << "/" << filename
                          << "." << suffix << " for writing ");
                continue;
            }
        }

        QFileIconProvider fileIconProvider;
        QIcon icon = fileIconProvider.icon(fileInfo);
        if (icon.isNull()) {
            QNTRACE("File icon provider returned null icon for file with suffix " << suffix);
        }

        QNTRACE("Returning the icon from file icon provider for mime type " << mimeTypeName);
        return icon;
    }

    QNTRACE("Couldn't find appropriate icon from either icon theme or fake file with QFileIconProvider, "
            "using \"unknown\" icon as a last resort");
    return m_fallbackResourceIcon;
}

QStringList NoteEditorPluginFactoryPrivate::getFileSuffixesForMimeType(const QString & mimeTypeName) const
{
    QNDEBUG("NoteEditorPluginFactoryPrivate::getFileSuffixesForMimeType: mime type name = " << mimeTypeName);

    QMimeType mimeType = m_mimeDatabase.mimeTypeForName(mimeTypeName);
    if (!mimeType.isValid()) {
        QNTRACE("Couldn't find valid mime type object for name/alias " << mimeTypeName);
        return QStringList();
    }

    return mimeType.suffixes();
}

} // namespace qute_note
