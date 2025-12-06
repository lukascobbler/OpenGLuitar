#pragma once
#include <xaudio2.h>
#include <string>
#include <vector>
#include <array>

class AudioEngine
{
public:
    static bool init();
    static void shutdown();

    static void collectGarbage();
    static void playNote(std::string stringName, int fretIndex, float volume = 1.0f);
    static void stopAllNotes();

private:
    struct Sound {
        WAVEFORMATEX wfx{};
        std::vector<BYTE> samples;
        bool loaded = false;
    };

    struct Voice {
        IXAudio2SourceVoice* voice = nullptr;
    };

    static IXAudio2* xaudio;
    static IXAudio2MasteringVoice* masterVoice;

    static std::vector<Voice> activeVoices;

    static constexpr int STRINGS = 6;
    static constexpr int FRETS = 21;

    static Sound cachedSounds[STRINGS][FRETS];

    static const std::array<std::string, STRINGS> stringNames;

    static bool loadWav(const std::string& path, Sound& out);
};
