cmake_minimum_required(VERSION 3.22.1)
project(euterpe)

# Align native libraries for Android 15+ 16 KB page-size devices.
add_link_options("-Wl,-z,max-page-size=16384")

# Google Oboe
set(OBOE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/oboe)
add_subdirectory(${OBOE_DIR} ./oboe)

# Lua 5.4 static library (excluding standalone lua.c / luac.c)
file(GLOB LUA54_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lua54/*.c")
list(FILTER LUA54_SRCS EXCLUDE REGEX ".*(lua|luac)\\.c$")

add_library(lua54 STATIC ${LUA54_SRCS})
target_compile_definitions(lua54 PRIVATE LUA_USE_POSIX)
target_include_directories(lua54 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lua54)

# Portable YAWN DSP subset
add_library(yawn_dsp STATIC
    yawn_dsp/src/instruments/KarplusStrong.cpp
    yawn_dsp/src/instruments/FMSynth.cpp
    yawn_dsp/src/instruments/ElectricPiano.cpp
    yawn_dsp/src/instruments/WavetableSynth.cpp
    yawn_dsp/src/instruments/SubtractiveSynth.cpp
    yawn_dsp/src/instruments/PolivoksSynth.cpp
    yawn_dsp/src/effects/Reverb.cpp
    yawn_dsp/src/effects/Delay.cpp
    yawn_dsp/src/effects/Limiter.cpp
)
set_target_properties(yawn_dsp PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(yawn_dsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/yawn_dsp/src
    ${CMAKE_CURRENT_SOURCE_DIR}/yawn_dsp/third_party
)

# Euterpe native audio & idea engine
add_library(euterpe-lib SHARED
    euterpe_jni.cpp
    src/EuterpeEngine.cpp
    src/WorldPercussionSynth.cpp
    src/CycleSequencer.cpp
    src/PerformanceLayer.cpp
    src/LuaIdeaHost.cpp
    src/LoopRecorder.cpp
)
set_target_properties(euterpe-lib PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(euterpe-lib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OBOE_DIR}/include
)

find_library(log-lib log)

target_link_libraries(euterpe-lib
    oboe
    yawn_dsp
    lua54
    ${log-lib}
)
