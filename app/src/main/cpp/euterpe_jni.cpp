#include <jni.h>
#include <memory>
#include <string>

#include "EuterpeEngine.h"

namespace {
std::unique_ptr<euterpe::EuterpeEngine> gEngine;

std::string jstringToStd(JNIEnv* env, jstring js) {
    if (!js) return {};
    const char* chars = env->GetStringUTFChars(js, nullptr);
    std::string out(chars ? chars : "");
    if (chars) env->ReleaseStringUTFChars(js, chars);
    return out;
}
} // namespace

extern "C" {

JNIEXPORT jboolean JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeInit(JNIEnv* env, jobject, jstring scriptsDir) {
    if (!gEngine) {
        gEngine = std::make_unique<euterpe::EuterpeEngine>();
    }
    return gEngine->init(jstringToStd(env, scriptsDir)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeShutdown(JNIEnv*, jobject) {
    if (gEngine) {
        gEngine->shutdown();
        gEngine.reset();
    }
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativePause(JNIEnv*, jobject) {
    if (gEngine) gEngine->pauseAudio();
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeResume(JNIEnv*, jobject) {
    if (gEngine) gEngine->resumeAudio();
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeNoteOn(JNIEnv*, jobject,
                                                          jint pointerId, jint midiNote,
                                                          jfloat cents, jfloat velocity) {
    if (gEngine) gEngine->noteOn(pointerId, midiNote, cents, velocity);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeNoteOff(JNIEnv*, jobject, jint pointerId) {
    if (gEngine) gEngine->noteOff(pointerId);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeAllNotesOff(JNIEnv*, jobject) {
    if (gEngine) gEngine->allNotesOff();
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetTradition(JNIEnv*, jobject, jint traditionId) {
    if (gEngine) gEngine->setTradition(traditionId);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetScale(JNIEnv* env, jobject,
                                                            jstring scaleId, jint rootSemitone) {
    if (gEngine) gEngine->setScale(jstringToStd(env, scaleId), rootSemitone);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetRhythm(JNIEnv* env, jobject, jstring rhythmId) {
    if (gEngine) gEngine->setRhythm(jstringToStd(env, rhythmId));
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetPerformanceMode(JNIEnv*, jobject, jint mode) {
    if (gEngine) gEngine->setPerformanceMode(mode);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetModifierMask(JNIEnv*, jobject, jint mask) {
    if (gEngine) gEngine->setModifierMask(mask);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetLatch(JNIEnv*, jobject, jboolean latch) {
    if (gEngine) gEngine->setLatch(latch == JNI_TRUE);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetInstrument(JNIEnv*, jobject, jint index) {
    if (gEngine) gEngine->setInstrument(index);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetBpm(JNIEnv*, jobject, jfloat bpm) {
    if (gEngine) gEngine->setBpm(bpm);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetTransportPlaying(JNIEnv*, jobject, jboolean playing) {
    if (gEngine) gEngine->setTransportPlaying(playing == JNI_TRUE);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetIdeaLoopEnabled(JNIEnv*, jobject, jboolean enabled) {
    if (gEngine) gEngine->setIdeaLoopEnabled(enabled == JNI_TRUE);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetBassLoopEnabled(JNIEnv*, jobject, jboolean enabled) {
    if (gEngine) gEngine->setBassLoopEnabled(enabled == JNI_TRUE);
}

JNIEXPORT jboolean JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeMutateIdea(JNIEnv*, jobject, jlong seed, jfloat density) {
    if (!gEngine) return JNI_FALSE;
    return gEngine->mutateIdea(static_cast<uint64_t>(seed), density) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetPercussionKit(JNIEnv*, jobject, jint kitId) {
    if (gEngine) gEngine->setPercussionKit(kitId);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeToggleCycleStep(JNIEnv*, jobject, jint stepIndex) {
    if (gEngine) gEngine->toggleCycleStep(stepIndex);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetSwing(JNIEnv*, jobject, jfloat swing) {
    if (gEngine) gEngine->setSwing(swing);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetSoundPreset(JNIEnv*, jobject, jint presetId) {
    if (gEngine) gEngine->setSoundPreset(presetId);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetSynthTone(JNIEnv*, jobject, jfloat tone) {
    if (gEngine) gEngine->setSynthTone(tone);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetSynthDecay(JNIEnv*, jobject, jfloat decay) {
    if (gEngine) gEngine->setSynthDecay(decay);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetSynthDrive(JNIEnv*, jobject, jfloat drive) {
    if (gEngine) gEngine->setSynthDrive(drive);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetCustomScale(JNIEnv* env, jobject,
                                                                  jint rootSemitone,
                                                                  jfloatArray degreesArr) {
    if (!gEngine || !degreesArr) return;
    jsize len = env->GetArrayLength(degreesArr);
    if (len <= 0) return;
    std::vector<float> degs(static_cast<size_t>(len));
    env->GetFloatArrayRegion(degreesArr, 0, len, degs.data());
    gEngine->setCustomScale(rootSemitone, degs);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetDroneLevel(JNIEnv*, jobject, jfloat level) {
    if (gEngine) gEngine->setDroneLevel(level);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetPercLevel(JNIEnv*, jobject, jfloat level) {
    if (gEngine) gEngine->setPercLevel(level);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetIdeaLevel(JNIEnv*, jobject, jfloat level) {
    if (gEngine) gEngine->setIdeaLevel(level);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetBassLevel(JNIEnv*, jobject, jfloat level) {
    if (gEngine) gEngine->setBassLevel(level);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetReverbMix(JNIEnv*, jobject, jfloat mix) {
    if (gEngine) gEngine->setReverbMix(mix);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetInsertFx(JNIEnv*, jobject,
                                                               jint fxId, jboolean enabled, jfloat amount) {
    if (gEngine) gEngine->setInsertFx(fxId, enabled == JNI_TRUE, amount);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetMasterFx(JNIEnv*, jobject,
                                                               jint fxId, jboolean enabled, jfloat amount) {
    if (gEngine) gEngine->setMasterFx(fxId, enabled == JNI_TRUE, amount);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeSetMasterVolume(JNIEnv*, jobject, jfloat vol) {
    if (gEngine) gEngine->setMasterVolume(vol);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeTriggerPercussion(JNIEnv*, jobject,
                                                                     jint voiceId, jfloat velocity) {
    if (gEngine) gEngine->triggerPercussion(voiceId, velocity);
}

JNIEXPORT jint JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeCurrentStep(JNIEnv*, jobject) {
    return gEngine ? gEngine->currentStep() : 0;
}

JNIEXPORT jint JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeStepCount(JNIEnv*, jobject) {
    return gEngine ? gEngine->stepCount() : 16;
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeLooperToggleRec(JNIEnv*, jobject) {
    if (gEngine) gEngine->looperToggleRec();
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeLooperTogglePlayAll(JNIEnv*, jobject) {
    if (gEngine) gEngine->looperTogglePlayAll();
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeLooperSelectLayer(JNIEnv*, jobject, jint layerIndex) {
    if (gEngine) gEngine->looperSelectLayer(layerIndex);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeLooperToggleMute(JNIEnv*, jobject, jint layerIndex) {
    if (gEngine) gEngine->looperToggleMute(layerIndex);
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeLooperUndo(JNIEnv*, jobject) {
    if (gEngine) gEngine->looperUndo();
}

JNIEXPORT void JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeLooperClear(JNIEnv*, jobject) {
    if (gEngine) gEngine->looperClear();
}

JNIEXPORT jint JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeLooperState(JNIEnv*, jobject) {
    return gEngine ? gEngine->looperState() : 0;
}

JNIEXPORT jint JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeLooperLayerCount(JNIEnv*, jobject) {
    return gEngine ? gEngine->looperLayerCount() : 0;
}

JNIEXPORT jint JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeLooperRecordedMask(JNIEnv*, jobject) {
    return gEngine ? gEngine->looperRecordedMask() : 0;
}

JNIEXPORT jint JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeLooperMutedMask(JNIEnv*, jobject) {
    return gEngine ? gEngine->looperMutedMask() : 0;
}

JNIEXPORT jfloat JNICALL
Java_gr_tkleisas_euterpe_audio_EuterpeEngine_nativeLooperProgress(JNIEnv*, jobject) {
    return gEngine ? gEngine->looperProgress() : 0.0f;
}

} // extern "C"
