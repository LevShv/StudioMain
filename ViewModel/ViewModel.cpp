#include "ViewModel.h"
#include <QtCore/QDebug>

ViewModel::ViewModel(QObject* parent) : QObject(parent), m_isPlaying(false)
{

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
