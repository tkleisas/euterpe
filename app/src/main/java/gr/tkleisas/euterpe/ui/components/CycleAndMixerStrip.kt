package gr.tkleisas.euterpe.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import gr.tkleisas.euterpe.model.KeyboardLayoutMode
import gr.tkleisas.euterpe.model.MusicCatalog
import gr.tkleisas.euterpe.model.PerformanceMode
import gr.tkleisas.euterpe.ui.theme.EuterpeColors
import gr.tkleisas.euterpe.viewmodel.EuterpeUiState

@Composable
fun CycleAndMixerStrip(
    state: EuterpeUiState,
    accent: Color,
    onSelectMode: (PerformanceMode) -> Unit,
    onToggleLatch: () -> Unit,
    onToggleIdeaLoop: () -> Unit,
    onToggleBassLoop: () -> Unit,
    onLooperToggleRec: () -> Unit,
    onLooperUndo: () -> Unit,
    onLooperClear: () -> Unit,
    onToggleCycleStep: (Int) -> Unit,
    onSelectInstrument: (Int) -> Unit,
    onToggleKeyboardLayout: () -> Unit,
    onShiftOctave: (Int) -> Unit,
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(38.dp)
            .background(EuterpeColors.ObsidianBg)
            .padding(horizontal = 8.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(4.dp)
    ) {
        // Performance Mode dropdown (compact so nothing on the right ever gets clipped!)
        var modeMenuOpen by remember { mutableStateOf(false) }
        Box {
            SmallPill(
                text = "${state.performanceMode.label} ▾",
                selected = state.performanceMode != PerformanceMode.FREE,
                accent = accent,
                onClick = { modeMenuOpen = true }
            )
            DropdownMenu(
                expanded = modeMenuOpen,
                onDismissRequest = { modeMenuOpen = false },
                modifier = Modifier.background(EuterpeColors.SurfaceElevated)
            ) {
                PerformanceMode.entries.forEach { mode ->
                    DropdownMenuItem(
                        text = { Text(mode.label, color = EuterpeColors.TextPrimary, fontSize = 13.sp) },
                        onClick = {
                            modeMenuOpen = false
                            onSelectMode(mode)
                        }
                    )
                }
            }
        }

        SmallPill(
            text = "Latch",
            selected = state.isLatched,
            accent = accent,
            onClick = onToggleLatch
        )

        SmallPill(
            text = "Idea",
            selected = state.isIdeaLoopEnabled,
            accent = EuterpeColors.LunarGold,
            onClick = onToggleIdeaLoop
        )

        SmallPill(
            text = "Bass",
            selected = state.isBassLoopEnabled,
            accent = Color(0xFF4FB3E8),
            onClick = onToggleBassLoop
        )

        // Multi-Layer Phrase Looper: strictly targets the currently selected layer (L1..L8)
        val recColor = Color(0xFFE54B4B)
        val selNum = state.selectedLayer + 1
        val looperActive = state.looperState == 1 || state.looperState == 3
        val selectedHasAudio = (state.recordedLayerMask and (1 shl state.selectedLayer)) != 0
        val looperLabel = if (looperActive) "■ L$selNum" else "● L$selNum"
        SmallPill(
            text = looperLabel,
            selected = looperActive || selectedHasAudio,
            accent = if (looperActive) recColor else Color(0xFF4CD97B),
            onClick = onLooperToggleRec
        )
        if (selectedHasAudio || looperActive) {
            SmallPill(
                text = "↩",
                selected = false,
                accent = accent,
                onClick = onLooperUndo
            )
        }
        if (state.looperLayers > 0) {
            SmallPill(
                text = "✕",
                selected = false,
                accent = recColor,
                onClick = onLooperClear
            )
        }

        // Interactive Live Asymmetric Cycle Step Visualizer (tap any step to toggle hit!)
        Row(
            modifier = Modifier
                .weight(1f)
                .height(26.dp)
                .clip(RoundedCornerShape(6.dp))
                .background(EuterpeColors.SurfaceDark)
                .border(1.dp, EuterpeColors.BorderSubtle, RoundedCornerShape(6.dp))
                .padding(horizontal = 3.dp, vertical = 3.dp),
            horizontalArrangement = Arrangement.spacedBy(2.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            val count = state.rhythm.stepCount.coerceIn(4, 24)
            for (step in 0 until count) {
                val isCurrent = state.isPlaying && (state.currentStep % count == step)
                val isActive = step in state.activeCycleSteps
                val color = when {
                    isCurrent -> accent
                    isActive -> accent.copy(alpha = 0.45f)
                    else -> EuterpeColors.SurfaceElevated
                }
                Box(
                    modifier = Modifier
                        .weight(1f)
                        .height(if (isActive) 18.dp else 11.dp)
                        .clip(RoundedCornerShape(3.dp))
                        .background(color)
                        .clickable { onToggleCycleStep(step) }
                )
            }
        }

        // 12 Curated Sound Presets selector
        var instMenuOpen by remember { mutableStateOf(false) }
        Box {
            SmallPill(
                text = "${MusicCatalog.instruments.getOrNull(state.instrumentIndex)?.name ?: "Oud"} ▾",
                selected = false,
                accent = accent,
                onClick = { instMenuOpen = true },
                modifier = Modifier.widthIn(max = 115.dp)
            )
            DropdownMenu(
                expanded = instMenuOpen,
                onDismissRequest = { instMenuOpen = false },
                modifier = Modifier.background(EuterpeColors.SurfaceElevated)
            ) {
                MusicCatalog.instruments.forEach { inst ->
                    DropdownMenuItem(
                        text = {
                            Text(
                                text = inst.name,
                                color = if (inst.index == state.instrumentIndex) accent else EuterpeColors.TextPrimary,
                                fontWeight = if (inst.index == state.instrumentIndex) FontWeight.Bold else FontWeight.Normal,
                                fontSize = 13.sp
                            )
                        },
                        onClick = {
                            instMenuOpen = false
                            onSelectInstrument(inst.index)
                        }
                    )
                }
            }
        }

        // Keyboard mode toggle: 12-Key Piano vs Scale-locked
        SmallPill(
            text = if (state.keyboardLayout == KeyboardLayoutMode.PIANO_12) "12-Key" else "Scale",
            selected = state.keyboardLayout == KeyboardLayoutMode.SCALE_LOCKED,
            accent = accent,
            onClick = onToggleKeyboardLayout
        )

        // Guaranteed-visible Octave Stepper (- O3 +)
        Row(
            modifier = Modifier
                .height(26.dp)
                .clip(RoundedCornerShape(6.dp))
                .background(EuterpeColors.SurfaceDark)
                .border(1.dp, EuterpeColors.BorderSubtle, RoundedCornerShape(6.dp)),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = "−",
                color = if (state.baseOctave > 0) EuterpeColors.TextPrimary else EuterpeColors.TextMuted.copy(alpha = 0.4f),
                fontSize = 14.sp,
                fontWeight = FontWeight.Bold,
                modifier = Modifier
                    .clickable { onShiftOctave(-1) }
                    .padding(horizontal = 7.dp, vertical = 2.dp)
            )
            Text(
                text = "O${state.baseOctave}",
                color = accent,
                fontSize = 11.sp,
                fontWeight = FontWeight.Bold
            )
            Text(
                text = "+",
                color = if (state.baseOctave < 7) EuterpeColors.TextPrimary else EuterpeColors.TextMuted.copy(alpha = 0.4f),
                fontSize = 14.sp,
                fontWeight = FontWeight.Bold,
                modifier = Modifier
                    .clickable { onShiftOctave(1) }
                    .padding(horizontal = 7.dp, vertical = 2.dp)
            )
        }
    }
}

@Composable
private fun SmallPill(
    text: String,
    selected: Boolean,
    accent: Color,
    onClick: () -> Unit,
    modifier: Modifier = Modifier,
) {
    Box(
        modifier = modifier
            .height(26.dp)
            .clip(RoundedCornerShape(6.dp))
            .background(if (selected) accent.copy(alpha = 0.24f) else EuterpeColors.SurfaceDark)
            .border(
                1.dp,
                if (selected) accent else EuterpeColors.BorderSubtle,
                RoundedCornerShape(6.dp)
            )
            .clickable { onClick() }
            .padding(horizontal = 7.dp),
        contentAlignment = Alignment.Center
    ) {
        Text(
            text = text,
            color = if (selected) accent else EuterpeColors.TextMuted,
            fontSize = 11.sp,
            fontWeight = if (selected) FontWeight.Bold else FontWeight.Medium,
            maxLines = 1,
            overflow = TextOverflow.Ellipsis
        )
    }
}
