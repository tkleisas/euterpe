package gr.tkleisas.euterpe

import gr.tkleisas.euterpe.model.KeyboardLayoutMode
import gr.tkleisas.euterpe.model.MusicCatalog
import gr.tkleisas.euterpe.model.Tradition
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Test
import kotlin.math.abs

class MusicCatalogTest {

    @Test
    fun allTraditionsHaveValidDefaults() {
        for (t in Tradition.entries) {
            val scale = MusicCatalog.findScale(t.defaultScaleId)
            val rhythm = MusicCatalog.findRhythm(t.defaultRhythmId)
            val mods = MusicCatalog.modifiersFor(t)
            assertEquals(t.defaultScaleId, scale.id)
            assertEquals(t.defaultRhythmId, rhythm.id)
            assertTrue("Tradition $t should have >= 6 modifiers", mods.size >= 6)
        }
    }

    @Test
    fun maqamBayatiScaleLockedKeyboardPreservesQuarterTones() {
        val bayati = MusicCatalog.findScale("bayati")
        val keys = MusicCatalog.buildTwoOctaveKeys(
            scale = bayati,
            rootSemitone = 2, // D
            baseOctave = 3,
            layout = KeyboardLayoutMode.SCALE_LOCKED
        )
        assertEquals(15, keys.size) // 7 * 2 + 1
        assertEquals(50, keys[0].midiNote) // D3
        assertTrue(keys[0].isRoot)
        // Degree 1 in Bayati is +1.5 semitones -> E half-flat (±50 cents badge)
        assertEquals(50f, abs(keys[1].cents), 1f)
        assertNotNull(keys[1].microBadge)
    }

    @Test
    fun gamelanSlendroProduces11ScaleLockedKeysForTwoOctaves() {
        val slendro = MusicCatalog.findScale("slendro")
        val keys = MusicCatalog.buildTwoOctaveKeys(
            scale = slendro,
            rootSemitone = 0,
            baseOctave = 3,
            layout = KeyboardLayoutMode.SCALE_LOCKED
        )
        assertEquals(11, keys.size) // 5 * 2 + 1
        assertEquals("1", keys[0].label)
        assertEquals("2", keys[1].label)
        assertTrue(abs(keys[1].cents - 40f) < 1f)
    }

    @Test
    fun piano12ModeProduces25KeysWithMicrotonalMarkers() {
        val rast = MusicCatalog.findScale("rast")
        val keys = MusicCatalog.buildTwoOctaveKeys(
            scale = rast,
            rootSemitone = 0, // C
            baseOctave = 3,
            layout = KeyboardLayoutMode.PIANO_12
        )
        assertEquals(25, keys.size)
        // Rast neutral 3rd is 3.5 semitones -> Eb or E gets a ±50c microtonal badge
        val hasMicro = keys.any { it.microBadge == "½♭" || it.microBadge == "½♯" }
        assertTrue("Rast in Piano mode should show quarter-tone badge", hasMicro)
    }
}
