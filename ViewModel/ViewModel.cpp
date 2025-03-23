#include "ViewModel.h"
#include <QtCore/QDebug>
#include <RtMidi.h>



ViewModel::ViewModel(QObject* parent) : QObject(parent), 
m_isPlaying(false),
m_volume(50) 
{
	
	engine.LoadToTrack("Misc/Step5.wav", 4, 2, 0);
	//engine.LoadToTrack("Misc/choose.wav", 1, 2, 1);
	//engine.LoadToTrack("Misc/choose.wav", 0.5, 2, 1);
	//engine.LoadToTrack("Misc/choose.wav", 2, 2, 1);
	engine.LoadToTrack("Misc/Happy.wav", 0, 2, 4);
  /*  engine.LoadToTrack("Misc/Village_party.wav", 4, 1, 2);*/

	//engine.LoadToTrack("Misc/Step5.wav", 4, 2, 0);
	//engine.LoadToTrack("Misc/choose.wav", 1, 2, 1);
	//engine.LoadToTrack("Misc/choose.wav", 0.5, 2, 1);
	//engine.LoadToTrack("Misc/choose.wav", 2, 2, 1);
	//engine.LoadToTrack("Misc/Happy.wav", 2, 2, 4);
	//engine.LoadToTrack("Misc/Village_party.wav", 4, 1, 2);

	//engine.LoadToTrack("Misc/Step5.wav", 4, 2, 0);
	//engine.LoadToTrack("Misc/choose.wav", 1, 2, 1);
	//engine.LoadToTrack("Misc/choose.wav", 0.5, 2, 1);
	//engine.LoadToTrack("Misc/choose.wav", 2, 2, 1);
	//engine.LoadToTrack("Misc/Happy.wav", 2, 2, 4);
	//engine.LoadToTrack("Misc/Village_party.wav", 4, 1, 2);

	//engine.LoadToTrack("Misc/Step5.wav", 4, 2, 0);
	//engine.LoadToTrack("Misc/choose.wav", 1, 2, 1);
	//engine.LoadToTrack("Misc/choose.wav", 0.5, 2, 1);
	//engine.LoadToTrack("Misc/choose.wav", 2, 2, 1);
	//engine.LoadToTrack("Misc/Happy.wav", 2, 2, 4);
	//engine.LoadToTrack("Misc/Village_party.wav", 4, 1, 2);

	/*engine.TestPlay();*/
	try {
		// Создаем объект для работы с MIDI-входом
		RtMidiIn midiIn;

		// Проверяем количество доступных MIDI-портов
		unsigned int portCount = midiIn.getPortCount();

		if (portCount == 0) {
			std::cout << "No MIDI input ports available!" << std::endl;
		}

		// Выводим список доступных MIDI-портов
		std::cout << "Available MIDI input ports:" << std::endl;
		for (unsigned int i = 0; i < portCount; i++) {
			std::string portName = midiIn.getPortName(i);
			std::cout << "  Port #" << i << ": " << portName << std::endl;
		}

		// Открываем первый MIDI-порт
		midiIn.openPort(0);

		std::cout << "Listening to MIDI input on port 0..." << std::endl;

		// Бесконечный цикл для чтения MIDI-сообщений
		std::vector<unsigned char> message;
		double stamp;
		while (true) {
			stamp = midiIn.getMessage(&message); // Получаем сообщение
			if (!message.empty()) {
				std::cout << "Received MIDI message: ";
				for (unsigned int i = 0; i < message.size(); i++) {
					std::cout << "Byte " << i << " = " << (int)message[i] << ", ";
				}
				std::cout << std::endl;
			}
		}
	}
	catch (RtMidiError& error) {
		// Обработка ошибок
		std::cerr << "RtMidi error: " << error.getMessage() << std::endl;
	}
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

Q_INVOKABLE void ViewModel::setPlayheadPosition(double position)
{
	engine.SetPlayheadPosition(position);
}

Q_INVOKABLE void ViewModel::moveClip(size_t trackIdx, size_t clipIdx, double newStartTime)
{
	engine.MoveClip(0, 0, newStartTime);
}


