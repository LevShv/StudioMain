#include "engine.h"
#include <thread>

void Engine::TestPlay() {

	Core core;
	FileManager filemanager;
	Track drumTrack, bassTrack;

	if (!filemanager.LoadTrack("Misc/Step5.wav", drumTrack) || !filemanager.LoadTrack("Misc/Village_party.wav", bassTrack)) {
		std::cerr << "Error loading samples!" << std::endl;
	}

	tracks.push_back(drumTrack);
	tracks.push_back(bassTrack);

	core.MixTracks(tracks, core.GetMaxSamples(tracks));
	core.StartPlayAllTracks();

	std::this_thread::sleep_for(std::chrono::seconds(5));
	
	filemanager.SaveToWav("Misc/mix.wav", core.mixBuffer, core.SAMPLE_RATE);

}

Engine::Core::Core()
{
    PaError err = Pa_Initialize();
    if (err != paNoError) {
       // throw std::runtime_error("PortAudio error: " + std::string(Pa_GetErrorText(err)));
    }
}

Engine::Core::~Core()
{
	PaError err = Pa_Terminate();
	if (err != paNoError) {
		//throw std::runtime_error("PortAudio error: " + std::string(Pa_GetErrorText(err)));
	}
}

int Engine::Core::AudioCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData)
{
	float* out = (float*)outputBuffer;
	std::vector<float>* buffer = static_cast<std::vector<float>*>(userData);

	for (unsigned long i = 0; i < framesPerBuffer; ++i) {
		if (i < buffer->size()) {
			out[i] = (*buffer)[i];
		}
		else {
			out[i] = 0.0f; // Заполнение нулями, если данные закончились
		}
	}

	return paContinue;
}

void Engine::Core::MixTracks(const std::vector<Track>& tracks, int numFrames)
{
	mixBuffer.assign(numFrames, 0.0f); // Очистка выходного буфера

	for (const auto& track : tracks) {
		if (track.isMuted) continue; // Пропуск отключенных дорожек

		for (size_t i = 0; i < numFrames; ++i) {
			if (i < track.samples.size()) { 
				mixBuffer[i] += track.samples[i] * track.volume; // Микширование
			}
		}
	}
}

void Engine::Core::StartPlayAllTracks()
{
	PaError err = Pa_OpenDefaultStream(&audioStream, 0, 1, paFloat32, SAMPLE_RATE,
		FRAMES_PER_BUFFER, AudioCallback, (void*)&mixBuffer);

	if (err != paNoError) {
		throw std::runtime_error("PortAudio error: " + std::string(Pa_GetErrorText(err)));
	}

	err = Pa_StartStream(audioStream);
	if (err != paNoError) {
		throw std::runtime_error("PortAudio error: " + std::string(Pa_GetErrorText(err)));
	}
}

void Engine::Core::StopPlayAllTracks()
{
	if (audioStream) {
		Pa_StopStream(audioStream);
		Pa_CloseStream(audioStream);
		audioStream = nullptr;
	}
}

size_t Engine::Core::GetMaxSamples(const std::vector<Track>& tracks)
{
	size_t maxSamples = 0;

	for (const auto& track : tracks) {
		if (track.samples.size() > maxSamples) {
			maxSamples = track.samples.size();
		}
	}
	return maxSamples;
}

bool Engine::FileManager::LoadTrack(const std::string& path, Track &track)
{
	SndfileHandle file(path);
	if (file.error()) {
		return false; // Ошибка загрузки файла
	}

	track.samples.resize(file.frames() * file.channels());
	file.read(track.samples.data(), track.samples.size());

	return true;
}

bool Engine::FileManager::SaveToWav(const std::string& path, const std::vector<float>& samples, const int SAMPLE_RATE)
{
	SndfileHandle file(path, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_FLOAT, 1, SAMPLE_RATE);
	if (file.error()) {
		return false; // Ошибка создания файла
	}

	file.write(samples.data(), samples.size());
	return true;
}
