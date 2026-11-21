package gr.tkleisas.euterpe.ui.components

import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Casino
import androidx.compose.material.icons.filled.Pause
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Tune
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import gr.tkleisas.euterpe.model.MusicCatalog
import gr.tkleisas.euterpe.model.RhythmSpec
import gr.tkleisas.euterpe.model.ScaleSpec
import gr.tkleisas.euterpe.model.Tradition
import gr.tkleisas.euterpe.ui.theme.EuterpeColors
import gr.tkleisas.euterpe.viewmodel.EuterpeUiState
import kotlin.math.roundToInt

@OptIn(ExperimentalFoundationApi::class)
@Composable
fun TopIdeaBar(
    state: EuterpeUiState,
    accent: Color,
    onSelectTradition: (Tradition) -> Unit,
    onSelectScale: (ScaleSpec) -> Unit,
    onSelectRoot: (Int) -> Unit,
    onSelectRhythm: (RhythmSpec) -> Unit,
    onSelectLayer: (Int) -> Unit,
    onToggleLayerMute: (Int) -> Unit,
    onToggleTransport: () -> Unit,
    onAdjustBpm: (Float) -> Unit,
    onMutateIdea: () -> Unit,
    onToggleMixer: () -> Unit,
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .height(44.dp)
            .background(EuterpeColors.SurfaceDark)
            .padding(horizontal = 8.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(6.dp)
    ) {
        // Mini Euterpe Logo (4-string lyre + 1/4 lit moon)
        MiniEuterpeMark(accent = accent)

        // Tradition Picker
        CompactDropdownChip(
            label = state.tradition.displayName,
            accent = accent,
            highlighted = true,
            items = Tradition.entries,
            itemLabel = { it.displayName },
            onSelect = onSelectTradition
        )

        // Scale / Maqam / Dromos / Raga Picker (current tradition first, all others available!)
        val scales = remember(state.tradition) {
            MusicCatalog.scalesFor(state.tradition) +
                MusicCatalog.scales.filter { it.tradition != state.tradition }
        }
        CompactDropdownChip(
            label = state.scale.name,
            accent = accent,
            items = scales,
            itemLabel = { s ->
                if (s.tradition == state.tradition) s.name
                else "[${s.tradition.displayName}] ${s.name}"
            },
            onSelect = onSelectScale
        )

        // Root Picker
        CompactDropdownChip(
            label = MusicCatalog.rootNames[state.rootSemitone],
            accent = accent,
            items = (0..11).toList(),
            itemLabel = { MusicCatalog.rootNames[it] },
            onSelect = onSelectRoot
        )

        // Rhythm / Iqa / Tala Picker (current tradition first, all traditions mixable!)
        val rhythms = remember(state.tradition) {
            MusicCatalog.rhythmsFor(state.tradition) +
                MusicCatalog.rhythms.filter { it.tradition != state.tradition }
        }
        CompactDropdownChip(
            label = "${state.rhythm.name} (${state.rhythm.meterLabel.substringBefore(" ·")})",
            accent = accent,
            items = rhythms,
            itemLabel = { r ->
                val tag = if (r.tradition == state.tradition) "" else "[${r.tradition.displayName}] "
                "$tag${r.name}  (${r.meterLabel})"
            },
            onSelect = onSelectRhythm
        )

        Spacer(modifier = Modifier.weight(1f))

        // 8 Selectable Layer Slots (Tap = select layer & its sound; Long-press = mute/unmute)
        Row(
            horizontalArrangement = Arrangement.spacedBy(3.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            for (i in 0 until 8) {
                val selected = state.selectedLayer == i
                val recorded = (state.recordedLayerMask and (1 shl i)) != 0
                val muted = (state.mutedLayerMask and (1 shl i)) != 0
                val bg = when {
                    selected -> accent.copy(alpha = 0.28f)
                    recorded && !muted -> Color(0xFF1F362B)
                    else -> EuterpeColors.SurfaceElevated
                }
                val borderCol = when {
                    selected -> accent
                    recorded && !muted -> Color(0xFF4CD97B)
                    muted -> EuterpeColors.TextMuted.copy(alpha = 0.4f)
                    else -> EuterpeColors.BorderSubtle
                }
                Box(
                    modifier = Modifier
                        .size(width = 22.dp, height = 28.dp)
                        .clip(RoundedCornerShape(6.dp))
                        .background(bg)
                        .border(if (selected) 1.5.dp else 1.dp, borderCol, RoundedCornerShape(6.dp))
                        .combinedClickable(
                            onClick = { onSelectLayer(i) },
                            onLongClick = { onToggleLayerMute(i) }
                        ),
                    contentAlignment = Alignment.Center
                ) {
                    Text(
                        text = "${i + 1}",
                        color = when {
                            muted -> EuterpeColors.TextMuted.copy(alpha = 0.45f)
                            selected -> accent
                            recorded -> Color(0xFF4CD97B)
                            else -> EuterpeColors.TextMuted
                        },
                        fontSize = 11.sp,
                        fontWeight = if (selected || recorded) FontWeight.Bold else FontWeight.Medium
                    )
                }
            }
        }

        Spacer(modifier = Modifier.weight(1f))

        // BPM Control
        Row(
            modifier = Modifier
                .clip(RoundedCornerShape(8.dp))
                .background(EuterpeColors.SurfaceElevated)
                .border(1.dp, EuterpeColors.BorderSubtle, RoundedCornerShape(8.dp))
                .padding(horizontal = 4.dp, vertical = 2.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = "−",
                color = EuterpeColors.TextMuted,
                fontSize = 15.sp,
                fontWeight = FontWeight.Bold,
                modifier = Modifier
                    .clickable { onAdjustBpm(-4f) }
                    .padding(horizontal = 6.dp, vertical = 2.dp)
            )
            Text(
                text = "${state.bpm.roundToInt()}",
                color = EuterpeColors.TextPrimary,
                fontSize = 12.sp,
                fontWeight = FontWeight.SemiBold,
                modifier = Modifier.padding(horizontal = 2.dp)
            )
            Text(
                text = "+",
                color = EuterpeColors.TextMuted,
                fontSize = 15.sp,
                fontWeight = FontWeight.Bold,
                modifier = Modifier
                    .clickable { onAdjustBpm(4f) }
                    .padding(horizontal = 6.dp, vertical = 2.dp)
            )
        }

        // Play / Stop Cycle Button
        Box(
            modifier = Modifier
                .size(34.dp)
                .clip(CircleShape)
                .background(if (state.isPlaying) accent else EuterpeColors.SurfaceElevated)
                .border(1.dp, if (state.isPlaying) accent else EuterpeColors.BorderSubtle, CircleShape)
                .clickable { onToggleTransport() },
            contentAlignment = Alignment.Center
        ) {
            Icon(
                imageVector = if (state.isPlaying) Icons.Default.Pause else Icons.Default.PlayArrow,
                contentDescription = "Play Rhythm",
                tint = if (state.isPlaying) EuterpeColors.ObsidianBg else EuterpeColors.TextPrimary,
                modifier = Modifier.size(20.dp)
            )
        }

        // Mutate / Generate Idea (Lua)
        Row(
            modifier = Modifier
                .height(34.dp)
                .clip(RoundedCornerShape(8.dp))
                .background(accent.copy(alpha = 0.20f))
                .border(1.dp, accent, RoundedCornerShape(8.dp))
                .clickable { onMutateIdea() }
                .padding(horizontal = 10.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.spacedBy(4.dp)
        ) {
            Icon(
                imageVector = Icons.Default.Casino,
                contentDescription = "Mutate Idea",
                tint = accent,
                modifier = Modifier.size(16.dp)
            )
            Text(
                text = "IDEA",
                color = accent,
                fontSize = 11.sp,
                fontWeight = FontWeight.Bold,
                letterSpacing = 0.6.sp
            )
        }

        // Mixer / Settings button
        Box(
            modifier = Modifier
                .size(34.dp)
                .clip(RoundedCornerShape(8.dp))
                .background(if (state.showMixerPopup) accent.copy(alpha = 0.25f) else EuterpeColors.SurfaceElevated)
                .border(1.dp, if (state.showMixerPopup) accent else EuterpeColors.BorderSubtle, RoundedCornerShape(8.dp))
                .clickable { onToggleMixer() },
            contentAlignment = Alignment.Center
        ) {
            Icon(
                imageVector = Icons.Default.Tune,
                contentDescription = "Mix",
                tint = if (state.showMixerPopup) accent else EuterpeColors.TextPrimary,
                modifier = Modifier.size(18.dp)
            )
        }
    }
}

@Composable
private fun <T> CompactDropdownChip(
    label: String,
    accent: Color,
    highlighted: Boolean = false,
    items: List<T>,
    itemLabel: (T) -> String,
    onSelect: (T) -> Unit,
) {
    var expanded by remember { mutableStateOf(false) }
    Box {
        Row(
            modifier = Modifier
                .height(32.dp)
                .clip(RoundedCornerShape(8.dp))
                .background(if (highlighted) accent.copy(alpha = 0.16f) else EuterpeColors.SurfaceElevated)
                .border(
                    1.dp,
                    if (highlighted) accent.copy(alpha = 0.7f) else EuterpeColors.BorderSubtle,
                    RoundedCornerShape(8.dp)
                )
                .clickable { expanded = true }
                .padding(horizontal = 9.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = label,
                color = if (highlighted) accent else EuterpeColors.TextPrimary,
                fontSize = 12.sp,
                fontWeight = FontWeight.SemiBold,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
            Spacer(modifier = Modifier.width(3.dp))
            Text(
                text = "▾",
                color = EuterpeColors.TextMuted,
                fontSize = 10.sp
            )
        }

        DropdownMenu(
            expanded = expanded,
            onDismissRequest = { expanded = false },
            modifier = Modifier.background(EuterpeColors.SurfaceElevated)
        ) {
            items.forEach { item ->
                DropdownMenuItem(
                    text = {
                        Text(
                            text = itemLabel(item),
                            color = EuterpeColors.TextPrimary,
                            fontSize = 13.sp
                        )
                    },
                    onClick = {
                        expanded = false
                        onSelect(item)
                    }
                )
            }
        }
    }
}

@Composable
private fun MiniEuterpeMark(accent: Color) {
    Canvas(modifier = Modifier.size(28.dp)) {
        val cx = size.width / 2f
        val cy = size.height / 2f
        val r = size.minDimension * 0.44f

        // 3/4 dark moon
        drawCircle(color = Color(0xFF1C2541), radius = r, center = Offset(cx, cy))
        // 1/4 lit waxing crescent (outer right semicircle + inner curved terminator)
        val crescent = androidx.compose.ui.graphics.Path().apply {
            arcTo(
                rect = androidx.compose.ui.geometry.Rect(cx - r, cy - r, cx + r, cy + r),
                startAngleDegrees = -90f,
                sweepAngleDegrees = 180f,
                forceMoveTo = false
            )
            arcTo(
                rect = androidx.compose.ui.geometry.Rect(cx - r * 0.52f, cy - r, cx + r * 0.52f, cy + r),
                startAngleDegrees = 90f,
                sweepAngleDegrees = -180f,
                forceMoveTo = false
            )
            close()
        }
        drawPath(path = crescent, color = EuterpeColors.LunarIvory)
        // Lyre crossbar
        drawLine(
            color = accent,
            start = Offset(cx - r * 0.55f, cy - r * 0.48f),
            end = Offset(cx + r * 0.55f, cy - r * 0.48f),
            strokeWidth = 2.4f,
            cap = StrokeCap.Round
        )
        // Lyre U-frame
        drawArc(
            color = accent,
            startAngle = 0f,
            sweepAngle = 180f,
            useCenter = false,
            topLeft = Offset(cx - r * 0.50f, cy - r * 0.45f),
            size = Size(r * 1.0f, r * 1.05f),
            style = Stroke(width = 2.4f, cap = StrokeCap.Round)
        )
        // 4 vertical strings
        for (i in 0 until 4) {
            val sx = cx - r * 0.27f + i * (r * 0.18f)
            drawLine(
                color = Color(0xFFFFF5D6),
                start = Offset(sx, cy - r * 0.46f),
                end = Offset(sx, cy + r * 0.48f),
                strokeWidth = 1.1f
            )
        }
    }
}
