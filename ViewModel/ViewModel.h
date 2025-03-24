#pragma once

#include <QObject>
#include <MainWindowView.h>
#include <engine.h>

class ViewModel : public QObject {
    Q_OBJECT
        Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged) // Свойство для состояния трека
        Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged) // Свойство для громкости
      
public:

	Engine engine;

    explicit ViewModel(QObject* parent = nullptr);

    Q_INVOKABLE void togglePlayback(); // Метод для переключения состояния         
    bool isPlaying() const; // Метод для чтения свойства

    Q_INVOKABLE void setVolume(int volume); // Метод для установки свойства volume
    int volume() const;// Метод для чтения свойства volume

	Q_INVOKABLE void setPlayheadPosition(double position); // Метод для установки позиции воспроизведения 
	Q_INVOKABLE void moveClip(size_t trackIdx, size_t clipIdx, double newStartTime); // Метод для перемещения клипа по дорожке
signals:
    void isPlayingChanged(); // Сигнал для уведомления об изменении свойства
    void volumeChanged(); // Сигнал для уведомления об изменении volume


private:
    //bool m_isPlaying; // Флаг, указывающий, играет ли трек
    int m_volume; // Текущая громкость

    std::atomic<bool> m_isPlaying{ false };

};