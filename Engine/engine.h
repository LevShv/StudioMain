#pragma once

#include <portaudio.h>
#include <sndfile.hh>
#include <iostream>
#include <vector>

class Engine {
public:

	void TestPlay();

	class Track {
	public:
		std::vector<float> samples; // Аудиоданные сэмпла
		bool isMuted = false;       // Флаг для отключения дорожки
		float volume = 1.0f;        // Громкость дорожки
	};

	std::vector<Track> tracks;
	size_t maxSamples;

	class Core {
	public:

		static const int SAMPLE_RATE = 44100;
		const int FRAMES_PER_BUFFER = 4096;
		std::vector<float> mixBuffer;
		
		size_t currentPosition = 0;   // Текущая позиция в миксе

		Core();
		~Core();

		static int AudioCallback(const void* inputBuffer, void* outputBuffer,
			unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo* timeInfo,
			PaStreamCallbackFlags statusFlags,
			void* userData);

		void MixTracks(const std::vector<Track>& tracks, int numFrames);
		void StartPlayAllTracks();
		void StopPlayAllTracks();
		size_t GetMaxSamples(const std::vector<Track>& tracks);


	private:
		
		 // Весь микс
		PaStream* audioStream = nullptr;   // Поток для воспроизведения
		SndfileHandle file;
	};

	class FileManager {
	public:
		bool LoadTrack(const std::string& path, Track track);
	   // void SaveTrack(const std::string& path, const Track& track);
		bool SaveToWav(const std::string& path, const std::vector<float>& samples, const int SAMPLE_RATE);
	};
private:

	

	
};