#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace euterpe {

struct EngineEvent {
    enum class Type : uint8_t {
        NoteOn = 0,
        NoteOff,
        PitchBend,          // per-pointer continuous slide (cents)
        AllNotesOff,
        SetTradition,
        SetRoot,
        SetPerformanceMode,
        SetModifierMask,
        SetLatch,
        SetInstrument,
        SetBpm,
        SetTransportPlaying,
        SetDroneLevel,
        SetPercLevel,
        SetIdeaLevel,
        SetBassLevel,
        SetReverbMix,
        SetMasterVolume,
        TriggerPercussion,
        SetPercussionKit,
        ToggleCycleStep,
        SetSwing,
        SetSoundPreset,
        SetSynthTone,
        SetSynthDecay,
        SetSynthDrive,
        LooperToggleRec,
        LooperTogglePlayAll,
        LooperUndo,
        LooperClear,
        LooperSelectLayer,
        LooperToggleMute,
        SetInsertFx,
        SetMasterFx,
    };

    Type type = Type::NoteOn;
    int32_t pointerId = 0;
    int32_t intParam1 = 0;  // midiNote / traditionId / mode / mask / voiceId
    int32_t intParam2 = 0;  // scaleDegree / extra
    float floatParam1 = 0.0f; // cents / bpm / level
    float floatParam2 = 0.8f; // velocity
};

// Lock-free single-consumer (audio thread), mutex-guarded multi-producer ring buffer.
template <size_t Capacity = 1024>
class EventQueue {
public:
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of two");

    bool push(const EngineEvent& ev) {
        size_t w = mWrite.load(std::memory_order_relaxed);
        size_t next = (w + 1) & (Capacity - 1);
        if (next == mRead.load(std::memory_order_acquire)) {
            return false; // full
        }
        mBuffer[w] = ev;
        mWrite.store(next, std::memory_order_release);
        return true;
    }

    bool pop(EngineEvent& out) {
        size_t r = mRead.load(std::memory_order_relaxed);
        if (r == mWrite.load(std::memory_order_acquire)) {
            return false; // empty
        }
        out = mBuffer[r];
        mRead.store((r + 1) & (Capacity - 1), std::memory_order_release);
        return true;
    }

    void clear() {
        mRead.store(mWrite.load(std::memory_order_acquire), std::memory_order_release);
    }

private:
    std::array<EngineEvent, Capacity> mBuffer{};
    std::atomic<size_t> mWrite{0};
    std::atomic<size_t> mRead{0};
};

} // namespace euterpe
