#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPP="$ROOT/app/src/main/cpp"
OUT="/tmp/euterpe_native_test"

# Compile Lua 5.4 C files (excluding standalone lua.c / luac.c)
LUA_SRCS=$(find "$CPP/third_party/lua54" -name '*.c' ! -name 'lua.c' ! -name 'luac.c')

cc -O2 -DLUA_USE_POSIX -c $LUA_SRCS -I"$CPP/third_party/lua54"
mkdir -p /tmp/euterpe_lua_objs
mv *.o /tmp/euterpe_lua_objs/

c++ -std=c++17 -O2 \
    -I"$CPP/include" \
    -I"$CPP/third_party/lua54" \
    "$ROOT/native-tests/euterpe_test.cpp" \
    "$CPP/src/WorldPercussionSynth.cpp" \
    "$CPP/src/CycleSequencer.cpp" \
    "$CPP/src/PerformanceLayer.cpp" \
    "$CPP/src/LuaIdeaHost.cpp" \
    "$CPP/src/LoopRecorder.cpp" \
    /tmp/euterpe_lua_objs/*.o \
    -lm -o "$OUT"

cd "$ROOT"
"$OUT"
