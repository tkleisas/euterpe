-- Euterpe real-time idea generator (runs inside YAWN's sandboxed Lua 5.4 host).
-- Generates a cycle-locked musical idea (melodic phrase + microtonal cents)
-- tailored to the active tradition, scale, root, rhythm length, and seed.

local catalog = require("euterpe_catalog")

local M = {}

local function lcg(state)
    state = (state * 1103515245 + 12345) % 2147483648
    return state, state / 2147483648.0
end

local function scalePitch(rootSemi, degrees, degIdx, octave)
    local n = #degrees
    local oct = math.floor(degIdx / n)
    local idx = degIdx - oct * n
    local semis = 60 + rootSemi + degrees[idx + 1] + 12 * (octave - 4 + oct)
    local midi = math.floor(semis + 0.5)
    local cents = (semis - midi) * 100.0
    return midi, cents
end

-- params = { seed, tradition, scaleId, rootSemi, stepCount }
function M.generateIdea(params)
    local seed = math.floor(params.seed or 1)
    if seed < 1 then seed = 1 end
    local state = seed * 2654435761 % 2147483648

    local tradition = params.tradition or "arabic"
    local scaleId = params.scaleId or "bayati"
    local rootSemi = (params.rootSemi or 2) % 12
    local stepCount = params.stepCount or 16
    local degrees = catalog.scales[scaleId] or catalog.scales.bayati
    local n = #degrees

    local notes = {}
    local curDeg = 0
    local baseOctave = (tradition == "gamelan") and 4 or 4

    -- Choose density & melodic contour based on tradition (or user override)
    local density = params.density
    if not density or density <= 0 then
        density = 0.52
        if tradition == "gamelan" then density = 0.68
        elseif tradition == "techno" then density = 0.72
        elseif tradition == "metal" then density = 0.78
        elseif tradition == "pop" then density = 0.54
        elseif tradition == "raga" then density = 0.44
        elseif tradition == "western" then density = 0.50
        end
    end

    local step = 0
    while step < stepCount do
        local r
        state, r = lcg(state)
        local isDownbeat = (step == 0)
        local shouldPlay = isDownbeat or (r < density)

        if shouldPlay then
            local moveR
            state, moveR = lcg(state)
            if isDownbeat then
                curDeg = 0 -- Anchor on tonic/Sam/Qarar
            elseif tradition == "gamelan" then
                -- Balungan stepwise + occasional kempyung leap
                local deltas = { -1, 1, 1, -1, 2, -2 }
                curDeg = curDeg + deltas[math.floor(moveR * #deltas) + 1]
            elseif tradition == "raga" or tradition == "arabic" or tradition == "greek" then
                -- Predominantly stepwise vocal/oud motion with characteristic arch
                local target = (step < stepCount * 0.6) and 1 or -1
                if moveR < 0.55 then
                    curDeg = curDeg + target
                elseif moveR < 0.80 then
                    curDeg = curDeg - target
                elseif moveR < 0.92 then
                    curDeg = curDeg + 2 * target
                end
            else
                -- Western melodic contour
                local deltas = { -2, -1, 0, 1, 2, 3 }
                curDeg = curDeg + deltas[math.floor(moveR * #deltas) + 1]
            end

            -- Keep inside comfortable 1.5-octave melodic range
            if curDeg < -2 then curDeg = 0 end
            if curDeg > n + 2 then curDeg = n - 1 end

            local durR
            state, durR = lcg(state)
            local dur = 1
            if tradition == "gamelan" then
                dur = (durR < 0.7) and 2 or 1
            elseif tradition == "raga" then
                dur = (durR < 0.45) and 3 or 2
            else
                dur = (durR < 0.35) and 1 or ((durR < 0.80) and 2 or 3)
            end
            if step + dur > stepCount then dur = stepCount - step end

            local midi, cents = scalePitch(rootSemi, degrees, curDeg, baseOctave)
            local vel = isDownbeat and 0.88 or (0.64 + 0.22 * durR)

            notes[#notes + 1] = {
                step = step,
                dur = dur,
                midi = midi,
                cents = cents,
                vel = vel,
            }
            step = step + dur
        else
            step = step + 1
        end
    end

    -- Generate a genre-aware groove-locked bassline in octave 2
    local bassNotes = {}
    local bStep = 0
    while bStep < stepCount do
        local br
        state, br = lcg(state)
        local isDown = (bStep == 0)
        local playBass = false
        local bDeg = 0
        local bDur = 2
        local bOct = 2

        if tradition == "techno" then
            -- Rolling 16th-note acid 303 / sub bassline with octave pops
            playBass = isDown or (bStep % 2 == 0) or (br < 0.45)
            bDur = 1
            if br < 0.55 then bDeg = 0
            elseif br < 0.75 then bDeg = 0; bOct = 3
            elseif br < 0.90 then bDeg = 1
            else bDeg = (n >= 5) and 4 or (n - 1) end
        elseif tradition == "metal" then
            -- Driving root-pedal gallop / chug bassline
            playBass = isDown or (bStep % 4 == 0) or (bStep % 4 == 2) or (bStep % 4 == 3 and br < 0.65)
            bDur = 1
            if isDown or br < 0.72 then bDeg = 0
            elseif br < 0.86 then bDeg = 1
            else bDeg = (n >= 5) and 4 or (n - 1) end
        elseif tradition == "pop" or tradition == "western" then
            -- Syncopated root / 5th / octave pop groove
            playBass = isDown or (bStep == math.floor(stepCount / 2)) or (br < 0.35 and bStep % 2 == 0)
            bDur = 2
            if isDown then bDeg = 0
            elseif br < 0.45 then bDeg = 0
            elseif br < 0.75 then bDeg = (n >= 5) and 4 or 2
            else bDeg = 0; bOct = 3 end
        else
            -- World traditions: modal tonic / dominant anchor pulse
            playBass = isDown or (bStep == math.floor(stepCount / 2)) or (br < 0.25 and bStep % 3 == 0)
            bDur = 3
            bDeg = (isDown or br < 0.65) and 0 or ((n >= 5) and 4 or 2)
        end

        if playBass then
            if bStep + bDur > stepCount then bDur = stepCount - bStep end
            local bMidi, bCents = scalePitch(rootSemi, degrees, bDeg, bOct)
            local bVel = isDown and 0.94 or (0.72 + 0.18 * br)
            bassNotes[#bassNotes + 1] = {
                step = bStep,
                dur = bDur,
                midi = bMidi,
                cents = bCents,
                vel = bVel,
            }
            bStep = bStep + bDur
        else
            bStep = bStep + 1
        end
    end

    return {
        stepCount = stepCount,
        notes = notes,
        bassNotes = bassNotes,
    }
end

function M.getRhythm(rhythmId)
    return catalog.rhythms[rhythmId] or catalog.rhythms.maqsum
end

function M.getScale(scaleId)
    return catalog.scales[scaleId] or catalog.scales.bayati
end

return M
