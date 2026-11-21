#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace euterpe {

enum class TraditionId : int {
    Western = 0,
    ArabicMaqam = 1,
    GreekDromoi = 2,
    IndianRaga = 3,
    Gamelan = 4,
    Techno = 5,
    Metal = 6,
    Pop = 7,
};

enum class PerformanceMode : int {
    Free = 0,
    Strum = 1,
    Arp = 2,
    Phrase = 3,
};

struct PitchSpec {
    int midiNote = 60;
    float cents = 0.0f;
    float velocity = 0.8f;
};

// Real-time Orchid-style performance layer:
// Translates 2-octave keyboard touches + left-hand tradition modifiers into
// microtonal chords, strums, arps, ornaments, phrases, kotekan, and drones.
class PerformanceLayer {
public:
    using NoteOnCb = std::function<void(int midiNote, float cents, float velocity)>;
    using NoteOffCb = std::function<void(int midiNote)>;
    using PercCb = std::function<void(int voiceId, float velocity)>;

    void init(double sampleRate);
    void reset();

    void setTradition(TraditionId t) { mTradition = t; }
    TraditionId tradition() const { return mTradition; }

    void setPerformanceMode(PerformanceMode mode) { mMode = mode; }
    PerformanceMode performanceMode() const { return mMode; }

    void setModifierMask(uint32_t mask) { mModifierMask = mask; }
    uint32_t modifierMask() const { return mModifierMask; }

    void setLatch(bool latch);
    bool isLatched() const { return mLatch; }

    void setBpm(float bpm);

    // Scale degrees in semitones relative to root (floats allow exact cents).
    void setScale(int rootSemitone, const std::vector<float>& degrees);

    // Compute the voiced pitches for a single key press given current state.
    std::vector<PitchSpec> buildVoicing(int midiNote, float cents, float velocity) const;

    void noteOn(int pointerId, int midiNote, float cents, float velocity,
                const NoteOnCb& onNoteOn, const NoteOffCb& onNoteOff, const PercCb& onPerc);
    void noteOff(int pointerId, const NoteOffCb& onNoteOff);
    void allNotesOff(const NoteOffCb& onNoteOff);

    // Advance scheduled strums, ornaments, phrases, tremolos, arps, and kotekan.
    void advance(int numFrames, const NoteOnCb& onNoteOn, const NoteOffCb& onNoteOff);

private:
    struct ScheduledEvent {
        int samplesRemaining = 0;
        bool isNoteOn = true;
        int midiNote = 60;
        float cents = 0.0f;
        float velocity = 0.8f;
        int pointerId = -1;
    };

    struct HeldTouch {
        int pointerId = -1;
        int rootMidi = 60;
        float rootCents = 0.0f;
        float velocity = 0.8f;
        std::vector<PitchSpec> voiced;
    };

    PitchSpec stepInScale(int baseMidi, float baseCents, int degreeDelta, float velocity) const;
    void releaseTouch(const HeldTouch& touch, const NoteOffCb& onNoteOff);

    double mSampleRate = 48000.0;
    float mBpm = 104.0f;
    TraditionId mTradition = TraditionId::ArabicMaqam;
    PerformanceMode mMode = PerformanceMode::Free;
    uint32_t mModifierMask = 0;
    bool mLatch = false;

    int mRootSemitone = 2; // D
    std::vector<float> mScaleDegrees{0.0f, 1.5f, 3.0f, 5.0f, 7.0f, 8.5f, 10.0f}; // Bayati default

    std::vector<HeldTouch> mHeldTouches;
    std::vector<ScheduledEvent> mScheduled;

    // Arp / Tremolo / Kotekan clock
    double mPatternTimerSamples = 0.0;
    int mPatternStep = 0;
    int mLastPatternNote = -1;
};

} // namespace euterpe
