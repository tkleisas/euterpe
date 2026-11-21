#include "EuterpeEngine.h"

#include "instruments/ElectricPiano.h"
#include "instruments/FMSynth.h"
#include "instruments/KarplusStrong.h"
#include "instruments/PolivoksSynth.h"
#include "instruments/SubtractiveSynth.h"
#include "instruments/WavetableSynth.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace euterpe {

namespace {
constexpr float kTwoPi = 6.28318530717958647692f;

float midiToHz(int midiNote) {
    return 440.0f * std::pow(2.0f, (static_cast<float>(midiNote) - 69.0f) / 12.0f);
}

const char* traditionString(int id) {
    switch (id) {
    case 0: return "western";
    case 1: return "arabic";
    case 2: return "greek";
    case 3: return "raga";
    case 4: return "gamelan";
    case 5: return "techno";
    case 6: return "metal";
    case 7: return "pop";
    default: return "arabic";
    }
}
} // namespace

EuterpeEngine::EuterpeEngine() = default;

EuterpeEngine::~EuterpeEngine() {
    shutdown();
}

bool EuterpeEngine::init(const std::string& scriptsDir) {
    std::lock_guard<std::mutex> lock(mStreamMutex);
    if (mInitialized) return true;

    mInstruments[0] = std::make_unique<yawn::instruments::KarplusStrong>();
    mInstruments[1] = std::make_unique<yawn::instruments::FMSynth>();
    mInstruments[2] = std::make_unique<yawn::instruments::ElectricPiano>();
    mInstruments[3] = std::make_unique<yawn::instruments::WavetableSynth>();
    mInstruments[4] = std::make_unique<yawn::instruments::SubtractiveSynth>();
    mInstruments[5] = std::make_unique<yawn::instruments::PolivoksSynth>();

    // Dedicated synth for the background Idea Loop so it doesn't fight live playing
    mIdeaSynth = std::make_unique<yawn::instruments::KarplusStrong>();

    // Dedicated resonant Acid/Sub Bass synth for generated basslines
    mBassSynth = std::make_unique<yawn::instruments::SubtractiveSynth>();

    mReverb = std::make_unique<yawn::effects::Reverb>();
    mLimiter = std::make_unique<yawn::effects::Limiter>();

    mSampleRate = 48000.0;
    for (int i = 0; i < kInstrumentCount; ++i) {
        mInstruments[i]->init(mSampleRate, kMaxFrames);
        configureInstrumentDefaults(i);
    }
    mIdeaSynth->init(mSampleRate, kMaxFrames);
    mIdeaSynth->setParameter(0, 0.35f);
    mIdeaSynth->setParameter(1, 0.72f);

    mBassSynth->init(mSampleRate, kMaxFrames);
    using SUB = yawn::instruments::SubtractiveSynth;
    mBassSynth->setParameter(SUB::kOsc1Wave, 1.0f);       // Saw
    mBassSynth->setParameter(SUB::kOsc1Level, 0.80f);
    mBassSynth->setParameter(SUB::kOsc2Wave, 2.0f);       // Square
    mBassSynth->setParameter(SUB::kOsc2Level, 0.25f);
    mBassSynth->setParameter(SUB::kSubLevel, 0.68f);      // Solid sub
    mBassSynth->setParameter(SUB::kFilterCutoff, 0.34f);  // Deep punchy cutoff
    mBassSynth->setParameter(SUB::kFilterResonance, 0.45f);
    mBassSynth->setParameter(SUB::kFilterEnvAmount, 0.55f);
    mBassSynth->setParameter(SUB::kAmpAttack, 0.003f);
    mBassSynth->setParameter(SUB::kAmpDecay, 0.24f);
    mBassSynth->setParameter(SUB::kAmpSustain, 0.45f);
    mBassSynth->setParameter(SUB::kAmpRelease, 0.14f);
    mBassSynth->setParameter(SUB::kFiltAttack, 0.002f);
    mBassSynth->setParameter(SUB::kFiltDecay, 0.16f);
    mBassSynth->setParameter(SUB::kFiltSustain, 0.18f);

    mReverb->init(mSampleRate, kMaxFrames);
    mReverb->setParameter(0, 0.62f); // room size
    mReverb->setParameter(1, 0.45f); // damping
    mReverb->setParameter(2, mReverbMix); // wet/dry

    mLimiter->init(mSampleRate, kMaxFrames);

    mPerf.init(mSampleRate);
    mSequencer.init(mSampleRate);
    mPerc.init(mSampleRate);
    mPerc.setTonicHz(midiToHz(60 + mRootSemitone));
    mLooper.init(mSampleRate);

    {
        std::lock_guard<std::mutex> luaLock(mLuaMutex);
        std::string err;
        mLua.init(scriptsDir, &err);
    }

    // Load default Arabic Maqam Bayati + Maqsum
    setScale("bayati", 2);
    setRhythm("maqsum");
    mutateIdea(1);

    mInitialized = true;
    return openStream();
}

void EuterpeEngine::configureInstrumentDefaults(int index) {
    if (index == 0) applySoundPreset(0);
}

void EuterpeEngine::applySoundPreset(int presetId) {
    mActivePreset = std::clamp(presetId, 0, 20);
    mDriveAmount = 0.0f;
    int targetInst = 0;
    if (mActivePreset <= 3 || mActivePreset == 15) targetInst = 0;       // KarplusStrong
    else if (mActivePreset <= 5 || mActivePreset == 14) targetInst = 1;  // FMSynth
    else if (mActivePreset <= 8) targetInst = 2;                         // ElectricPiano
    else if (mActivePreset <= 10 || mActivePreset == 16) targetInst = 3; // WavetableSynth
    else if (mActivePreset >= 18) targetInst = 5;                        // Polivoks 1982!
    else targetInst = 4;                                                 // SubtractiveSynth

    if (targetInst != mActiveInstrument && mInstruments[mActiveInstrument]) {
        mInstruments[mActiveInstrument]->reset();
    }
    mActiveInstrument = targetInst;
    auto* inst = mInstruments[mActiveInstrument].get();
    if (!inst) return;

    using KS = yawn::instruments::KarplusStrong;
    using FM = yawn::instruments::FMSynth;
    using EP = yawn::instruments::ElectricPiano;
    using WT = yawn::instruments::WavetableSynth;
    using SUB = yawn::instruments::SubtractiveSynth;
    using PV = yawn::instruments::PolivoksSynth;

    switch (mActivePreset) {
    case 0: // Oud (Warm Pluck)
        inst->setParameter(KS::kExciter, 0.0f);
        inst->setParameter(KS::kDamping, 0.52f);
        inst->setParameter(KS::kBrightness, 0.58f);
        inst->setParameter(KS::kDecay, 0.78f);
        inst->setParameter(KS::kBody, 0.72f);
        inst->setParameter(KS::kStretch, 0.05f);
        inst->setParameter(KS::kAttack, 0.004f);
        break;
    case 1: // Bouzouki (Bright Pluck)
        inst->setParameter(KS::kExciter, 1.0f);
        inst->setParameter(KS::kDamping, 0.22f);
        inst->setParameter(KS::kBrightness, 0.88f);
        inst->setParameter(KS::kDecay, 0.74f);
        inst->setParameter(KS::kBody, 0.58f);
        inst->setParameter(KS::kStretch, 0.20f);
        inst->setParameter(KS::kAttack, 0.002f);
        break;
    case 2: // Sitar (Resonant Jawari)
        inst->setParameter(KS::kExciter, 2.0f);
        inst->setParameter(KS::kDamping, 0.32f);
        inst->setParameter(KS::kBrightness, 0.82f);
        inst->setParameter(KS::kDecay, 0.86f);
        inst->setParameter(KS::kBody, 0.84f);
        inst->setParameter(KS::kStretch, 0.48f);
        inst->setParameter(KS::kAttack, 0.005f);
        break;
    case 3: // Kanun / Harp (Crisp)
        inst->setParameter(KS::kExciter, 1.0f);
        inst->setParameter(KS::kDamping, 0.16f);
        inst->setParameter(KS::kBrightness, 0.92f);
        inst->setParameter(KS::kDecay, 0.90f);
        inst->setParameter(KS::kBody, 0.42f);
        inst->setParameter(KS::kStretch, 0.06f);
        inst->setParameter(KS::kAttack, 0.0015f);
        break;
    case 4: // Gamelan Saron (Bronze)
        inst->setParameter(FM::kAlgorithm, 4.0f);
        inst->setParameter(FM::kFeedback, 0.18f);
        inst->setParameter(FM::kOp1Level, 0.90f);
        inst->setParameter(FM::kOp1Ratio, 1.0f);
        inst->setParameter(FM::kOp1Attack, 0.002f);
        inst->setParameter(FM::kOp1Release, 1.15f);
        inst->setParameter(FM::kOp2Level, 0.55f);
        inst->setParameter(FM::kOp2Ratio, 2.76f);
        inst->setParameter(FM::kOp2Attack, 0.001f);
        inst->setParameter(FM::kOp2Release, 0.45f);
        inst->setParameter(FM::kOp3Level, 0.32f);
        inst->setParameter(FM::kOp3Ratio, 5.40f);
        break;
    case 5: // Balinese Bell / Gender
        inst->setParameter(FM::kAlgorithm, 6.0f);
        inst->setParameter(FM::kFeedback, 0.10f);
        inst->setParameter(FM::kOp1Level, 0.88f);
        inst->setParameter(FM::kOp1Ratio, 1.0f);
        inst->setParameter(FM::kOp1Attack, 0.004f);
        inst->setParameter(FM::kOp1Release, 1.80f);
        inst->setParameter(FM::kOp2Level, 0.42f);
        inst->setParameter(FM::kOp2Ratio, 2.01f);
        inst->setParameter(FM::kOp2Release, 1.35f);
        inst->setParameter(FM::kOp3Level, 0.28f);
        inst->setParameter(FM::kOp3Ratio, 3.98f);
        break;
    case 6: // Rhodes EP (Warm Orchid)
        inst->setParameter(EP::kMode, 0.0f);
        inst->setParameter(EP::kBellMix, 0.34f);
        inst->setParameter(EP::kHardness, 0.48f);
        inst->setParameter(EP::kDecay, 3.2f);
        inst->setParameter(EP::kRelease, 0.45f);
        inst->setParameter(EP::kTremRate, 4.2f);
        inst->setParameter(EP::kTremDepth, 0.22f);
        break;
    case 7: // Wurlitzer EP (Bark)
        inst->setParameter(EP::kMode, 1.0f);
        inst->setParameter(EP::kBellMix, 0.55f);
        inst->setParameter(EP::kHardness, 0.72f);
        inst->setParameter(EP::kDecay, 2.4f);
        inst->setParameter(EP::kRelease, 0.32f);
        inst->setParameter(EP::kTremRate, 5.8f);
        inst->setParameter(EP::kTremDepth, 0.38f);
        break;
    case 8: // Suitcase EP (Dreamy)
        inst->setParameter(EP::kMode, 2.0f);
        inst->setParameter(EP::kBellMix, 0.50f);
        inst->setParameter(EP::kHardness, 0.42f);
        inst->setParameter(EP::kDecay, 5.2f);
        inst->setParameter(EP::kRelease, 0.85f);
        inst->setParameter(EP::kTremRate, 3.2f);
        inst->setParameter(EP::kTremDepth, 0.45f);
        break;
    case 9: // Ney / Formant Wind
        inst->setParameter(WT::kTable, 2.0f); // Formant
        inst->setParameter(WT::kPosition, 0.35f);
        inst->setParameter(WT::kFilterCutoff, 0.58f);
        inst->setParameter(WT::kFilterResonance, 0.25f);
        inst->setParameter(WT::kAmpAttack, 0.08f);
        inst->setParameter(WT::kAmpSustain, 0.85f);
        inst->setParameter(WT::kAmpRelease, 0.45f);
        break;
    case 10: // Harmonic Pad (Evolving)
        inst->setParameter(WT::kTable, 3.0f); // Harmonic
        inst->setParameter(WT::kPosition, 0.52f);
        inst->setParameter(WT::kFilterCutoff, 0.65f);
        inst->setParameter(WT::kFilterResonance, 0.18f);
        inst->setParameter(WT::kAmpAttack, 0.25f);
        inst->setParameter(WT::kAmpSustain, 0.88f);
        inst->setParameter(WT::kAmpRelease, 1.25f);
        break;
    case 11: // Analog Lead / Duduk
        inst->setParameter(SUB::kOsc1Wave, 1.0f);
        inst->setParameter(SUB::kOsc1Level, 0.80f);
        inst->setParameter(SUB::kOsc2Wave, 2.0f);
        inst->setParameter(SUB::kOsc2Level, 0.35f);
        inst->setParameter(SUB::kOsc2Detune, 0.08f);
        inst->setParameter(SUB::kSubLevel, 0.30f);
        inst->setParameter(SUB::kFilterCutoff, 0.54f);
        inst->setParameter(SUB::kFilterResonance, 0.28f);
        inst->setParameter(SUB::kAmpAttack, 0.045f);
        inst->setParameter(SUB::kAmpSustain, 0.85f);
        inst->setParameter(SUB::kAmpRelease, 0.42f);
        break;
    case 12: // Acid TB-303 (Squelchy Resonant Saw + Overdrive)
        inst->setParameter(SUB::kOsc1Wave, 1.0f); // Saw
        inst->setParameter(SUB::kOsc1Level, 0.92f);
        inst->setParameter(SUB::kOsc2Wave, 2.0f); // Square
        inst->setParameter(SUB::kOsc2Level, 0.18f);
        inst->setParameter(SUB::kOsc2Detune, 0.0f);
        inst->setParameter(SUB::kSubLevel, 0.22f);
        inst->setParameter(SUB::kFilterCutoff, 0.24f);
        inst->setParameter(SUB::kFilterResonance, 0.78f);
        inst->setParameter(SUB::kFilterEnvAmount, 0.88f);
        inst->setParameter(SUB::kAmpAttack, 0.002f);
        inst->setParameter(SUB::kAmpDecay, 0.26f);
        inst->setParameter(SUB::kAmpSustain, 0.35f);
        inst->setParameter(SUB::kAmpRelease, 0.14f);
        inst->setParameter(SUB::kFiltAttack, 0.002f);
        inst->setParameter(SUB::kFiltDecay, 0.16f);
        inst->setParameter(SUB::kFiltSustain, 0.12f);
        mDriveAmount = 0.55f;
        break;
    case 13: // Deep Sub Bass
        inst->setParameter(SUB::kOsc1Wave, 0.0f); // Sine/Warm
        inst->setParameter(SUB::kOsc1Level, 0.85f);
        inst->setParameter(SUB::kOsc2Wave, 1.0f);
        inst->setParameter(SUB::kOsc2Level, 0.22f);
        inst->setParameter(SUB::kSubLevel, 0.88f);
        inst->setParameter(SUB::kFilterCutoff, 0.28f);
        inst->setParameter(SUB::kFilterResonance, 0.18f);
        inst->setParameter(SUB::kFilterEnvAmount, 0.25f);
        inst->setParameter(SUB::kAmpAttack, 0.004f);
        inst->setParameter(SUB::kAmpSustain, 0.82f);
        inst->setParameter(SUB::kAmpRelease, 0.22f);
        break;
    case 14: // FM Techno Bass
        inst->setParameter(FM::kAlgorithm, 0.0f);
        inst->setParameter(FM::kFeedback, 0.35f);
        inst->setParameter(FM::kOp1Level, 0.92f);
        inst->setParameter(FM::kOp1Ratio, 1.0f);
        inst->setParameter(FM::kOp1Attack, 0.002f);
        inst->setParameter(FM::kOp1Release, 0.35f);
        inst->setParameter(FM::kOp2Level, 0.68f);
        inst->setParameter(FM::kOp2Ratio, 2.0f);
        inst->setParameter(FM::kOp2Attack, 0.001f);
        inst->setParameter(FM::kOp2Release, 0.14f);
        inst->setParameter(FM::kOp3Level, 0.25f);
        inst->setParameter(FM::kOp3Ratio, 1.0f);
        mDriveAmount = 0.25f;
        break;
    case 15: // Metal Riff Guitar (High-Gain Karplus Drive)
        inst->setParameter(KS::kExciter, 2.0f); // Saw
        inst->setParameter(KS::kDamping, 0.28f);
        inst->setParameter(KS::kBrightness, 0.95f);
        inst->setParameter(KS::kDecay, 0.82f);
        inst->setParameter(KS::kBody, 0.65f);
        inst->setParameter(KS::kStretch, 0.12f);
        inst->setParameter(KS::kAttack, 0.002f);
        mDriveAmount = 0.85f;
        break;
    case 16: // Pop Pluck Synth
        inst->setParameter(WT::kTable, 0.0f);
        inst->setParameter(WT::kPosition, 0.62f);
        inst->setParameter(WT::kFilterCutoff, 0.42f);
        inst->setParameter(WT::kFilterResonance, 0.32f);
        inst->setParameter(WT::kFilterEnvAmount, 0.65f);
        inst->setParameter(WT::kAmpAttack, 0.003f);
        inst->setParameter(WT::kAmpDecay, 0.38f);
        inst->setParameter(WT::kAmpSustain, 0.25f);
        inst->setParameter(WT::kAmpRelease, 0.28f);
        inst->setParameter(WT::kFiltDecay, 0.22f);
        break;
    case 17: // Warm Synth Brass (Pop / 80s Chords)
        inst->setParameter(SUB::kOsc1Wave, 1.0f);
        inst->setParameter(SUB::kOsc1Level, 0.78f);
        inst->setParameter(SUB::kOsc2Wave, 1.0f);
        inst->setParameter(SUB::kOsc2Level, 0.72f);
        inst->setParameter(SUB::kOsc2Detune, 0.16f);
        inst->setParameter(SUB::kSubLevel, 0.24f);
        inst->setParameter(SUB::kFilterCutoff, 0.44f);
        inst->setParameter(SUB::kFilterResonance, 0.22f);
        inst->setParameter(SUB::kFilterEnvAmount, 0.48f);
        inst->setParameter(SUB::kAmpAttack, 0.035f);
        inst->setParameter(SUB::kAmpSustain, 0.86f);
        inst->setParameter(SUB::kAmpRelease, 0.48f);
        inst->setParameter(SUB::kFiltAttack, 0.055f);
        inst->setParameter(SUB::kFiltDecay, 0.45f);
        break;
    case 18: // Polivoks 1982 Lead (Raw Soviet Analog)
        inst->setParameter(PV::kProgram, 0.0f);
        inst->setParameter(PV::kCutoff, 0.56f);
        inst->setParameter(PV::kResonance, 0.62f);
        inst->setParameter(PV::kEnvAmount, 0.65f);
        inst->setParameter(PV::kARelease, 0.35f);
        mDriveAmount = 0.35f;
        break;
    case 19: // Polivoks Acid Bass (Screaming Resonant Filter)
        inst->setParameter(PV::kProgram, 1.0f);
        inst->setParameter(PV::kCutoff, 0.30f);
        inst->setParameter(PV::kResonance, 0.84f);
        inst->setParameter(PV::kEnvAmount, 0.85f);
        inst->setParameter(PV::kFDecay, 0.25f);
        inst->setParameter(PV::kARelease, 0.20f);
        mDriveAmount = 0.55f;
        break;
    case 20: // Polivoks Dark Pad
        inst->setParameter(PV::kProgram, 2.0f);
        inst->setParameter(PV::kCutoff, 0.42f);
        inst->setParameter(PV::kResonance, 0.48f);
        inst->setParameter(PV::kAAttack, 0.35f);
        inst->setParameter(PV::kARelease, 0.72f);
        break;
    }
}

void EuterpeEngine::applyToneAndDecayMacros() {
    auto* inst = mInstruments[mActiveInstrument].get();
    if (!inst) return;

    using KS = yawn::instruments::KarplusStrong;
    using FM = yawn::instruments::FMSynth;
    using EP = yawn::instruments::ElectricPiano;
    using WT = yawn::instruments::WavetableSynth;
    using SUB = yawn::instruments::SubtractiveSynth;
    using PV = yawn::instruments::PolivoksSynth;

    switch (mActiveInstrument) {
    case 0: // KarplusStrong
        inst->setParameter(KS::kBrightness, std::clamp(mSynthTone, 0.05f, 1.0f));
        inst->setParameter(KS::kDamping, std::clamp(1.0f - mSynthTone * 0.85f, 0.05f, 0.95f));
        inst->setParameter(KS::kDecay, std::clamp(0.35f + mSynthDecay * 0.60f, 0.30f, 0.96f));
        break;
    case 1: // FMSynth
        inst->setParameter(FM::kOp2Level, std::clamp(0.10f + mSynthTone * 0.75f, 0.05f, 0.90f));
        inst->setParameter(FM::kOp1Release, std::clamp(0.15f + mSynthDecay * 2.4f, 0.10f, 3.0f));
        break;
    case 2: // ElectricPiano
        inst->setParameter(EP::kBellMix, std::clamp(mSynthTone * 0.85f, 0.05f, 0.95f));
        inst->setParameter(EP::kDecay, std::clamp(0.8f + mSynthDecay * 6.0f, 0.6f, 8.0f));
        inst->setParameter(EP::kRelease, std::clamp(0.12f + mSynthDecay * 1.1f, 0.08f, 1.5f));
        break;
    case 3: // WavetableSynth
        inst->setParameter(WT::kFilterCutoff, std::clamp(0.18f + mSynthTone * 0.78f, 0.15f, 0.98f));
        inst->setParameter(WT::kAmpRelease, std::clamp(0.10f + mSynthDecay * 1.8f, 0.08f, 2.5f));
        break;
    case 4: // SubtractiveSynth
        inst->setParameter(SUB::kFilterCutoff, std::clamp(0.18f + mSynthTone * 0.78f, 0.15f, 0.98f));
        inst->setParameter(SUB::kFilterResonance, std::clamp(0.12f + mDriveAmount * 0.78f, 0.10f, 0.92f));
        inst->setParameter(SUB::kAmpRelease, std::clamp(0.10f + mSynthDecay * 1.6f, 0.08f, 2.2f));
        break;
    case 5: { // Polivoks 1982
        using PV = yawn::instruments::PolivoksSynth;
        inst->setParameter(PV::kCutoff, std::clamp(0.14f + mSynthTone * 0.80f, 0.10f, 0.96f));
        inst->setParameter(PV::kResonance, std::clamp(0.18f + mDriveAmount * 0.76f, 0.10f, 0.94f));
        inst->setParameter(PV::kARelease, std::clamp(0.12f + mSynthDecay * 0.75f, 0.10f, 0.92f));
        break;
    }
    }
}

bool EuterpeEngine::openStream() {
    closeStream();

    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Output)
           ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
           ->setSharingMode(oboe::SharingMode::Exclusive)
           ->setFormat(oboe::AudioFormat::Float)
           ->setChannelCount(oboe::ChannelCount::Stereo)
           ->setCallback(this);

    oboe::Result result = builder.openStream(mStream);
    if (result != oboe::Result::OK || !mStream) {
        // Fallback to Shared mode if Exclusive is unavailable
        builder.setSharingMode(oboe::SharingMode::Shared);
        result = builder.openStream(mStream);
    }
    if (result != oboe::Result::OK || !mStream) {
        return false;
    }

    const double sr = static_cast<double>(mStream->getSampleRate());
    if (sr > 8000.0 && std::fabs(sr - mSampleRate) > 1.0) {
        mSampleRate = sr;
        for (int i = 0; i < kInstrumentCount; ++i) {
            mInstruments[i]->init(mSampleRate, kMaxFrames);
            configureInstrumentDefaults(i);
        }
        mIdeaSynth->init(mSampleRate, kMaxFrames);
        mReverb->init(mSampleRate, kMaxFrames);
        mLimiter->init(mSampleRate, kMaxFrames);
        mPerf.init(mSampleRate);
        mSequencer.init(mSampleRate);
        mPerc.init(mSampleRate);
        mPerc.setTonicHz(midiToHz(60 + mRootSemitone));
        mLooper.init(mSampleRate);
    }

    mStream->requestStart();
    return true;
}

void EuterpeEngine::closeStream() {
    if (mStream) {
        mStream->requestStop();
        mStream->close();
        mStream.reset();
    }
}

void EuterpeEngine::shutdown() {
    std::lock_guard<std::mutex> lock(mStreamMutex);
    closeStream();
    mInitialized = false;
}

void EuterpeEngine::pauseAudio() {
    std::lock_guard<std::mutex> lock(mStreamMutex);
    if (mStream) {
        mStream->requestPause();
    }
}

void EuterpeEngine::resumeAudio() {
    std::lock_guard<std::mutex> lock(mStreamMutex);
    if (!mInitialized) return;
    if (!mStream || mStream->getState() == oboe::StreamState::Closed ||
        mStream->getState() == oboe::StreamState::Disconnected) {
        openStream();
    } else {
        mStream->requestStart();
    }
}

void EuterpeEngine::onErrorAfterClose(oboe::AudioStream*, oboe::Result error) {
    if (error == oboe::Result::ErrorDisconnected && mInitialized) {
        std::lock_guard<std::mutex> lock(mStreamMutex);
        openStream();
    }
}

// ---------------------------------------------------------------------------
// Thread-safe UI Control API
// ---------------------------------------------------------------------------

void EuterpeEngine::noteOn(int pointerId, int midiNote, float cents, float velocity) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::NoteOn;
    ev.pointerId = pointerId;
    ev.intParam1 = midiNote;
    ev.floatParam1 = cents;
    ev.floatParam2 = velocity;
    mQueue.push(ev);
}

void EuterpeEngine::noteOff(int pointerId) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::NoteOff;
    ev.pointerId = pointerId;
    mQueue.push(ev);
}

void EuterpeEngine::allNotesOff() {
    EngineEvent ev;
    ev.type = EngineEvent::Type::AllNotesOff;
    mQueue.push(ev);
}

void EuterpeEngine::setTradition(int traditionId) {
    mActiveTraditionStr = traditionString(traditionId);
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetTradition;
    ev.intParam1 = traditionId;
    mQueue.push(ev);
}

void EuterpeEngine::setScale(const std::string& scaleId, int rootSemitone) {
    mActiveScaleId = scaleId;
    mRootSemitone = ((rootSemitone % 12) + 12) % 12;

    std::vector<float> degs;
    {
        std::lock_guard<std::mutex> luaLock(mLuaMutex);
        if (mLua.isReady()) {
            mLua.loadScale(scaleId, degs);
        }
    }
    if (degs.empty()) {
        degs = {0.0f, 2.0f, 4.0f, 5.0f, 7.0f, 9.0f, 11.0f};
    }

    {
        std::lock_guard<std::mutex> swapLock(mPatternSwapMutex);
        mPendingRoot = mRootSemitone;
        mPendingScaleDegrees = std::move(degs);
        mHasPendingScale = true;
    }

    EngineEvent ev;
    ev.type = EngineEvent::Type::SetRoot;
    ev.intParam1 = mRootSemitone;
    mQueue.push(ev);
}

void EuterpeEngine::setCustomScale(int rootSemitone, const std::vector<float>& degrees) {
    mRootSemitone = ((rootSemitone % 12) + 12) % 12;
    if (degrees.empty()) return;
    {
        std::lock_guard<std::mutex> swapLock(mPatternSwapMutex);
        mPendingRoot = mRootSemitone;
        mPendingScaleDegrees = degrees;
        mHasPendingScale = true;
    }
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetRoot;
    ev.intParam1 = mRootSemitone;
    mQueue.push(ev);
}

void EuterpeEngine::setSoundPreset(int presetId) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetSoundPreset;
    ev.intParam1 = std::clamp(presetId, 0, 20);
    mQueue.push(ev);
}

void EuterpeEngine::setSynthTone(float tone) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetSynthTone;
    ev.floatParam1 = std::clamp(tone, 0.0f, 1.0f);
    mQueue.push(ev);
}

void EuterpeEngine::setSynthDecay(float decay) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetSynthDecay;
    ev.floatParam1 = std::clamp(decay, 0.0f, 1.0f);
    mQueue.push(ev);
}

void EuterpeEngine::setSynthDrive(float drive) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetSynthDrive;
    ev.floatParam1 = std::clamp(drive, 0.0f, 1.0f);
    mQueue.push(ev);
}

void EuterpeEngine::setRhythm(const std::string& rhythmId) {
    mActiveRhythmId = rhythmId;
    RhythmPattern pat;
    bool loaded = false;
    {
        std::lock_guard<std::mutex> luaLock(mLuaMutex);
        if (mLua.isReady()) {
            loaded = mLua.loadRhythm(rhythmId, pat);
        }
    }
    if (!loaded) {
        pat.stepCount = 16;
        pat.stepsPerBeat = 4.0f;
        pat.steps.resize(16);
        pat.steps[0].push_back({WorldPercussionSynth::DarbukaDum, 0.9f});
        pat.steps[6].push_back({WorldPercussionSynth::DarbukaTak, 0.8f});
        pat.steps[8].push_back({WorldPercussionSynth::DarbukaDum, 0.85f});
        pat.steps[12].push_back({WorldPercussionSynth::DarbukaTak, 0.8f});
    }
    std::lock_guard<std::mutex> swapLock(mPatternSwapMutex);
    mPendingRhythm = std::move(pat);
    mHasPendingRhythm = true;
}

void EuterpeEngine::setPerformanceMode(int mode) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetPerformanceMode;
    ev.intParam1 = mode;
    mQueue.push(ev);
}

void EuterpeEngine::setModifierMask(int mask) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetModifierMask;
    ev.intParam1 = mask;
    mQueue.push(ev);
}

void EuterpeEngine::setLatch(bool latch) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetLatch;
    ev.intParam1 = latch ? 1 : 0;
    mQueue.push(ev);
}

void EuterpeEngine::setInstrument(int instrumentIndex) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetInstrument;
    ev.intParam1 = std::clamp(instrumentIndex, 0, kInstrumentCount - 1);
    mQueue.push(ev);
}

void EuterpeEngine::setBpm(float bpm) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetBpm;
    ev.floatParam1 = bpm;
    mQueue.push(ev);
}

void EuterpeEngine::setTransportPlaying(bool playing) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetTransportPlaying;
    ev.intParam1 = playing ? 1 : 0;
    mQueue.push(ev);
}

void EuterpeEngine::setIdeaLoopEnabled(bool enabled) {
    mSequencer.setIdeaEnabled(enabled);
}

void EuterpeEngine::setBassLoopEnabled(bool enabled) {
    mSequencer.setBassEnabled(enabled);
}

bool EuterpeEngine::mutateIdea(uint64_t seed, float density) {
    IdeaPattern idea;
    int steps = std::max(7, mSequencer.stepCount());
    {
        std::lock_guard<std::mutex> luaLock(mLuaMutex);
        if (!mLua.isReady()) return false;
        if (!mLua.generateIdea(seed, mActiveTraditionStr, mActiveScaleId,
                               mRootSemitone, steps, idea, nullptr, density)) {
            return false;
        }
    }
    std::lock_guard<std::mutex> swapLock(mPatternSwapMutex);
    mPendingIdea = std::move(idea);
    mHasPendingIdea = true;
    return true;
}

void EuterpeEngine::setPercussionKit(int kitId) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetPercussionKit;
    ev.intParam1 = kitId;
    mQueue.push(ev);
}

void EuterpeEngine::toggleCycleStep(int stepIndex) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::ToggleCycleStep;
    ev.intParam1 = stepIndex;
    mQueue.push(ev);
}

void EuterpeEngine::setSwing(float swing) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetSwing;
    ev.floatParam1 = swing;
    mQueue.push(ev);
}

void EuterpeEngine::setDroneLevel(float level) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetDroneLevel;
    ev.floatParam1 = std::clamp(level, 0.0f, 1.0f);
    mQueue.push(ev);
}

void EuterpeEngine::setPercLevel(float level) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetPercLevel;
    ev.floatParam1 = std::clamp(level, 0.0f, 1.0f);
    mQueue.push(ev);
}

void EuterpeEngine::setIdeaLevel(float level) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetIdeaLevel;
    ev.floatParam1 = std::clamp(level, 0.0f, 1.0f);
    mQueue.push(ev);
}

void EuterpeEngine::setBassLevel(float level) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetBassLevel;
    ev.floatParam1 = std::clamp(level, 0.0f, 1.0f);
    mQueue.push(ev);
}

void EuterpeEngine::setReverbMix(float mix) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetReverbMix;
    ev.floatParam1 = std::clamp(mix, 0.0f, 0.85f);
    mQueue.push(ev);
}

void EuterpeEngine::setMasterVolume(float volume) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::SetMasterVolume;
    ev.floatParam1 = std::clamp(volume, 0.0f, 1.2f);
    mQueue.push(ev);
}

void EuterpeEngine::triggerPercussion(int voiceId, float velocity) {
    EngineEvent ev;
    ev.type = EngineEvent::Type::TriggerPercussion;
    ev.intParam1 = voiceId;
    ev.floatParam2 = velocity;
    mQueue.push(ev);
}

void EuterpeEngine::looperToggleRec() {
    EngineEvent ev;
    ev.type = EngineEvent::Type::LooperToggleRec;
    mQueue.push(ev);
}

void EuterpeEngine::looperUndo() {
    EngineEvent ev;
    ev.type = EngineEvent::Type::LooperUndo;
    mQueue.push(ev);
}

void EuterpeEngine::looperClear() {
    EngineEvent ev;
    ev.type = EngineEvent::Type::LooperClear;
    mQueue.push(ev);
}

// ---------------------------------------------------------------------------
// Audio Thread Rendering
// ---------------------------------------------------------------------------

void EuterpeEngine::pushMicrotonalNoteOn(yawn::midi::MidiBuffer& buf,
                                         int midiNote, float cents, float velocity) {
    const uint8_t note = static_cast<uint8_t>(std::clamp(midiNote, 0, 127));
    const uint8_t vel7 = static_cast<uint8_t>(std::clamp(static_cast<int>(velocity * 127.0f), 1, 127));
    const float bendNorm = std::clamp(cents / 200.0f, -1.0f, 1.0f);

    yawn::midi::MidiMessage pb;
    pb.type = yawn::midi::MidiMessage::Type::PitchBend;
    pb.channel = 0;
    pb.value = yawn::midi::Convert::floatToPb32(bendNorm);
    buf.addMessage(pb);

    buf.addMessage(yawn::midi::MidiMessage::noteOn(0, note, vel7, 0));

    // Also emit PerNotePitchBend for Wavetable/Subtractive polyphonic microtuning
    yawn::midi::MidiMessage pnpb;
    pnpb.type = yawn::midi::MidiMessage::Type::PerNotePitchBend;
    pnpb.channel = 0;
    pnpb.note = note;
    pnpb.value = yawn::midi::Convert::floatToPb32(bendNorm);
    buf.addMessage(pnpb);
}

void EuterpeEngine::pushNoteOff(yawn::midi::MidiBuffer& buf, int midiNote) {
    const uint8_t note = static_cast<uint8_t>(std::clamp(midiNote, 0, 127));
    buf.addMessage(yawn::midi::MidiMessage::noteOff(0, note, 0, 0));
}

void EuterpeEngine::processEvents(yawn::midi::MidiBuffer& midiOut) {
    // Non-blocking check for swapped scale / rhythm / idea patterns
    if (mPatternSwapMutex.try_lock()) {
        if (mHasPendingScale) {
            mPerf.setScale(mPendingRoot, mPendingScaleDegrees);
            mHasPendingScale = false;
        }
        if (mHasPendingRhythm) {
            mSequencer.setRhythmPattern(std::move(mPendingRhythm));
            mHasPendingRhythm = false;
        }
        if (mHasPendingIdea) {
            mSequencer.setIdeaPattern(std::move(mPendingIdea));
            mHasPendingIdea = false;
        }
        mPatternSwapMutex.unlock();
    }

    auto onNoteOn = [&](int midi, float cents, float vel) {
        pushMicrotonalNoteOn(midiOut, midi, cents, vel);
    };
    auto onNoteOff = [&](int midi) {
        pushNoteOff(midiOut, midi);
    };
    auto onPerc = [&](int voiceId, float vel) {
        mPerc.trigger(voiceId, vel);
    };

    EngineEvent ev;
    while (mQueue.pop(ev)) {
        switch (ev.type) {
        case EngineEvent::Type::NoteOn:
            mPerf.noteOn(ev.pointerId, ev.intParam1, ev.floatParam1, ev.floatParam2,
                         onNoteOn, onNoteOff, onPerc);
            break;
        case EngineEvent::Type::NoteOff:
            mPerf.noteOff(ev.pointerId, onNoteOff);
            break;
        case EngineEvent::Type::AllNotesOff:
            mPerf.allNotesOff(onNoteOff);
            midiOut.addMessage(yawn::midi::MidiMessage::cc(0, 123, 0, 0));
            break;
        case EngineEvent::Type::SetTradition:
            mPerf.allNotesOff(onNoteOff);
            mPerf.setTradition(static_cast<TraditionId>(ev.intParam1));
            break;
        case EngineEvent::Type::SetRoot:
            mRootSemitone = ((ev.intParam1 % 12) + 12) % 12;
            mPerc.setTonicHz(midiToHz(60 + mRootSemitone));
            break;
        case EngineEvent::Type::SetPerformanceMode:
            mPerf.setPerformanceMode(static_cast<PerformanceMode>(ev.intParam1));
            break;
        case EngineEvent::Type::SetModifierMask: {
            uint32_t mask = static_cast<uint32_t>(ev.intParam1);
            mPerf.setModifierMask(mask);
            // Bit 0 in ArabicMaqam / GreekDromoi / IndianRaga activates continuous Ison / Tanpura drone
            const auto t = mPerf.tradition();
            mDroneFromModifier = (t == TraditionId::ArabicMaqam ||
                                  t == TraditionId::GreekDromoi ||
                                  t == TraditionId::IndianRaga) && ((mask & 1u) != 0);
            break;
        }
        case EngineEvent::Type::SetLatch:
            mPerf.setLatch(ev.intParam1 != 0);
            if (ev.intParam1 == 0) {
                mPerf.allNotesOff(onNoteOff);
            }
            break;
        case EngineEvent::Type::SetInstrument:
            if (ev.intParam1 != mActiveInstrument && ev.intParam1 >= 0 && ev.intParam1 < kInstrumentCount) {
                mPerf.allNotesOff(onNoteOff);
                mInstruments[mActiveInstrument]->reset();
                mActiveInstrument = ev.intParam1;
            }
            break;
        case EngineEvent::Type::SetBpm:
            mSequencer.setBpm(ev.floatParam1);
            mPerf.setBpm(ev.floatParam1);
            break;
        case EngineEvent::Type::SetTransportPlaying:
            mSequencer.setPlaying(ev.intParam1 != 0);
            break;
        case EngineEvent::Type::SetDroneLevel:
            mDroneLevel = ev.floatParam1;
            break;
        case EngineEvent::Type::SetPercLevel:
            mPercLevel = ev.floatParam1;
            break;
        case EngineEvent::Type::SetIdeaLevel:
            mIdeaLevel = ev.floatParam1;
            break;
        case EngineEvent::Type::SetBassLevel:
            mBassLevel = ev.floatParam1;
            break;
        case EngineEvent::Type::SetReverbMix:
            mReverbMix = ev.floatParam1;
            if (mReverb) mReverb->setParameter(2, mReverbMix);
            break;
        case EngineEvent::Type::SetMasterVolume:
            mMasterVolume = ev.floatParam1;
            break;
        case EngineEvent::Type::TriggerPercussion:
            mPerc.trigger(ev.intParam1, ev.floatParam2);
            break;
        case EngineEvent::Type::SetPercussionKit:
            mPerc.setKit(ev.intParam1);
            break;
        case EngineEvent::Type::ToggleCycleStep:
            mSequencer.toggleStep(ev.intParam1, WorldPercussionSynth::DarbukaDum);
            break;
        case EngineEvent::Type::SetSwing:
            mSequencer.setSwingOverride(ev.floatParam1);
            break;
        case EngineEvent::Type::SetSoundPreset:
            mPerf.allNotesOff(onNoteOff);
            applySoundPreset(ev.intParam1);
            break;
        case EngineEvent::Type::SetSynthTone:
            mSynthTone = ev.floatParam1;
            applyToneAndDecayMacros();
            break;
        case EngineEvent::Type::SetSynthDecay:
            mSynthDecay = ev.floatParam1;
            applyToneAndDecayMacros();
            break;
        case EngineEvent::Type::SetSynthDrive:
            mDriveAmount = ev.floatParam1;
            applyToneAndDecayMacros();
            break;
        case EngineEvent::Type::LooperToggleRec:
            mLooper.toggleRecord();
            break;
        case EngineEvent::Type::LooperUndo:
            mLooper.undoLayer();
            break;
        case EngineEvent::Type::LooperClear:
            mLooper.clear();
            break;
        default:
            break;
        }
    }
}

void EuterpeEngine::renderDrone(float* buffer, int numFrames, int numChannels) {
    const float targetGain = std::max(mDroneLevel, mDroneFromModifier ? 0.34f : 0.0f);
    if (targetGain < 0.0005f && mDroneSmoothGain < 0.0005f) return;

    const float rootHz = midiToHz(36 + mRootSemitone); // Low C2..B2 octave
    const float fifthHz = rootHz * 1.498307f;          // Pure 3:2 fifth (Pa)
    const float octHz = rootHz * 2.0f;                 // Octave Sa
    const float invSr = 1.0f / static_cast<float>(mSampleRate);

    for (int i = 0; i < numFrames; ++i) {
        mDroneSmoothGain += (targetGain - mDroneSmoothGain) * 0.0015f;

        mDronePhase1 += kTwoPi * rootHz * invSr;
        if (mDronePhase1 > kTwoPi) mDronePhase1 -= kTwoPi;
        mDronePhase2 += kTwoPi * fifthHz * invSr;
        if (mDronePhase2 > kTwoPi) mDronePhase2 -= kTwoPi;
        mDronePhase3 += kTwoPi * octHz * invSr;
        if (mDronePhase3 > kTwoPi) mDronePhase3 -= kTwoPi;
        mDroneLfoPhase += kTwoPi * 0.35f * invSr;
        if (mDroneLfoPhase > kTwoPi) mDroneLfoPhase -= kTwoPi;

        const float shimmer = 0.85f + 0.15f * std::sin(mDroneLfoPhase);
        // Warm harmonic tanpura/ison timbre (fundamental + 2nd + soft 3rd)
        const float rootSig = std::sin(mDronePhase1) * 0.55f + std::sin(mDronePhase1 * 2.0f) * 0.22f
                              + std::sin(mDronePhase1 * 3.0f) * 0.10f * shimmer;
        const float fifthSig = std::sin(mDronePhase2) * 0.28f + std::sin(mDronePhase2 * 2.0f) * 0.10f;
        const float octSig = std::sin(mDronePhase3) * 0.20f * shimmer;

        const float sampleL = (rootSig + fifthSig * 0.85f + octSig) * mDroneSmoothGain * 0.32f;
        const float sampleR = (rootSig + fifthSig * 1.05f + octSig * 0.9f) * mDroneSmoothGain * 0.32f;

        if (numChannels == 1) {
            buffer[i] += 0.5f * (sampleL + sampleR);
        } else {
            buffer[i * numChannels] += sampleL;
            buffer[i * numChannels + 1] += sampleR;
        }
    }
}

oboe::DataCallbackResult EuterpeEngine::onAudioReady(oboe::AudioStream* stream,
                                                     void* audioData,
                                                     int32_t numFrames) {
    auto* out = static_cast<float*>(audioData);
    const int channels = stream ? stream->getChannelCount() : 2;
    const int frames = std::min(static_cast<int>(numFrames), kMaxFrames);
    std::memset(out, 0, static_cast<size_t>(frames * channels) * sizeof(float));

    yawn::midi::MidiBuffer liveMidi;
    yawn::midi::MidiBuffer ideaMidi;
    yawn::midi::MidiBuffer bassMidi;

    processEvents(liveMidi);

    // Advance realtime strums, arps, ornaments, kotekan, tremolo, gallop, acid roll
    mPerf.advance(frames,
                  [&](int midi, float cents, float vel) { pushMicrotonalNoteOn(liveMidi, midi, cents, vel); },
                  [&](int midi) { pushNoteOff(liveMidi, midi); });

    // Advance asymmetric cycle sequencer (percussion + optional Lua idea & bassline loops)
    mSequencer.advance(frames,
                       [&](int voiceId, float vel) { mPerc.trigger(voiceId, vel); },
                       [&](int midi, float cents, float vel) { pushMicrotonalNoteOn(ideaMidi, midi, cents, vel); },
                       [&](int midi) { pushNoteOff(ideaMidi, midi); },
                       [&](int midi, float cents, float vel) { pushMicrotonalNoteOn(bassMidi, midi, cents, vel); },
                       [&](int midi) { pushNoteOff(bassMidi, midi); });

    // 1. Render active live instrument (+ optional acid/metal overdrive)
    std::fill_n(mMelodicBuf.data(), frames * 2, 0.0f);
    if (mInstruments[mActiveInstrument]) {
        mInstruments[mActiveInstrument]->process(mMelodicBuf.data(), frames, 2, liveMidi);
    }
    if (mDriveAmount > 0.01f) {
        const float preGain = 1.0f + mDriveAmount * 7.5f;
        const float postGain = 1.0f / (1.0f + mDriveAmount * 0.85f);
        for (int i = 0; i < frames * 2; ++i) {
            mMelodicBuf[static_cast<size_t>(i)] = std::tanh(mMelodicBuf[static_cast<size_t>(i)] * preGain) * postGain;
        }
    }

    // 2. Render continuous Ison / Tanpura drone into melodic bus
    renderDrone(mMelodicBuf.data(), frames, 2);

    // 3. Render background Lua Idea Loop
    std::fill_n(mIdeaBuf.data(), frames * 2, 0.0f);
    if (mIdeaSynth && (mSequencer.isIdeaEnabled() || ideaMidi.count() > 0)) {
        mIdeaSynth->process(mIdeaBuf.data(), frames, 2, ideaMidi);
    }
    for (int i = 0; i < frames * 2; ++i) {
        mMelodicBuf[static_cast<size_t>(i)] += mIdeaBuf[static_cast<size_t>(i)] * mIdeaLevel;
    }

    // 3b. Render background Lua Bassline Loop (tight, dry punch)
    std::fill_n(mBassBuf.data(), frames * 2, 0.0f);
    if (mBassSynth && (mSequencer.isBassEnabled() || bassMidi.count() > 0)) {
        mBassSynth->process(mBassBuf.data(), frames, 2, bassMidi);
    }

    // 4. Warm room Reverb on melodic + drone + idea bus
    if (mReverb && mReverbMix > 0.005f) {
        mReverb->process(mMelodicBuf.data(), frames, 2);
    }

    // 4b. Multi-layer phrase looper: records live melodic bus & adds playback layers
    std::array<float, kMaxFrames * 2> loopPlayBuf{};
    mLooper.process(mMelodicBuf.data(), loopPlayBuf.data(), frames, 2, 0.92f);
    for (int i = 0; i < frames * 2; ++i) {
        mMelodicBuf[static_cast<size_t>(i)] += loopPlayBuf[static_cast<size_t>(i)];
    }

    // 5. Render synthesized World Percussion (tight, punchy, mostly dry)
    std::fill_n(mPercBuf.data(), frames * 2, 0.0f);
    mPerc.process(mPercBuf.data(), frames, 2, mPercLevel);

    // 6. Sum to output + master volume + safety limiter
    for (int i = 0; i < frames; ++i) {
        float l = (mMelodicBuf[static_cast<size_t>(i * 2)] +
                   mBassBuf[static_cast<size_t>(i * 2)] * mBassLevel +
                   mPercBuf[static_cast<size_t>(i * 2)]) * mMasterVolume;
        float r = (mMelodicBuf[static_cast<size_t>(i * 2 + 1)] +
                   mBassBuf[static_cast<size_t>(i * 2 + 1)] * mBassLevel +
                   mPercBuf[static_cast<size_t>(i * 2 + 1)]) * mMasterVolume;
        if (channels == 1) {
            out[i] = 0.5f * (l + r);
        } else {
            out[i * channels] = l;
            out[i * channels + 1] = r;
        }
    }

    if (mLimiter && channels == 2) {
        mLimiter->process(out, frames, 2);
    }

    return oboe::DataCallbackResult::Continue;
}

} // namespace euterpe
