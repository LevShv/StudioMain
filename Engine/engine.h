#pragma once

#include <portaudio.h>
#include <sndfile.hh>
#include <iostream>
#include <vector>

class Engine {
public:

	class Core {
	public:

		size_t currentPosition = 0;   // Текущая позиция в миксе

		Core();
		~Core()
			;
		static int AudioCallback(const void* inputBuffer, void* outputBuffer,
			unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* timeInfo,
			PaStreamCallbackFlags statusFlags,
			void* userData);

		//void OpenStream();
		//void CloseStream();
		//void StartStream();
		//void StopStream();
	private:
		const int SAMPLE_RATE = 44100;
		const int FRAMES_PER_BUFFER = 512;

		std::vector<float> mixBuffer; // Весь микс
		PaStream* stream = nullptr;
		SndfileHandle file;
	};
private:

	

	
};