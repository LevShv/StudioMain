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
    QString cleanPath = fileUrl;

    // Удаляем префикс qrc:/ если есть
    cleanPath.remove("qrc:/");

    // Если путь уже начинается с / или C:/, оставляем как есть
    if (!cleanPath.startsWith("/") && !cleanPath.contains(":/")) {
        // Добавляем / в начало для относительных путей
        if (!cleanPath.startsWith("/")) {
            cleanPath.prepend("/");
        }
    }

    // Создаем URL - важно указать схему "file://"
    QUrl url;
    if (cleanPath.startsWith("/")) {
        url = QUrl::fromLocalFile(cleanPath);
    }
    else {
        url = QUrl(cleanPath);
    }

    if (!url.isValid()) {
        qWarning() << "Неверный URL:" << fileUrl;
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
