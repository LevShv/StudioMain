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
    QString cleanPath = folder;

    // Удаляем qrc:/ если есть
    cleanPath = cleanPath.replace("qrc:/", "");

    // Заменяем слеши на бэкслеши для Windows
    cleanPath = QDir::toNativeSeparators(cleanPath);

    // Исправляем путь к диску (C/ ? C:/)
    if (cleanPath.length() >= 2 && cleanPath[1] != ':') {
        cleanPath = QString(cleanPath[0]) + ":" + cleanPath.mid(1); // Явное преобразование QChar в QString
    }

    QDir dir(cleanPath);
    if (!dir.exists()) {
        qWarning() << "Directory does not exist:" << cleanPath;
        emit errorOccurred(tr("Directory does not exist: %1").arg(cleanPath));
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
    if (dir.cdUp()) {
        QString parentPath = dir.canonicalPath();
        qDebug() << "Parent folder:" << parentPath;
        return parentPath;
    }
    qDebug() << "Already at root, returning:" << m_currentFolder;
    return m_currentFolder;
}

Q_INVOKABLE void FileBrowser::openFile(const QString& fileUrl)
{
    // Преобразуем URL в локальный путь
    QUrl url(fileUrl);
    QString localPath = url.toLocalFile();

    // Убедимся, что путь абсолютный
    QFileInfo fileInfo(localPath);
    if (!fileInfo.exists()) {
        qWarning() << "File does not exist:" << localPath;
        emit errorOccurred(tr("File does not exist: %1").arg(localPath));
        return;
    }

    // Открываем файл с помощью стандартного приложения
    if (!QDesktopServices::openUrl(url)) {
        qWarning() << "Failed to open file:" << localPath;
        emit errorOccurred(tr("Failed to open file: %1").arg(localPath));
    }
}

bool FileBrowser::isDir(const QString& path) const
{
    return QFileInfo(path).isDir();
}