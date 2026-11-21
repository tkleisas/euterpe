package gr.tkleisas.euterpe

import android.os.Bundle
import android.view.WindowManager
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.viewModels
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import gr.tkleisas.euterpe.model.MusicCatalog
import gr.tkleisas.euterpe.model.PercussionKit
import gr.tkleisas.euterpe.ui.components.CycleAndMixerStrip
import gr.tkleisas.euterpe.ui.components.ModifierPadGrid
import gr.tkleisas.euterpe.ui.components.TopIdeaBar
import gr.tkleisas.euterpe.ui.components.TwoOctaveKeyboard
import gr.tkleisas.euterpe.ui.theme.EuterpeColors
import gr.tkleisas.euterpe.ui.theme.EuterpeTheme
import gr.tkleisas.euterpe.viewmodel.EuterpeUiState
import gr.tkleisas.euterpe.viewmodel.EuterpeViewModel
import kotlin.math.roundToInt

class MainActivity : ComponentActivity() {

    private val viewModel: EuterpeViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        hideSystemBars()

        setContent {
            EuterpeTheme {
                EuterpeScreen(viewModel = viewModel)
            }
        }
    }

    override fun onResume() {
        super.onResume()
        hideSystemBars()
        viewModel.startEngine(this)
    }

    override fun onPause() {
        super.onPause()
        viewModel.pauseEngine()
    }

    private fun hideSystemBars() {
        WindowCompat.setDecorFitsSystemWindows(window, false)
        val controller = WindowInsetsControllerCompat(window, window.decorView)
        controller.hide(WindowInsetsCompat.Type.systemBars())
        controller.systemBarsBehavior =
            WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
    }
}

@Composable
private fun EuterpeScreen(viewModel: EuterpeViewModel) {
    val state by viewModel.uiState.collectAsState()
    val accent = EuterpeColors.accentFor(state.tradition)

    val modifiers = remember(state.modifierBank) { MusicCatalog.modifiersFor(state.modifierBank) }
    val keys = remember(state.scale, state.rootSemitone, state.baseOctave, state.keyboardLayout) {
        MusicCatalog.buildTwoOctaveKeys(
            scale = state.scale,
            rootSemitone = state.rootSemitone,
            baseOctave = state.baseOctave,
            layout = state.keyboardLayout
        )
    }

    Box(
        modifier = Modifier
            .fillMaxSize()
            .background(EuterpeColors.ObsidianBg)
    ) {
        Column(modifier = Modifier.fillMaxSize()) {
            TopIdeaBar(
                state = state,
                accent = accent,
                onSelectTradition = viewModel::selectTradition,
                onSelectScale = viewModel::selectScale,
                onSelectRoot = viewModel::selectRoot,
                onSelectRhythm = viewModel::selectRhythm,
                onSelectLayer = viewModel::selectLayer,
                onToggleLayerMute = viewModel::toggleLayerMute,
                onToggleTransport = viewModel::toggleTransport,
                onAdjustBpm = viewModel::adjustBpm,
                onMutateIdea = viewModel::mutateIdea,
                onToggleMixer = viewModel::toggleMixerPopup
            )

            CycleAndMixerStrip(
                state = state,
                accent = accent,
                onSelectMode = viewModel::selectPerformanceMode,
                onToggleLatch = viewModel::toggleLatch,
                onToggleIdeaLoop = viewModel::toggleIdeaLoop,
                onToggleBassLoop = viewModel::toggleBassLoop,
                onLooperToggleRec = viewModel::looperToggleRec,
                onLooperUndo = viewModel::looperUndo,
                onLooperClear = viewModel::looperClear,
                onToggleCycleStep = viewModel::toggleCycleStep,
                onSelectInstrument = viewModel::selectInstrument,
                onToggleKeyboardLayout = viewModel::toggleKeyboardLayout,
                onShiftOctave = viewModel::shiftOctave
            )

            Row(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(start = 8.dp, end = 8.dp, top = 4.dp, bottom = 8.dp),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                ModifierPadGrid(
                    modifierBank = state.modifierBank,
                    modifiers = modifiers,
                    activeMask = state.modifierMask,
                    accent = accent,
                    onSelectModifierBank = viewModel::selectModifierBank,
                    onToggleModifier = viewModel::toggleModifier,
                    modifier = Modifier.weight(0.24f)
                )

                TwoOctaveKeyboard(
                    keys = keys,
                    layout = state.keyboardLayout,
                    accent = accent,
                    onNoteOn = viewModel::noteOn,
                    onNoteOff = viewModel::noteOff,
                    modifier = Modifier.weight(0.76f)
                )
            }
        }

        if (state.showMixerPopup) {
            LabOverlayCard(
                state = state,
                accent = accent,
                onSelectKit = viewModel::selectPercussionKit,
                onFusionRandomize = viewModel::fusionRandomize,
                onDroneChange = viewModel::setDroneLevel,
                onPercChange = viewModel::setPercLevel,
                onIdeaChange = viewModel::setIdeaLevel,
                onBassChange = viewModel::setBassLevel,
                onReverbChange = viewModel::setReverbMix,
                onSetInsertFx = viewModel::setInsertFx,
                onSetMasterFx = viewModel::setMasterFx,
                onToneChange = viewModel::setSynthTone,
                onDecayChange = viewModel::setSynthDecay,
                onDriveChange = viewModel::setSynthDrive,
                onApplyMacro = viewModel::applySoundMacro,
                onDensityChange = viewModel::setIdeaDensity,
                onSwingChange = viewModel::setSwing,
                onNudgeDegreeCents = viewModel::nudgeScaleDegreeCents,
                onResetTuning = viewModel::resetScaleTuning,
                modifier = Modifier
                    .align(Alignment.TopEnd)
                    .padding(top = 48.dp, end = 10.dp, bottom = 10.dp)
            )
        }
    }
}

@Composable
private fun LabOverlayCard(
    state: EuterpeUiState,
    accent: Color,
    onSelectKit: (PercussionKit) -> Unit,
    onFusionRandomize: () -> Unit,
    onDroneChange: (Float) -> Unit,
    onPercChange: (Float) -> Unit,
    onIdeaChange: (Float) -> Unit,
    onBassChange: (Float) -> Unit,
    onReverbChange: (Float) -> Unit,
    onSetInsertFx: (Int, Boolean, Float) -> Unit,
    onSetMasterFx: (Int, Boolean, Float) -> Unit,
    onToneChange: (Float) -> Unit,
    onDecayChange: (Float) -> Unit,
    onDriveChange: (Float) -> Unit,
    onApplyMacro: (String) -> Unit,
    onDensityChange: (Float) -> Unit,
    onSwingChange: (Float) -> Unit,
    onNudgeDegreeCents: (Int, Float) -> Unit,
    onResetTuning: () -> Unit,
    modifier: Modifier = Modifier,
) {
    var activeTab by remember { mutableIntStateOf(0) } // 0 = Mix/Kit, 1 = FX, 2 = Sound, 3 = Tune
    val tabs = listOf("MIX", "FX", "SOUND", "TUNE")

    Column(
        modifier = modifier
            .width(280.dp)
            .background(EuterpeColors.SurfaceDark.copy(alpha = 0.97f), RoundedCornerShape(12.dp))
            .border(1.dp, accent.copy(alpha = 0.65f), RoundedCornerShape(12.dp))
            .padding(10.dp),
        verticalArrangement = Arrangement.spacedBy(6.dp)
    ) {
        // Tab bar
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(4.dp)
        ) {
            tabs.forEachIndexed { idx, title ->
                val sel = activeTab == idx
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .height(26.dp)
                        .clip(RoundedCornerShape(6.dp))
                        .background(if (sel) accent.copy(alpha = 0.25f) else EuterpeColors.SurfaceElevated)
                        .border(1.dp, if (sel) accent else EuterpeColors.BorderSubtle, RoundedCornerShape(6.dp))
                        .clickable { activeTab = idx },
                    contentAlignment = Alignment.Center
                ) {
                    Text(
                        text = title,
                        color = if (sel) accent else EuterpeColors.TextMuted,
                        fontSize = 10.sp,
                        fontWeight = FontWeight.Bold
                    )
                }
            }
        }

        Column(
            modifier = Modifier
                .fillMaxWidth()
                .verticalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(5.dp)
        ) {
            when (activeTab) {
                0 -> {
                    // Exotic Fusion button
                    Box(
                        modifier = Modifier
                            .fillMaxWidth()
                            .height(30.dp)
                            .clip(RoundedCornerShape(8.dp))
                            .background(accent.copy(alpha = 0.22f))
                            .border(1.dp, accent, RoundedCornerShape(8.dp))
                            .clickable { onFusionRandomize() },
                        contentAlignment = Alignment.Center
                    ) {
                        Text(
                            text = "🎲 EXOTIC FUSION (MIX CULTURES)",
                            color = accent,
                            fontSize = 10.sp,
                            fontWeight = FontWeight.Bold,
                            letterSpacing = 0.5.sp
                        )
                    }

                    // Swappable Percussion Kit selector
                    Text("PERCUSSION KIT", color = EuterpeColors.TextMuted, fontSize = 9.sp, fontWeight = FontWeight.Bold)
                    PercussionKit.entries.chunked(3).forEach { rowKits ->
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            horizontalArrangement = Arrangement.spacedBy(4.dp)
                        ) {
                            rowKits.forEach { kit ->
                                val sel = state.percussionKit == kit
                                Box(
                                    modifier = Modifier
                                        .weight(1f)
                                        .height(24.dp)
                                        .clip(RoundedCornerShape(6.dp))
                                        .background(if (sel) accent.copy(alpha = 0.25f) else EuterpeColors.SurfaceElevated)
                                        .border(1.dp, if (sel) accent else EuterpeColors.BorderSubtle, RoundedCornerShape(6.dp))
                                        .clickable { onSelectKit(kit) },
                                    contentAlignment = Alignment.Center
                                ) {
                                    Text(
                                        text = kit.label,
                                        color = if (sel) accent else EuterpeColors.TextPrimary,
                                        fontSize = 10.sp,
                                        fontWeight = if (sel) FontWeight.Bold else FontWeight.Normal
                                    )
                                }
                            }
                        }
                    }

                    MixerSliderRow("Drone / Ison", state.droneLevel, accent, onDroneChange)
                    MixerSliderRow("Percussion", state.percLevel, accent, onPercChange)
                    MixerSliderRow("Bassline", state.bassLevel, accent, onBassChange)
                    MixerSliderRow("Idea Loop", state.ideaLevel, accent, onIdeaChange)
                    MixerSliderRow("Reverb", state.reverbMix / 0.85f, accent) { onReverbChange(it * 0.85f) }
                }

                1 -> {
                    Text("INSERT FX (INSTRUMENT)", color = EuterpeColors.TextMuted, fontSize = 9.sp, fontWeight = FontWeight.Bold)
                    FxSlotRow(
                        name = "Tube Drive",
                        enabled = state.insertDistOn,
                        amount = state.insertDistAmt,
                        accent = accent,
                        onToggle = { onSetInsertFx(0, !state.insertDistOn, state.insertDistAmt) },
                        onAmount = { onSetInsertFx(0, state.insertDistOn, it) }
                    )
                    FxSlotRow(
                        name = "Stereo Chorus",
                        enabled = state.insertChorusOn,
                        amount = state.insertChorusAmt,
                        accent = accent,
                        onToggle = { onSetInsertFx(1, !state.insertChorusOn, state.insertChorusAmt) },
                        onAmount = { onSetInsertFx(1, state.insertChorusOn, it) }
                    )
                    FxSlotRow(
                        name = "Ping-Pong Delay",
                        enabled = state.insertDelayOn,
                        amount = state.insertDelayAmt,
                        accent = accent,
                        onToggle = { onSetInsertFx(2, !state.insertDelayOn, state.insertDelayAmt) },
                        onAmount = { onSetInsertFx(2, state.insertDelayOn, it) }
                    )

                    Text("MASTER FX (FULL MIX)", color = EuterpeColors.TextMuted, fontSize = 9.sp, fontWeight = FontWeight.Bold)
                    FxSlotRow(
                        name = "Master Reverb",
                        enabled = state.masterReverbOn,
                        amount = state.masterReverbAmt,
                        accent = accent,
                        onToggle = { onSetMasterFx(0, !state.masterReverbOn, state.masterReverbAmt) },
                        onAmount = { onSetMasterFx(0, state.masterReverbOn, it) }
                    )
                    FxSlotRow(
                        name = "Glue Compressor",
                        enabled = state.masterCompOn,
                        amount = state.masterCompAmt,
                        accent = accent,
                        onToggle = { onSetMasterFx(1, !state.masterCompOn, state.masterCompAmt) },
                        onAmount = { onSetMasterFx(1, state.masterCompOn, it) }
                    )
                }

                2 -> {
                    Text("SOUND MACROS", color = EuterpeColors.TextMuted, fontSize = 9.sp, fontWeight = FontWeight.Bold)
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(4.dp)
                    ) {
                        listOf("WARM", "ACID", "GRIT", "DREAM").forEach { macro ->
                            Box(
                                modifier = Modifier
                                    .weight(1f)
                                    .height(26.dp)
                                    .clip(RoundedCornerShape(6.dp))
                                    .background(accent.copy(alpha = 0.18f))
                                    .border(1.dp, accent.copy(alpha = 0.65f), RoundedCornerShape(6.dp))
                                    .clickable { onApplyMacro(macro) },
                                contentAlignment = Alignment.Center
                            ) {
                                Text(
                                    text = macro,
                                    color = accent,
                                    fontSize = 9.sp,
                                    fontWeight = FontWeight.Bold
                                )
                            }
                        }
                    }
                    MixerSliderRow("Tone / Cutoff", state.synthTone, accent, onToneChange)
                    MixerSliderRow("Decay / Release", state.synthDecay, accent, onDecayChange)
                    MixerSliderRow("Resonance / Drive", state.synthDrive, accent, onDriveChange)
                    MixerSliderRow("Idea Density", (state.ideaDensity - 0.15f) / 0.80f, accent) {
                        onDensityChange(0.15f + it * 0.80f)
                    }
                    MixerSliderRow("Groove Swing", state.swing / 0.30f, accent) {
                        onSwingChange(it * 0.30f)
                    }
                }

                3 -> {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text(
                            text = "${state.scale.name} (Cents)",
                            color = accent,
                            fontSize = 11.sp,
                            fontWeight = FontWeight.Bold
                        )
                        Text(
                            text = "RESET",
                            color = EuterpeColors.TextMuted,
                            fontSize = 10.sp,
                            fontWeight = FontWeight.Bold,
                            modifier = Modifier
                                .clip(RoundedCornerShape(4.dp))
                                .background(EuterpeColors.SurfaceElevated)
                                .clickable { onResetTuning() }
                                .padding(horizontal = 6.dp, vertical = 2.dp)
                        )
                    }
                    state.scale.degrees.forEachIndexed { idx, semi ->
                        val totalCents = (semi * 100f).roundToInt()
                        Row(
                            modifier = Modifier.fillMaxWidth(),
                            verticalAlignment = Alignment.CenterVertically,
                            horizontalArrangement = Arrangement.SpaceBetween
                        ) {
                            Text(
                                text = "Deg ${idx + 1}",
                                color = EuterpeColors.TextPrimary,
                                fontSize = 11.sp
                            )
                            Text(
                                text = "${totalCents}¢",
                                color = accent,
                                fontSize = 11.sp,
                                fontWeight = FontWeight.SemiBold
                            )
                            Row(horizontalArrangement = Arrangement.spacedBy(4.dp)) {
                                Text(
                                    text = "−10¢",
                                    color = EuterpeColors.TextPrimary,
                                    fontSize = 10.sp,
                                    modifier = Modifier
                                        .clip(RoundedCornerShape(4.dp))
                                        .background(EuterpeColors.SurfaceElevated)
                                        .clickable { onNudgeDegreeCents(idx, -10f) }
                                        .padding(horizontal = 6.dp, vertical = 3.dp)
                                )
                                Text(
                                    text = "+10¢",
                                    color = EuterpeColors.TextPrimary,
                                    fontSize = 10.sp,
                                    modifier = Modifier
                                        .clip(RoundedCornerShape(4.dp))
                                        .background(EuterpeColors.SurfaceElevated)
                                        .clickable { onNudgeDegreeCents(idx, 10f) }
                                        .padding(horizontal = 6.dp, vertical = 3.dp)
                                )
                            }
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun FxSlotRow(
    name: String,
    enabled: Boolean,
    amount: Float,
    accent: Color,
    onToggle: () -> Unit,
    onAmount: (Float) -> Unit,
) {
    Column {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(6.dp)
            ) {
                Box(
                    modifier = Modifier
                        .clip(RoundedCornerShape(4.dp))
                        .background(if (enabled) accent.copy(alpha = 0.25f) else EuterpeColors.SurfaceElevated)
                        .border(1.dp, if (enabled) accent else EuterpeColors.BorderSubtle, RoundedCornerShape(4.dp))
                        .clickable { onToggle() }
                        .padding(horizontal = 6.dp, vertical = 2.dp)
                ) {
                    Text(
                        text = if (enabled) "ON" else "OFF",
                        color = if (enabled) accent else EuterpeColors.TextMuted,
                        fontSize = 9.sp,
                        fontWeight = FontWeight.Bold
                    )
                }
                Text(name, color = EuterpeColors.TextPrimary, fontSize = 11.sp)
            }
            Text("${(amount * 100f).roundToInt()}%", color = EuterpeColors.TextMuted, fontSize = 11.sp)
        }
        Slider(
            value = amount.coerceIn(0f, 1f),
            onValueChange = onAmount,
            modifier = Modifier.height(22.dp),
            colors = SliderDefaults.colors(
                thumbColor = if (enabled) accent else EuterpeColors.TextMuted,
                activeTrackColor = if (enabled) accent else EuterpeColors.TextMuted,
                inactiveTrackColor = EuterpeColors.SurfaceElevated
            )
        )
    }
}

@Composable
private fun MixerSliderRow(
    label: String,
    value: Float,
    accent: Color,
    onValueChange: (Float) -> Unit,
) {
    Column {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(label, color = EuterpeColors.TextPrimary, fontSize = 11.sp)
            Text("${(value * 100f).roundToInt()}%", color = EuterpeColors.TextMuted, fontSize = 11.sp)
        }
        Slider(
            value = value.coerceIn(0f, 1f),
            onValueChange = onValueChange,
            modifier = Modifier.height(24.dp),
            colors = SliderDefaults.colors(
                thumbColor = accent,
                activeTrackColor = accent,
                inactiveTrackColor = EuterpeColors.SurfaceElevated
            )
        )
    }
}
