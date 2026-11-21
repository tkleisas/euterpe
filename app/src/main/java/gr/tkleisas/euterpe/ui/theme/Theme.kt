package gr.tkleisas.euterpe.ui.theme

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color
import gr.tkleisas.euterpe.model.Tradition

object EuterpeColors {
    val ObsidianBg = Color(0xFF0E111A)
    val SurfaceDark = Color(0xFF161B29)
    val SurfaceElevated = Color(0xFF1E2538)
    val BorderSubtle = Color(0xFF2B344D)
    val LunarGold = Color(0xFFE8B959)
    val LunarIvory = Color(0xFFF4E3B2)
    val TextPrimary = Color(0xFFF1F3F8)
    val TextMuted = Color(0xFF919BB3)

    // Tradition-specific studio accents
    fun accentFor(tradition: Tradition): Color = when (tradition) {
        Tradition.ARABIC -> Color(0xFFE8B959)   // Warm Oud Gold
        Tradition.GREEK -> Color(0xFF4FB3E8)    // Aegean Azure
        Tradition.RAGA -> Color(0xFFFF8A4C)     // Saffron Amber
        Tradition.GAMELAN -> Color(0xFF3FD0A4)  // Bronze Jade
        Tradition.WESTERN -> Color(0xFF9C7CFF)  // Studio Violet
        Tradition.TECHNO -> Color(0xFF39FF14)   // Acid 303 Neon
        Tradition.METAL -> Color(0xFFFF4D5A)    // Crimson Riff
        Tradition.POP -> Color(0xFFFF6EC7)      // Synth Pop Magenta
    }
}

private val EuterpeColorScheme = darkColorScheme(
    primary = EuterpeColors.LunarGold,
    onPrimary = Color(0xFF11131A),
    background = EuterpeColors.ObsidianBg,
    onBackground = EuterpeColors.TextPrimary,
    surface = EuterpeColors.SurfaceDark,
    onSurface = EuterpeColors.TextPrimary,
    surfaceVariant = EuterpeColors.SurfaceElevated,
    onSurfaceVariant = EuterpeColors.TextMuted,
    outline = EuterpeColors.BorderSubtle,
)

@Composable
fun EuterpeTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = EuterpeColorScheme,
        content = content
    )
}
