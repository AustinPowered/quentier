#include "ResourceInfoJavaScriptHandler.h"
#include "../ResourceInfo.h"

namespace qute_note {

ResourceInfoJavaScriptHandler::ResourceInfoJavaScriptHandler(const ResourceInfo & resourceInfo,
                                                             QObject *parent) :
    QObject(parent),
    m_resourceInfo(resourceInfo)
{}

void ResourceInfoJavaScriptHandler::findResourceInfo(const QString & resourceHash)
{
    QString resourceDisplayName;
    QString resourceDisplaySize;
    QString resourceLocalFilePath;

    bool found = m_resourceInfo.findResourceInfo(resourceHash, resourceDisplayName,
                                                 resourceDisplaySize, resourceLocalFilePath);
    if (found) {
        emit notifyResourceInfo(resourceHash, resourceLocalFilePath, resourceDisplayName, resourceDisplaySize);
    }
}

} // namespace qute_note