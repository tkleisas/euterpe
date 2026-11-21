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
    mCommittedLayers = 0;
    mRecordingLayer = -1;
    updateAtomics();
}

void LoopRecorder::updateAtomics() {
    mStateAtomic.store(static_cast<int>(mState), std::memory_order_relaxed);
    mLayerCountAtomic.store(mCommittedLayers, std::memory_order_relaxed);
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

    // Gentle fade-in at start & fade-out at tail so loop seam is click-free
    for (int i = 0; i < fadeFrames; ++i) {
        float g = static_cast<float>(i) / static_cast<float>(fadeFrames);
        buf[static_cast<size_t>(i * 2)] *= g;
        buf[static_cast<size_t>(i * 2 + 1)] *= g;
        int tail = lengthFrames - 1 - i;
        buf[static_cast<size_t>(tail * 2)] *= g;
        buf[static_cast<size_t>(tail * 2 + 1)] *= g;
    }
}

void LoopRecorder::toggleRecord() {
    switch (mState) {
    case State::Empty:
        mLoopLengthFrames = 0;
        mPlayPos = 0;
        mCommittedLayers = 0;
        mRecordingLayer = 0;
        std::fill(mLayers[0].begin(), mLayers[0].end(), 0.0f);
        mState = State::RecordingFirst;
        break;

    case State::RecordingFirst:
        if (mLoopLengthFrames < 256) {
            // Too short, cancel cleanly
            reset();
        } else {
            applySeamCrossfade(mLayers[0], mLoopLengthFrames);
            mCommittedLayers = 1;
            mRecordingLayer = -1;
            mPlayPos = 0;
            mState = State::Playing;
        }
        break;

    case State::Playing:
        if (mCommittedLayers < kMaxLayers) {
            mRecordingLayer = mCommittedLayers;
            std::fill_n(mLayers[static_cast<size_t>(mRecordingLayer)].data(),
                        static_cast<size_t>(mLoopLengthFrames) * 2, 0.0f);
            mState = State::Overdubbing;
        }
        break;

    case State::Overdubbing:
        if (mRecordingLayer >= 0 && mRecordingLayer < kMaxLayers) {
            applySeamCrossfade(mLayers[static_cast<size_t>(mRecordingLayer)], mLoopLengthFrames);
            mCommittedLayers = std::min(kMaxLayers, mRecordingLayer + 1);
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
        // Cancel current overdub layer
        mRecordingLayer = -1;
        mState = (mCommittedLayers > 0) ? State::Playing : State::Empty;
        updateAtomics();
        return;
    }
    if (mCommittedLayers > 1) {
        mCommittedLayers -= 1;
        mState = State::Playing;
    } else {
        reset();
    }
    updateAtomics();
}

void LoopRecorder::clear() {
    reset();
}

void LoopRecorder::process(const float* liveIn, float* mixOut, int numFrames, int numChannels, float playbackGain) {
    if (!liveIn || !mixOut || numFrames <= 0 || numChannels <= 0) return;

    if (mState == State::Empty) {
        return;
    }

    if (mState == State::RecordingFirst) {
        auto& dst = mLayers[0];
        for (int i = 0; i < numFrames; ++i) {
            if (mLoopLengthFrames >= mMaxFrames) {
                // Auto-close loop at max length
                applySeamCrossfade(dst, mLoopLengthFrames);
                mCommittedLayers = 1;
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

    // Playing or Overdubbing: sum all committed layers into mixOut, and if Overdubbing,
    // record liveIn into mLayers[mRecordingLayer] (summing if it wraps around).
    const float layerScale = playbackGain / std::sqrt(static_cast<float>(std::max(1, mCommittedLayers)));

    for (int i = 0; i < numFrames; ++i) {
        const size_t idx = static_cast<size_t>(mPlayPos) * 2;
        float sumL = 0.0f;
        float sumR = 0.0f;
        for (int layer = 0; layer < mCommittedLayers; ++layer) {
            sumL += mLayers[static_cast<size_t>(layer)][idx];
            sumR += mLayers[static_cast<size_t>(layer)][idx + 1];
        }

        if (mState == State::Overdubbing && mRecordingLayer >= 0 && mRecordingLayer < kMaxLayers) {
            float inL = liveIn[i * numChannels];
            float inR = (numChannels > 1) ? liveIn[i * numChannels + 1] : inL;
            auto& recBuf = mLayers[static_cast<size_t>(mRecordingLayer)];
            // Add previous wrap content to output while recording
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
