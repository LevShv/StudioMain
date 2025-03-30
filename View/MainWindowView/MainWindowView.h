#pragma once

#include <QObject>

class MainViewModel : public QObject {
    Q_OBJECT
    //Q_PROPERTY(QStringList tracks READ tracks NOTIFY tracksChanged)

public:
    explicit MainViewModel(QObject* parent = nullptr);


signals:
    void tracksChanged();

private:

};
