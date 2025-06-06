#include <log.h>
#include "engine.h"
#include "JuceHeader.h"

#pragma region Saver 

void Engine::Saver::SaveProject(const std::string filePath)
{
    juce::var projectJson = juce::var(new juce::DynamicObject());

    // Глобальные настройки
    projectJson.getDynamicObject()->setProperty("bpm", m_core.bpm);
    projectJson.getDynamicObject()->setProperty("position", m_core.position);
    projectJson.getDynamicObject()->setProperty("version", "1.0"); // Для совместимости

    juce::Array<juce::var> tracksArray;

    // Дорожки
    for (const auto& track : m_core.tracks) {
        juce::DynamicObject::Ptr trackJson = new juce::DynamicObject();
        trackJson->setProperty("isMidiTrack", track.isMidiTrack);
        trackJson->setProperty("isSamplerTrack", track.isSamplerTrack);
        trackJson->setProperty("gain", track.gain);
        trackJson->setProperty("muted", track.muted);

        juce::Array<juce::var> clipsArray;

        for (const auto& clip : track.clips) {
            juce::DynamicObject::Ptr clipJson = new juce::DynamicObject();
            clipJson->setProperty("clipID", juce::String(clip->clipID)); // Преобразуем std::string в juce::String
            clipJson->setProperty("startBeats", clip->startBeats);
            clipJson->setProperty("durationBeats", clip->durationBeats);
            clipJson->setProperty("gain", clip->gain);
            clipJson->setProperty("muted", clip->muted);

            if (auto* cloneClip = dynamic_cast<Engine::CloneClip*>(clip.get())) {
                clipJson->setProperty("type", "clone");
                clipJson->setProperty("masterClipID", juce::String(cloneClip->masterClipID)); // Исправлено
                LOG("Saved clone clip with clipID=" << clip->clipID << ", masterClipID=" << cloneClip->masterClipID);
            }
            else if (auto* audioClip = dynamic_cast<Engine::AudioClip*>(clip.get())) {
                clipJson->setProperty("type", "audio");
                clipJson->setProperty("filePath", audioClip->file.getFullPathName());
                clipJson->setProperty("useRAM", audioClip->useRAM);
                LOG("Saved audio clip with clipID=" << clip->clipID);
            }
            else if (auto* midiClip = dynamic_cast<Engine::MidiClip*>(clip.get())) {
                clipJson->setProperty("type", "midi");
                juce::MidiFile midiFile;
                midiFile.addTrack(midiClip->midiSequence);
                midiFile.setTicksPerQuarterNote(960);

                juce::MemoryOutputStream midiStream;
                if (!midiFile.writeTo(midiStream)) {
                    LOG_ERROR("Failed to write MIDI sequence to stream for clipID=" << clip->clipID);
                    continue;
                }

                juce::String midiBase64 = juce::Base64::toBase64(midiStream.getData(), midiStream.getDataSize());
                clipJson->setProperty("midiData", midiBase64);
                LOG("Saved MIDI clip with clipID=" << clip->clipID);
            }

            clipsArray.add(juce::var(clipJson));
        }

        trackJson->setProperty("clips", clipsArray);

        // Плагины
        juce::Array<juce::var> pluginsArray;
        for (const auto& plugin : track.plugins) {
            juce::DynamicObject::Ptr pluginJson = new juce::DynamicObject();
            pluginJson->setProperty("pluginPath", plugin->plugin ? juce::String(plugin->Path) : "");
            pluginJson->setProperty("bypass", plugin->bypass);

            if (plugin->plugin && plugin->plugin->getName().isNotEmpty()) {
                juce::MemoryBlock state;
                plugin->plugin->getStateInformation(state);
                if (state.getSize() > 0) {
                    plugin->state = state;
                    juce::String stateBase64 = juce::Base64::toBase64(state.getData(), state.getSize());
                    pluginJson->setProperty("state", stateBase64);
                    LOG("Saved plugin state for " << plugin->plugin->getName().toStdString() << ", size: " << state.getSize());
                }
                else {
                    LOG_ERROR("Failed to get plugin state for " << plugin->plugin->getName().toStdString());
                }
            }
            pluginsArray.add(juce::var(pluginJson));
        }

        trackJson->setProperty("plugins", pluginsArray);
        tracksArray.add(juce::var(trackJson));
    }

    projectJson.getDynamicObject()->setProperty("tracks", tracksArray);

    // Сохранение в файл
    juce::File projectFile(filePath);
    if (projectFile.existsAsFile()) projectFile.deleteFile();

    juce::FileOutputStream os(projectFile);
    if (!os.openedOk()) {
        LOG_ERROR("Failed to open file for saving: " << filePath);
        return;
    }

    juce::JSON::writeToStream(os, projectJson, true);
    os.flush();

    LOG_SUCCESS("Project saved successfully to: " << filePath);
}

bool Engine::Saver::LoadProject(const std::string filePath)
{
    juce::File projectFile(filePath);
    if (!projectFile.existsAsFile()) {
        LOG_ERROR("Project file does not exist: " << filePath);
        return false;
    }

    juce::FileInputStream inputStream(projectFile);
    if (!inputStream.openedOk()) {
        LOG_ERROR("Failed to open file for loading: " << filePath);
        return false;
    }

    juce::var json = juce::JSON::parse(inputStream);
    if (json.isUndefined() || !json.isObject()) {
        LOG_ERROR("Failed to parse project JSON from file: " << filePath);
        return false;
    }

    m_core.stop();
    m_core.tracks.clear();

    // Загрузка глобальных настроек
    if (json.hasProperty("bpm")) {
        m_core.setBPM(json["bpm"]);
        LOG("Set BPM: " << m_core.bpm);
    }
    if (json.hasProperty("position")) {
        m_core.setPosition(json["position"]);
        LOG("Set position: " << m_core.position);
    }

    // Карта для хранения мастер-клипов
    std::map<std::string, Engine::ClipBase*> masterClips;

    // Загрузка дорожек
    if (json.hasProperty("tracks")) {
        const juce::var& tracksArray = json["tracks"];
        for (const auto& trackVar : *tracksArray.getArray()) {
            Track track;
            track.isMidiTrack = trackVar["isMidiTrack"];
            track.isSamplerTrack = trackVar["isSamplerTrack"];
            track.gain = trackVar["gain"];
            track.muted = trackVar["muted"];

            std::vector<juce::var> cloneClips;
            if (trackVar.hasProperty("clips")) {
                for (const auto& clipVar : *trackVar["clips"].getArray()) {
                    std::string clipType = clipVar["type"].toString().toStdString();
                    std::string clipID = clipVar["clipID"].toString().toStdString();
                    double startBeats = clipVar["startBeats"];
                    double durationBeats = clipVar["durationBeats"];
                    float gain = clipVar["gain"];
                    bool muted = clipVar["muted"];

                    if (clipType == "clone") {
                        cloneClips.push_back(clipVar);
                        continue;
                    }

                    std::unique_ptr<ClipBase> clip;
                    if (clipType == "audio") {
                        auto audioClip = std::make_unique<AudioClip>();
                        audioClip->clipID = clipID;
                        audioClip->startBeats = startBeats;
                        audioClip->durationBeats = durationBeats;
                        audioClip->gain = gain;
                        audioClip->muted = muted;
                        audioClip->file = juce::File(clipVar["filePath"].toString());
                        audioClip->useRAM = clipVar["useRAM"];
                        audioClip->startTime = m_core.beatsToSeconds(startBeats);
                        audioClip->duration = m_core.beatsToSeconds(durationBeats);

                        if (audioClip->useRAM && audioClip->file.existsAsFile()) {
                            m_core.loadClipToRAM(*audioClip);
                        }
                        else if (!audioClip->file.existsAsFile()) {
                            LOG_ERROR("Audio file not found: " << audioClip->file.getFullPathName().toStdString());
                        }
                        clip = std::move(audioClip);
                    }
                    else if (clipType == "midi") {
                        auto midiClip = std::make_unique<MidiClip>();
                        midiClip->clipID = clipID;
                        midiClip->startBeats = startBeats;
                        midiClip->durationBeats = durationBeats;
                        midiClip->gain = gain;
                        midiClip->muted = muted;
                        midiClip->startTime = m_core.beatsToSeconds(startBeats);
                        midiClip->duration = m_core.beatsToSeconds(durationBeats);

                        juce::String midiBase64 = clipVar["midiData"].toString();
                        juce::MemoryOutputStream midiOutputStream;
                        if (!juce::Base64::convertFromBase64(midiOutputStream, midiBase64)) {
                            LOG_ERROR("Failed to decode Base64 MIDI data for clipID=" << clipID);
                            continue;
                        }

                        juce::MemoryInputStream midiStream(midiOutputStream.getData(), midiOutputStream.getDataSize(), false);
                        juce::MidiFile midiFile;
                        if (!midiFile.readFrom(midiStream)) {
                            LOG_ERROR("Failed to read MIDI sequence from stream for clipID=" << clipID);
                            continue;
                        }

                        // Устанавливаем ticksPerQuarterNote для согласованности
                        midiFile.setTicksPerQuarterNote(960);

                        if (midiFile.getNumTracks() > 0) {
                            midiClip->midiSequence = *(midiFile.getTrack(0));
                            m_core.cleanMidiSequence(midiClip.get());
                            LOG("Loaded MIDI clip with clipID=" << clipID << ", events: " << midiClip->midiSequence.getNumEvents());
                        }
                        else {
                            LOG_ERROR("No MIDI tracks found in loaded data for clipID=" << clipID);
                        }
                        clip = std::move(midiClip);
                    }

                    if (clip) {
                        track.clips.push_back(std::move(clip));
                        masterClips[clipID] = track.clips.back().get();
                        LOG("Loaded master clip with clipID=" << clipID);
                    }
                }
            }

            m_core.tracks.emplace_back(std::move(track));
            int currentTrackIndex = m_core.tracks.size() - 1;

            // Загрузка плагинов
            if (trackVar.hasProperty("plugins")) {
                for (const auto& pluginVar : *trackVar["plugins"].getArray()) {
                    std::string pluginPath = pluginVar["pluginPath"].toString().toStdString();
                    if (!pluginPath.empty()) {
                        m_core.addPluginToTrack(currentTrackIndex, pluginPath);
                        auto& plugins = m_core.tracks[currentTrackIndex].plugins;
                        if (plugins.empty() || !plugins.back()->plugin) {
                            LOG_ERROR("Failed to load plugin at path: " << pluginPath);
                            continue;
                        }

                        auto* pluginInstance = plugins.back().get();
                        pluginInstance->bypass = pluginVar["bypass"];

                        if (pluginVar.hasProperty("state")) {
                            juce::String stateBase64 = pluginVar["state"].toString();
                            juce::MemoryOutputStream stateStream;
                            if (juce::Base64::convertFromBase64(stateStream, stateBase64)) {
                                juce::MemoryBlock state(stateStream.getData(), stateStream.getDataSize());
                                pluginInstance->plugin->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
                                pluginInstance->state = state;
                                LOG("Restored plugin state for " << pluginPath << ", size: " << state.getSize());
                            }
                            else {
                                LOG_ERROR("Failed to decode plugin state for " << pluginPath);
                            }
                        }
                    }
                }
            }

            // Загрузка клонов
            for (const auto& clipVar : cloneClips) {
                std::string clipID = clipVar["clipID"].toString().toStdString();
                std::string masterClipID = clipVar["masterClipID"].toString().toStdString();
                double startBeats = clipVar["startBeats"];
                double durationBeats = clipVar["durationBeats"];
                float gain = clipVar["gain"];
                bool muted = clipVar["muted"];

                if (masterClips.find(masterClipID) == masterClips.end()) {
                    LOG_ERROR("Master clip with clipID=" << masterClipID << " not found for clone clipID=" << clipID);
                    continue;
                }

                auto cloneClip = std::make_unique<CloneClip>(masterClips[masterClipID], startBeats);
                cloneClip->clipID = clipID;
                cloneClip->masterClipID = masterClipID;
                cloneClip->gain = gain;
                cloneClip->muted = muted;
                cloneClip->startTime = m_core.beatsToSeconds(startBeats);
                cloneClip->durationBeats = durationBeats;
                cloneClip->duration = m_core.beatsToSeconds(durationBeats);

                m_core.tracks[currentTrackIndex].clips.push_back(std::move(cloneClip));
                LOG("Loaded clone clip with clipID=" << clipID << ", masterClipID=" << masterClipID);
            }
        }
    }

    m_core.updateActiveClips();
    LOG_SUCCESS("Project loaded successfully from: " << filePath);
    return true;
}

#pragma endregion