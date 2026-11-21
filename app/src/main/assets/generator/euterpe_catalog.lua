-- Euterpe Musical Traditions, Exotic Scales (cents-accurate), and World Rhythms
local M = {}

M.scales = {
    -- Western
    major            = { 0, 2, 4, 5, 7, 9, 11 },
    natural_minor    = { 0, 2, 3, 5, 7, 8, 10 },
    dorian           = { 0, 2, 3, 5, 7, 9, 10 },
    lydian           = { 0, 2, 4, 6, 7, 9, 11 },
    mixolydian       = { 0, 2, 4, 5, 7, 9, 10 },
    pentatonic_minor = { 0, 3, 5, 7, 10 },

    -- Arabic / Levantine Maqams
    rast             = { 0, 2, 3.5, 5, 7, 9, 10.5 },
    bayati           = { 0, 1.5, 3, 5, 7, 8.5, 10 },
    hijaz            = { 0, 1, 4, 5, 7, 8, 10 },
    hijaz_kar        = { 0, 1, 4, 5, 7, 8, 11 },
    saba             = { 0, 1.5, 3, 4, 7, 8, 10 },
    kurd             = { 0, 1, 3, 5, 7, 8, 10 },
    nahawand         = { 0, 2, 3, 5, 7, 8, 11 },
    segah            = { 0, 1.5, 3.5, 5.5, 7, 8.5, 10.5 },
    ajam             = { 0, 2, 4, 5, 7, 9, 11 },

    -- Greek Dromoi
    ousak            = { 0, 1.15, 3, 5, 7, 8, 10 },
    hitzaz           = { 0, 1, 4, 5, 7, 8, 10 },
    hitzazkiar       = { 0, 1, 4, 5, 7, 8, 11 },
    niavent          = { 0, 2, 3, 5, 7, 8, 11 },
    greek_rast       = { 0, 2, 3.65, 5, 7, 9, 10.65 },
    houzam           = { 0, 3, 4, 5, 7, 8, 11 },
    sabah            = { 0, 1.25, 3, 4, 7, 8, 10 },
    pireotikos       = { 0, 1, 4, 6, 7, 8, 11 },

    -- Indian Ragas
    raga_yaman       = { 0, 2, 4, 6, 7, 9, 11 },
    raga_bhairav     = { 0, 1, 4, 5, 7, 8, 11 },
    raga_bhimpalasi  = { 0, 2, 3, 5, 7, 8, 10 },
    raga_bageshri    = { 0, 2, 3, 5, 7, 9, 10 },
    raga_todi        = { 0, 1, 3, 6, 7, 8, 11 },

    -- Indonesian Gamelan
    slendro          = { 0, 2.40, 4.82, 7.22, 9.60 },
    pelog_bem        = { 0, 1.20, 2.75, 6.75, 7.85 },
    pelog_barang     = { 0, 1.55, 4.00, 6.65, 9.45 },
    pelog_7          = { 0, 1.20, 2.75, 5.20, 6.75, 7.85, 9.65 },

    -- Exotic / Adventurous World & Microtonal Scales
    persian_shur     = { 0, 1.40, 3.0, 5.0, 7.0, 8.15, 10.0 },    -- Persian Koron 2nd & 6th
    persian_chahargah= { 0, 1.40, 4.0, 5.0, 7.0, 8.40, 11.0 },    -- Persian Chahargah
    turkish_huseyni  = { 0, 1.65, 3.0, 5.0, 7.0, 8.80, 10.0 },    -- Turkish comma-tuned
    japanese_hirajoshi = { 0, 2, 3, 7, 8 },
    japanese_insen   = { 0, 1, 5, 7, 10 },
    japanese_iwato   = { 0, 1, 5, 6, 10 },
    ethiopian_tizita = { 0, 2, 4, 7, 9 },
    ethiopian_anchihoye = { 0, 1, 5, 6, 9 },
    hungarian_gypsy  = { 0, 2, 3, 6, 7, 8, 11 },
    harmonic_7th_ji  = { 0, 2.0391, 3.8631, 4.9804, 7.0196, 8.8436, 9.6883 }, -- 7/4 septimal!
    prometheus_mystic= { 0, 2, 4, 6, 9, 10 },
    tet17_neutral    = { 0, 1.41, 3.53, 4.94, 7.06, 8.47, 10.59 }, -- 17-EDO neutral mode
}

-- Asymmetric & cycle-based world percussion patterns.
M.rhythms = {
    -- Arabic / Levantine Iqa'at
    maqsum = {
        stepCount = 16, stepsPerBeat = 4.0, swing = 0.04,
        hits = {
            [0]  = { {"dum", 0.95}, {"bendir", 0.55} },
            [2]  = { {"tak", 0.78} },
            [4]  = { {"ka",  0.50} },
            [6]  = { {"tak", 0.84}, {"riq", 0.42} },
            [8]  = { {"dum", 0.90} },
            [10] = { {"ka",  0.48} },
            [12] = { {"tak", 0.86}, {"riq", 0.45} },
            [14] = { {"ka",  0.52} },
        },
    },
    baladi = {
        stepCount = 16, stepsPerBeat = 4.0, swing = 0.06,
        hits = {
            [0]  = { {"dum", 0.98}, {"bendir", 0.65} },
            [2]  = { {"dum", 0.85} },
            [4]  = { {"ka",  0.45} },
            [6]  = { {"tak", 0.82}, {"riq", 0.45} },
            [8]  = { {"dum", 0.92} },
            [10] = { {"ka",  0.48} },
            [12] = { {"tak", 0.88}, {"slap", 0.42} },
            [14] = { {"ka",  0.55}, {"riq", 0.35} },
        },
    },
    saidi = {
        stepCount = 16, stepsPerBeat = 4.0, swing = 0.05,
        hits = {
            [0]  = { {"dum", 0.96}, {"bendir", 0.60} },
            [2]  = { {"tak", 0.80} },
            [6]  = { {"dum", 0.90} },
            [8]  = { {"dum", 0.92} },
            [12] = { {"tak", 0.86}, {"riq", 0.50} },
            [14] = { {"ka",  0.52} },
        },
    },
    samai_thaqil = {
        stepCount = 10, stepsPerBeat = 2.0, swing = 0.0,
        hits = {
            [0] = { {"dum", 0.96}, {"bendir", 0.55} },
            [3] = { {"tak", 0.82}, {"riq", 0.42} },
            [5] = { {"dum", 0.88} },
            [6] = { {"dum", 0.86} },
            [7] = { {"tak", 0.84}, {"riq", 0.45} },
            [9] = { {"ka",  0.52} },
        },
    },
    malfuf = {
        stepCount = 8, stepsPerBeat = 4.0, swing = 0.04,
        hits = {
            [0] = { {"dum", 0.95}, {"bendir", 0.50} },
            [3] = { {"tak", 0.84}, {"riq", 0.44} },
            [6] = { {"tak", 0.80} },
            [7] = { {"ka",  0.48} },
        },
    },
    murabba_13 = {
        -- 13/8 Arabic Murabba (3 + 3 + 3 + 2 + 2)
        stepCount = 13, stepsPerBeat = 2.0, swing = 0.0,
        hits = {
            [0]  = { {"dum", 0.96}, {"bendir", 0.60} },
            [3]  = { {"tak", 0.82}, {"riq", 0.44} },
            [6]  = { {"dum", 0.88} },
            [9]  = { {"tak", 0.84}, {"slap", 0.42} },
            [11] = { {"ka",  0.60}, {"riq", 0.40} },
        },
    },

    -- Greek & Balkan Rhythms
    zeibekiko = {
        stepCount = 18, stepsPerBeat = 4.0, swing = 0.03,
        hits = {
            [0]  = { {"daouli", 0.95}, {"dum", 0.80} },
            [2]  = { {"tak", 0.65} },
            [4]  = { {"dum", 0.84} },
            [6]  = { {"tak", 0.72} },
            [8]  = { {"daouli", 0.88} },
            [10] = { {"tak", 0.76}, {"riq", 0.38} },
            [12] = { {"dum", 0.92}, {"daouli", 0.72} },
            [14] = { {"tak", 0.78} },
            [16] = { {"ka",  0.60}, {"slap", 0.45} },
        },
    },
    kalamatianos = {
        stepCount = 14, stepsPerBeat = 4.0, swing = 0.0,
        hits = {
            [0]  = { {"daouli", 0.96}, {"dum", 0.78} },
            [2]  = { {"ka", 0.45} },
            [4]  = { {"tak", 0.72} },
            [6]  = { {"dum", 0.86} },
            [8]  = { {"tak", 0.76}, {"riq", 0.42} },
            [10] = { {"dum", 0.84} },
            [12] = { {"tak", 0.80}, {"riq", 0.40} },
        },
    },
    karsilamas = {
        stepCount = 9, stepsPerBeat = 2.0, swing = 0.0,
        hits = {
            [0] = { {"dum", 0.96}, {"daouli", 0.70} },
            [2] = { {"tak", 0.84}, {"riq", 0.45} },
            [4] = { {"dum", 0.90} },
            [6] = { {"tak", 0.86}, {"slap", 0.45} },
            [7] = { {"ka",  0.55} },
            [8] = { {"tak", 0.74} },
        },
    },
    tsifteteli = {
        stepCount = 16, stepsPerBeat = 4.0, swing = 0.05,
        hits = {
            [0]  = { {"dum", 0.95}, {"bendir", 0.55} },
            [2]  = { {"ka",  0.48} },
            [3]  = { {"tak", 0.82} },
            [6]  = { {"tak", 0.84}, {"riq", 0.46} },
            [8]  = { {"dum", 0.92} },
            [10] = { {"ka",  0.48} },
            [12] = { {"tak", 0.88}, {"slap", 0.42} },
            [14] = { {"ka",  0.54}, {"riq", 0.40} },
        },
    },
    kopanitsa_11 = {
        -- Balkan 11/8 Kopanitsa (2 + 2 + 3 + 2 + 2)
        stepCount = 11, stepsPerBeat = 2.0, swing = 0.0,
        hits = {
            [0]  = { {"daouli", 0.96}, {"dum", 0.80} },
            [2]  = { {"tak", 0.80} },
            [4]  = { {"daouli", 0.90}, {"riq", 0.48} },
            [6]  = { {"ka",  0.55} },
            [7]  = { {"dum", 0.86} },
            [9]  = { {"tak", 0.84}, {"slap", 0.45} },
            [10] = { {"ka",  0.50} },
        },
    },
    paidushko_5 = {
        -- Balkan 5/8 Paidushko (2 + 3)
        stepCount = 10, stepsPerBeat = 4.0, swing = 0.0,
        hits = {
            [0] = { {"daouli", 0.96}, {"dum", 0.78} },
            [2] = { {"tak", 0.72} },
            [4] = { {"dum", 0.90}, {"riq", 0.45} },
            [6] = { {"ka",  0.55} },
            [8] = { {"tak", 0.82}, {"slap", 0.42} },
        },
    },

    -- Indian Talas
    teentaal = {
        stepCount = 16, stepsPerBeat = 2.0, swing = 0.0,
        hits = {
            [0]  = { {"bayan", 0.96}, {"dayan", 0.90} },
            [1]  = { {"bayan", 0.78}, {"dayan", 0.80} },
            [2]  = { {"bayan", 0.76}, {"dayan", 0.78} },
            [3]  = { {"bayan", 0.84}, {"dayan", 0.84} },
            [4]  = { {"bayan", 0.88}, {"dayan", 0.86} },
            [5]  = { {"bayan", 0.76}, {"dayan", 0.78} },
            [6]  = { {"bayan", 0.76}, {"dayan", 0.78} },
            [7]  = { {"bayan", 0.82}, {"dayan", 0.82} },
            [8]  = { {"dayan", 0.86} },
            [9]  = { {"dayan", 0.76}, {"tik", 0.55} },
            [10] = { {"dayan", 0.74}, {"tik", 0.52} },
            [11] = { {"tik",   0.78} },
            [12] = { {"tik",   0.82}, {"dayan", 0.72} },
            [13] = { {"bayan", 0.80}, {"dayan", 0.78} },
            [14] = { {"bayan", 0.78}, {"dayan", 0.78} },
            [15] = { {"bayan", 0.88}, {"dayan", 0.85} },
        },
    },
    jhaptaal = {
        stepCount = 10, stepsPerBeat = 2.0, swing = 0.0,
        hits = {
            [0] = { {"bayan", 0.96}, {"dayan", 0.88} },
            [1] = { {"dayan", 0.82} },
            [2] = { {"bayan", 0.88}, {"dayan", 0.84} },
            [3] = { {"bayan", 0.80}, {"dayan", 0.78} },
            [4] = { {"dayan", 0.80} },
            [5] = { {"tik",   0.82} },
            [6] = { {"dayan", 0.76} },
            [7] = { {"bayan", 0.88}, {"dayan", 0.84} },
            [8] = { {"bayan", 0.80}, {"dayan", 0.78} },
            [9] = { {"dayan", 0.82} },
        },
    },
    rupak = {
        stepCount = 7, stepsPerBeat = 2.0, swing = 0.0,
        hits = {
            [0] = { {"dayan", 0.90}, {"tik", 0.55} },
            [1] = { {"dayan", 0.78} },
            [2] = { {"dayan", 0.76} },
            [3] = { {"bayan", 0.92}, {"dayan", 0.84} },
            [4] = { {"dayan", 0.78} },
            [5] = { {"bayan", 0.90}, {"dayan", 0.84} },
            [6] = { {"dayan", 0.78} },
        },
    },
    dhamar_14 = {
        -- 14-beat Indian Dhamar Tala (5 + 2 + 3 + 4)
        stepCount = 14, stepsPerBeat = 2.0, swing = 0.0,
        hits = {
            [0]  = { {"bayan", 0.96}, {"dayan", 0.90} },
            [2]  = { {"dayan", 0.80} },
            [3]  = { {"bayan", 0.84}, {"dayan", 0.82} },
            [5]  = { {"bayan", 0.90}, {"dayan", 0.85} },
            [7]  = { {"tik",   0.82} },
            [8]  = { {"dayan", 0.78} },
            [10] = { {"bayan", 0.88}, {"dayan", 0.84} },
            [12] = { {"tik",   0.74}, {"dayan", 0.78} },
        },
    },

    -- Gamelan Colotomic Cycles
    lancaran = {
        stepCount = 16, stepsPerBeat = 4.0, swing = 0.0,
        hits = {
            [0]  = { {"gong", 0.95}, {"kendang_low", 0.72} },
            [2]  = { {"kendang_high", 0.62} },
            [4]  = { {"kenong", 0.82}, {"kendang_low", 0.68} },
            [6]  = { {"kempul", 0.78} },
            [8]  = { {"kenong", 0.84}, {"kendang_low", 0.75} },
            [10] = { {"kempul", 0.80}, {"kendang_high", 0.68} },
            [12] = { {"kenong", 0.86}, {"kendang_low", 0.72} },
            [14] = { {"kempul", 0.82}, {"kendang_high", 0.72} },
        },
    },
    ketawang = {
        stepCount = 16, stepsPerBeat = 2.0, swing = 0.0,
        hits = {
            [0]  = { {"gong", 0.95} },
            [4]  = { {"kendang_low", 0.70} },
            [8]  = { {"kenong", 0.88}, {"kendang_high", 0.65} },
            [12] = { {"kempul", 0.84}, {"kendang_low", 0.72} },
            [14] = { {"kendang_high", 0.68} },
        },
    },

    -- Western, Techno, Metal, Pop & Euclidean Polyrhythms
    west_groove = {
        stepCount = 16, stepsPerBeat = 4.0, swing = 0.08,
        hits = {
            [0]  = { {"kick", 0.95}, {"hat", 0.65} },
            [2]  = { {"hat", 0.45} },
            [4]  = { {"snare", 0.88}, {"hat", 0.65} },
            [6]  = { {"hat", 0.50} },
            [8]  = { {"hat", 0.62} },
            [10] = { {"kick", 0.86}, {"hat", 0.52} },
            [12] = { {"snare", 0.90}, {"clap", 0.45}, {"hat", 0.65} },
            [14] = { {"openhat", 0.55} },
        },
    },
    techno_4x4 = {
        stepCount = 16, stepsPerBeat = 4.0, swing = 0.02,
        hits = {
            [0]  = { {"kick", 0.98}, {"hat", 0.45} },
            [2]  = { {"openhat", 0.72} },
            [4]  = { {"kick", 0.96}, {"clap", 0.82}, {"hat", 0.45} },
            [6]  = { {"openhat", 0.72} },
            [8]  = { {"kick", 0.98}, {"hat", 0.48} },
            [10] = { {"openhat", 0.74} },
            [12] = { {"kick", 0.96}, {"clap", 0.85}, {"hat", 0.45} },
            [14] = { {"openhat", 0.74} },
            [15] = { {"hat", 0.42} },
        },
    },
    techno_break = {
        stepCount = 16, stepsPerBeat = 4.0, swing = 0.04,
        hits = {
            [0]  = { {"kick", 0.98}, {"hat", 0.60} },
            [2]  = { {"hat", 0.48} },
            [4]  = { {"snare", 0.90}, {"clap", 0.60} },
            [6]  = { {"kick", 0.84}, {"hat", 0.55} },
            [8]  = { {"hat", 0.62} },
            [10] = { {"kick", 0.92}, {"openhat", 0.62} },
            [12] = { {"snare", 0.92}, {"clap", 0.65} },
            [14] = { {"hat", 0.52} },
        },
    },
    metal_gallop = {
        stepCount = 16, stepsPerBeat = 4.0, swing = 0.0,
        hits = {
            [0]  = { {"kick", 0.98}, {"hat", 0.72} },
            [2]  = { {"kick", 0.84} },
            [3]  = { {"kick", 0.88} },
            [4]  = { {"snare", 0.96}, {"hat", 0.72} },
            [6]  = { {"kick", 0.86} },
            [7]  = { {"kick", 0.88} },
            [8]  = { {"kick", 0.98}, {"openhat", 0.68} },
            [10] = { {"kick", 0.84} },
            [11] = { {"kick", 0.88} },
            [12] = { {"snare", 0.98}, {"hat", 0.72} },
            [14] = { {"kick", 0.88} },
            [15] = { {"kick", 0.90} },
        },
    },
    pop_four = {
        stepCount = 16, stepsPerBeat = 4.0, swing = 0.05,
        hits = {
            [0]  = { {"kick", 0.95}, {"hat", 0.62} },
            [2]  = { {"hat", 0.52} },
            [4]  = { {"snare", 0.88}, {"clap", 0.55}, {"hat", 0.62} },
            [6]  = { {"hat", 0.52} },
            [8]  = { {"kick", 0.90}, {"hat", 0.62} },
            [10] = { {"kick", 0.80}, {"hat", 0.52} },
            [12] = { {"snare", 0.90}, {"clap", 0.60}, {"hat", 0.64} },
            [14] = { {"openhat", 0.52} },
        },
    },
    west_waltz = {
        stepCount = 12, stepsPerBeat = 4.0, swing = 0.0,
        hits = {
            [0]  = { {"kick", 0.92}, {"hat", 0.60} },
            [4]  = { {"snare", 0.75}, {"hat", 0.55} },
            [8]  = { {"snare", 0.72}, {"hat", 0.58} },
            [10] = { {"hat", 0.42} },
        },
    },
    euclidean_5_13 = {
        -- Adventurous E(5,13) polyrhythm!
        stepCount = 13, stepsPerBeat = 4.0, swing = 0.0,
        hits = {
            [0]  = { {"kick", 0.95}, {"gong", 0.45} },
            [3]  = { {"snare", 0.82} },
            [5]  = { {"hat", 0.68} },
            [8]  = { {"kick", 0.88}, {"clap", 0.55} },
            [10] = { {"snare", 0.84}, {"openhat", 0.52} },
        },
    },
}

return M
