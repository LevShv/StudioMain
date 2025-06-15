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
        localPath = cleanPath;
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

Q_INVOKABLE QString FileBrowser::NormalizePath(QString path)
{
    if (IsPathNormalized(path)) {
        return path;
    }

    QString cleanPath = path;

    cleanPath = cleanPath.replace("qrc:/", "").replace("file://", "");

    if (cleanPath.startsWith('/')) cleanPath.remove(0, 1);

    cleanPath[0].toUpper();

    if (cleanPath[1] != ':') cleanPath.insert(1, ':');
    cleanPath = QDir::toNativeSeparators(cleanPath);

    QDir dir(cleanPath);
    if (!dir.exists()) {
        qWarning() << "Directory does not exist:" << cleanPath;
        emit errorOccurred(tr("Directory does not exist: %1").arg(cleanPath));
    }

    LOG_INFO("Dir after normalize: " + cleanPath.toStdString());
    return cleanPath;
}

bool FileBrowser::IsPathNormalized(const QString& path)
{

    if (path.contains("qrc:/") || path.contains("file://")) {
        return false;
    }
    if (path.length() < 2 || !path[0].isLetter() || path[1] != ':' ) {
        return false;
    }
    QString nativeSeparator = QDir::separator();
    QString oppositeSeparator = (nativeSeparator == "/") ? "\\" : "/";

    if (path.contains(oppositeSeparator)) {
        return false;
    }

    return true;
}

QString FileBrowser::applicationHomeFolder() const
{
    QString appDir = QCoreApplication::applicationDirPath();

    QString homePath = QDir::cleanPath(appDir + "/HomeLeTo");

    QDir dir(homePath);
    if (!dir.exists()) {
        qWarning() << "HomeLeTo directory does not exist:" << homePath;
        return appDir;
    }

    LOG_INFO("Application home folder: " + homePath.toStdString());
    return homePath;
}