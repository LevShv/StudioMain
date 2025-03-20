#include "engine.h"

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
	return 0;
}
