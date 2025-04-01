#include "filebrowser.h"

FileBrowser::FileBrowser(QObject* parent) : QObject(parent)
{
    m_currentFolder = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
}

QString FileBrowser::currentFolder() const
{
    return m_currentFolder;
}

void FileBrowser::setCurrentFolder(const QString& folder)
{
    if (m_currentFolder != folder) {
        m_currentFolder = folder;
        emit currentFolderChanged();
    }
}

QString FileBrowser::homeFolder() const
{
    return QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
}

QString FileBrowser::parentFolder() const
{
    QDir dir(m_currentFolder);
    if (dir.cdUp()) {
        return dir.absolutePath();
    }
    return m_currentFolder;
}

void FileBrowser::openFile(const QString& filePath)
{
    QUrl url = QUrl::fromLocalFile(filePath);
    if (!QDesktopServices::openUrl(url)) {
        qWarning() << "Failed to open file:" << filePath;
    }
}

bool FileBrowser::isDir(const QString& path) const
{
    return QFileInfo(path).isDir();
}