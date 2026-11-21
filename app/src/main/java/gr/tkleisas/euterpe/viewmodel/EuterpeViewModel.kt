package gr.tkleisas.euterpe.viewmodel

import android.content.Context
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import gr.tkleisas.euterpe.audio.EuterpeEngine
import gr.tkleisas.euterpe.model.KeyboardLayoutMode
import gr.tkleisas.euterpe.model.MusicCatalog
import gr.tkleisas.euterpe.model.PercussionKit
import gr.tkleisas.euterpe.model.PerformanceMode
import gr.tkleisas.euterpe.model.RhythmSpec
import gr.tkleisas.euterpe.model.ScaleSpec
import gr.tkleisas.euterpe.model.Tradition
import kotlin.random.Random
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch

data class LayerConfig(
    val instrumentIndex: Int,
    val synthTone: Float = 0.62f,
    val synthDecay: Float = 0.68f,
    val synthDrive: Float = 0.15f,
    val insertDistOn: Boolean = false,
    val insertDistAmt: Float = 0.45f,
    val insertChorusOn: Boolean = false,
    val insertChorusAmt: Float = 0.45f,
    val insertDelayOn: Boolean = false,
    val insertDelayAmt: Float = 0.40f,
)

private val defaultLayerConfigs = listOf(
    LayerConfig(0),  // L1: Oud / Lead
    LayerConfig(13), // L2: Deep Sub Bass
    LayerConfig(6),  // L3: Rhodes EP
    LayerConfig(9),  // L4: Ney / Wind
    LayerConfig(4),  // L5: Gamelan Saron
    LayerConfig(12), // L6: Acid TB-303
    LayerConfig(18), // L7: Polivoks Lead
    LayerConfig(10), // L8: Harmonic Pad
)

data class EuterpeUiState(
    val tradition: Tradition = Tradition.ARABIC,
    val modifierBank: Tradition = Tradition.ARABIC,
    val scale: ScaleSpec = MusicCatalog.findScale(Tradition.ARABIC.defaultScaleId),
    val rootSemitone: Int = Tradition.ARABIC.defaultRoot,
    val rhythm: RhythmSpec = MusicCatalog.findRhythm(Tradition.ARABIC.defaultRhythmId),
    val percussionKit: PercussionKit = PercussionKit.NATIVE,
    val instrumentIndex: Int = Tradition.ARABIC.defaultInstrumentIndex,
    val performanceMode: PerformanceMode = PerformanceMode.FREE,
    val keyboardLayout: KeyboardLayoutMode = KeyboardLayoutMode.SCALE_LOCKED,
    val baseOctave: Int = 3,
    val modifierMask: Int = 0,
    val isLatched: Boolean = false,
    val isPlaying: Boolean = false,
    val isIdeaLoopEnabled: Boolean = false,
    val isBassLoopEnabled: Boolean = false,
    val bpm: Float = Tradition.ARABIC.defaultBpm,
    val seed: Long = 1L,
    val currentStep: Int = 0,
    val stepCount: Int = 16,
    val activeCycleSteps: Set<Int> = MusicCatalog.findRhythm(Tradition.ARABIC.defaultRhythmId).accentSteps,
    val droneLevel: Float = 0.0f,
    val percLevel: Float = 0.85f,
    val ideaLevel: Float = 0.72f,
    val bassLevel: Float = 0.80f,
    val ideaDensity: Float = 0.55f,
    val swing: Float = 0.04f,
    val synthTone: Float = 0.62f,
    val synthDecay: Float = 0.68f,
    val synthDrive: Float = 0.15f,
    val reverbMix: Float = 0.24f,
    val insertDistOn: Boolean = false,
    val insertDistAmt: Float = 0.45f,
    val insertChorusOn: Boolean = false,
    val insertChorusAmt: Float = 0.45f,
    val insertDelayOn: Boolean = false,
    val insertDelayAmt: Float = 0.40f,
    val masterReverbOn: Boolean = true,
    val masterReverbAmt: Float = 0.28f,
    val masterCompOn: Boolean = false,
    val masterCompAmt: Float = 0.45f,
    val looperState: Int = 0, // 0=Empty, 1=Rec1, 2=Playing, 3=Overdub
    val looperLayers: Int = 0,
    val selectedLayer: Int = 0, // 0..7 (Layer 1..8)
    val recordedLayerMask: Int = 0,
    val mutedLayerMask: Int = 0,
    val showMixerPopup: Boolean = false,
)

class EuterpeViewModel : ViewModel() {

    private val engine = EuterpeEngine()
    private var engineInitialized = false
    private val layerConfigs = defaultLayerConfigs.toMutableList()

    private val _uiState = MutableStateFlow(EuterpeUiState())
    val uiState: StateFlow<EuterpeUiState> = _uiState.asStateFlow()

    init {
        viewModelScope.launch {
            while (isActive) {
                if (engineInitialized) {
                    val step = if (_uiState.value.isPlaying) engine.currentStep() else _uiState.value.currentStep
                    val count = if (_uiState.value.isPlaying) engine.stepCount().coerceAtLeast(1) else _uiState.value.stepCount
                    val lState = engine.looperState()
                    val lLayers = engine.looperLayerCount()
                    val recMask = engine.looperRecordedMask()
                    val muteMask = engine.looperMutedMask()
                    val cur = _uiState.value
                    if (step != cur.currentStep || count != cur.stepCount ||
                        lState != cur.looperState || lLayers != cur.looperLayers ||
                        recMask != cur.recordedLayerMask || muteMask != cur.mutedLayerMask
                    ) {
                        _uiState.update {
                            it.copy(
                                currentStep = step,
                                stepCount = count,
                                looperState = lState,
                                looperLayers = lLayers,
                                recordedLayerMask = recMask,
                                mutedLayerMask = muteMask
                            )
                        }
                    }
                }
                delay(35L)
            }
        }
    }

    fun startEngine(context: Context) {
        if (!engineInitialized) {
            engineInitialized = engine.init(context.applicationContext)
            syncStateToEngine(_uiState.value)
        } else {
            engine.resume()
        }
    }

    fun pauseEngine() {
        if (engineInitialized) {
            engine.allNotesOff()
            engine.pause()
        }
    }

    override fun onCleared() {
        super.onCleared()
        if (engineInitialized) {
            engine.shutdown()
            engineInitialized = false
        }
    }

    private fun syncStateToEngine(s: EuterpeUiState) {
        if (!engineInitialized) return
        engine.setTradition(s.modifierBank.nativeId)
        engine.setScale(s.scale.id, s.rootSemitone)
        engine.setRhythm(s.rhythm.id)
        engine.setPercussionKit(s.percussionKit.nativeId)
        engine.setInstrument(s.instrumentIndex)
        engine.setPerformanceMode(s.performanceMode.nativeId)
        engine.setModifierMask(s.modifierMask)
        engine.setLatch(s.isLatched)
        engine.setBpm(s.bpm)
        engine.setSwing(s.swing)
        engine.setDroneLevel(s.droneLevel)
        engine.setPercLevel(s.percLevel)
        engine.setIdeaLevel(s.ideaLevel)
        engine.setBassLevel(s.bassLevel)
        engine.setInsertFx(0, s.insertDistOn, s.insertDistAmt)
        engine.setInsertFx(1, s.insertChorusOn, s.insertChorusAmt)
        engine.setInsertFx(2, s.insertDelayOn, s.insertDelayAmt)
        engine.setMasterFx(0, s.masterReverbOn, s.masterReverbAmt)
        engine.setMasterFx(1, s.masterCompOn, s.masterCompAmt)
        engine.setIdeaLoopEnabled(s.isIdeaLoopEnabled)
        engine.setBassLoopEnabled(s.isBassLoopEnabled)
        engine.setTransportPlaying(s.isPlaying)
        engine.mutateIdea(s.seed, s.ideaDensity)
    }

    fun selectTradition(tradition: Tradition) {
        val defaultScale = MusicCatalog.findScale(tradition.defaultScaleId)
        val defaultRhythm = MusicCatalog.findRhythm(tradition.defaultRhythmId)
        val defaultLayout = if (tradition == Tradition.WESTERN ||
            tradition == Tradition.TECHNO || tradition == Tradition.METAL || tradition == Tradition.POP
        ) {
            KeyboardLayoutMode.PIANO_12
        } else {
            KeyboardLayoutMode.SCALE_LOCKED
        }
        val defaultMask = when (tradition) {
            Tradition.WESTERN, Tradition.POP, Tradition.METAL -> 1
            else -> 0
        }
        _uiState.update {
            it.copy(
                tradition = tradition,
                modifierBank = tradition,
                scale = defaultScale,
                rootSemitone = tradition.defaultRoot,
                rhythm = defaultRhythm,
                percussionKit = PercussionKit.NATIVE,
                instrumentIndex = tradition.defaultInstrumentIndex,
                keyboardLayout = defaultLayout,
                bpm = tradition.defaultBpm,
                modifierMask = defaultMask,
                stepCount = defaultRhythm.stepCount,
                activeCycleSteps = defaultRhythm.accentSteps,
                currentStep = 0,
            )
        }
        if (engineInitialized) {
            val s = _uiState.value
            engine.setTradition(s.modifierBank.nativeId)
            engine.setScale(s.scale.id, s.rootSemitone)
            engine.setRhythm(s.rhythm.id)
            engine.setPercussionKit(s.percussionKit.nativeId)
            engine.setInstrument(s.instrumentIndex)
            engine.setBpm(s.bpm)
            engine.setModifierMask(s.modifierMask)
            engine.mutateIdea(s.seed, s.ideaDensity)
        }
    }

    // Independent Modifier Bank switch — does NOT overwrite Scale, Rhythm, Kit, or Synth!
    fun selectModifierBank(bank: Tradition) {
        val nextMask = if (bank == Tradition.WESTERN) 1 else 0
        _uiState.update { it.copy(modifierBank = bank, modifierMask = nextMask) }
        if (engineInitialized) {
            engine.setTradition(bank.nativeId)
            engine.setModifierMask(nextMask)
        }
    }

    // Adventurous cross-cultural fusion generator!
    fun fusionRandomize() {
        val rng = Random(System.nanoTime())
        val scale = MusicCatalog.scales[rng.nextInt(MusicCatalog.scales.size)]
        val rhythm = MusicCatalog.rhythms[rng.nextInt(MusicCatalog.rhythms.size)]
        val kit = PercussionKit.entries[rng.nextInt(PercussionKit.entries.size)]
        val modBank = Tradition.entries[rng.nextInt(Tradition.entries.size)]
        val inst = rng.nextInt(MusicCatalog.instruments.size)
        val root = rng.nextInt(12)
        val nextSeed = _uiState.value.seed + 1L
        val modMask = if (modBank == Tradition.WESTERN) 1 else 0
        _uiState.update {
            it.copy(
                tradition = scale.tradition,
                modifierBank = modBank,
                scale = scale,
                rootSemitone = root,
                rhythm = rhythm,
                percussionKit = kit,
                instrumentIndex = inst,
                modifierMask = modMask,
                stepCount = rhythm.stepCount,
                activeCycleSteps = rhythm.accentSteps,
                currentStep = 0,
                seed = nextSeed,
                isIdeaLoopEnabled = true,
                isPlaying = true,
            )
        }
        syncStateToEngine(_uiState.value)
    }

    fun selectScale(scale: ScaleSpec) {
        _uiState.update { it.copy(scale = scale) }
        if (engineInitialized) {
            engine.setScale(scale.id, _uiState.value.rootSemitone)
            engine.mutateIdea(_uiState.value.seed)
        }
    }

    fun selectRoot(rootSemitone: Int) {
        val norm = ((rootSemitone % 12) + 12) % 12
        _uiState.update { it.copy(rootSemitone = norm) }
        if (engineInitialized) {
            engine.setScale(_uiState.value.scale.id, norm)
            engine.mutateIdea(_uiState.value.seed)
        }
    }

    fun selectRhythm(rhythm: RhythmSpec) {
        _uiState.update {
            it.copy(
                rhythm = rhythm,
                stepCount = rhythm.stepCount,
                activeCycleSteps = rhythm.accentSteps,
                currentStep = 0
            )
        }
        if (engineInitialized) {
            engine.setRhythm(rhythm.id)
            engine.mutateIdea(_uiState.value.seed, _uiState.value.ideaDensity)
        }
    }

    fun selectPercussionKit(kit: PercussionKit) {
        _uiState.update { it.copy(percussionKit = kit) }
        if (engineInitialized) engine.setPercussionKit(kit.nativeId)
    }

    fun toggleCycleStep(stepIndex: Int) {
        _uiState.update { s ->
            val next = if (stepIndex in s.activeCycleSteps) {
                s.activeCycleSteps - stepIndex
            } else {
                s.activeCycleSteps + stepIndex
            }
            s.copy(activeCycleSteps = next)
        }
        if (engineInitialized) engine.toggleCycleStep(stepIndex)
    }

    fun setIdeaDensity(density: Float) {
        val d = density.coerceIn(0.15f, 0.95f)
        _uiState.update { it.copy(ideaDensity = d) }
        if (engineInitialized) engine.mutateIdea(_uiState.value.seed, d)
    }

    fun setSwing(swing: Float) {
        val sw = swing.coerceIn(0f, 0.30f)
        _uiState.update { it.copy(swing = sw) }
        if (engineInitialized) engine.setSwing(sw)
    }

    fun setSynthTone(tone: Float) {
        val t = tone.coerceIn(0f, 1f)
        _uiState.update { it.copy(synthTone = t) }
        if (engineInitialized) engine.setSynthTone(t)
    }

    fun setSynthDecay(decay: Float) {
        val d = decay.coerceIn(0f, 1f)
        _uiState.update { it.copy(synthDecay = d) }
        if (engineInitialized) engine.setSynthDecay(d)
    }

    fun setSynthDrive(drive: Float) {
        val dr = drive.coerceIn(0f, 1f)
        _uiState.update { it.copy(synthDrive = dr) }
        if (engineInitialized) engine.setSynthDrive(dr)
    }

    fun applySoundMacro(macro: String) {
        when (macro) {
            "WARM" -> { setSynthTone(0.38f); setSynthDecay(0.75f); setSynthDrive(0.08f); setReverbMix(0.32f) }
            "ACID" -> { setSynthTone(0.28f); setSynthDecay(0.32f); setSynthDrive(0.72f); setReverbMix(0.16f) }
            "GRIT" -> { setSynthTone(0.82f); setSynthDecay(0.48f); setSynthDrive(0.88f); setReverbMix(0.12f) }
            "DREAM" -> { setSynthTone(0.55f); setSynthDecay(0.92f); setSynthDrive(0.05f); setReverbMix(0.65f) }
        }
    }

    fun nudgeScaleDegreeCents(degreeIndex: Int, deltaCents: Float) {
        val cur = _uiState.value.scale
        if (degreeIndex !in cur.degrees.indices) return
        val nextDegrees = cur.degrees.toMutableList()
        nextDegrees[degreeIndex] = (nextDegrees[degreeIndex] + deltaCents / 100f).coerceIn(0f, 11.95f)
        val updatedScale = cur.copy(degrees = nextDegrees)
        _uiState.update { it.copy(scale = updatedScale) }
        if (engineInitialized) {
            engine.setCustomScale(_uiState.value.rootSemitone, nextDegrees.toFloatArray())
        }
    }

    fun resetScaleTuning() {
        val orig = MusicCatalog.findScale(_uiState.value.scale.id)
        _uiState.update { it.copy(scale = orig) }
        if (engineInitialized) {
            engine.setScale(orig.id, _uiState.value.rootSemitone)
        }
    }

    fun selectInstrument(index: Int) {
        _uiState.update { it.copy(instrumentIndex = index) }
        if (engineInitialized) engine.setInstrument(index)
    }

    fun selectPerformanceMode(mode: PerformanceMode) {
        _uiState.update { it.copy(performanceMode = mode) }
        if (engineInitialized) engine.setPerformanceMode(mode.nativeId)
    }

    fun toggleKeyboardLayout() {
        _uiState.update {
            val next = if (it.keyboardLayout == KeyboardLayoutMode.PIANO_12) {
                KeyboardLayoutMode.SCALE_LOCKED
            } else {
                KeyboardLayoutMode.PIANO_12
            }
            it.copy(keyboardLayout = next)
        }
    }

    fun shiftOctave(delta: Int) {
        _uiState.update { it.copy(baseOctave = (it.baseOctave + delta).coerceIn(0, 7)) }
    }

    fun toggleModifier(bit: Int) {
        val current = _uiState.value
        var mask = current.modifierMask
        // In Western modifier bank, Maj/Min/Dim/Aug (bits 0..3) are mutually exclusive chord qualities
        if (current.modifierBank == Tradition.WESTERN && bit in 0..3) {
            val wasSet = (mask and (1 shl bit)) != 0
            mask = mask and 0x0F.inv()
            if (!wasSet) mask = mask or (1 shl bit)
        } else {
            mask = mask xor (1 shl bit)
        }
        _uiState.update { it.copy(modifierMask = mask) }
        if (engineInitialized) engine.setModifierMask(mask)
    }

    fun toggleLatch() {
        val next = !_uiState.value.isLatched
        _uiState.update { it.copy(isLatched = next) }
        if (engineInitialized) engine.setLatch(next)
    }

    fun toggleTransport() {
        val next = !_uiState.value.isPlaying
        _uiState.update { it.copy(isPlaying = next, currentStep = if (!next) 0 else it.currentStep) }
        if (engineInitialized) engine.setTransportPlaying(next)
    }

    fun toggleIdeaLoop() {
        val next = !_uiState.value.isIdeaLoopEnabled
        val startTransport = next && !_uiState.value.isPlaying
        _uiState.update {
            it.copy(
                isIdeaLoopEnabled = next,
                isPlaying = if (startTransport) true else it.isPlaying,
            )
        }
        if (engineInitialized) {
            engine.setIdeaLoopEnabled(next)
            if (startTransport) engine.setTransportPlaying(true)
        }
    }

    fun toggleBassLoop() {
        val next = !_uiState.value.isBassLoopEnabled
        val startTransport = next && !_uiState.value.isPlaying
        _uiState.update {
            it.copy(
                isBassLoopEnabled = next,
                isPlaying = if (startTransport) true else it.isPlaying,
            )
        }
        if (engineInitialized) {
            engine.setBassLoopEnabled(next)
            if (startTransport) engine.setTransportPlaying(true)
        }
    }

    fun mutateIdea() {
        val nextSeed = _uiState.value.seed + 1L
        val startTransport = !_uiState.value.isPlaying
        _uiState.update {
            it.copy(
                seed = nextSeed,
                isIdeaLoopEnabled = true,
                isBassLoopEnabled = true,
                isPlaying = if (startTransport) true else it.isPlaying,
            )
        }
        if (engineInitialized) {
            engine.mutateIdea(nextSeed, _uiState.value.ideaDensity)
            engine.setIdeaLoopEnabled(true)
            engine.setBassLoopEnabled(true)
            if (startTransport) engine.setTransportPlaying(true)
        }
    }

    fun adjustBpm(delta: Float) {
        val next = (_uiState.value.bpm + delta).coerceIn(45f, 200f)
        _uiState.update { it.copy(bpm = next) }
        if (engineInitialized) engine.setBpm(next)
    }

    fun toggleMixerPopup() {
        _uiState.update { it.copy(showMixerPopup = !it.showMixerPopup) }
    }

    fun setDroneLevel(level: Float) {
        _uiState.update { it.copy(droneLevel = level) }
        if (engineInitialized) engine.setDroneLevel(level)
    }

    fun setPercLevel(level: Float) {
        _uiState.update { it.copy(percLevel = level) }
        if (engineInitialized) engine.setPercLevel(level)
    }

    fun setIdeaLevel(level: Float) {
        _uiState.update { it.copy(ideaLevel = level) }
        if (engineInitialized) engine.setIdeaLevel(level)
    }

    fun setBassLevel(level: Float) {
        _uiState.update { it.copy(bassLevel = level) }
        if (engineInitialized) engine.setBassLevel(level)
    }

    fun setReverbMix(mix: Float) {
        val amt = (mix / 0.85f).coerceIn(0f, 1f)
        _uiState.update { it.copy(reverbMix = mix, masterReverbAmt = amt) }
        if (engineInitialized) engine.setMasterFx(0, _uiState.value.masterReverbOn, amt)
    }

    fun setInsertFx(fxId: Int, enabled: Boolean, amount: Float) {
        val a = amount.coerceIn(0f, 1f)
        _uiState.update {
            when (fxId) {
                0 -> it.copy(insertDistOn = enabled, insertDistAmt = a)
                1 -> it.copy(insertChorusOn = enabled, insertChorusAmt = a)
                2 -> it.copy(insertDelayOn = enabled, insertDelayAmt = a)
                else -> it
            }
        }
        if (engineInitialized) engine.setInsertFx(fxId, enabled, a)
    }

    fun setMasterFx(fxId: Int, enabled: Boolean, amount: Float) {
        val a = amount.coerceIn(0f, 1f)
        _uiState.update {
            when (fxId) {
                0 -> it.copy(masterReverbOn = enabled, masterReverbAmt = a, reverbMix = a * 0.85f)
                1 -> it.copy(masterCompOn = enabled, masterCompAmt = a)
                else -> it
            }
        }
        if (engineInitialized) engine.setMasterFx(fxId, enabled, a)
    }

    fun noteOn(pointerId: Int, midiNote: Int, cents: Float, velocity: Float) {
        if (engineInitialized) engine.noteOn(pointerId, midiNote, cents, velocity)
    }

    fun noteOff(pointerId: Int) {
        if (engineInitialized) engine.noteOff(pointerId)
    }

    private fun saveCurrentToActiveLayer() {
        val s = _uiState.value
        val idx = s.selectedLayer.coerceIn(0, 7)
        layerConfigs[idx] = LayerConfig(
            instrumentIndex = s.instrumentIndex,
            synthTone = s.synthTone,
            synthDecay = s.synthDecay,
            synthDrive = s.synthDrive,
            insertDistOn = s.insertDistOn,
            insertDistAmt = s.insertDistAmt,
            insertChorusOn = s.insertChorusOn,
            insertChorusAmt = s.insertChorusAmt,
            insertDelayOn = s.insertDelayOn,
            insertDelayAmt = s.insertDelayAmt,
        )
    }

    fun selectLayer(layerIndex: Int) {
        val idx = layerIndex.coerceIn(0, 7)
        saveCurrentToActiveLayer()
        val cfg = layerConfigs[idx]
        _uiState.update {
            it.copy(
                selectedLayer = idx,
                instrumentIndex = cfg.instrumentIndex,
                synthTone = cfg.synthTone,
                synthDecay = cfg.synthDecay,
                synthDrive = cfg.synthDrive,
                insertDistOn = cfg.insertDistOn,
                insertDistAmt = cfg.insertDistAmt,
                insertChorusOn = cfg.insertChorusOn,
                insertChorusAmt = cfg.insertChorusAmt,
                insertDelayOn = cfg.insertDelayOn,
                insertDelayAmt = cfg.insertDelayAmt,
            )
        }
        if (engineInitialized) {
            engine.looperSelectLayer(idx)
            engine.setInstrument(cfg.instrumentIndex)
            engine.setSynthTone(cfg.synthTone)
            engine.setSynthDecay(cfg.synthDecay)
            engine.setSynthDrive(cfg.synthDrive)
            engine.setInsertFx(0, cfg.insertDistOn, cfg.insertDistAmt)
            engine.setInsertFx(1, cfg.insertChorusOn, cfg.insertChorusAmt)
            engine.setInsertFx(2, cfg.insertDelayOn, cfg.insertDelayAmt)
        }
    }

    fun toggleLayerMute(layerIndex: Int) {
        if (engineInitialized) engine.looperToggleMute(layerIndex.coerceIn(0, 7))
    }

    fun looperToggleRec() {
        if (engineInitialized) engine.looperToggleRec()
    }

    fun looperTogglePlayAll() {
        if (engineInitialized) engine.looperTogglePlayAll()
    }

    fun looperUndo() {
        if (engineInitialized) engine.looperUndo()
    }

    fun looperClear() {
        if (engineInitialized) engine.looperClear()
    }
}
