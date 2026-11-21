#include "CycleSequencer.h"

#include <algorithm>

namespace euterpe {

void CycleSequencer::init(double sampleRate) {
    mSampleRate = sampleRate > 8000.0 ? sampleRate : 48000.0;
    reset();
}

void CycleSequencer::reset() {
    mCurrentStep = 0;
    mIdeaStep = 0;
    mSamplesUntilNextStep = 0.0;
    mJustStarted = true;
    mActiveIdeaNotes.clear();
    mActiveBassNotes.clear();
    mCurrentStepAtomic.store(0, std::memory_order_relaxed);
}

void CycleSequencer::setBpm(float bpm) {
    mBpm = std::clamp(bpm, 35.0f, 240.0f);
}

void CycleSequencer::setPlaying(bool playing) {
    if (playing && !mPlaying) {
        mCurrentStep = 0;
        mIdeaStep = 0;
        mSamplesUntilNextStep = 0.0;
        mJustStarted = true;
    }
    mPlaying = playing;
}

void CycleSequencer::setRhythmPattern(RhythmPattern pattern) {
    if (pattern.stepCount < 1) pattern.stepCount = 1;
    if (pattern.stepsPerBeat < 0.5f) pattern.stepsPerBeat = 2.0f;
    pattern.steps.resize(static_cast<size_t>(pattern.stepCount));
    mRhythm = std::move(pattern);
    if (mCurrentStep >= mRhythm.stepCount) {
        mCurrentStep = 0;
    }
    mStepCountAtomic.store(mRhythm.stepCount, std::memory_order_relaxed);
}

void CycleSequencer::setIdeaPattern(IdeaPattern pattern) {
    if (pattern.stepCount < 1) pattern.stepCount = 16;
    mIdea = std::move(pattern);
    if (mIdeaStep >= mIdea.stepCount) {
        mIdeaStep = 0;
    }
}

void CycleSequencer::toggleStep(int stepIndex, int defaultVoiceId) {
    if (stepIndex < 0 || stepIndex >= static_cast<int>(mRhythm.steps.size())) return;
    auto& hits = mRhythm.steps[static_cast<size_t>(stepIndex)];
    if (!hits.empty()) {
        hits.clear();
    } else {
        hits.push_back({defaultVoiceId, (stepIndex == 0) ? 0.95f : 0.82f});
    }
}

double CycleSequencer::samplesForStep(int stepIndex) const {
    const double beatsPerSecond = static_cast<double>(mBpm) / 60.0;
    const double stepsPerSecond = beatsPerSecond * static_cast<double>(mRhythm.stepsPerBeat);
    double baseSamples = mSampleRate / std::max(0.5, stepsPerSecond);
    const float swingVal = (mSwingOverride >= 0.0f) ? mSwingOverride : mRhythm.swing;
    if (swingVal > 0.001f) {
        const double swingAmt = std::clamp(static_cast<double>(swingVal), 0.0, 0.35);
        if ((stepIndex & 1) == 0) {
            baseSamples *= (1.0 + swingAmt);
        } else {
            baseSamples *= (1.0 - swingAmt);
        }
    }
    return std::max(64.0, baseSamples);
}

void CycleSequencer::triggerStep(int stepIndex,
                                 const PercCallback& onPerc,
                                 const NoteOnCallback& onIdeaNoteOn,
                                 const NoteOffCallback& onIdeaNoteOff,
                                 const NoteOnCallback& onBassNoteOn,
                                 const NoteOffCallback& onBassNoteOff) {
    mCurrentStepAtomic.store(stepIndex, std::memory_order_relaxed);

    // 1. Trigger percussion hits for this step
    if (onPerc && stepIndex >= 0 && stepIndex < static_cast<int>(mRhythm.steps.size())) {
        for (const auto& hit : mRhythm.steps[static_cast<size_t>(stepIndex)]) {
            onPerc(hit.voiceId, hit.velocity);
        }
    }

    // 2. Release expired idea & bass notes
    for (auto it = mActiveIdeaNotes.begin(); it != mActiveIdeaNotes.end();) {
        it->remainingSteps -= 1;
        if (it->remainingSteps <= 0) {
            if (onIdeaNoteOff) onIdeaNoteOff(it->midiNote);
            it = mActiveIdeaNotes.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = mActiveBassNotes.begin(); it != mActiveBassNotes.end();) {
        it->remainingSteps -= 1;
        if (it->remainingSteps <= 0) {
            if (onBassNoteOff) onBassNoteOff(it->midiNote);
            it = mActiveBassNotes.erase(it);
        } else {
            ++it;
        }
    }

    // 3. Trigger new idea notes if enabled
    if (mIdeaEnabled && onIdeaNoteOn && mIdea.stepCount > 0) {
        for (const auto& note : mIdea.notes) {
            if (note.step == mIdeaStep) {
                onIdeaNoteOn(note.midiNote, note.cents, note.velocity);
                mActiveIdeaNotes.push_back({note.midiNote, std::max(1, note.durationSteps)});
            }
        }
    }

    // 4. Trigger new bassline notes if enabled
    if (mBassEnabled && onBassNoteOn && mIdea.stepCount > 0) {
        for (const auto& note : mIdea.bassNotes) {
            if (note.step == mIdeaStep) {
                onBassNoteOn(note.midiNote, note.cents, note.velocity);
                mActiveBassNotes.push_back({note.midiNote, std::max(1, note.durationSteps)});
            }
        }
    }
}

void CycleSequencer::advance(int numFrames,
                             const PercCallback& onPerc,
                             const NoteOnCallback& onIdeaNoteOn,
                             const NoteOffCallback& onIdeaNoteOff,
                             const NoteOnCallback& onBassNoteOn,
                             const NoteOffCallback& onBassNoteOff) {
    if (!mPlaying) {
        if (!mActiveIdeaNotes.empty()) {
            for (const auto& n : mActiveIdeaNotes) {
                if (onIdeaNoteOff) onIdeaNoteOff(n.midiNote);
            }
            mActiveIdeaNotes.clear();
        }
        if (!mActiveBassNotes.empty()) {
            for (const auto& n : mActiveBassNotes) {
                if (onBassNoteOff) onBassNoteOff(n.midiNote);
            }
            mActiveBassNotes.clear();
        }
        return;
    }

    if (mRhythm.stepCount <= 0) return;

    double remaining = static_cast<double>(numFrames);
    while (remaining > 0.0) {
        if (mJustStarted) {
            mJustStarted = false;
            triggerStep(mCurrentStep, onPerc, onIdeaNoteOn, onIdeaNoteOff, onBassNoteOn, onBassNoteOff);
            mSamplesUntilNextStep = samplesForStep(mCurrentStep);
        }

        if (remaining >= mSamplesUntilNextStep) {
            remaining -= mSamplesUntilNextStep;
            mCurrentStep = (mCurrentStep + 1) % mRhythm.stepCount;
            if (mIdea.stepCount > 0) {
                mIdeaStep = (mIdeaStep + 1) % mIdea.stepCount;
            }
            triggerStep(mCurrentStep, onPerc, onIdeaNoteOn, onIdeaNoteOff, onBassNoteOn, onBassNoteOff);
            mSamplesUntilNextStep = samplesForStep(mCurrentStep);
        } else {
            mSamplesUntilNextStep -= remaining;
            remaining = 0.0;
        }
    }
}

} // namespace euterpe
