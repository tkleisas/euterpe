#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace euterpe {

// Pre-allocated multi-layer stereo audio looper (up to 4 independent overdub layers).
// Records the live melodic/phrase bus so the user can layer different instruments,
// scales, or phrases, undo the last layer, or clear cleanly with zero clicks.
class LoopRecorder {
public:
    static constexpr int kMaxLayers = 8;
    static constexpr int kMaxSeconds = 24; // Up to 8 independent multi-bar instrument layers

    enum class State : int {
        Empty = 0,
        RecordingFirst = 1,
        Playing = 2,
        Overdubbing = 3,
    };

    void init(double sampleRate);
    void reset();

    // Toggle record / overdub:
    // - Empty -> RecordingFirst
    // - RecordingFirst -> Playing (locks loop length + applies crossfade)
    // - Playing -> Overdubbing (starts next layer if < kMaxLayers)
    // - Overdubbing -> Playing (commits overdub layer)
    void toggleRecord();

    // Remove the most recently recorded layer (or clear if only 1 layer).
    void undoLayer();

    // Clear all layers and stop playback.
    void clear();

    // Process one stereo interleaved audio block:
    // `liveIn` is the live melodic bus to record; `mixOut` receives summed loop playback.
    void process(const float* liveIn, float* mixOut, int numFrames, int numChannels, float playbackGain = 1.0f);

    State state() const { return static_cast<State>(mStateAtomic.load(std::memory_order_relaxed)); }
    int layerCount() const { return mLayerCountAtomic.load(std::memory_order_relaxed); }
    float progress() const { return mProgressAtomic.load(std::memory_order_relaxed); }

private:
    void applySeamCrossfade(std::vector<float>& buf, int lengthFrames);
    void updateAtomics();

    double mSampleRate = 48000.0;
    int mMaxFrames = 48000 * kMaxSeconds;
    int mLoopLengthFrames = 0;
    int mPlayPos = 0;

    State mState = State::Empty;
    int mCommittedLayers = 0;
    int mRecordingLayer = -1;

    std::array<std::vector<float>, kMaxLayers> mLayers;

    std::atomic<int> mStateAtomic{0};
    std::atomic<int> mLayerCountAtomic{0};
    std::atomic<float> mProgressAtomic{0.0f};
};

} // namespace euterpe
