#include "PerformanceLayer.h"
#include "WorldPercussionSynth.h"

#include <algorithm>
#include <cmath>

namespace euterpe {

namespace {
PitchSpec fromContinuousSemitone(float semis, float velocity) {
    int midi = static_cast<int>(std::floor(semis + 0.5f));
    midi = std::clamp(midi, 12, 120);
    float cents = (semis - static_cast<float>(midi)) * 100.0f;
    return {midi, cents, velocity};
}
} // namespace

void PerformanceLayer::init(double sampleRate) {
    mSampleRate = sampleRate > 8000.0 ? sampleRate : 48000.0;
    reset();
}

void PerformanceLayer::reset() {
    mHeldTouches.clear();
    mScheduled.clear();
    mPatternTimerSamples = 0.0;
    mPatternStep = 0;
    mLastPatternNote = -1;
}

void PerformanceLayer::setBpm(float bpm) {
    mBpm = std::clamp(bpm, 35.0f, 240.0f);
}

void PerformanceLayer::setScale(int rootSemitone, const std::vector<float>& degrees) {
    mRootSemitone = ((rootSemitone % 12) + 12) % 12;
    if (!degrees.empty()) {
        mScaleDegrees = degrees;
    }
}

void PerformanceLayer::setLatch(bool latch) {
    mLatch = latch;
}

PitchSpec PerformanceLayer::stepInScale(int baseMidi, float baseCents, int degreeDelta, float velocity) const {
    if (mScaleDegrees.empty()) {
        return fromContinuousSemitone(static_cast<float>(baseMidi) + baseCents / 100.0f + degreeDelta, velocity);
    }

    const float absSemis = static_cast<float>(baseMidi) + baseCents / 100.0f;
    // Find nearest scale degree index to absSemis
    const int n = static_cast<int>(mScaleDegrees.size());
    float relToRoot = absSemis - static_cast<float>(mRootSemitone);
    int baseOct = static_cast<int>(std::floor(relToRoot / 12.0f));
    float inOct = relToRoot - static_cast<float>(baseOct * 12);

    int bestIdx = 0;
    float bestDist = 100.0f;
    for (int i = 0; i < n; ++i) {
        float d = std::fabs(mScaleDegrees[static_cast<size_t>(i)] - inOct);
        if (d < bestDist) {
            bestDist = d;
            bestIdx = i;
        }
    }

    int targetTotal = baseOct * n + bestIdx + degreeDelta;
    int targetOct = static_cast<int>(std::floor(static_cast<float>(targetTotal) / static_cast<float>(n)));
    int targetIdx = targetTotal - targetOct * n;
    float targetSemi = static_cast<float>(mRootSemitone) + static_cast<float>(targetOct * 12)
                       + mScaleDegrees[static_cast<size_t>(targetIdx)];
    return fromContinuousSemitone(targetSemi, velocity);
}

std::vector<PitchSpec> PerformanceLayer::buildVoicing(int midiNote, float cents, float velocity) const {
    std::vector<PitchSpec> notes;
    const float baseSemi = static_cast<float>(midiNote) + cents / 100.0f;

    if (mTradition == TraditionId::Western) {
        const bool maj = (mModifierMask & (1u << 0)) != 0;
        const bool min = (mModifierMask & (1u << 1)) != 0;
        const bool dim = (mModifierMask & (1u << 2)) != 0;
        const bool aug = (mModifierMask & (1u << 3)) != 0;
        const bool add7 = (mModifierMask & (1u << 4)) != 0;
        const bool add9 = (mModifierMask & (1u << 5)) != 0;
        const bool sus4 = (mModifierMask & (1u << 6)) != 0;
        const bool invert = (mModifierMask & (1u << 7)) != 0;

        if (!maj && !min && !dim && !aug && !add7 && !add9 && !sus4) {
            notes.push_back({midiNote, cents, velocity});
            return notes;
        }

        std::vector<float> intervals;
        intervals.push_back(0.0f);
        if (sus4) {
            intervals.push_back(5.0f);
        } else if (dim) {
            intervals.push_back(3.0f);
        } else if (min) {
            intervals.push_back(3.0f);
        } else {
            intervals.push_back(4.0f); // Maj or Aug
        }

        if (dim) intervals.push_back(6.0f);
        else if (aug) intervals.push_back(8.0f);
        else intervals.push_back(7.0f);

        if (add7) intervals.push_back((min || dim) ? 10.0f : 11.0f);
        if (add9) intervals.push_back(14.0f);

        if (invert && intervals.size() >= 3) {
            intervals[0] += 12.0f; // 1st inversion
            notes.push_back(fromContinuousSemitone(baseSemi - 12.0f, velocity * 0.72f)); // Bass
        }
        for (float iv : intervals) {
            notes.push_back(fromContinuousSemitone(baseSemi + iv, velocity * 0.85f));
        }
        return notes;
    }

    if (mTradition == TraditionId::ArabicMaqam || mTradition == TraditionId::GreekDromoi) {
        const bool oct5 = (mModifierMask & (1u << 1)) != 0;
        const bool modalChord = (mModifierMask & (1u << 2)) != 0;
        const bool jinsShift = (mModifierMask & (1u << 5)) != 0;
        const float shiftCents = jinsShift ? 50.0f : 0.0f;

        notes.push_back(fromContinuousSemitone(baseSemi + shiftCents / 100.0f, velocity));
        if (oct5) {
            notes.push_back(fromContinuousSemitone(baseSemi - 12.0f, velocity * 0.72f));
            notes.push_back(fromContinuousSemitone(baseSemi + 7.0f, velocity * 0.65f));
        }
        if (modalChord) {
            notes.push_back(stepInScale(midiNote, cents, 2, velocity * 0.78f));
            notes.push_back(stepInScale(midiNote, cents, 4, velocity * 0.75f));
        }
        return notes;
    }

    if (mTradition == TraditionId::IndianRaga) {
        const bool octave = (mModifierMask & (1u << 4)) != 0;
        const bool fifth = (mModifierMask & (1u << 5)) != 0;
        notes.push_back({midiNote, cents, velocity});
        if (octave) notes.push_back(fromContinuousSemitone(baseSemi - 12.0f, velocity * 0.68f));
        if (fifth) notes.push_back(fromContinuousSemitone(baseSemi + 7.0196f, velocity * 0.64f)); // Pure 3:2 Pa
        return notes;
    }

    if (mTradition == TraditionId::Gamelan) {
        const bool balungan = (mModifierMask & (1u << 0)) != 0;
        const bool ombak = (mModifierMask & (1u << 2)) != 0;
        const bool kempyung = (mModifierMask & (1u << 4)) != 0;
        const bool gembyang = (mModifierMask & (1u << 5)) != 0;

        notes.push_back({midiNote, cents, velocity});
        if (balungan || gembyang) {
            notes.push_back(fromContinuousSemitone(baseSemi - 12.0f, velocity * 0.78f));
        }
        if (gembyang) {
            notes.push_back(fromContinuousSemitone(baseSemi + 12.0f, velocity * 0.68f));
        }
        if (kempyung) {
            // Javanese kempyung: 3 scale steps above in slendro/pelog
            notes.push_back(stepInScale(midiNote, cents, 3, velocity * 0.76f));
        }
        if (ombak) {
            // Paired detuned shimmer (~14 cents beating)
            notes.push_back(fromContinuousSemitone(baseSemi + 0.14f, velocity * 0.72f));
        }
        return notes;
    }

    if (mTradition == TraditionId::Techno) {
        const bool acidStab = (mModifierMask & (1u << 0)) != 0;
        const bool subOct = (mModifierMask & (1u << 1)) != 0;
        const bool min7 = (mModifierMask & (1u << 2)) != 0;
        const bool fifth = (mModifierMask & (1u << 4)) != 0;
        const bool highOct = (mModifierMask & (1u << 5)) != 0;

        notes.push_back({midiNote, cents, velocity});
        if (acidStab) {
            notes.push_back(fromContinuousSemitone(baseSemi + 3.0f, velocity * 0.80f));
            notes.push_back(fromContinuousSemitone(baseSemi + 7.0f, velocity * 0.80f));
        }
        if (min7) {
            notes.push_back(fromContinuousSemitone(baseSemi + 10.0f, velocity * 0.76f));
        }
        if (subOct) notes.push_back(fromContinuousSemitone(baseSemi - 12.0f, velocity * 0.85f));
        if (fifth) notes.push_back(fromContinuousSemitone(baseSemi + 7.0f, velocity * 0.78f));
        if (highOct) notes.push_back(fromContinuousSemitone(baseSemi + 12.0f, velocity * 0.72f));
        return notes;
    }

    if (mTradition == TraditionId::Metal) {
        const bool power5 = (mModifierMask & (1u << 0)) != 0;
        const bool dropOct = (mModifierMask & (1u << 1)) != 0;
        const bool tritone = (mModifierMask & (1u << 3)) != 0;
        const bool highOct = (mModifierMask & (1u << 5)) != 0;

        notes.push_back({midiNote, cents, velocity});
        if (power5) {
            notes.push_back(fromContinuousSemitone(baseSemi + 7.0f, velocity * 0.90f));
            notes.push_back(fromContinuousSemitone(baseSemi + 12.0f, velocity * 0.78f));
        }
        if (dropOct) notes.push_back(fromContinuousSemitone(baseSemi - 12.0f, velocity * 0.88f));
        if (tritone) notes.push_back(fromContinuousSemitone(baseSemi + 6.0f, velocity * 0.85f));
        if (highOct) notes.push_back(fromContinuousSemitone(baseSemi + 12.0f, velocity * 0.75f));
        return notes;
    }

    if (mTradition == TraditionId::Pop) {
        const bool maj = (mModifierMask & (1u << 0)) != 0;
        const bool min = (mModifierMask & (1u << 1)) != 0;
        const bool sus2 = (mModifierMask & (1u << 2)) != 0;
        const bool add9 = (mModifierMask & (1u << 3)) != 0;
        const bool add7 = (mModifierMask & (1u << 4)) != 0;
        const bool octBass = (mModifierMask & (1u << 5)) != 0;

        notes.push_back({midiNote, cents, velocity});
        if (maj || min || sus2) {
            notes.push_back(fromContinuousSemitone(baseSemi + (sus2 ? 2.0f : (min ? 3.0f : 4.0f)), velocity * 0.82f));
            notes.push_back(fromContinuousSemitone(baseSemi + 7.0f, velocity * 0.82f));
        }
        if (add7) notes.push_back(fromContinuousSemitone(baseSemi + (min ? 10.0f : 11.0f), velocity * 0.75f));
        if (add9) notes.push_back(fromContinuousSemitone(baseSemi + 14.0f, velocity * 0.72f));
        if (octBass) notes.push_back(fromContinuousSemitone(baseSemi - 12.0f, velocity * 0.82f));
        return notes;
    }

    notes.push_back({midiNote, cents, velocity});
    return notes;
}

void PerformanceLayer::releaseTouch(const HeldTouch& touch, const NoteOffCb& onNoteOff) {
    if (!onNoteOff) return;
    for (const auto& n : touch.voiced) {
        onNoteOff(n.midiNote);
    }
}

void PerformanceLayer::noteOn(int pointerId, int midiNote, float cents, float velocity,
                              const NoteOnCb& onNoteOn, const NoteOffCb& onNoteOff, const PercCb& onPerc) {
    // If latched and pressing a new note, clear previous latched touches so harmonies stay clean
    if (mLatch && !mHeldTouches.empty()) {
        for (const auto& t : mHeldTouches) releaseTouch(t, onNoteOff);
        mHeldTouches.clear();
    }

    // Remove any existing touch with same pointerId
    for (auto it = mHeldTouches.begin(); it != mHeldTouches.end();) {
        if (it->pointerId == pointerId) {
            releaseTouch(*it, onNoteOff);
            it = mHeldTouches.erase(it);
        } else {
            ++it;
        }
    }

    auto voiced = buildVoicing(midiNote, cents, velocity);
    mHeldTouches.push_back({pointerId, midiNote, cents, velocity, voiced});

    // Gamelan Gong punctuation modifier (bit 3)
    if (mTradition == TraditionId::Gamelan && (mModifierMask & (1u << 3)) && onPerc) {
        onPerc(midiNote < 54 ? WorldPercussionSynth::Kempul : WorldPercussionSynth::Kenong, velocity * 0.85f);
    }

    // Phrase Mode or Raga Taan (bit 3) or Maqam Ornament (bit 3)
    const bool maqamOrnament = (mTradition == TraditionId::ArabicMaqam || mTradition == TraditionId::GreekDromoi)
                               && ((mModifierMask & (1u << 3)) != 0);
    const bool ragaTaan = (mTradition == TraditionId::IndianRaga) && ((mModifierMask & (1u << 3)) != 0);
    const bool ragaMeend = (mTradition == TraditionId::IndianRaga) && ((mModifierMask & (1u << 1)) != 0);

    if (mMode == PerformanceMode::Phrase || ragaTaan) {
        // Trigger first note immediately; advance() continuously weaves an evolving
        // 16-step idiomatic phrase around the held/latched key!
        PitchSpec first = stepInScale(midiNote, cents, 0, velocity * 0.92f);
        if (onNoteOn) onNoteOn(first.midiNote, first.cents, first.velocity);
        mLastPatternNote = first.midiNote;
        mPatternStep = 1;
        mPatternTimerSamples = mSampleRate * (60.0 / mBpm) * 0.25;
        return;
    }

    if (mMode == PerformanceMode::Arp) {
        // Trigger first note immediately; advance() drives subsequent arp steps
        if (!voiced.empty() && onNoteOn) {
            onNoteOn(voiced[0].midiNote, voiced[0].cents, voiced[0].velocity);
            mLastPatternNote = voiced[0].midiNote;
        }
        mPatternStep = 1;
        mPatternTimerSamples = mSampleRate * (60.0 / mBpm) * 0.25;
        return;
    }

    if (maqamOrnament || ragaMeend) {
        // Quick upper/lower grace note mordent before settling on main pitch
        PitchSpec grace = stepInScale(midiNote, cents, maqamOrnament ? 1 : -1, velocity * 0.74f);
        int graceSamples = static_cast<int>(mSampleRate * 0.045);
        if (onNoteOn) onNoteOn(grace.midiNote, grace.cents, grace.velocity);
        mScheduled.push_back({graceSamples, false, grace.midiNote, 0.0f, 0.0f, pointerId});
        for (const auto& n : voiced) {
            mScheduled.push_back({graceSamples + 32, true, n.midiNote, n.cents, n.velocity, pointerId});
        }
        return;
    }

    if (mMode == PerformanceMode::Strum && voiced.size() > 1) {
        const int strumStep = static_cast<int>(mSampleRate * 0.024); // 24 ms strum
        for (size_t i = 0; i < voiced.size(); ++i) {
            if (i == 0) {
                if (onNoteOn) onNoteOn(voiced[i].midiNote, voiced[i].cents, voiced[i].velocity);
            } else {
                mScheduled.push_back({static_cast<int>(i) * strumStep, true,
                                      voiced[i].midiNote, voiced[i].cents, voiced[i].velocity, pointerId});
            }
        }
        return;
    }

    // Free mode: trigger all voiced notes immediately
    if (onNoteOn) {
        for (const auto& n : voiced) {
            onNoteOn(n.midiNote, n.cents, n.velocity);
        }
    }
}

void PerformanceLayer::noteOff(int pointerId, const NoteOffCb& onNoteOff) {
    if (mLatch) return; // Latched notes hold until next touch or latch off

    for (auto it = mHeldTouches.begin(); it != mHeldTouches.end();) {
        if (it->pointerId == pointerId) {
            releaseTouch(*it, onNoteOff);
            it = mHeldTouches.erase(it);
        } else {
            ++it;
        }
    }

    if (mHeldTouches.empty() && mLastPatternNote >= 0 && onNoteOff) {
        onNoteOff(mLastPatternNote);
        mLastPatternNote = -1;
    }
}

void PerformanceLayer::allNotesOff(const NoteOffCb& onNoteOff) {
    for (const auto& t : mHeldTouches) {
        releaseTouch(t, onNoteOff);
    }
    mHeldTouches.clear();
    mScheduled.clear();
    if (mLastPatternNote >= 0 && onNoteOff) {
        onNoteOff(mLastPatternNote);
        mLastPatternNote = -1;
    }
}

void PerformanceLayer::advance(int numFrames, const NoteOnCb& onNoteOn, const NoteOffCb& onNoteOff) {
    // 1. Process scheduled strums / ornaments / phrase notes
    for (auto it = mScheduled.begin(); it != mScheduled.end();) {
        it->samplesRemaining -= numFrames;
        if (it->samplesRemaining <= 0) {
            if (it->isNoteOn) {
                if (onNoteOn) onNoteOn(it->midiNote, it->cents, it->velocity);
            } else {
                if (onNoteOff) onNoteOff(it->midiNote);
            }
            it = mScheduled.erase(it);
        } else {
            ++it;
        }
    }

    if (mHeldTouches.empty()) return;

    // 2. Active continuous pattern modes while keys are held (or latched):
    //    - Arp mode
    //    - Phrase mode / Raga Taan (modifier bit 3)
    //    - Gamelan Kotekan (modifier bit 1)
    //    - Maqam/Greek Tremolo (modifier bit 4)
    const bool ragaTaan = (mTradition == TraditionId::IndianRaga) && ((mModifierMask & (1u << 3)) != 0);
    const bool phraseActive = (mMode == PerformanceMode::Phrase) || ragaTaan;
    const bool kotekan = (mTradition == TraditionId::Gamelan) && ((mModifierMask & (1u << 1)) != 0);
    const bool tremolo = (mTradition == TraditionId::ArabicMaqam || mTradition == TraditionId::GreekDromoi)
                         && ((mModifierMask & (1u << 4)) != 0);
    const bool metalGallop = (mTradition == TraditionId::Metal) && ((mModifierMask & (1u << 2)) != 0);
    const bool acidRoll = (mTradition == TraditionId::Techno) && ((mModifierMask & (1u << 3)) != 0);

    if (mMode != PerformanceMode::Arp && !phraseActive && !kotekan && !tremolo && !metalGallop && !acidRoll) return;

    mPatternTimerSamples -= static_cast<double>(numFrames);
    if (mPatternTimerSamples > 0.0) return;

    const double stepDur = mSampleRate * (60.0 / mBpm) * (tremolo ? 0.166 : 0.25);
    mPatternTimerSamples += std::max(512.0, stepDur);

    if (mLastPatternNote >= 0 && onNoteOff) {
        onNoteOff(mLastPatternNote);
        mLastPatternNote = -1;
    }

    if (phraseActive) {
        // 32-step (2-bar) call-and-response melodic phrase around the active key
        static constexpr int kRest = 99;
        static constexpr int kPhraseDegrees[32] = {
            // Bar 1 (Call): rise from tonic to 4th/5th and pause
            0, 1, 2, kRest, 3, 2, 1, 2,
            4, 3, 2, kRest, 1, 2, 0, kRest,
            // Bar 2 (Response): explore lower neighbor, climb to 5th, resolve home
            -1, 0, 1, 3, 4, kRest, 5, 4,
            3, 2, 1, kRest, 2, 1, 0, kRest
        };
        const auto& base = mHeldTouches.back();
        const int deg = kPhraseDegrees[mPatternStep & 31];
        if (deg != kRest && onNoteOn) {
            const float accentVel = (mPatternStep % 4 == 0) ? 0.94f : 0.74f;
            PitchSpec p = stepInScale(base.rootMidi, base.rootCents, deg, base.velocity * accentVel);
            onNoteOn(p.midiNote, p.cents, p.velocity);
            mLastPatternNote = p.midiNote;
        }
        mPatternStep++;
    } else if (mMode == PerformanceMode::Arp) {
        std::vector<PitchSpec> pool;
        for (const auto& t : mHeldTouches) {
            pool.insert(pool.end(), t.voiced.begin(), t.voiced.end());
        }
        if (!pool.empty() && onNoteOn) {
            const auto& p = pool[static_cast<size_t>(mPatternStep % static_cast<int>(pool.size()))];
            onNoteOn(p.midiNote, p.cents, p.velocity * 0.9f);
            mLastPatternNote = p.midiNote;
            mPatternStep++;
        }
    } else if (kotekan) {
        // Interlocking Polos/Sangsih high metallophone pattern around the active Balungan note
        const auto& base = mHeldTouches.back();
        static constexpr int kKotekanPattern[8] = {0, 1, 2, 1, 0, 2, 3, 1};
        int deg = kKotekanPattern[mPatternStep & 7];
        PitchSpec p = stepInScale(base.rootMidi + 12, base.rootCents, deg, base.velocity * 0.72f);
        if (onNoteOn) onNoteOn(p.midiNote, p.cents, p.velocity);
        mLastPatternNote = p.midiNote;
        mPatternStep++;
    } else if (acidRoll) {
        // Acid TB-303 rolling 16th-note octave/accent pattern
        static constexpr int kAcidOffsets[8] = {0, 0, 12, 0, 1, 0, 12, 7};
        const auto& base = mHeldTouches.back();
        int semiOff = kAcidOffsets[mPatternStep & 7];
        float vel = ((mPatternStep & 3) == 0 || semiOff == 12) ? 0.96f : 0.68f;
        PitchSpec p = fromContinuousSemitone(static_cast<float>(base.rootMidi) + base.rootCents / 100.0f + semiOff, base.velocity * vel);
        if (onNoteOn) {
            onNoteOn(p.midiNote, p.cents, p.velocity);
            mLastPatternNote = p.midiNote;
        }
        mPatternStep++;
    } else if (metalGallop) {
        // Tight metal gallop (1 - - 1 1 -)
        const int pos = mPatternStep % 4;
        const auto& base = mHeldTouches.back();
        if (pos != 1 && onNoteOn) {
            for (const auto& n : base.voiced) {
                onNoteOn(n.midiNote, n.cents, n.velocity * (pos == 0 ? 0.96f : 0.78f));
            }
            mLastPatternNote = base.rootMidi;
        }
        mPatternStep++;
    } else if (tremolo) {
        const auto& base = mHeldTouches.back();
        if (onNoteOn) {
            onNoteOn(base.rootMidi, base.rootCents, base.velocity * ((mPatternStep & 1) ? 0.68f : 0.86f));
            mLastPatternNote = base.rootMidi;
        }
        mPatternStep++;
    }
}

} // namespace euterpe
