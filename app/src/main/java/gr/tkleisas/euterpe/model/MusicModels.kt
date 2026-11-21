package gr.tkleisas.euterpe.model

import kotlin.math.abs
import kotlin.math.floor
import kotlin.math.roundToInt

enum class Tradition(
    val nativeId: Int,
    val displayName: String,
    val defaultScaleId: String,
    val defaultRhythmId: String,
    val defaultInstrumentIndex: Int,
    val defaultRoot: Int,
    val defaultBpm: Float,
) {
    ARABIC(1, "Maqam", "bayati", "maqsum", 0, 2, 102f),
    GREEK(2, "Greek", "ousak", "zeibekiko", 1, 2, 88f),
    RAGA(3, "Raga", "raga_yaman", "teentaal", 2, 0, 84f),
    GAMELAN(4, "Gamelan", "slendro", "lancaran", 4, 0, 96f),
    WESTERN(0, "Western", "dorian", "west_groove", 6, 0, 110f),
    TECHNO(5, "Techno", "natural_minor", "techno_4x4", 12, 5, 130f),
    METAL(6, "Metal", "hungarian_gypsy", "metal_gallop", 15, 4, 142f),
    POP(7, "Pop", "major", "pop_four", 17, 0, 118f),
}

data class ScaleSpec(
    val id: String,
    val name: String,
    val tradition: Tradition,
    val degrees: List<Float>, // semitones relative to root (floats carry cents)
    val degreeLabels: List<String> = emptyList(),
)

data class RhythmSpec(
    val id: String,
    val name: String,
    val meterLabel: String, // e.g. "9/8 · 2+2+2+3"
    val tradition: Tradition,
    val stepCount: Int,
    val accentSteps: Set<Int> = setOf(0),
)

data class ModifierSpec(
    val bit: Int,
    val label: String,
    val subLabel: String,
    val isToggle: Boolean = true,
)

enum class PerformanceMode(val nativeId: Int, val label: String) {
    FREE(0, "Free"),
    STRUM(1, "Strum"),
    ARP(2, "Arp"),
    PHRASE(3, "Phrase"),
}

enum class KeyboardLayoutMode(val label: String) {
    PIANO_12("Piano"),
    SCALE_LOCKED("Scale"),
}

enum class PercussionKit(val nativeId: Int, val label: String) {
    NATIVE(0, "Native"),
    ARABIC(1, "Darbuka"),
    GREEK(2, "Daouli"),
    TABLA(3, "Tabla"),
    GAMELAN(4, "Gamelan"),
    WESTERN(5, "West Kit"),
}

data class InstrumentSpec(
    val index: Int,
    val name: String,
)

data class KeySpec(
    val index: Int,
    val midiNote: Int,
    val cents: Float,
    val label: String,
    val microBadge: String?,
    val isBlackKey: Boolean,
    val isInScale: Boolean,
    val isRoot: Boolean,
)

object MusicCatalog {
    val rootNames = listOf("C", "C♯", "D", "E♭", "E", "F", "F♯", "G", "A♭", "A", "B♭", "B")

    val instruments = listOf(
        InstrumentSpec(0, "Oud"),
        InstrumentSpec(1, "Bouzouki"),
        InstrumentSpec(2, "Sitar"),
        InstrumentSpec(3, "Kanun / Harp"),
        InstrumentSpec(4, "Gamelan Saron"),
        InstrumentSpec(5, "Balinese Bell"),
        InstrumentSpec(6, "Rhodes EP"),
        InstrumentSpec(7, "Wurlitzer EP"),
        InstrumentSpec(8, "Suitcase EP"),
        InstrumentSpec(9, "Ney / Wind"),
        InstrumentSpec(10, "Harmonic Pad"),
        InstrumentSpec(11, "Analog Duduk"),
        InstrumentSpec(12, "Acid TB-303"),
        InstrumentSpec(13, "Deep Sub Bass"),
        InstrumentSpec(14, "FM Techno Bass"),
        InstrumentSpec(15, "Metal Guitar"),
        InstrumentSpec(16, "Pop Pluck"),
        InstrumentSpec(17, "Synth Brass"),
        InstrumentSpec(18, "Polivoks Lead"),
        InstrumentSpec(19, "Polivoks Acid"),
        InstrumentSpec(20, "Polivoks Pad"),
    )

    val scales: List<ScaleSpec> = listOf(
        // Arabic / Levantine Maqams
        ScaleSpec("bayati", "Bayati", Tradition.ARABIC, listOf(0f, 1.5f, 3f, 5f, 7f, 8.5f, 10f)),
        ScaleSpec("rast", "Rast", Tradition.ARABIC, listOf(0f, 2f, 3.5f, 5f, 7f, 9f, 10.5f)),
        ScaleSpec("hijaz", "Hijaz", Tradition.ARABIC, listOf(0f, 1f, 4f, 5f, 7f, 8f, 10f)),
        ScaleSpec("hijaz_kar", "Hijaz Kar", Tradition.ARABIC, listOf(0f, 1f, 4f, 5f, 7f, 8f, 11f)),
        ScaleSpec("saba", "Saba", Tradition.ARABIC, listOf(0f, 1.5f, 3f, 4f, 7f, 8f, 10f)),
        ScaleSpec("kurd", "Kurd", Tradition.ARABIC, listOf(0f, 1f, 3f, 5f, 7f, 8f, 10f)),
        ScaleSpec("nahawand", "Nahawand", Tradition.ARABIC, listOf(0f, 2f, 3f, 5f, 7f, 8f, 11f)),
        ScaleSpec("segah", "Segah", Tradition.ARABIC, listOf(0f, 1.5f, 3.5f, 5.5f, 7f, 8.5f, 10.5f)),

        // Greek Dromoi
        ScaleSpec("ousak", "Ousak", Tradition.GREEK, listOf(0f, 1.15f, 3f, 5f, 7f, 8f, 10f)),
        ScaleSpec("hitzaz", "Hitzaz", Tradition.GREEK, listOf(0f, 1f, 4f, 5f, 7f, 8f, 10f)),
        ScaleSpec("hitzazkiar", "Hitzazkiar", Tradition.GREEK, listOf(0f, 1f, 4f, 5f, 7f, 8f, 11f)),
        ScaleSpec("niavent", "Niavent", Tradition.GREEK, listOf(0f, 2f, 3f, 5f, 7f, 8f, 11f)),
        ScaleSpec("greek_rast", "Rast (Gr)", Tradition.GREEK, listOf(0f, 2f, 3.65f, 5f, 7f, 9f, 10.65f)),
        ScaleSpec("houzam", "Houzam", Tradition.GREEK, listOf(0f, 3f, 4f, 5f, 7f, 8f, 11f)),
        ScaleSpec("sabah", "Sabah", Tradition.GREEK, listOf(0f, 1.25f, 3f, 4f, 7f, 8f, 10f)),
        ScaleSpec("pireotikos", "Pireotikos", Tradition.GREEK, listOf(0f, 1f, 4f, 6f, 7f, 8f, 11f)),

        // Indian Ragas
        ScaleSpec(
            "raga_yaman", "Yaman", Tradition.RAGA,
            listOf(0f, 2f, 4f, 6f, 7f, 9f, 11f),
            listOf("Sa", "Re", "Ga", "Ma♯", "Pa", "Dha", "Ni")
        ),
        ScaleSpec(
            "raga_bhairav", "Bhairav", Tradition.RAGA,
            listOf(0f, 1f, 4f, 5f, 7f, 8f, 11f),
            listOf("Sa", "re", "Ga", "Ma", "Pa", "dha", "Ni")
        ),
        ScaleSpec(
            "raga_bhimpalasi", "Bhimpalasi", Tradition.RAGA,
            listOf(0f, 2f, 3f, 5f, 7f, 8f, 10f),
            listOf("Sa", "Re", "ga", "Ma", "Pa", "dha", "ni")
        ),
        ScaleSpec(
            "raga_bageshri", "Bageshri", Tradition.RAGA,
            listOf(0f, 2f, 3f, 5f, 7f, 9f, 10f),
            listOf("Sa", "Re", "ga", "Ma", "Pa", "Dha", "ni")
        ),
        ScaleSpec(
            "raga_todi", "Todi", Tradition.RAGA,
            listOf(0f, 1f, 3f, 6f, 7f, 8f, 11f),
            listOf("Sa", "re", "ga", "Ma♯", "Pa", "dha", "Ni")
        ),

        // Gamelan
        ScaleSpec(
            "slendro", "Sléndro", Tradition.GAMELAN,
            listOf(0f, 2.40f, 4.82f, 7.22f, 9.60f),
            listOf("1", "2", "3", "5", "6")
        ),
        ScaleSpec(
            "pelog_bem", "Pélog Bem", Tradition.GAMELAN,
            listOf(0f, 1.20f, 2.75f, 6.75f, 7.85f),
            listOf("1", "2", "3", "5", "6")
        ),
        ScaleSpec(
            "pelog_barang", "Pélog Barang", Tradition.GAMELAN,
            listOf(0f, 1.55f, 4.00f, 6.65f, 9.45f),
            listOf("2", "3", "5", "6", "7")
        ),
        ScaleSpec(
            "pelog_7", "Pélog 7", Tradition.GAMELAN,
            listOf(0f, 1.20f, 2.75f, 5.20f, 6.75f, 7.85f, 9.65f),
            listOf("1", "2", "3", "4", "5", "6", "7")
        ),

        // Western
        ScaleSpec("major", "Major", Tradition.WESTERN, listOf(0f, 2f, 4f, 5f, 7f, 9f, 11f)),
        ScaleSpec("natural_minor", "Minor", Tradition.WESTERN, listOf(0f, 2f, 3f, 5f, 7f, 8f, 10f)),
        ScaleSpec("dorian", "Dorian", Tradition.WESTERN, listOf(0f, 2f, 3f, 5f, 7f, 9f, 10f)),
        ScaleSpec("lydian", "Lydian", Tradition.WESTERN, listOf(0f, 2f, 4f, 6f, 7f, 9f, 11f)),
        ScaleSpec("mixolydian", "Mixolydian", Tradition.WESTERN, listOf(0f, 2f, 4f, 5f, 7f, 9f, 10f)),
        ScaleSpec("pentatonic_minor", "Pent Min", Tradition.WESTERN, listOf(0f, 3f, 5f, 7f, 10f)),

        // Exotic & Adventurous (Persian, Turkish, Japanese, Ethiopian, Microtonal)
        ScaleSpec("persian_shur", "Persian Shur", Tradition.ARABIC, listOf(0f, 1.40f, 3f, 5f, 7f, 8.15f, 10f)),
        ScaleSpec("persian_chahargah", "Chahargah", Tradition.ARABIC, listOf(0f, 1.40f, 4f, 5f, 7f, 8.40f, 11f)),
        ScaleSpec("turkish_huseyni", "Hüseyni", Tradition.ARABIC, listOf(0f, 1.65f, 3f, 5f, 7f, 8.80f, 10f)),
        ScaleSpec("japanese_hirajoshi", "Hirajoshi", Tradition.GAMELAN, listOf(0f, 2f, 3f, 7f, 8f)),
        ScaleSpec("japanese_insen", "In-Sen", Tradition.GAMELAN, listOf(0f, 1f, 5f, 7f, 10f)),
        ScaleSpec("japanese_iwato", "Iwato", Tradition.GAMELAN, listOf(0f, 1f, 5f, 6f, 10f)),
        ScaleSpec("ethiopian_tizita", "Tizita", Tradition.RAGA, listOf(0f, 2f, 4f, 7f, 9f)),
        ScaleSpec("ethiopian_anchihoye", "Anchihoye", Tradition.RAGA, listOf(0f, 1f, 5f, 6f, 9f)),
        ScaleSpec("hungarian_gypsy", "Hungarian Gypsy", Tradition.GREEK, listOf(0f, 2f, 3f, 6f, 7f, 8f, 11f)),
        ScaleSpec("harmonic_7th_ji", "Harmonic 7th JI", Tradition.WESTERN, listOf(0f, 2.04f, 3.86f, 4.98f, 7.02f, 8.84f, 9.69f)),
        ScaleSpec("prometheus_mystic", "Prometheus", Tradition.WESTERN, listOf(0f, 2f, 4f, 6f, 9f, 10f)),
        ScaleSpec("tet17_neutral", "17-TET Neutral", Tradition.ARABIC, listOf(0f, 1.41f, 3.53f, 4.94f, 7.06f, 8.47f, 10.59f)),
    )

    val rhythms: List<RhythmSpec> = listOf(
        // Arabic / Levantine
        RhythmSpec("maqsum", "Maqsum", "4/4 · D-T- -T-D -T-", Tradition.ARABIC, 16, setOf(0, 6, 8, 12)),
        RhythmSpec("baladi", "Baladi", "4/4 · D-D- -T-D -T-", Tradition.ARABIC, 16, setOf(0, 2, 6, 8, 12)),
        RhythmSpec("saidi", "Saidi", "4/4 · D-T- -D-D -T-", Tradition.ARABIC, 16, setOf(0, 6, 8, 12)),
        RhythmSpec("samai_thaqil", "Samai Thaqil", "10/8 · 3+2+2+3", Tradition.ARABIC, 10, setOf(0, 3, 5, 7)),
        RhythmSpec("malfuf", "Malfuf", "2/4 · D--T --T-", Tradition.ARABIC, 8, setOf(0, 3, 6)),
        RhythmSpec("murabba_13", "Murabba", "13/8 · 3+3+3+2+2", Tradition.ARABIC, 13, setOf(0, 3, 6, 9, 11)),

        // Greek & Balkan
        RhythmSpec("zeibekiko", "Zeibekiko", "9/8 · 2+2+2+3", Tradition.GREEK, 18, setOf(0, 4, 8, 12)),
        RhythmSpec("kalamatianos", "Kalamatianos", "7/8 · 3+2+2", Tradition.GREEK, 14, setOf(0, 6, 10)),
        RhythmSpec("karsilamas", "Karsilamas", "9/8 · 2+2+2+3", Tradition.GREEK, 9, setOf(0, 2, 4, 6)),
        RhythmSpec("tsifteteli", "Tsifteteli", "4/4 · D--T -T-D -T-", Tradition.GREEK, 16, setOf(0, 3, 6, 8, 12)),
        RhythmSpec("kopanitsa_11", "Kopanitsa", "11/8 · 2+2+3+2+2", Tradition.GREEK, 11, setOf(0, 2, 4, 7, 9)),
        RhythmSpec("paidushko_5", "Paidushko", "5/8 · 2+3", Tradition.GREEK, 10, setOf(0, 4, 8)),

        // Indian Tala
        RhythmSpec("teentaal", "Teentaal", "16 · 4+4+4+4", Tradition.RAGA, 16, setOf(0, 4, 8, 12)),
        RhythmSpec("jhaptaal", "Jhaptaal", "10 · 2+3+2+3", Tradition.RAGA, 10, setOf(0, 2, 5, 7)),
        RhythmSpec("rupak", "Rupak", "7 · 3+2+2 (Khali)", Tradition.RAGA, 7, setOf(0, 3, 5)),
        RhythmSpec("dhamar_14", "Dhamar", "14 · 5+2+3+4", Tradition.RAGA, 14, setOf(0, 5, 7, 10)),

        // Gamelan
        RhythmSpec("lancaran", "Lancaran", "16 · Gong Cycle", Tradition.GAMELAN, 16, setOf(0, 4, 8, 12)),
        RhythmSpec("ketawang", "Ketawang", "16 · Slow Gong", Tradition.GAMELAN, 16, setOf(0, 8, 12)),

        // Western, Techno, Metal, Pop & Euclidean
        RhythmSpec("west_groove", "Groove", "4/4 · 16-step", Tradition.WESTERN, 16, setOf(0, 4, 10, 12)),
        RhythmSpec("west_waltz", "Waltz", "3/4 · 12-step", Tradition.WESTERN, 12, setOf(0, 4, 8)),
        RhythmSpec("techno_4x4", "Techno 4x4", "4/4 · Driving", Tradition.TECHNO, 16, setOf(0, 4, 8, 12)),
        RhythmSpec("techno_break", "Electro Break", "4/4 · Syncopated", Tradition.TECHNO, 16, setOf(0, 4, 10, 12)),
        RhythmSpec("metal_gallop", "Metal Gallop", "4/4 · Double Kick", Tradition.METAL, 16, setOf(0, 3, 4, 7, 8, 12)),
        RhythmSpec("pop_four", "Pop Beat", "4/4 · Modern", Tradition.POP, 16, setOf(0, 4, 8, 12)),
        RhythmSpec("euclidean_5_13", "Euclidean 5/13", "13 · E(5,13)", Tradition.WESTERN, 13, setOf(0, 3, 5, 8, 10)),
    )

    fun scalesFor(tradition: Tradition): List<ScaleSpec> =
        scales.filter { it.tradition == tradition }

    fun rhythmsFor(tradition: Tradition): List<RhythmSpec> =
        rhythms.filter { it.tradition == tradition }

    fun findScale(id: String): ScaleSpec =
        scales.firstOrNull { it.id == id } ?: scales.first()

    fun findRhythm(id: String): RhythmSpec =
        rhythms.firstOrNull { it.id == id } ?: rhythms.first()

    fun modifiersFor(tradition: Tradition): List<ModifierSpec> = when (tradition) {
        Tradition.WESTERN -> listOf(
            ModifierSpec(0, "MAJ", "Triad"),
            ModifierSpec(1, "MIN", "Triad"),
            ModifierSpec(2, "DIM", "♭5"),
            ModifierSpec(3, "AUG", "♯5"),
            ModifierSpec(4, "7TH", "Ext"),
            ModifierSpec(5, "9TH", "Ext"),
            ModifierSpec(6, "SUS4", "Sus"),
            ModifierSpec(7, "INV", "Bass"),
        )
        Tradition.ARABIC -> listOf(
            ModifierSpec(0, "ISON", "Drone"),
            ModifierSpec(1, "OCT+5", "Power"),
            ModifierSpec(2, "MODAL", "Chord"),
            ModifierSpec(3, "ORN", "Mordent"),
            ModifierSpec(4, "TREM", "Oud"),
            ModifierSpec(5, "JINS", "+50¢"),
        )
        Tradition.GREEK -> listOf(
            ModifierSpec(0, "ISON", "Drone"),
            ModifierSpec(1, "OCT+5", "Power"),
            ModifierSpec(2, "MODAL", "Bouzouki"),
            ModifierSpec(3, "ORN", "Grace"),
            ModifierSpec(4, "TREM", "Tremolo"),
            ModifierSpec(5, "JINS", "Color"),
        )
        Tradition.RAGA -> listOf(
            ModifierSpec(0, "TANPURA", "Sa-Pa"),
            ModifierSpec(1, "MEEND", "Glide"),
            ModifierSpec(2, "GAMAKA", "Wave"),
            ModifierSpec(3, "TAAN", "Run"),
            ModifierSpec(4, "OCT", "Sa-Sa"),
            ModifierSpec(5, "PA", "3:2 Fifth"),
        )
        Tradition.GAMELAN -> listOf(
            ModifierSpec(0, "BALUNG", "Octave"),
            ModifierSpec(1, "KOTEKAN", "Interlock"),
            ModifierSpec(2, "OMBAK", "Shimmer"),
            ModifierSpec(3, "GONG", "Kempul"),
            ModifierSpec(4, "KEMPY", "3-Deg"),
            ModifierSpec(5, "GEMBY", "2-Oct"),
        )
        Tradition.TECHNO -> listOf(
            ModifierSpec(0, "STAB", "Min Chord"),
            ModifierSpec(1, "SUB", "-1 Oct"),
            ModifierSpec(2, "MIN7", "Dub"),
            ModifierSpec(3, "303 ROLL", "16th Acid"),
            ModifierSpec(4, "5TH", "Power"),
            ModifierSpec(5, "+OCT", "High"),
        )
        Tradition.METAL -> listOf(
            ModifierSpec(0, "PWR 5", "Chug"),
            ModifierSpec(1, "DROP", "-1 Oct"),
            ModifierSpec(2, "GALLOP", "16th Riff"),
            ModifierSpec(3, "TRITONE", "♭5 Evil"),
            ModifierSpec(4, "SHRED", "Fast"),
            ModifierSpec(5, "+OCT", "Lead"),
        )
        Tradition.POP -> listOf(
            ModifierSpec(0, "MAJ", "Triad"),
            ModifierSpec(1, "MIN", "Triad"),
            ModifierSpec(2, "SUS2", "Open"),
            ModifierSpec(3, "ADD9", "Lush"),
            ModifierSpec(4, "7TH", "Color"),
            ModifierSpec(5, "BASS", "-1 Oct"),
        )
    }

    private fun formatMicroBadge(cents: Float): String? {
        if (abs(cents) < 6f) return null
        return when {
            abs(cents + 50f) < 8f -> "½♭"
            abs(cents - 50f) < 8f -> "½♯"
            cents > 0f -> "+${cents.roundToInt()}¢"
            else -> "${cents.roundToInt()}¢"
        }
    }

    // Build 2-octave keyboard keys for either 12-key Piano (with microtonal detune)
    // or Scale-Locked mode (2 full octaves of pure in-scale degrees).
    fun buildTwoOctaveKeys(
        scale: ScaleSpec,
        rootSemitone: Int,
        baseOctave: Int = 3,
        layout: KeyboardLayoutMode,
    ): List<KeySpec> {
        val rootNorm = ((rootSemitone % 12) + 12) % 12
        val baseMidi = 12 * (baseOctave + 1) // C3 = 48

        if (layout == KeyboardLayoutMode.SCALE_LOCKED) {
            val n = scale.degrees.size.coerceAtLeast(1)
            val totalKeys = n * 2 + 1 // 2 full octaves + top tonic
            return (0 until totalKeys).map { idx ->
                val oct = idx / n
                val deg = idx % n
                val semis = baseMidi + rootNorm + oct * 12 + scale.degrees[deg]
                val midi = floor(semis + 0.5f).toInt()
                val cents = (semis - midi) * 100f
                val baseLabel = scale.degreeLabels.getOrNull(deg)
                    ?: rootNames[((midi % 12) + 12) % 12]
                val isNumericCipher = baseLabel.firstOrNull()?.isDigit() == true
                val label = if (deg == 0 && !isNumericCipher) "$baseLabel${baseOctave + oct}" else baseLabel
                KeySpec(
                    index = idx,
                    midiNote = midi,
                    cents = cents,
                    label = label,
                    microBadge = formatMicroBadge(cents),
                    isBlackKey = false,
                    isInScale = true,
                    isRoot = (deg == 0),
                )
            }
        }

        // 12-Key Piano Mode: 25 keys (2 full octaves from C to C)
        val blackMask = setOf(1, 3, 6, 8, 10)
        // Map each rounded pitch class relative to root -> exact cents offset from scale
        val scalePitchMap = mutableMapOf<Int, Float>()
        for (deg in scale.degrees) {
            val rounded = floor(deg + 0.5f).toInt()
            val pc = ((rounded % 12) + 12) % 12
            val cents = (deg - rounded) * 100f
            scalePitchMap[pc] = cents
        }

        return (0..24).map { i ->
            val midi = baseMidi + i
            val pc = midi % 12
            val relPc = ((pc - rootNorm) % 12 + 12) % 12
            val inScale = scalePitchMap.containsKey(relPc)
            val cents = scalePitchMap[relPc] ?: 0f
            val octNum = (midi / 12) - 1
            KeySpec(
                index = i,
                midiNote = midi,
                cents = cents,
                label = if (relPc == 0) "${rootNames[pc]}$octNum" else rootNames[pc],
                microBadge = if (inScale) formatMicroBadge(cents) else null,
                isBlackKey = pc in blackMask,
                isInScale = inScale,
                isRoot = (relPc == 0),
            )
        }
    }
}
