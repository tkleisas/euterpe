#pragma once

#include <oboe/Oboe.h>

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "CycleSequencer.h"
#include "EventQueue.h"
#include "LoopRecorder.h"
#include "LuaIdeaHost.h"
#include "PerformanceLayer.h"
#include "WorldPercussionSynth.h"
#include "effects/Chorus.h"
#include "effects/Compressor.h"
#include "effects/Delay.h"
#include "effects/Distortion.h"
#include "effects/Limiter.h"
#include "effects/Reverb.h"
#include "instruments/Instrument.h"
#include "midi/MidiTypes.h"

namespace euterpe {

class EuterpeEngine : public oboe::AudioStreamCallback {
public:
    static constexpr int kMaxFrames = 2048;
    static constexpr int kInstrumentCount = 6;

    EuterpeEngine();
    ~EuterpeEngine() override;

    bool init(const std::string& scriptsDir);
    void shutdown();
    void pauseAudio();
    void resumeAudio();

    // Thread-safe UI control methods
    void noteOn(int pointerId, int midiNote, float cents, float velocity);
    void noteOff(int pointerId);
    void allNotesOff();

    void setTradition(int traditionId);
    void setScale(const std::string& scaleId, int rootSemitone);
    void setCustomScale(int rootSemitone, const std::vector<float>& degrees);
    void setRhythm(const std::string& rhythmId);
    void setPerformanceMode(int mode);
    void setModifierMask(int mask);
    void setLatch(bool latch);
    void setInstrument(int instrumentIndex);
    void setSoundPreset(int presetId);
    void setSynthTone(float tone);
    void setSynthDecay(float decay);
    void setSynthDrive(float drive);

    void setBpm(float bpm);
    void setTransportPlaying(bool playing);
    void setIdeaLoopEnabled(bool enabled);
    void setBassLoopEnabled(bool enabled);
    bool mutateIdea(uint64_t seed, float density = -1.0f);

    void setPercussionKit(int kitId);
    void toggleCycleStep(int stepIndex);
    void setSwing(float swing);

    void setDroneLevel(float level);
    void setPercLevel(float level);
    void setIdeaLevel(float level);
    void setBassLevel(float level);
    void setReverbMix(float mix);
    void setInsertFx(int fxId, bool enabled, float amount);
    void setMasterFx(int fxId, bool enabled, float amount);
    void setMasterVolume(float volume);
    void triggerPercussion(int voiceId, float velocity);

    // Multi-layer phrase looper controls
    void looperToggleRec();
    void looperUndo();
    void looperClear();
    int looperState() const { return static_cast<int>(mLooper.state()); }
    int looperLayerCount() const { return mLooper.layerCount(); }
    float looperProgress() const { return mLooper.progress(); }

    // Live UI polling
    int currentStep() const { return mSequencer.currentStep(); }
    int stepCount() const { return mSequencer.stepCount(); }

    // Oboe callback
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* stream,
                                          void* audioData,
                                          int32_t numFrames) override;
    void onErrorAfterClose(oboe::AudioStream* stream, oboe::Result error) override;

private:
    bool openStream();
    void closeStream();
    void configureInstrumentDefaults(int index);
    void applySoundPreset(int presetId);
    void applyToneAndDecayMacros();
    void processEvents(yawn::midi::MidiBuffer& midiOut);
    void pushMicrotonalNoteOn(yawn::midi::MidiBuffer& buf, int midiNote, float cents, float velocity);
    void pushNoteOff(yawn::midi::MidiBuffer& buf, int midiNote);
    void renderDrone(float* buffer, int numFrames, int numChannels);

    std::shared_ptr<oboe::AudioStream> mStream;
    std::mutex mStreamMutex;
    std::mutex mLuaMutex;
    std::mutex mPatternSwapMutex;

    double mSampleRate = 48000.0;
    bool mInitialized = false;

    EventQueue<1024> mQueue;
    PerformanceLayer mPerf;
    CycleSequencer mSequencer;
    WorldPercussionSynth mPerc;
    LuaIdeaHost mLua;
    LoopRecorder mLooper;

    std::array<std::unique_ptr<yawn::instruments::Instrument>, kInstrumentCount> mInstruments;
    std::unique_ptr<yawn::instruments::Instrument> mIdeaSynth;
    std::unique_ptr<yawn::instruments::Instrument> mBassSynth;
    std::unique_ptr<yawn::effects::Distortion> mInsertDist;
    std::unique_ptr<yawn::effects::Chorus> mInsertChorus;
    std::unique_ptr<yawn::effects::Delay> mInsertDelay;
    std::unique_ptr<yawn::effects::Reverb> mReverb;
    std::unique_ptr<yawn::effects::Compressor> mMasterComp;
    std::unique_ptr<yawn::effects::Limiter> mLimiter;

    bool mInsertDistOn = false;
    bool mInsertChorusOn = false;
    bool mInsertDelayOn = false;
    bool mMasterReverbOn = true;
    bool mMasterCompOn = false;

    int mActiveInstrument = 0;
    int mActivePreset = 0;
    float mSynthTone = 0.60f;
    float mSynthDecay = 0.65f;
    float mDriveAmount = 0.0f;
    int mRootSemitone = 2; // D
    std::string mActiveTraditionStr = "arabic";
    std::string mActiveScaleId = "bayati";
    std::string mActiveRhythmId = "maqsum";

    // Mix levels
    float mDroneLevel = 0.0f;
    float mPercLevel = 0.85f;
    float mIdeaLevel = 0.72f;
    float mBassLevel = 0.80f;
    float mReverbMix = 0.24f;
    float mMasterVolume = 0.85f;
    bool mDroneFromModifier = false;

    // Continuous Ison / Tanpura Drone state
    float mDronePhase1 = 0.0f; // Root
    float mDronePhase2 = 0.0f; // Pure 3:2 Fifth (Pa)
    float mDronePhase3 = 0.0f; // Octave
    float mDroneLfoPhase = 0.0f;
    float mDroneSmoothGain = 0.0f;

    // Pending swaps from UI/Lua thread -> audio thread
    bool mHasPendingRhythm = false;
    RhythmPattern mPendingRhythm;
    bool mHasPendingIdea = false;
    IdeaPattern mPendingIdea;
    bool mHasPendingScale = false;
    int mPendingRoot = 2;
    std::vector<float> mPendingScaleDegrees;

    // Scratch buffers (audio thread only)
    std::array<float, kMaxFrames * 2> mMelodicBuf{};
    std::array<float, kMaxFrames * 2> mIdeaBuf{};
    std::array<float, kMaxFrames * 2> mBassBuf{};
    std::array<float, kMaxFrames * 2> mPercBuf{};
};

} // namespace euterpe
