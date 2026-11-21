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

    // Toggle record / overdub into the currently selected layer slot (0..7):
    void toggleRecord();

    // Select active layer slot (0..7) for recording/overdubbing.
    void selectLayer(int layerIndex);

    // Mute or unmute a specific layer (0..7).
    void toggleLayerMute(int layerIndex);

    // Clear the currently selected layer (or last recorded layer).
    void undoLayer();

    // Clear all layers and stop playback.
    void clear();

    // Process one stereo interleaved audio block:
    // `liveIn` is the live melodic bus to record; `mixOut` receives summed loop playback.
    void process(const float* liveIn, float* mixOut, int numFrames, int numChannels, float playbackGain = 1.0f);

    State state() const { return static_cast<State>(mStateAtomic.load(std::memory_order_relaxed)); }
    int layerCount() const { return mLayerCountAtomic.load(std::memory_order_relaxed); }
    int recordedMask() const { return mRecordedMaskAtomic.load(std::memory_order_relaxed); }
    int mutedMask() const { return mMutedMaskAtomic.load(std::memory_order_relaxed); }
    float progress() const { return mProgressAtomic.load(std::memory_order_relaxed); }

private:
    void applySeamCrossfade(std::vector<float>& buf, int lengthFrames);
    void updateAtomics();

    double mSampleRate = 48000.0;
    int mMaxFrames = 48000 * kMaxSeconds;
    int mLoopLengthFrames = 0;
    int mPlayPos = 0;

    State mState = State::Empty;
    int mSelectedLayer = 0;
    int mRecordingLayer = -1;
    std::array<bool, kMaxLayers> mLayerHasAudio{};
    std::array<bool, kMaxLayers> mLayerMuted{};

    std::array<std::vector<float>, kMaxLayers> mLayers;

    std::atomic<int> mStateAtomic{0};
    std::atomic<int> mLayerCountAtomic{0};
    std::atomic<int> mRecordedMaskAtomic{0};
    std::atomic<int> mMutedMaskAtomic{0};
    std::atomic<float> mProgressAtomic{0.0f};
};

} // namespace euterpe
