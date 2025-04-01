#pragma once

#include <QObject>
#include <QString>
#include <QStandardPaths>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QDesktopServices>  // Добавьте эту строку в начале файла

class FileBrowser : public QObject
{
    Q_OBJECT
        Q_PROPERTY(QString currentFolder READ currentFolder WRITE setCurrentFolder NOTIFY currentFolderChanged)
        Q_PROPERTY(QString homeFolder READ homeFolder CONSTANT)

public:
    explicit FileBrowser(QObject* parent = nullptr);

    QString currentFolder() const;
    void setCurrentFolder(const QString& folder);

    QString homeFolder() const;

    Q_INVOKABLE QString parentFolder() const;
    Q_INVOKABLE void openFile(const QString& filePath);
    Q_INVOKABLE bool isDir(const QString& path) const;

signals:
    void currentFolderChanged();

private:
    QString m_currentFolder;
};

