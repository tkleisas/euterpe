#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <vector>

namespace euterpe {

struct PercHit {
    int voiceId = 0;
    float velocity = 0.8f;
};

struct RhythmPattern {
    int stepCount = 16;          // e.g. 16 (4/4), 14 (7/8), 18 (9/8), 20 (10/8), 16 (Teentaal)
    float stepsPerBeat = 4.0f;   // 4.0 = 16th notes, 2.0 = 8th notes
    float swing = 0.0f;          // 0.0 .. 0.35
    std::vector<std::vector<PercHit>> steps;
};

struct IdeaNote {
    int step = 0;
    int durationSteps = 2;
    int midiNote = 60;
    float cents = 0.0f;
    float velocity = 0.78f;
};

struct IdeaPattern {
    int stepCount = 16;
    std::vector<IdeaNote> notes;
    std::vector<IdeaNote> bassNotes;
};

// Sample-accurate asymmetric cycle clock driving both world percussion and
// optional generated melodic idea loops.
class CycleSequencer {
public:
    using PercCallback = std::function<void(int voiceId, float velocity)>;
    using NoteOnCallback = std::function<void(int midiNote, float cents, float velocity)>;
    using NoteOffCallback = std::function<void(int midiNote)>;

    void init(double sampleRate);
    void reset();

    void setBpm(float bpm);
    float bpm() const { return mBpm; }

    void setPlaying(bool playing);
    bool isPlaying() const { return mPlaying; }

    void setRhythmPattern(RhythmPattern pattern);
    void toggleStep(int stepIndex, int defaultVoiceId = 0);
    void setSwingOverride(float swing) { mSwingOverride = swing; }
    void setIdeaPattern(IdeaPattern pattern);
    void setIdeaEnabled(bool enabled) { mIdeaEnabled = enabled; }
    bool isIdeaEnabled() const { return mIdeaEnabled; }
    void setBassEnabled(bool enabled) { mBassEnabled = enabled; }
    bool isBassEnabled() const { return mBassEnabled; }

    // Advance by `numFrames` samples, invoking callbacks on step boundaries.
    void advance(int numFrames,
                 const PercCallback& onPerc,
                 const NoteOnCallback& onIdeaNoteOn,
                 const NoteOffCallback& onIdeaNoteOff,
                 const NoteOnCallback& onBassNoteOn = nullptr,
                 const NoteOffCallback& onBassNoteOff = nullptr);

    int currentStep() const { return mCurrentStepAtomic.load(std::memory_order_relaxed); }
    int stepCount() const { return mStepCountAtomic.load(std::memory_order_relaxed); }

private:
    struct ActiveIdeaNote {
        int midiNote = 60;
        int remainingSteps = 0;
    };

    double samplesForStep(int stepIndex) const;
    void triggerStep(int stepIndex,
                     const PercCallback& onPerc,
                     const NoteOnCallback& onIdeaNoteOn,
                     const NoteOffCallback& onIdeaNoteOff,
                     const NoteOnCallback& onBassNoteOn,
                     const NoteOffCallback& onBassNoteOff);

    double mSampleRate = 48000.0;
    float mBpm = 104.0f;
    float mSwingOverride = -1.0f; // < 0 uses pattern's native swing
    bool mPlaying = false;
    bool mIdeaEnabled = false;
    bool mBassEnabled = false;

    RhythmPattern mRhythm;
    IdeaPattern mIdea;

    int mCurrentStep = 0;
    int mIdeaStep = 0;
    double mSamplesUntilNextStep = 0.0;
    bool mJustStarted = true;

    std::vector<ActiveIdeaNote> mActiveIdeaNotes;
    std::vector<ActiveIdeaNote> mActiveBassNotes;
    std::atomic<int> mCurrentStepAtomic{0};
    std::atomic<int> mStepCountAtomic{16};
};

} // namespace euterpe
