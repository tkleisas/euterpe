package gr.tkleisas.euterpe.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.systemGestureExclusion
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.input.pointer.PointerEventType
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.unit.dp
import gr.tkleisas.euterpe.model.KeySpec
import gr.tkleisas.euterpe.model.KeyboardLayoutMode
import gr.tkleisas.euterpe.ui.theme.EuterpeColors

private data class KeyRect(
    val key: KeySpec,
    val rect: Rect,
)

@Composable
fun TwoOctaveKeyboard(
    keys: List<KeySpec>,
    layout: KeyboardLayoutMode,
    accent: Color,
    onNoteOn: (pointerId: Int, midiNote: Int, cents: Float, velocity: Float) -> Unit,
    onNoteOff: (pointerId: Int) -> Unit,
    modifier: Modifier = Modifier,
) {
    val currentKeys by rememberUpdatedState(keys)
    val currentLayout by rememberUpdatedState(layout)
    val currentOnNoteOn by rememberUpdatedState(onNoteOn)
    val currentOnNoteOff by rememberUpdatedState(onNoteOff)

    // pointerId (Long) -> active KeySpec.index
    val activePointers = remember { mutableStateMapOf<Long, Int>() }

    fun computeKeyRects(width: Float, height: Float, keyList: List<KeySpec>, mode: KeyboardLayoutMode): List<KeyRect> {
        if (width <= 0f || height <= 0f || keyList.isEmpty()) return emptyList()

        if (mode == KeyboardLayoutMode.SCALE_LOCKED) {
            val w = width / keyList.size
            val gap = 3f
            return keyList.mapIndexed { i, k ->
                KeyRect(k, Rect(i * w + gap * 0.5f, 4f, (i + 1) * w - gap * 0.5f, height - 4f))
            }
        }

        // 12-Key Piano layout: draw white keys first, black keys on top
        val whiteKeys = keyList.filter { !it.isBlackKey }
        val whiteW = width / whiteKeys.size.coerceAtLeast(1)
        val blackW = whiteW * 0.62f
        val blackH = height * 0.58f

        val rects = mutableListOf<KeyRect>()
        val whiteIndexByKey = mutableMapOf<Int, Int>()
        var wIdx = 0
        for (k in keyList) {
            if (!k.isBlackKey) {
                whiteIndexByKey[k.index] = wIdx
                rects.add(KeyRect(k, Rect(wIdx * whiteW + 1.5f, 2f, (wIdx + 1) * whiteW - 1.5f, height - 2f)))
                wIdx++
            }
        }
        // Position each black key centered on the boundary after its preceding white key
        for (k in keyList) {
            if (k.isBlackKey) {
                val prevWhite = whiteIndexByKey[k.index - 1] ?: continue
                val centerX = (prevWhite + 1) * whiteW
                rects.add(KeyRect(k, Rect(centerX - blackW * 0.5f, 2f, centerX + blackW * 0.5f, blackH)))
            }
        }
        return rects
    }

    fun hitTest(pos: Offset, rects: List<KeyRect>): KeyRect? {
        // Check black keys (later in list) first so they take priority over white keys
        for (i in rects.indices.reversed()) {
            if (rects[i].rect.contains(pos)) return rects[i]
        }
        return null
    }

    Box(
        modifier = modifier
            .fillMaxSize()
            .systemGestureExclusion()
            .clip(RoundedCornerShape(12.dp))
            .background(EuterpeColors.SurfaceDark)
            .border(1.dp, EuterpeColors.BorderSubtle, RoundedCornerShape(12.dp))
            .pointerInput(Unit) {
                awaitPointerEventScope {
                    while (true) {
                        val event = awaitPointerEvent()
                        val rects = computeKeyRects(
                            size.width.toFloat(),
                            size.height.toFloat(),
                            currentKeys,
                            currentLayout
                        )
                        for (change in event.changes) {
                            val pid = change.id.value
                            val pidInt = (pid and 0x7FFFFFFFL).toInt()

                            if (change.pressed) {
                                val hit = hitTest(change.position, rects)
                                val prevIdx = activePointers[pid]
                                if (hit != null && hit.key.index != prevIdx) {
                                    if (prevIdx != null) {
                                        currentOnNoteOff(pidInt)
                                    }
                                    activePointers[pid] = hit.key.index
                                    val relY = ((change.position.y - hit.rect.top) / hit.rect.height.coerceAtLeast(1f))
                                        .coerceIn(0f, 1f)
                                    val vel = 0.55f + 0.42f * relY
                                    currentOnNoteOn(pidInt, hit.key.midiNote, hit.key.cents, vel)
                                } else if (hit == null && prevIdx != null) {
                                    activePointers.remove(pid)
                                    currentOnNoteOff(pidInt)
                                }
                            } else if (event.type == PointerEventType.Release || !change.pressed) {
                                if (activePointers.remove(pid) != null) {
                                    currentOnNoteOff(pidInt)
                                }
                            }
                            change.consume()
                        }
                    }
                }
            }
    ) {
        val labelPaint = remember {
            android.graphics.Paint().apply {
                isAntiAlias = true
                textAlign = android.graphics.Paint.Align.CENTER
                isFakeBoldText = true
            }
        }
        val badgePaint = remember {
            android.graphics.Paint().apply {
                isAntiAlias = true
                textAlign = android.graphics.Paint.Align.CENTER
                isFakeBoldText = true
            }
        }

        Canvas(modifier = Modifier.fillMaxSize()) {
            val rects = computeKeyRects(size.width, size.height, keys, layout)
            val pressedIndices = activePointers.values.toSet()

            for (kr in rects) {
                val k = kr.key
                val r = kr.rect
                val pressed = k.index in pressedIndices

                val fillColor = when {
                    pressed -> accent
                    layout == KeyboardLayoutMode.SCALE_LOCKED && k.isRoot -> accent.copy(alpha = 0.24f)
                    layout == KeyboardLayoutMode.SCALE_LOCKED -> EuterpeColors.SurfaceElevated
                    k.isBlackKey -> if (k.isInScale) Color(0xFF1E2438) else Color(0xFF10131D)
                    k.isRoot -> Color(0xFFECE3CE)
                    k.isInScale -> Color(0xFFDDE2EC)
                    else -> Color(0xFF8A92A6)
                }

                val corner = if (k.isBlackKey) CornerRadius(6f, 6f) else CornerRadius(10f, 10f)
                drawRoundRect(
                    color = fillColor,
                    topLeft = r.topLeft,
                    size = Size(r.width, r.height),
                    cornerRadius = corner
                )

                val strokeColor = when {
                    pressed -> EuterpeColors.LunarIvory
                    k.isRoot -> accent
                    layout == KeyboardLayoutMode.SCALE_LOCKED -> EuterpeColors.BorderSubtle
                    k.isBlackKey -> Color(0xFF2D3652)
                    else -> Color(0xFF394158)
                }
                drawRoundRect(
                    color = strokeColor,
                    topLeft = r.topLeft,
                    size = Size(r.width, r.height),
                    cornerRadius = corner,
                    style = Stroke(width = if (k.isRoot || pressed) 2.5f else 1.2f)
                )

                // In-scale indicator dot in Piano mode
                if (layout == KeyboardLayoutMode.PIANO_12 && k.isInScale && !pressed) {
                    drawCircle(
                        color = if (k.isRoot) accent else accent.copy(alpha = 0.72f),
                        radius = if (k.isBlackKey) 4.5f else 5.5f,
                        center = Offset(r.center.x, r.bottom - (if (k.isBlackKey) 26f else 36f))
                    )
                }

                // Draw key label + microtonal badge
                val textColor = when {
                    pressed -> EuterpeColors.ObsidianBg
                    layout == KeyboardLayoutMode.SCALE_LOCKED -> if (k.isRoot) accent else EuterpeColors.TextPrimary
                    k.isBlackKey -> EuterpeColors.TextPrimary
                    else -> Color(0xFF161B29)
                }
                labelPaint.color = textColor.toArgb()
                labelPaint.textSize = (r.width * 0.34f).coerceIn(18f, 34f)
                drawContext.canvas.nativeCanvas.drawText(
                    k.label,
                    r.center.x,
                    r.bottom - 14f,
                    labelPaint
                )

                if (k.microBadge != null) {
                    badgePaint.color = (if (pressed) EuterpeColors.ObsidianBg else accent).toArgb()
                    badgePaint.textSize = (r.width * 0.28f).coerceIn(15f, 26f)
                    drawContext.canvas.nativeCanvas.drawText(
                        k.microBadge,
                        r.center.x,
                        r.top + 30f,
                        badgePaint
                    )
                }
            }
        }
    }
}
