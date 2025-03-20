#include "ViewModel.h"
#include <QtCore/QDebug>
#include <engine.h>


ViewModel::ViewModel(QObject* parent) : QObject(parent), 
m_isPlaying(false),
m_volume(50) 
{
	Engine engine;
	//engine.TestPlay();

	

}

Q_INVOKABLE void ViewModel::togglePlayback()
{
	m_isPlaying = !m_isPlaying; // Переключение состояния

	if(m_isPlaying) qDebug() << "Track is playing: la la la o o o  ";
	else qDebug() << "Track is stopped";

	emit isPlayingChanged(); // Уведомление об изменении состояния
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


