package gr.tkleisas.euterpe.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
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
import gr.tkleisas.euterpe.model.ModifierSpec
import gr.tkleisas.euterpe.model.Tradition
import gr.tkleisas.euterpe.ui.theme.EuterpeColors

@Composable
fun ModifierPadGrid(
    modifierBank: Tradition,
    modifiers: List<ModifierSpec>,
    activeMask: Int,
    accent: Color,
    onSelectModifierBank: (Tradition) -> Unit,
    onToggleModifier: (Int) -> Unit,
    modifier: Modifier = Modifier,
) {
    var bankMenuOpen by remember { mutableStateOf(false) }
    val rows = modifiers.chunked(2)

    Column(
        modifier = modifier
            .fillMaxHeight()
            .background(EuterpeColors.SurfaceDark, RoundedCornerShape(12.dp))
            .border(1.dp, EuterpeColors.BorderSubtle, RoundedCornerShape(12.dp))
            .padding(6.dp),
        verticalArrangement = Arrangement.spacedBy(5.dp)
    ) {
        // Independent Modifier Bank picker (doesn't clobber Scale / Rhythm / Synth!)
        Box(modifier = Modifier.fillMaxWidth()) {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(22.dp)
                    .clip(RoundedCornerShape(6.dp))
                    .background(accent.copy(alpha = 0.14f))
                    .border(1.dp, accent.copy(alpha = 0.45f), RoundedCornerShape(6.dp))
                    .clickable { bankMenuOpen = true }
                    .padding(horizontal = 6.dp),
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text(
                    text = "MODS: ${modifierBank.displayName.uppercase()}",
                    color = accent,
                    fontSize = 9.sp,
                    fontWeight = FontWeight.Bold,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis
                )
                Text(text = "▾", color = accent, fontSize = 10.sp)
            }
            DropdownMenu(
                expanded = bankMenuOpen,
                onDismissRequest = { bankMenuOpen = false },
                modifier = Modifier.background(EuterpeColors.SurfaceElevated)
            ) {
                Tradition.entries.forEach { bank ->
                    DropdownMenuItem(
                        text = {
                            Text(
                                text = "${bank.displayName} Modifiers",
                                color = EuterpeColors.TextPrimary,
                                fontSize = 12.sp
                            )
                        },
                        onClick = {
                            bankMenuOpen = false
                            onSelectModifierBank(bank)
                        }
                    )
                }
            }
        }

        rows.forEach { rowItems ->
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .weight(1f),
                horizontalArrangement = Arrangement.spacedBy(5.dp)
            ) {
                rowItems.forEach { mod ->
                    val active = (activeMask and (1 shl mod.bit)) != 0
                    Column(
                        modifier = Modifier
                            .weight(1f)
                            .fillMaxHeight()
                            .clip(RoundedCornerShape(9.dp))
                            .background(
                                if (active) accent.copy(alpha = 0.28f)
                                else EuterpeColors.SurfaceElevated
                            )
                            .border(
                                width = if (active) 1.5.dp else 1.dp,
                                color = if (active) accent else EuterpeColors.BorderSubtle,
                                shape = RoundedCornerShape(9.dp)
                            )
                            .clickable { onToggleModifier(mod.bit) }
                            .padding(horizontal = 3.dp, vertical = 2.dp),
                        verticalArrangement = Arrangement.Center,
                        horizontalAlignment = Alignment.CenterHorizontally
                    ) {
                        Text(
                            text = mod.label,
                            color = if (active) accent else EuterpeColors.TextPrimary,
                            fontSize = 11.sp,
                            fontWeight = FontWeight.Bold,
                            maxLines = 1,
                            overflow = TextOverflow.Ellipsis
                        )
                        Text(
                            text = mod.subLabel,
                            color = if (active) EuterpeColors.LunarIvory else EuterpeColors.TextMuted,
                            fontSize = 8.sp,
                            maxLines = 1,
                            overflow = TextOverflow.Ellipsis
                        )
                    }
                }
            }
        }
    }
}
