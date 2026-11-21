#pragma once

#include <array>
#include <cstdint>

namespace euterpe {

// Synthesized world percussion instrument — modal / physical-modeled C++ voices
// covering Arabic/Levantine, Greek, Indian, Gamelan, and Western percussion.
class WorldPercussionSynth {
public:
    enum VoiceId : int {
        // Arabic / Levantine & Greek
        DarbukaDum = 0,
        DarbukaTak = 1,
        DarbukaKa = 2,
        DarbukaSlap = 3,
        Bendir = 4,
        Daouli = 5,
        Riq = 6,
        // Indian Tabla
        TablaBayan = 7,     // Pitch-modulated bass membrane (Ge / Ghe)
        TablaDayan = 8,     // Tuned harmonic treble drum (Na / Tin / Tun)
        TablaTik = 9,       // Crisp damped stroke (Te / Re)
        // Gamelan Colotomic + Kendang
        GongAgeng = 10,     // Deep resonant gong with slow ombak beating
        Kempul = 11,        // Mid hanging gong
        Kenong = 12,        // Bright kettle gong
        KendangLow = 13,    // Low barrel drum
        KendangHigh = 14,   // High slap barrel drum
        // Western compact kit
        WesternKick = 15,
        WesternSnare = 16,
        WesternHatClosed = 17,
        WesternHatOpen = 18,
        WesternClap = 19,
        kVoiceCount = 20,
    };

    enum KitId : int {
        KitNative = 0,
        KitArabic = 1,
        KitGreek = 2,
        KitTabla = 3,
        KitGamelan = 4,
        KitWestern = 5,
    };

    WorldPercussionSynth();

    void init(double sampleRate);
    void reset();

    // Tune tonic-sensitive drums (Tabla Dayan, Gamelan Kenong/Kempul) to active root Hz.
    void setTonicHz(float tonicHz);
    void setKit(int kitId) { mKitId = kitId; }
    int kit() const { return mKitId; }

    void trigger(int voiceId, float velocity = 0.85f);
    static int remapVoiceForKit(int voiceId, int kitId);

    // Renders interleaved stereo and ADDS into `buffer`.
    void process(float* buffer, int numFrames, int numChannels, float masterGain = 1.0f);

    static int voiceIdFromName(const char* name);

private:
    struct ModalPartial {
        float phase = 0.0f;
        float freq = 100.0f;
        float amp = 0.0f;
        float decay = 0.999f;
    };

    struct PercVoice {
        bool active = false;
        int id = 0;
        float velocity = 0.0f;
        float timeSec = 0.0f;
        float durationSec = 0.5f;
        float pan = 0.0f; // -1..1

        // Primary pitch-swept membrane oscillator
        float phase1 = 0.0f;
        float phase2 = 0.0f;
        float startFreq = 120.0f;
        float endFreq = 60.0f;
        float pitchTau = 0.03f;
        float ampEnv = 0.0f;
        float ampDecay = 0.9995f;
        float ombakRate = 0.0f;  // Hz for gong beating
        float ombakDepth = 0.0f;

        // Secondary modal partials (up to 4)
        std::array<ModalPartial, 4> partials{};
        int partialCount = 0;

        // Filtered noise burst for rim / slap / riq / snare
        float noiseEnv = 0.0f;
        float noiseDecay = 0.99f;
        float noiseGain = 0.0f;
        float bpState1 = 0.0f;
        float bpState2 = 0.0f;
        float bpF = 0.2f;
        float bpQ = 0.7f;
    };

    float nextNoise();
    void configureVoice(PercVoice& v, int voiceId, float velocity);

    double mSampleRate = 48000.0;
    float mTonicHz = 261.6256f; // C4 default
    int mKitId = KitNative;
    uint32_t mRng = 0x12345678u;
    std::array<PercVoice, kVoiceCount> mVoices{};
};

} // namespace euterpe
