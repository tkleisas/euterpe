package gr.tkleisas.euterpe.audio

import android.content.Context
import java.io.File

class EuterpeEngine {

    companion object {
        init {
            System.loadLibrary("euterpe-lib")
        }
    }

    fun init(context: Context): Boolean {
        val dir = extractLuaAssets(context)
        return nativeInit(dir.absolutePath)
    }

    private fun extractLuaAssets(context: Context): File {
        val destRoot = File(context.filesDir, "generator")
        destRoot.mkdirs()
        copyAssetDir(context, "generator", destRoot)
        return destRoot
    }

    private fun copyAssetDir(context: Context, assetPath: String, destDir: File) {
        val items = context.assets.list(assetPath) ?: return
        destDir.mkdirs()
        for (item in items) {
            val childAsset = "$assetPath/$item"
            val subItems = context.assets.list(childAsset)
            if (!subItems.isNullOrEmpty()) {
                copyAssetDir(context, childAsset, File(destDir, item))
            } else {
                val outFile = File(destDir, item)
                context.assets.open(childAsset).use { input ->
                    outFile.outputStream().use { output ->
                        input.copyTo(output)
                    }
                }
            }
        }
    }

    fun shutdown() = nativeShutdown()
    fun pause() = nativePause()
    fun resume() = nativeResume()

    fun noteOn(pointerId: Int, midiNote: Int, cents: Float, velocity: Float) =
        nativeNoteOn(pointerId, midiNote, cents, velocity)

    fun noteOff(pointerId: Int) = nativeNoteOff(pointerId)
    fun allNotesOff() = nativeAllNotesOff()

    fun setTradition(traditionId: Int) = nativeSetTradition(traditionId)
    fun setScale(scaleId: String, rootSemitone: Int) = nativeSetScale(scaleId, rootSemitone)
    fun setRhythm(rhythmId: String) = nativeSetRhythm(rhythmId)
    fun setPerformanceMode(mode: Int) = nativeSetPerformanceMode(mode)
    fun setModifierMask(mask: Int) = nativeSetModifierMask(mask)
    fun setLatch(latch: Boolean) = nativeSetLatch(latch)
    fun setInstrument(index: Int) = nativeSetSoundPreset(index)
    fun setSynthTone(tone: Float) = nativeSetSynthTone(tone)
    fun setSynthDecay(decay: Float) = nativeSetSynthDecay(decay)
    fun setSynthDrive(drive: Float) = nativeSetSynthDrive(drive)
    fun setCustomScale(rootSemitone: Int, degrees: FloatArray) = nativeSetCustomScale(rootSemitone, degrees)

    fun setBpm(bpm: Float) = nativeSetBpm(bpm)
    fun setTransportPlaying(playing: Boolean) = nativeSetTransportPlaying(playing)
    fun setIdeaLoopEnabled(enabled: Boolean) = nativeSetIdeaLoopEnabled(enabled)
    fun setBassLoopEnabled(enabled: Boolean) = nativeSetBassLoopEnabled(enabled)
    fun mutateIdea(seed: Long, density: Float = -1f): Boolean = nativeMutateIdea(seed, density)
    fun setPercussionKit(kitId: Int) = nativeSetPercussionKit(kitId)
    fun toggleCycleStep(stepIndex: Int) = nativeToggleCycleStep(stepIndex)
    fun setSwing(swing: Float) = nativeSetSwing(swing)

    fun setDroneLevel(level: Float) = nativeSetDroneLevel(level)
    fun setPercLevel(level: Float) = nativeSetPercLevel(level)
    fun setIdeaLevel(level: Float) = nativeSetIdeaLevel(level)
    fun setBassLevel(level: Float) = nativeSetBassLevel(level)
    fun setReverbMix(mix: Float) = nativeSetReverbMix(mix)
    fun setInsertFx(fxId: Int, enabled: Boolean, amount: Float) = nativeSetInsertFx(fxId, enabled, amount)
    fun setMasterFx(fxId: Int, enabled: Boolean, amount: Float) = nativeSetMasterFx(fxId, enabled, amount)
    fun setMasterVolume(vol: Float) = nativeSetMasterVolume(vol)
    fun triggerPercussion(voiceId: Int, velocity: Float) = nativeTriggerPercussion(voiceId, velocity)

    fun currentStep(): Int = nativeCurrentStep()
    fun stepCount(): Int = nativeStepCount()

    fun looperToggleRec() = nativeLooperToggleRec()
    fun looperUndo() = nativeLooperUndo()
    fun looperClear() = nativeLooperClear()
    fun looperState(): Int = nativeLooperState()
    fun looperLayerCount(): Int = nativeLooperLayerCount()
    fun looperProgress(): Float = nativeLooperProgress()

    private external fun nativeInit(scriptsDir: String): Boolean
    private external fun nativeShutdown()
    private external fun nativePause()
    private external fun nativeResume()
    private external fun nativeNoteOn(pointerId: Int, midiNote: Int, cents: Float, velocity: Float)
    private external fun nativeNoteOff(pointerId: Int)
    private external fun nativeAllNotesOff()
    private external fun nativeSetTradition(traditionId: Int)
    private external fun nativeSetScale(scaleId: String, rootSemitone: Int)
    private external fun nativeSetRhythm(rhythmId: String)
    private external fun nativeSetPerformanceMode(mode: Int)
    private external fun nativeSetModifierMask(mask: Int)
    private external fun nativeSetLatch(latch: Boolean)
    private external fun nativeSetInstrument(index: Int)
    private external fun nativeSetBpm(bpm: Float)
    private external fun nativeSetTransportPlaying(playing: Boolean)
    private external fun nativeSetIdeaLoopEnabled(enabled: Boolean)
    private external fun nativeSetBassLoopEnabled(enabled: Boolean)
    private external fun nativeMutateIdea(seed: Long, density: Float): Boolean
    private external fun nativeSetPercussionKit(kitId: Int)
    private external fun nativeToggleCycleStep(stepIndex: Int)
    private external fun nativeSetSwing(swing: Float)
    private external fun nativeSetSoundPreset(presetId: Int)
    private external fun nativeSetSynthTone(tone: Float)
    private external fun nativeSetSynthDecay(decay: Float)
    private external fun nativeSetSynthDrive(drive: Float)
    private external fun nativeSetCustomScale(rootSemitone: Int, degrees: FloatArray)
    private external fun nativeSetDroneLevel(level: Float)
    private external fun nativeSetPercLevel(level: Float)
    private external fun nativeSetIdeaLevel(level: Float)
    private external fun nativeSetBassLevel(level: Float)
    private external fun nativeSetReverbMix(mix: Float)
    private external fun nativeSetInsertFx(fxId: Int, enabled: Boolean, amount: Float)
    private external fun nativeSetMasterFx(fxId: Int, enabled: Boolean, amount: Float)
    private external fun nativeSetMasterVolume(vol: Float)
    private external fun nativeTriggerPercussion(voiceId: Int, velocity: Float)
    private external fun nativeCurrentStep(): Int
    private external fun nativeStepCount(): Int
    private external fun nativeLooperToggleRec()
    private external fun nativeLooperUndo()
    private external fun nativeLooperClear()
    private external fun nativeLooperState(): Int
    private external fun nativeLooperLayerCount(): Int
    private external fun nativeLooperProgress(): Float
}
