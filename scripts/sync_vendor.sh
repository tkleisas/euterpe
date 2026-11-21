#!/usr/bin/env bash
# Sync vendored C++ dependencies from sibling volna & yawn checkouts:
# - Google Oboe from ../volna/app/src/main/cpp/oboe
# - YAWN DSP subset (instruments, effects, midi, core) from ../yawn
# - Lua 5.4 C sources & GeneratorEngine from ../yawn
# - nlohmann/json from ../volna/app/src/main/cpp/yawn_dsp/third_party
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VOLNA="${1:-$ROOT/../volna}"
YAWN="${2:-$ROOT/../yawn}"
CPP="$ROOT/app/src/main/cpp"

echo "Syncing from VOLNA=$VOLNA and YAWN=$YAWN ..."

# 1. Gradle wrapper from volna
mkdir -p "$ROOT/gradle/wrapper"
cp "$VOLNA/gradlew" "$ROOT/gradlew"
cp "$VOLNA/gradlew.bat" "$ROOT/gradlew.bat"
cp "$VOLNA/gradle/wrapper/gradle-wrapper.jar" "$ROOT/gradle/wrapper/gradle-wrapper.jar"
cp "$VOLNA/gradle/wrapper/gradle-wrapper.properties" "$ROOT/gradle/wrapper/gradle-wrapper.properties"
chmod +x "$ROOT/gradlew"

# 2. Google Oboe from volna
if [[ ! -d "$CPP/oboe/include" ]]; then
    mkdir -p "$CPP"
    cp -a "$VOLNA/app/src/main/cpp/oboe" "$CPP/oboe"
    rm -rf "$CPP/oboe/.git"
fi

# 3. nlohmann/json from volna
mkdir -p "$CPP/yawn_dsp/third_party"
cp -a "$VOLNA/app/src/main/cpp/yawn_dsp/third_party/." "$CPP/yawn_dsp/third_party/"

# 4. YAWN DSP subset
DEST="$CPP/yawn_dsp/src"
mkdir -p "$DEST"/{audio,core,effects,instruments,midi,util,generate}

copy_yawn() {
    local rel="$1"
    local src="$YAWN/src/$rel"
    local dst="$DEST/$rel"
    if [[ ! -f "$src" ]]; then
        echo "missing: $src" >&2
        exit 1
    fi
    mkdir -p "$(dirname "$dst")"
    cp "$src" "$dst"
}

for f in \
    core/Constants.h core/ParameterInfo.h WidgetHint.h \
    util/RtRetireList.h util/Logger.h \
    audio/Oversampling.h \
    midi/MidiTypes.h \
    instruments/Instrument.h instruments/Envelope.h instruments/Oscillator.h \
    instruments/KarplusStrong.h instruments/KarplusStrong.cpp \
    instruments/FMSynth.h instruments/FMSynth.cpp \
    instruments/ElectricPiano.h instruments/ElectricPiano.cpp \
    instruments/WavetableSynth.h instruments/WavetableSynth.cpp \
    instruments/SubtractiveSynth.h instruments/SubtractiveSynth.cpp \
    instruments/DrumSynth.h instruments/DrumSynth.cpp \
    effects/AudioEffect.h effects/Biquad.h effects/FreqMap.h \
    effects/Delay.h effects/Delay.cpp \
    effects/Reverb.h effects/Reverb.cpp \
    effects/PlateReverb.h effects/PlateReverb.cpp \
    effects/Limiter.h effects/Limiter.cpp \
    generate/GeneratorTypes.h \
    generate/GeneratorEngine.h generate/GeneratorEngine.cpp
do
    copy_yawn "$f"
done

# YAWN relies on its PCH for core/Constants.h; ensure explicit includes.
python3 - "$DEST" <<'PY'
from pathlib import Path
import sys
root = Path(sys.argv[1])
for rel in ["effects/AudioEffect.h", "instruments/Instrument.h"]:
    p = root / rel
    text = p.read_text()
    if '#include "core/Constants.h"' not in text:
        text = text.replace('#pragma once\n', '#pragma once\n\n#include "core/Constants.h"\n', 1)
        p.write_text(text)
PY

# 5. Lua 5.4 C sources from yawn
mkdir -p "$CPP/third_party/lua54"
cp -a "$YAWN/third_party/lua54/." "$CPP/third_party/lua54/"

# 6. Base Lua generator scripts from yawn (as starting point for Euterpe's expanded traditions)
mkdir -p "$ROOT/app/src/main/assets/generator"
cp -a "$YAWN/assets/generator/." "$ROOT/app/src/main/assets/generator/"

echo "Vendor sync complete."
