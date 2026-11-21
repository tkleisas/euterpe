#include "WorldPercussionSynth.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace euterpe {

namespace {
constexpr float kTwoPi = 6.28318530717958647692f;

float decayCoeff(double sr, float timeSeconds) {
    if (timeSeconds <= 0.0005f) return 0.0f;
    // Reach -60 dB (0.001) in timeSeconds
    return std::exp(-6.907755f / (static_cast<float>(sr) * timeSeconds));
}
} // namespace

WorldPercussionSynth::WorldPercussionSynth() {
    reset();
}

void WorldPercussionSynth::init(double sampleRate) {
    mSampleRate = sampleRate > 8000.0 ? sampleRate : 48000.0;
    reset();
}

void WorldPercussionSynth::reset() {
    for (auto& v : mVoices) {
        v = PercVoice{};
    }
}

void WorldPercussionSynth::setTonicHz(float tonicHz) {
    if (tonicHz > 40.0f && tonicHz < 1200.0f) {
        mTonicHz = tonicHz;
    }
}

float WorldPercussionSynth::nextNoise() {
    mRng = mRng * 1664525u + 1013904223u;
    return (static_cast<float>(mRng >> 8) / 8388608.0f) - 1.0f;
}

int WorldPercussionSynth::voiceIdFromName(const char* name) {
    if (!name) return -1;
    if (std::strcmp(name, "dum") == 0) return DarbukaDum;
    if (std::strcmp(name, "tak") == 0) return DarbukaTak;
    if (std::strcmp(name, "ka") == 0) return DarbukaKa;
    if (std::strcmp(name, "slap") == 0 || std::strcmp(name, "sak") == 0) return DarbukaSlap;
    if (std::strcmp(name, "bendir") == 0) return Bendir;
    if (std::strcmp(name, "daouli") == 0) return Daouli;
    if (std::strcmp(name, "riq") == 0) return Riq;
    if (std::strcmp(name, "bayan") == 0 || std::strcmp(name, "ge") == 0) return TablaBayan;
    if (std::strcmp(name, "dayan") == 0 || std::strcmp(name, "na") == 0 || std::strcmp(name, "tin") == 0) return TablaDayan;
    if (std::strcmp(name, "tik") == 0 || std::strcmp(name, "te") == 0) return TablaTik;
    if (std::strcmp(name, "gong") == 0 || std::strcmp(name, "gong_ageng") == 0) return GongAgeng;
    if (std::strcmp(name, "kempul") == 0) return Kempul;
    if (std::strcmp(name, "kenong") == 0) return Kenong;
    if (std::strcmp(name, "kendang_low") == 0) return KendangLow;
    if (std::strcmp(name, "kendang_high") == 0) return KendangHigh;
    if (std::strcmp(name, "kick") == 0) return WesternKick;
    if (std::strcmp(name, "snare") == 0) return WesternSnare;
    if (std::strcmp(name, "hat") == 0 || std::strcmp(name, "chh") == 0) return WesternHatClosed;
    if (std::strcmp(name, "openhat") == 0 || std::strcmp(name, "ohh") == 0) return WesternHatOpen;
    if (std::strcmp(name, "clap") == 0) return WesternClap;
    return -1;
}

int WorldPercussionSynth::remapVoiceForKit(int voiceId, int kitId) {
    if (kitId <= KitNative || kitId > KitWestern) return voiceId;

    // Classify incoming voice into 5 universal percussive roles:
    // 0 = Bass/Dum/Kick, 1 = Accent/Tak/Snare, 2 = Ghost/Ka/Hat, 3 = Slap/Clap/Kempul, 4 = Metal/Riq/Gong
    int role = 0;
    switch (voiceId) {
    case DarbukaDum: case Bendir: case Daouli: case TablaBayan: case KendangLow: case WesternKick:
        role = 0; break;
    case DarbukaTak: case TablaDayan: case Kenong: case WesternSnare:
        role = 1; break;
    case DarbukaKa: case TablaTik: case KendangHigh: case WesternHatClosed:
        role = 2; break;
    case DarbukaSlap: case Kempul: case WesternClap:
        role = 3; break;
    case Riq: case GongAgeng: case WesternHatOpen:
        role = 4; break;
    default:
        role = 0; break;
    }

    static constexpr int kKitMap[5][5] = {
        // KitArabic
        {DarbukaDum, DarbukaTak, DarbukaKa, DarbukaSlap, Riq},
        // KitGreek
        {Daouli, DarbukaTak, DarbukaKa, DarbukaSlap, Riq},
        // KitTabla
        {TablaBayan, TablaDayan, TablaTik, TablaDayan, TablaTik},
        // KitGamelan
        {GongAgeng, Kenong, KendangHigh, Kempul, GongAgeng},
        // KitWestern
        {WesternKick, WesternSnare, WesternHatClosed, WesternClap, WesternHatOpen},
    };
    return kKitMap[kitId - 1][role];
}

void WorldPercussionSynth::trigger(int voiceId, float velocity) {
    if (voiceId < 0 || voiceId >= kVoiceCount) return;
    int mapped = remapVoiceForKit(voiceId, mKitId);
    float vel = std::clamp(velocity, 0.05f, 1.0f);

    // Choke open hi-hat when closed hi-hat strikes
    if (mapped == WesternHatClosed) {
        mVoices[WesternHatOpen].ampEnv *= 0.15f;
        mVoices[WesternHatOpen].noiseEnv *= 0.15f;
    }

    configureVoice(mVoices[mapped], mapped, vel);
}

void WorldPercussionSynth::configureVoice(PercVoice& v, int voiceId, float vel) {
    v = PercVoice{};
    v.active = true;
    v.id = voiceId;
    v.velocity = vel;
    v.timeSec = 0.0f;
    v.ampEnv = 1.0f;

    const float sr = static_cast<float>(mSampleRate);
    // Normalize tonic into ~130–260 Hz range for tabla/gamelan tuning
    float tuned = mTonicHz;
    while (tuned > 320.0f) tuned *= 0.5f;
    while (tuned < 130.0f) tuned *= 2.0f;

    auto setBandpass = [&](float cutoffHz, float q) {
        v.bpF = std::clamp(2.0f * std::sin(3.14159265f * std::min(cutoffHz, sr * 0.42f) / sr), 0.01f, 1.6f);
        v.bpQ = std::clamp(q, 0.1f, 1.8f);
    };

    switch (voiceId) {
    case DarbukaDum:
        v.startFreq = 165.0f;
        v.endFreq = 86.0f;
        v.pitchTau = 0.028f;
        v.ampDecay = decayCoeff(sr, 0.34f);
        v.durationSec = 0.42f;
        v.pan = -0.08f;
        v.partialCount = 2;
        v.partials[0] = {0.0f, 172.0f, 0.25f, decayCoeff(sr, 0.14f)};
        v.partials[1] = {0.0f, 260.0f, 0.10f, decayCoeff(sr, 0.08f)};
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.018f);
        v.noiseGain = 0.16f;
        setBandpass(900.0f, 0.7f);
        break;

    case DarbukaTak:
        v.startFreq = 580.0f;
        v.endFreq = 410.0f;
        v.pitchTau = 0.012f;
        v.ampDecay = decayCoeff(sr, 0.11f);
        v.durationSec = 0.16f;
        v.pan = 0.22f;
        v.partialCount = 3;
        v.partials[0] = {0.0f, 890.0f, 0.38f, decayCoeff(sr, 0.085f)};
        v.partials[1] = {0.0f, 1420.0f, 0.28f, decayCoeff(sr, 0.06f)};
        v.partials[2] = {0.0f, 2350.0f, 0.18f, decayCoeff(sr, 0.04f)};
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.032f);
        v.noiseGain = 0.42f;
        setBandpass(2800.0f, 0.55f);
        break;

    case DarbukaKa:
        v.startFreq = 640.0f;
        v.endFreq = 460.0f;
        v.pitchTau = 0.010f;
        v.ampDecay = decayCoeff(sr, 0.08f);
        v.durationSec = 0.12f;
        v.pan = -0.22f;
        v.partialCount = 2;
        v.partials[0] = {0.0f, 1080.0f, 0.30f, decayCoeff(sr, 0.06f)};
        v.partials[1] = {0.0f, 1850.0f, 0.22f, decayCoeff(sr, 0.045f)};
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.025f);
        v.noiseGain = 0.38f;
        setBandpass(3400.0f, 0.5f);
        break;

    case DarbukaSlap:
        v.startFreq = 320.0f;
        v.endFreq = 220.0f;
        v.pitchTau = 0.008f;
        v.ampDecay = decayCoeff(sr, 0.055f);
        v.durationSec = 0.09f;
        v.pan = 0.05f;
        v.partialCount = 2;
        v.partials[0] = {0.0f, 1250.0f, 0.22f, decayCoeff(sr, 0.04f)};
        v.partials[1] = {0.0f, 2600.0f, 0.25f, decayCoeff(sr, 0.03f)};
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.038f);
        v.noiseGain = 0.80f;
        setBandpass(2200.0f, 0.4f);
        break;

    case Bendir:
        v.startFreq = 128.0f;
        v.endFreq = 74.0f;
        v.pitchTau = 0.035f;
        v.ampDecay = decayCoeff(sr, 0.45f);
        v.durationSec = 0.55f;
        v.pan = -0.18f;
        v.partialCount = 2;
        v.partials[0] = {0.0f, 148.0f, 0.25f, decayCoeff(sr, 0.28f)};
        v.partials[1] = {0.0f, 222.0f, 0.14f, decayCoeff(sr, 0.18f)};
        // Characteristic bendir gut-snare buzz
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.16f);
        v.noiseGain = 0.22f;
        setBandpass(1800.0f, 0.35f);
        break;

    case Daouli:
        v.startFreq = 110.0f;
        v.endFreq = 54.0f;
        v.pitchTau = 0.042f;
        v.ampDecay = decayCoeff(sr, 0.48f);
        v.durationSec = 0.60f;
        v.pan = -0.10f;
        v.partialCount = 2;
        v.partials[0] = {0.0f, 112.0f, 0.28f, decayCoeff(sr, 0.22f)};
        v.partials[1] = {0.0f, 168.0f, 0.14f, decayCoeff(sr, 0.14f)};
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.028f);
        v.noiseGain = 0.24f;
        setBandpass(650.0f, 0.6f);
        break;

    case Riq:
        v.startFreq = 1400.0f;
        v.endFreq = 1400.0f;
        v.pitchTau = 0.01f;
        v.ampDecay = decayCoeff(sr, 0.04f);
        v.durationSec = 0.22f;
        v.pan = 0.32f;
        v.partialCount = 4;
        // Metallic jingles
        v.partials[0] = {0.0f, 4350.0f, 0.26f, decayCoeff(sr, 0.14f)};
        v.partials[1] = {0.0f, 5920.0f, 0.24f, decayCoeff(sr, 0.12f)};
        v.partials[2] = {0.0f, 7410.0f, 0.20f, decayCoeff(sr, 0.10f)};
        v.partials[3] = {0.0f, 9120.0f, 0.16f, decayCoeff(sr, 0.08f)};
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.11f);
        v.noiseGain = 0.48f;
        setBandpass(6800.0f, 0.45f);
        break;

    case TablaBayan:
        // Characteristic Bayan upward-then-settle pitch inflection!
        v.startFreq = 72.0f;
        v.endFreq = 128.0f; // sweeps UP like wrist pressure on bayan
        v.pitchTau = 0.075f;
        v.ampDecay = decayCoeff(sr, 0.46f);
        v.durationSec = 0.56f;
        v.pan = -0.25f;
        v.partialCount = 2;
        v.partials[0] = {0.0f, 196.0f, 0.22f, decayCoeff(sr, 0.24f)};
        v.partials[1] = {0.0f, 310.0f, 0.10f, decayCoeff(sr, 0.14f)};
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.015f);
        v.noiseGain = 0.12f;
        setBandpass(700.0f, 0.6f);
        break;

    case TablaDayan:
        // Tuned harmonic treble drum locked to active raga tonic (Sa)
        v.startFreq = tuned * 1.04f;
        v.endFreq = tuned;
        v.pitchTau = 0.014f;
        v.ampDecay = decayCoeff(sr, 0.36f);
        v.durationSec = 0.46f;
        v.pan = 0.25f;
        v.partialCount = 3;
        // Syahi black-spot harmonic overtone series (2x, 3x, 4x)
        v.partials[0] = {0.0f, tuned * 2.0f, 0.45f, decayCoeff(sr, 0.28f)};
        v.partials[1] = {0.0f, tuned * 3.0f, 0.26f, decayCoeff(sr, 0.19f)};
        v.partials[2] = {0.0f, tuned * 4.0f, 0.14f, decayCoeff(sr, 0.11f)};
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.016f);
        v.noiseGain = 0.24f;
        setBandpass(2400.0f, 0.6f);
        break;

    case TablaTik:
        v.startFreq = tuned * 1.8f;
        v.endFreq = tuned * 1.4f;
        v.pitchTau = 0.008f;
        v.ampDecay = decayCoeff(sr, 0.045f);
        v.durationSec = 0.07f;
        v.pan = 0.18f;
        v.partialCount = 2;
        v.partials[0] = {0.0f, tuned * 3.1f, 0.28f, decayCoeff(sr, 0.035f)};
        v.partials[1] = {0.0f, tuned * 4.7f, 0.18f, decayCoeff(sr, 0.025f)};
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.022f);
        v.noiseGain = 0.50f;
        setBandpass(3200.0f, 0.5f);
        break;

    case GongAgeng:
        v.startFreq = tuned * 0.375f;
        v.endFreq = tuned * 0.375f;
        v.pitchTau = 0.05f;
        v.ampDecay = decayCoeff(sr, 2.2f);
        v.durationSec = 2.6f;
        v.pan = 0.0f;
        v.ombakRate = 2.6f;   // Slow hypnotic ombak wave
        v.ombakDepth = 0.38f;
        v.partialCount = 4;
        v.partials[0] = {0.0f, v.endFreq * 1.52f, 0.35f, decayCoeff(sr, 1.8f)};
        v.partials[1] = {0.0f, v.endFreq * 2.03f, 0.28f, decayCoeff(sr, 1.4f)};
        v.partials[2] = {0.0f, v.endFreq * 2.98f, 0.18f, decayCoeff(sr, 0.9f)};
        v.partials[3] = {0.0f, v.endFreq * 4.11f, 0.10f, decayCoeff(sr, 0.55f)};
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.04f);
        v.noiseGain = 0.12f;
        setBandpass(600.0f, 0.5f);
        break;

    case Kempul:
        v.startFreq = tuned * 0.75f;
        v.endFreq = tuned * 0.75f;
        v.pitchTau = 0.02f;
        v.ampDecay = decayCoeff(sr, 1.15f);
        v.durationSec = 1.4f;
        v.pan = -0.28f;
        v.ombakRate = 4.2f;
        v.ombakDepth = 0.25f;
        v.partialCount = 3;
        v.partials[0] = {0.0f, v.endFreq * 1.98f, 0.32f, decayCoeff(sr, 0.85f)};
        v.partials[1] = {0.0f, v.endFreq * 2.74f, 0.22f, decayCoeff(sr, 0.55f)};
        v.partials[2] = {0.0f, v.endFreq * 3.92f, 0.12f, decayCoeff(sr, 0.35f)};
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.025f);
        v.noiseGain = 0.10f;
        setBandpass(900.0f, 0.5f);
        break;

    case Kenong:
        v.startFreq = tuned * 1.5f;
        v.endFreq = tuned * 1.5f;
        v.pitchTau = 0.01f;
        v.ampDecay = decayCoeff(sr, 0.75f);
        v.durationSec = 0.95f;
        v.pan = 0.28f;
        v.ombakRate = 5.5f;
        v.ombakDepth = 0.18f;
        v.partialCount = 3;
        v.partials[0] = {0.0f, v.endFreq * 2.02f, 0.38f, decayCoeff(sr, 0.52f)};
        v.partials[1] = {0.0f, v.endFreq * 2.91f, 0.24f, decayCoeff(sr, 0.36f)};
        v.partials[2] = {0.0f, v.endFreq * 4.23f, 0.14f, decayCoeff(sr, 0.22f)};
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.014f);
        v.noiseGain = 0.14f;
        setBandpass(1800.0f, 0.5f);
        break;

    case KendangLow:
        v.startFreq = 155.0f;
        v.endFreq = 88.0f;
        v.pitchTau = 0.025f;
        v.ampDecay = decayCoeff(sr, 0.24f);
        v.durationSec = 0.32f;
        v.pan = -0.14f;
        v.partialCount = 1;
        v.partials[0] = {0.0f, 185.0f, 0.22f, decayCoeff(sr, 0.12f)};
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.016f);
        v.noiseGain = 0.18f;
        setBandpass(1100.0f, 0.6f);
        break;

    case KendangHigh:
        v.startFreq = 390.0f;
        v.endFreq = 260.0f;
        v.pitchTau = 0.012f;
        v.ampDecay = decayCoeff(sr, 0.085f);
        v.durationSec = 0.12f;
        v.pan = 0.16f;
        v.partialCount = 2;
        v.partials[0] = {0.0f, 680.0f, 0.28f, decayCoeff(sr, 0.055f)};
        v.partials[1] = {0.0f, 1290.0f, 0.18f, decayCoeff(sr, 0.035f)};
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.024f);
        v.noiseGain = 0.44f;
        setBandpass(2300.0f, 0.5f);
        break;

    case WesternKick:
        v.startFreq = 165.0f;
        v.endFreq = 48.0f;
        v.pitchTau = 0.032f;
        v.ampDecay = decayCoeff(sr, 0.32f);
        v.durationSec = 0.40f;
        v.pan = 0.0f;
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.010f);
        v.noiseGain = 0.15f;
        setBandpass(1200.0f, 0.6f);
        break;

    case WesternSnare:
        v.startFreq = 230.0f;
        v.endFreq = 165.0f;
        v.pitchTau = 0.016f;
        v.ampDecay = decayCoeff(sr, 0.14f);
        v.durationSec = 0.24f;
        v.pan = 0.04f;
        v.partialCount = 1;
        v.partials[0] = {0.0f, 340.0f, 0.22f, decayCoeff(sr, 0.09f)};
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.16f);
        v.noiseGain = 0.72f;
        setBandpass(2600.0f, 0.45f);
        break;

    case WesternHatClosed:
        v.startFreq = 4000.0f;
        v.endFreq = 4000.0f;
        v.ampDecay = decayCoeff(sr, 0.02f);
        v.durationSec = 0.08f;
        v.pan = 0.26f;
        v.partialCount = 3;
        v.partials[0] = {0.0f, 5400.0f, 0.18f, decayCoeff(sr, 0.045f)};
        v.partials[1] = {0.0f, 7850.0f, 0.16f, decayCoeff(sr, 0.038f)};
        v.partials[2] = {0.0f, 9600.0f, 0.14f, decayCoeff(sr, 0.032f)};
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.052f);
        v.noiseGain = 0.55f;
        setBandpass(8200.0f, 0.4f);
        break;

    case WesternHatOpen:
        v.startFreq = 4000.0f;
        v.endFreq = 4000.0f;
        v.ampDecay = decayCoeff(sr, 0.04f);
        v.durationSec = 0.36f;
        v.pan = 0.26f;
        v.partialCount = 3;
        v.partials[0] = {0.0f, 5400.0f, 0.16f, decayCoeff(sr, 0.28f)};
        v.partials[1] = {0.0f, 7850.0f, 0.14f, decayCoeff(sr, 0.24f)};
        v.partials[2] = {0.0f, 9600.0f, 0.12f, decayCoeff(sr, 0.20f)};
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.30f);
        v.noiseGain = 0.52f;
        setBandpass(7800.0f, 0.4f);
        break;

    case WesternClap:
        v.startFreq = 900.0f;
        v.endFreq = 900.0f;
        v.ampDecay = decayCoeff(sr, 0.02f);
        v.durationSec = 0.20f;
        v.pan = -0.12f;
        v.noiseEnv = 1.0f;
        v.noiseDecay = decayCoeff(sr, 0.14f);
        v.noiseGain = 0.78f;
        setBandpass(1600.0f, 0.38f);
        break;

    default:
        v.active = false;
        break;
    }
}

void WorldPercussionSynth::process(float* buffer, int numFrames, int numChannels, float masterGain) {
    if (!buffer || numFrames <= 0 || numChannels <= 0 || masterGain <= 0.0001f) return;

    const float invSr = 1.0f / static_cast<float>(mSampleRate);

    for (auto& v : mVoices) {
        if (!v.active) continue;

        const float gainL = masterGain * v.velocity * std::sqrt(0.5f * (1.0f - v.pan));
        const float gainR = masterGain * v.velocity * std::sqrt(0.5f * (1.0f + v.pan));

        for (int i = 0; i < numFrames; ++i) {
            const float t = v.timeSec;
            v.timeSec += invSr;

            // Pitch-swept membrane fundamental
            const float expT = std::exp(-t / std::max(0.001f, v.pitchTau));
            const float freq = v.endFreq + (v.startFreq - v.endFreq) * expT;
            v.phase1 += kTwoPi * freq * invSr;
            if (v.phase1 > kTwoPi) v.phase1 -= kTwoPi;

            float ombakMod = 1.0f;
            if (v.ombakRate > 0.0f) {
                v.phase2 += kTwoPi * v.ombakRate * invSr;
                if (v.phase2 > kTwoPi) v.phase2 -= kTwoPi;
                ombakMod = 1.0f - v.ombakDepth * (0.5f - 0.5f * std::cos(v.phase2));
            }

            float sample = std::sin(v.phase1) * v.ampEnv * ombakMod;
            v.ampEnv *= v.ampDecay;

            // Modal overtone partials
            for (int p = 0; p < v.partialCount; ++p) {
                auto& mp = v.partials[p];
                mp.phase += kTwoPi * mp.freq * invSr;
                if (mp.phase > kTwoPi) mp.phase -= kTwoPi;
                sample += std::sin(mp.phase) * mp.amp * ombakMod;
                mp.amp *= mp.decay;
            }

            // State-variable bandpass filtered noise transient
            if (v.noiseEnv > 0.0005f && v.noiseGain > 0.0f) {
                float n = nextNoise();
                // Multi-tap clap re-envelope
                float clapMult = 1.0f;
                if (v.id == WesternClap && t < 0.032f) {
                    float seg = std::fmod(t, 0.0105f) / 0.0105f;
                    clapMult = 1.0f - 0.7f * seg;
                }
                float hp = n - v.bpState2 - v.bpQ * v.bpState1;
                v.bpState1 += v.bpF * hp;
                v.bpState2 += v.bpF * v.bpState1;
                sample += v.bpState1 * v.noiseEnv * v.noiseGain * clapMult;
                v.noiseEnv *= v.noiseDecay;
            }

            // Gentle soft clip per voice to keep transients punchy
            sample = std::tanh(sample * 1.15f) * 0.65f;

            if (numChannels == 1) {
                buffer[i] += sample * masterGain * v.velocity;
            } else {
                buffer[i * numChannels] += sample * gainL;
                buffer[i * numChannels + 1] += sample * gainR;
            }
        }

        if (v.timeSec >= v.durationSec && v.ampEnv < 0.001f && v.noiseEnv < 0.001f) {
            v.active = false;
        }
    }
}

} // namespace euterpe
