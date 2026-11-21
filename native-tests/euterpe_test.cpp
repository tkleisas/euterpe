#include "CycleSequencer.h"
#include "LoopRecorder.h"
#include "LuaIdeaHost.h"
#include "PerformanceLayer.h"
#include "WorldPercussionSynth.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace euterpe;

static void testWorldPercussionSynth() {
    WorldPercussionSynth perc;
    perc.init(48000.0);
    perc.setTonicHz(293.66f); // D4

    for (int v = 0; v < WorldPercussionSynth::kVoiceCount; ++v) {
        perc.reset();
        perc.trigger(v, 0.9f);
        std::vector<float> buf(2048 * 2, 0.0f);
        perc.process(buf.data(), 2048, 2, 1.0f);

        float peak = 0.0f;
        for (float s : buf) {
            assert(std::isfinite(s));
            peak = std::max(peak, std::fabs(s));
        }
        assert(peak > 0.01f && peak <= 1.5f);
    }
    assert(WorldPercussionSynth::voiceIdFromName("dum") == WorldPercussionSynth::DarbukaDum);
    assert(WorldPercussionSynth::voiceIdFromName("bayan") == WorldPercussionSynth::TablaBayan);
    assert(WorldPercussionSynth::voiceIdFromName("gong") == WorldPercussionSynth::GongAgeng);
    std::cout << "[PASS] testWorldPercussionSynth (all " << WorldPercussionSynth::kVoiceCount << " voices)\n";
}

static void testCycleSequencerAsymmetric() {
    CycleSequencer seq;
    seq.init(48000.0);
    seq.setBpm(120.0f);

    // 7/8 Kalamatianos (14 steps at 16th-note resolution -> 3.5 beats -> 1.75 seconds at 120 BPM)
    RhythmPattern pat;
    pat.stepCount = 14;
    pat.stepsPerBeat = 4.0f;
    pat.steps.resize(14);
    pat.steps[0].push_back({WorldPercussionSynth::Daouli, 0.95f});
    pat.steps[6].push_back({WorldPercussionSynth::DarbukaDum, 0.85f});
    pat.steps[10].push_back({WorldPercussionSynth::DarbukaDum, 0.85f});
    seq.setRhythmPattern(pat);
    seq.setPlaying(true);

    int hits = 0;
    // Advance 1.75 seconds = 84000 samples (one full 7/8 cycle)
    for (int i = 0; i < 84000; i += 256) {
        seq.advance(256,
                    [&](int, float) { hits++; },
                    nullptr, nullptr);
    }
    assert(hits >= 3);
    assert(seq.stepCount() == 14);
    std::cout << "[PASS] testCycleSequencerAsymmetric (7/8 hits=" << hits << ")\n";
}

static void testPerformanceLayerTraditions() {
    PerformanceLayer perf;
    perf.init(48000.0);

    // 1. Western Major 7th chord
    perf.setTradition(TraditionId::Western);
    perf.setModifierMask((1u << 0) | (1u << 4)); // Maj + 7th
    auto westVoicing = perf.buildVoicing(60, 0.0f, 0.9f);
    assert(westVoicing.size() == 4);
    assert(westVoicing[0].midiNote == 60);
    assert(westVoicing[1].midiNote == 64);
    assert(westVoicing[2].midiNote == 67);
    assert(westVoicing[3].midiNote == 71);

    // 2. Arabic Maqam Bayati modal chord (microtonal neutral 2nd = 1.5 semitones!)
    perf.setTradition(TraditionId::ArabicMaqam);
    perf.setScale(2, {0.0f, 1.5f, 3.0f, 5.0f, 7.0f, 8.5f, 10.0f});
    perf.setModifierMask(1u << 2); // Modal Chord
    auto maqamVoicing = perf.buildVoicing(62, 0.0f, 0.9f); // D4
    assert(maqamVoicing.size() == 3);
    assert(maqamVoicing[0].midiNote == 62);
    assert(maqamVoicing[1].midiNote == 65); // F4 (+3)
    assert(maqamVoicing[2].midiNote == 69); // A4 (+7)

    // 3. Gamelan Slendro with Ombak shimmer
    perf.setTradition(TraditionId::Gamelan);
    perf.setScale(0, {0.0f, 2.40f, 4.82f, 7.22f, 9.60f});
    perf.setModifierMask((1u << 0) | (1u << 2)); // Balungan + Ombak
    auto gamelanVoicing = perf.buildVoicing(60, 0.0f, 0.85f);
    assert(gamelanVoicing.size() == 3);
    assert(std::fabs(gamelanVoicing[2].cents - 14.0f) < 1.0f);

    // 4. Phrase + Latch continues generating notes after key release
    perf.reset();
    perf.setTradition(TraditionId::ArabicMaqam);
    perf.setScale(2, {0.0f, 1.5f, 3.0f, 5.0f, 7.0f, 8.5f, 10.0f});
    perf.setPerformanceMode(PerformanceMode::Phrase);
    perf.setLatch(true);
    perf.setBpm(120.0f);

    int phraseNoteOns = 0;
    auto onOn = [&](int, float, float) { phraseNoteOns++; };
    auto onOff = [&](int) {};
    perf.noteOn(1, 62, 0.0f, 0.85f, onOn, onOff, nullptr);
    perf.noteOff(1, onOff); // release finger while latched
    // Advance 2 seconds (16 sixteenth steps at 120 BPM)
    for (int i = 0; i < 96000; i += 256) {
        perf.advance(256, onOn, onOff);
    }
    assert(phraseNoteOns >= 8);

    std::cout << "[PASS] testPerformanceLayerTraditions (phrase+latch notes=" << phraseNoteOns << ")\n";
}

static void testLuaIdeaHost() {
    LuaIdeaHost lua;
    std::string err;
    bool ok = lua.init("app/src/main/assets/generator", &err);
    if (!ok) {
        std::cerr << "Lua init failed: " << err << "\n";
        assert(false);
    }

    // Load microtonal scales
    std::vector<float> bayati;
    assert(lua.loadScale("bayati", bayati, &err));
    assert(bayati.size() == 7);
    assert(std::fabs(bayati[1] - 1.5f) < 0.01f);

    std::vector<float> slendro;
    assert(lua.loadScale("slendro", slendro, &err));
    assert(slendro.size() == 5);
    assert(std::fabs(slendro[1] - 2.40f) < 0.01f);

    // Load asymmetric Greek 9/8 Zeibekiko & Indian 7-beat Rupak
    RhythmPattern zeibekiko;
    assert(lua.loadRhythm("zeibekiko", zeibekiko, &err));
    assert(zeibekiko.stepCount == 18);
    assert(!zeibekiko.steps[0].empty());

    RhythmPattern rupak;
    assert(lua.loadRhythm("rupak", rupak, &err));
    assert(rupak.stepCount == 7);

    // Generate deterministic ideas across all 5 traditions
    const char* traditions[] = {"western", "arabic", "greek", "raga", "gamelan"};
    const char* scales[] = {"dorian", "bayati", "ousak", "raga_yaman", "slendro"};
    for (int i = 0; i < 5; ++i) {
        IdeaPattern a, b;
        assert(lua.generateIdea(42, traditions[i], scales[i], 2, 16, a, &err));
        assert(lua.generateIdea(42, traditions[i], scales[i], 2, 16, b, &err));
        assert(!a.notes.empty());
        assert(a.notes.size() == b.notes.size());
        assert(a.notes[0].midiNote == b.notes[0].midiNote);
    }

    std::cout << "[PASS] testLuaIdeaHost (all 5 traditions + scales + rhythms)\n";
}

static void testLoopRecorderLayers() {
    LoopRecorder looper;
    looper.init(48000.0);
    assert(looper.state() == LoopRecorder::State::Empty);
    assert(looper.layerCount() == 0);

    // Record Layer 1 (4096 frames of 0.4 amplitude)
    looper.toggleRecord();
    assert(looper.state() == LoopRecorder::State::RecordingFirst);
    std::vector<float> in1(4096 * 2, 0.4f);
    std::vector<float> out(4096 * 2, 0.0f);
    looper.process(in1.data(), out.data(), 4096, 2);

    // Stop recording -> Playing Layer 1
    looper.toggleRecord();
    assert(looper.state() == LoopRecorder::State::Playing);
    assert(looper.layerCount() == 1);

    std::fill(out.begin(), out.end(), 0.0f);
    std::vector<float> silence(4096 * 2, 0.0f);
    looper.process(silence.data(), out.data(), 4096, 2);
    // Middle sample (away from 15ms crossfade) should play back ~0.4
    assert(std::fabs(out[2048 * 2] - 0.4f) < 0.02f);

    // Overdub Layer 2 (+0.3 amplitude) by explicitly selecting layer 1
    looper.selectLayer(1);
    looper.toggleRecord();
    assert(looper.state() == LoopRecorder::State::Overdubbing);
    std::vector<float> in2(4096 * 2, 0.3f);
    std::fill(out.begin(), out.end(), 0.0f);
    looper.process(in2.data(), out.data(), 4096, 2);
    looper.toggleRecord(); // Commit Layer 2
    assert(looper.state() == LoopRecorder::State::Playing);
    assert(looper.layerCount() == 2);
    assert(looper.recordedMask() == 0b11);

    // Playback should sum Layer 1 + Layer 2 (scaled by 1/sqrt(2))
    std::fill(out.begin(), out.end(), 0.0f);
    looper.process(silence.data(), out.data(), 4096, 2);
    float expected2 = (0.4f + 0.3f) / std::sqrt(2.0f);
    assert(std::fabs(out[2048 * 2] - expected2) < 0.03f);

    // Stop ALL layers without clearing them
    looper.togglePlayAll();
    assert(looper.state() == LoopRecorder::State::Stopped);
    assert(looper.layerCount() == 2);
    std::fill(out.begin(), out.end(), 0.0f);
    looper.process(silence.data(), out.data(), 4096, 2);
    assert(std::fabs(out[2048 * 2]) < 0.0001f);

    // Resume ALL layers + mute individual layer 1
    looper.togglePlayAll();
    assert(looper.state() == LoopRecorder::State::Playing);
    looper.toggleLayerMute(1);
    assert(looper.mutedMask() == 0b10);
    std::fill(out.begin(), out.end(), 0.0f);
    looper.process(silence.data(), out.data(), 4096, 2);
    assert(std::fabs(out[2048 * 2] - 0.4f) < 0.02f); // Only layer 0 playing!
    looper.toggleLayerMute(1); // Unmute layer 1

    // Undo strictly clears the selected layer (layer 1) -> back to Layer 1 only
    looper.undoLayer();
    assert(looper.layerCount() == 1);
    assert(looper.recordedMask() == 0b01);
    std::fill(out.begin(), out.end(), 0.0f);
    looper.process(silence.data(), out.data(), 4096, 2);
    assert(std::fabs(out[2048 * 2] - 0.4f) < 0.02f);

    // Clear
    looper.clear();
    assert(looper.state() == LoopRecorder::State::Empty);
    assert(looper.layerCount() == 0);

    std::cout << "[PASS] testLoopRecorderLayers (record, overdub, undo, clear)\n";
}

int main() {
    testWorldPercussionSynth();
    testCycleSequencerAsymmetric();
    testPerformanceLayerTraditions();
    testLuaIdeaHost();
    testLoopRecorderLayers();
    std::cout << "ALL NATIVE TESTS PASSED!\n";
    return 0;
}
