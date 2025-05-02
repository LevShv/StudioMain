#pragma once

#include <QObject>
#include <QString>
#include <QDir>
#include <QDebug>
#include <QStandardPaths>
#include <QUrl>
#include <QDesktopServices>

class FileBrowser : public QObject
{
    Q_OBJECT
        Q_PROPERTY(QString currentFolder /* */ READ currentFolder WRITE setCurrentFolder NOTIFY currentFolderChanged)
        Q_PROPERTY(QString homeFolder READ homeFolder CONSTANT)

public:
    explicit FileBrowser(QObject* parent = nullptr);

    Q_INVOKABLE QString currentFolder() const;
    Q_INVOKABLE void setCurrentFolder(const QString& folder);

    Q_INVOKABLE QString homeFolder() const;
    Q_INVOKABLE QString parentFolder() const;
    Q_INVOKABLE void openFile(const QString& filePath);
    Q_INVOKABLE bool isDir(const QString& path) const;
    Q_INVOKABLE QString getFilePathForDrag(const QString& fileName) const;

signals:
    void currentFolderChanged();
    void errorOccurred(const QString& message);

private:
    QString m_currentFolder;
    
};