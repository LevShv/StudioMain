#include "filebrowser.h"
#include "log.h"


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
    QDir dir(NormalizePath(folder));

    QString canonicalPath = dir.canonicalPath();
    if (m_currentFolder != canonicalPath) {
        m_currentFolder = canonicalPath;
        qDebug() << "Folder changed to:" << m_currentFolder;
        emit currentFolderChanged();
    }
}

QString FileBrowser::homeFolder() const
{
    QString Homefolder = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
    LOG_INFO("Home folder " + Homefolder.toStdString());
    return Homefolder;
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

Q_INVOKABLE void FileBrowser::openFile(const QString& filepath)
{
    QString cleanPath = NormalizePath(filepath);

    // Создаем URL - важно указать схему "file://"
    QUrl url;
    if (cleanPath.startsWith("/")) {
        url = QUrl::fromLocalFile(cleanPath);
    }
    else {
        url = QUrl(cleanPath);
    }

    if (!url.isValid()) {
        qWarning() << "Неверный URL:" << cleanPath;
        emit errorOccurred(tr("Wrong way"));
        return;
    }

    QString localPath = url.toLocalFile();
    if (localPath.isEmpty()) {
        localPath = cleanPath; // Используем исходный путь как fallback
    }

    QFileInfo fileInfo(localPath);

    if (!fileInfo.exists()) {
        qWarning() << "Файл не существует:" << localPath;
        emit errorOccurred(tr("Файл не существует: %1").arg(localPath));
        return;
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(localPath))) {
        qWarning() << "Не удалось открыть файл:" << localPath;
        emit errorOccurred(tr("Не удалось открыть файл: %1").arg(localPath));
    }
}

bool FileBrowser::isDir(const QString& path) const
{
    return QFileInfo(path).isDir();
}

QString FileBrowser::getFilePathForDrag(const QString& fileName) const
{
    // Используем currentFolder() вместо прямой работы с m_currentFolder
    QString fullPath = QDir::cleanPath(currentFolder() + QDir::separator() + fileName);
    return QUrl::fromLocalFile(fullPath).toString();
}

Q_INVOKABLE void FileBrowser::viewClick(QString currentPath, QString obj)
{
    LOG_INFO("Current path: " + currentPath.toStdString() + "  " + obj.toStdString());
    currentPath = NormalizePath(currentPath);

    QString fullPath = currentPath + "/" + obj;
    LOG_INFO("Go to: " + fullPath.toStdString());

    if (isDir(fullPath)) {
        setCurrentFolder(fullPath);
    }
    else {
        openFile(fullPath);
    }
}

QString FileBrowser::NormalizePath(QString path)
{
    QString cleanPath = path;

    cleanPath = cleanPath.replace("qrc:/", "").replace("file://", "");

    if (cleanPath.startsWith('/')) cleanPath.remove(0,1);

    cleanPath[0].toUpper();

    if (cleanPath[1] != ':') cleanPath.insert(1, ':');

    // Заменяем слеши на бэкслеши для Windows
    cleanPath = QDir::toNativeSeparators(cleanPath);

   
    QDir dir(cleanPath);
    if (!dir.exists()) {
        qWarning() << "Directory does not exist:" << cleanPath;
        emit errorOccurred(tr("Directory does not exist: %1").arg(cleanPath));
    }

    LOG_INFO("Dir after normalize: " + cleanPath.toStdString());
    return cleanPath;
}
