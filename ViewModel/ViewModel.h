#pragma once

#include <QObject>
#include <MainWindowView.h>


class ViewModel : public QObject {
    Q_OBJECT
        Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged) 

public:
    explicit ViewModel(QObject* parent = nullptr);

    Q_INVOKABLE void togglePlayback(); // Метод для переключения состояния
    bool isPlaying() const; // Метод для чтения свойства

signals:
    void isPlayingChanged(); // Сигнал для уведомления об изменении свойства

private:
    bool m_isPlaying; // Флаг, указывающий, играет ли трек
};