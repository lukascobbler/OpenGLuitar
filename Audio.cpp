#include "Audio.h"
#include <fstream>
#include <windows.h>

#pragma comment(lib, "xaudio2.lib")

IXAudio2* AudioEngine::xaudio = nullptr;
IXAudio2MasteringVoice* AudioEngine::masterVoice = nullptr;
std::vector<AudioEngine::Voice> AudioEngine::activeVoices;

const std::array<std::string, 6> AudioEngine::stringNames = {
    "E", "A", "D", "G", "B", "Eh"
};

AudioEngine::Sound AudioEngine::cachedSounds[STRINGS][FRETS];

bool AudioEngine::loadWav(const std::string& path, Sound& out)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    char id[4];
    uint32_t size;

    f.read(id, 4);
    f.read((char*)&size, 4);
    f.read(id, 4);

    while (true) {
        f.read(id, 4);
        f.read((char*)&size, 4);
        if (!memcmp(id, "fmt ", 4)) break;
        f.seekg(size, std::ios::cur);
    }

    f.read((char*)&out.wfx, size);

    while (true) {
        f.read(id, 4);
        f.read((char*)&size, 4);
        if (!memcmp(id, "data", 4)) break;
        f.seekg(size, std::ios::cur);
    }

    out.samples.resize(size);
    f.read((char*)out.samples.data(), size);
    out.loaded = true;

    return true;
}

bool AudioEngine::init()
{
    if (FAILED(XAudio2Create(&xaudio, 0)))
        return false;

    if (FAILED(xaudio->CreateMasteringVoice(&masterVoice)))
        return false;

    for (int s = 0; s < STRINGS; s++)
    {
        for (int f = 0; f < FRETS; f++)
        {
            std::string path =
                "res/audio/" + stringNames[s] + "/" + std::to_string(f) + ".wav";

            loadWav(path, cachedSounds[s][f]);
        }
    }

    return true;
}

void AudioEngine::shutdown()
{
    for (auto& v : activeVoices)
        if (v.voice) v.voice->DestroyVoice();

    activeVoices.clear();

    if (masterVoice) masterVoice->DestroyVoice();
    if (xaudio) xaudio->Release();
}

void AudioEngine::playNote(std::string stringName, int fretIndex, float volume)
{
    int stringIndex = -1;

    if (stringName == "E") stringIndex = 0;
    else if (stringName == "A") stringIndex = 1;
    else if (stringName == "D") stringIndex = 2;
    else if (stringName == "G") stringIndex = 3;
    else if (stringName == "B") stringIndex = 4;
    else if (stringName == "Eh") stringIndex = 5;
    else return;

    if (stringIndex < 0 || stringIndex >= STRINGS ||
        fretIndex < 0 || fretIndex >= FRETS)
        return;

    Sound& snd = cachedSounds[stringIndex][fretIndex];
    if (!snd.loaded) return;

    Voice inst;
    if (FAILED(xaudio->CreateSourceVoice(&inst.voice, &snd.wfx)))
        return;

    XAUDIO2_BUFFER buf{};
    buf.AudioBytes = snd.samples.size();
    buf.pAudioData = snd.samples.data();
    buf.Flags = XAUDIO2_END_OF_STREAM;

    inst.voice->SubmitSourceBuffer(&buf);
    inst.voice->SetVolume(volume);
    inst.voice->Start();

    activeVoices.push_back(inst);
}

void AudioEngine::collectGarbage()
{
    for (size_t i = 0; i < activeVoices.size(); )
    {
        XAUDIO2_VOICE_STATE st{};
        activeVoices[i].voice->GetState(&st);

        if (st.BuffersQueued == 0)
        {
            activeVoices[i].voice->DestroyVoice();
            activeVoices.erase(activeVoices.begin() + i);
        } else {
            i++;
        }
    }
}
