#include "LoopRecorder.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace euterpe {

void LoopRecorder::init(double sampleRate) {
    mSampleRate = sampleRate > 8000.0 ? sampleRate : 48000.0;
    mMaxFrames = static_cast<int>(mSampleRate) * kMaxSeconds;
    for (auto& layer : mLayers) {
        layer.assign(static_cast<size_t>(mMaxFrames) * 2, 0.0f);
    }
    reset();
}

void LoopRecorder::reset() {
    mLoopLengthFrames = 0;
    mPlayPos = 0;
    mState = State::Empty;
    mRecordingLayer = -1;
    mLayerHasAudio.fill(false);
    mLayerMuted.fill(false);
    updateAtomics();
}

void LoopRecorder::updateAtomics() {
    int count = 0;
    int recMask = 0;
    int muteMask = 0;
    for (int i = 0; i < kMaxLayers; ++i) {
        if (mLayerHasAudio[static_cast<size_t>(i)]) {
            count++;
            recMask |= (1 << i);
        }
        if (mLayerMuted[static_cast<size_t>(i)]) {
            muteMask |= (1 << i);
        }
    }
    mStateAtomic.store(static_cast<int>(mState), std::memory_order_relaxed);
    mLayerCountAtomic.store(count, std::memory_order_relaxed);
    mRecordedMaskAtomic.store(recMask, std::memory_order_relaxed);
    mMutedMaskAtomic.store(muteMask, std::memory_order_relaxed);
    float prog = 0.0f;
    if (mState == State::RecordingFirst && mMaxFrames > 0) {
        prog = static_cast<float>(mLoopLengthFrames) / static_cast<float>(mMaxFrames);
    } else if (mLoopLengthFrames > 0) {
        prog = static_cast<float>(mPlayPos) / static_cast<float>(mLoopLengthFrames);
    }
    mProgressAtomic.store(std::clamp(prog, 0.0f, 1.0f), std::memory_order_relaxed);
}

void LoopRecorder::applySeamCrossfade(std::vector<float>& buf, int lengthFrames) {
    if (lengthFrames <= 64) return;
    int fadeFrames = std::min(static_cast<int>(mSampleRate * 0.015), lengthFrames / 4);
    if (fadeFrames <= 1) return;

    for (int i = 0; i < fadeFrames; ++i) {
        float g = static_cast<float>(i) / static_cast<float>(fadeFrames);
        buf[static_cast<size_t>(i * 2)] *= g;
        buf[static_cast<size_t>(i * 2 + 1)] *= g;
        int tail = lengthFrames - 1 - i;
        buf[static_cast<size_t>(tail * 2)] *= g;
        buf[static_cast<size_t>(tail * 2 + 1)] *= g;
    }
}

void LoopRecorder::selectLayer(int layerIndex) {
    mSelectedLayer = std::clamp(layerIndex, 0, kMaxLayers - 1);
}

void LoopRecorder::toggleLayerMute(int layerIndex) {
    if (layerIndex < 0 || layerIndex >= kMaxLayers) return;
    mLayerMuted[static_cast<size_t>(layerIndex)] = !mLayerMuted[static_cast<size_t>(layerIndex)];
    updateAtomics();
}

void LoopRecorder::toggleRecord() {
    switch (mState) {
    case State::Empty: {
        mLoopLengthFrames = 0;
        mPlayPos = 0;
        mRecordingLayer = mSelectedLayer;
        mLayerHasAudio.fill(false);
        mLayerMuted[static_cast<size_t>(mRecordingLayer)] = false;
        std::fill(mLayers[static_cast<size_t>(mRecordingLayer)].begin(),
                  mLayers[static_cast<size_t>(mRecordingLayer)].end(), 0.0f);
        mState = State::RecordingFirst;
        break;
    }

    case State::RecordingFirst:
        if (mLoopLengthFrames < 256 || mRecordingLayer < 0) {
            reset();
        } else {
            applySeamCrossfade(mLayers[static_cast<size_t>(mRecordingLayer)], mLoopLengthFrames);
            mLayerHasAudio[static_cast<size_t>(mRecordingLayer)] = true;
            mRecordingLayer = -1;
            mPlayPos = 0;
            mState = State::Playing;
        }
        break;

    case State::Playing: {
        int slot = mSelectedLayer;
        // If selected slot already has audio, auto-advance to first free slot if available;
        // otherwise re-record into the selected slot cleanly.
        if (mLayerHasAudio[static_cast<size_t>(slot)]) {
            for (int i = 0; i < kMaxLayers; ++i) {
                if (!mLayerHasAudio[static_cast<size_t>(i)]) {
                    slot = i;
                    mSelectedLayer = i;
                    break;
                }
            }
        }
        mRecordingLayer = slot;
        mLayerHasAudio[static_cast<size_t>(slot)] = false;
        mLayerMuted[static_cast<size_t>(slot)] = false;
        std::fill_n(mLayers[static_cast<size_t>(slot)].data(),
                    static_cast<size_t>(mLoopLengthFrames) * 2, 0.0f);
        mState = State::Overdubbing;
        break;
    }

    case State::Overdubbing:
        if (mRecordingLayer >= 0 && mRecordingLayer < kMaxLayers) {
            applySeamCrossfade(mLayers[static_cast<size_t>(mRecordingLayer)], mLoopLengthFrames);
            mLayerHasAudio[static_cast<size_t>(mRecordingLayer)] = true;
            mRecordingLayer = -1;
        }
        mState = State::Playing;
        break;
    }
    updateAtomics();
}

void LoopRecorder::undoLayer() {
    if (mState == State::RecordingFirst) {
        reset();
        return;
    }
    if (mState == State::Overdubbing) {
        mRecordingLayer = -1;
        bool any = false;
        for (bool b : mLayerHasAudio) if (b) any = true;
        mState = any ? State::Playing : State::Empty;
        updateAtomics();
        return;
    }
    // Clear selected layer if it has audio; otherwise clear the highest-index recorded layer
    if (mLayerHasAudio[static_cast<size_t>(mSelectedLayer)]) {
        mLayerHasAudio[static_cast<size_t>(mSelectedLayer)] = false;
        mLayerMuted[static_cast<size_t>(mSelectedLayer)] = false;
    } else {
        for (int i = kMaxLayers - 1; i >= 0; --i) {
            if (mLayerHasAudio[static_cast<size_t>(i)]) {
                mLayerHasAudio[static_cast<size_t>(i)] = false;
                mLayerMuted[static_cast<size_t>(i)] = false;
                break;
            }
        }
    }
    bool any = false;
    for (bool b : mLayerHasAudio) if (b) any = true;
    if (!any) {
        reset();
    } else {
        mState = State::Playing;
        updateAtomics();
    }
}

void LoopRecorder::clear() {
    reset();
}

void LoopRecorder::process(const float* liveIn, float* mixOut, int numFrames, int numChannels, float playbackGain) {
    if (!liveIn || !mixOut || numFrames <= 0 || numChannels <= 0) return;

    if (mState == State::Empty) {
        return;
    }

    if (mState == State::RecordingFirst && mRecordingLayer >= 0) {
        auto& dst = mLayers[static_cast<size_t>(mRecordingLayer)];
        for (int i = 0; i < numFrames; ++i) {
            if (mLoopLengthFrames >= mMaxFrames) {
                applySeamCrossfade(dst, mLoopLengthFrames);
                mLayerHasAudio[static_cast<size_t>(mRecordingLayer)] = true;
                mRecordingLayer = -1;
                mPlayPos = 0;
                mState = State::Playing;
                break;
            }
            float l = liveIn[i * numChannels];
            float r = (numChannels > 1) ? liveIn[i * numChannels + 1] : l;
            dst[static_cast<size_t>(mLoopLengthFrames * 2)] = l;
            dst[static_cast<size_t>(mLoopLengthFrames * 2 + 1)] = r;
            mLoopLengthFrames++;
        }
        updateAtomics();
        return;
    }

    if (mLoopLengthFrames <= 0) return;

    int activePlayLayers = 0;
    for (int l = 0; l < kMaxLayers; ++l) {
        if (mLayerHasAudio[static_cast<size_t>(l)] && !mLayerMuted[static_cast<size_t>(l)]) {
            activePlayLayers++;
        }
    }
    const float layerScale = playbackGain / std::sqrt(static_cast<float>(std::max(1, activePlayLayers)));

    for (int i = 0; i < numFrames; ++i) {
        const size_t idx = static_cast<size_t>(mPlayPos) * 2;
        float sumL = 0.0f;
        float sumR = 0.0f;
        for (int layer = 0; layer < kMaxLayers; ++layer) {
            if (mLayerHasAudio[static_cast<size_t>(layer)] && !mLayerMuted[static_cast<size_t>(layer)]) {
                sumL += mLayers[static_cast<size_t>(layer)][idx];
                sumR += mLayers[static_cast<size_t>(layer)][idx + 1];
            }
        }

        if (mState == State::Overdubbing && mRecordingLayer >= 0 && mRecordingLayer < kMaxLayers) {
            float inL = liveIn[i * numChannels];
            float inR = (numChannels > 1) ? liveIn[i * numChannels + 1] : inL;
            auto& recBuf = mLayers[static_cast<size_t>(mRecordingLayer)];
            sumL += recBuf[idx];
            sumR += recBuf[idx + 1];
            recBuf[idx] = std::clamp(recBuf[idx] + inL, -1.5f, 1.5f);
            recBuf[idx + 1] = std::clamp(recBuf[idx + 1] + inR, -1.5f, 1.5f);
        }

        if (numChannels == 1) {
            mixOut[i] += 0.5f * (sumL + sumR) * layerScale;
        } else {
            mixOut[i * numChannels] += sumL * layerScale;
            mixOut[i * numChannels + 1] += sumR * layerScale;
        }

        mPlayPos++;
        if (mPlayPos >= mLoopLengthFrames) {
            mPlayPos = 0;
        }
    }
    updateAtomics();
}

} // namespace euterpe
