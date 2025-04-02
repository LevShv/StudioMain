#include "filebrowser.h"

FileBrowser::FileBrowser(QObject* parent) : QObject(parent)
{
    m_currentFolder = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
    qDebug() << "Initial folder:" << m_currentFolder;
}

QString FileBrowser::currentFolder() const
{
    return m_currentFolder;
}

void FileBrowser::setCurrentFolder(const QString& folder)
{
    QDir dir(folder);
    if (!dir.exists()) {
        emit errorOccurred(tr("Directory does not exist: %1").arg(folder));
        return;
    }

    QString canonicalPath = dir.canonicalPath();
    if (m_currentFolder != canonicalPath) {
        m_currentFolder = canonicalPath;
        qDebug() << "Folder changed to:" << m_currentFolder;
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
    return dir.cdUp() ? dir.canonicalPath() : m_currentFolder;
}

void FileBrowser::openFile(const QString& filePath)
{
    QUrl url = QUrl::fromLocalFile(filePath);
    if (!QDesktopServices::openUrl(url)) {
        emit errorOccurred(tr("Failed to open file: %1").arg(filePath));
    }
}

bool FileBrowser::isDir(const QString& path) const
{
    return QFileInfo(path).isDir();
}