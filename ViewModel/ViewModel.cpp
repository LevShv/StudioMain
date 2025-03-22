#include "ViewModel.h"
#include <QtCore/QDebug>


ViewModel::ViewModel(QObject* parent) : QObject(parent), 
m_isPlaying(false),
m_volume(50) 
{
	
	//engine.LoadToTrack("Misc/Step5.wav", 0.0, 1, 0);
	engine.LoadToTrack("Misc/choose.wav", 2.0, 1, 1);
	//engine.LoadToTrack("Misc/Happy.wav", 0.0, 2, 2);
    //engine.LoadToTrack("Misc/Village_party.wav", 15.0, 2, 3);

	/*engine.TestPlay();*/

}

Q_INVOKABLE void ViewModel::togglePlayback()
{
	if (engine.isPlaying()) {
		engine.StopPlayback(); // Останавливаем воспроизведение
		m_isPlaying = false;
	}
	else {
		engine.StartPlayback(); // Запускаем воспроизведение
		m_isPlaying = true;
	}

	emit isPlayingChanged(); // Уведомляем об изменении состояния

	// Отладочный вывод
	if (m_isPlaying) {
		qDebug() << "Track is playing: la la la o o o";
	}
	else {
		qDebug() << "Track is stopped";
	}
}

bool ViewModel::isPlaying() const
{
	return m_isPlaying;
}

void ViewModel::setVolume(int volume) {
	if (m_volume != volume) {
		m_volume = volume;
		qDebug() << "Volume changed on:" << m_volume;
		emit volumeChanged(); // Уведомление об изменении громкости
	}
}

int ViewModel::volume() const {
	return m_volume;
}


