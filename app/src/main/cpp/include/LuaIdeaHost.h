#pragma once

#include "CycleSequencer.h"

#include <cstdint>
#include <string>
#include <vector>

struct lua_State;

namespace euterpe {

// Sandboxed Lua 5.4 host for Euterpe's data-driven scales, asymmetric world
// rhythms, and seeded musical idea generation.
class LuaIdeaHost {
public:
    LuaIdeaHost();
    ~LuaIdeaHost();

    LuaIdeaHost(const LuaIdeaHost&) = delete;
    LuaIdeaHost& operator=(const LuaIdeaHost&) = delete;

    // Directory containing euterpe_catalog.lua & euterpe_ideas.lua.
    bool init(const std::string& scriptsDir, std::string* error = nullptr);
    bool isReady() const { return mReady; }

    bool loadScale(const std::string& scaleId, std::vector<float>& outDegrees,
                   std::string* error = nullptr);
    bool loadRhythm(const std::string& rhythmId, RhythmPattern& outPattern,
                    std::string* error = nullptr);
    bool generateIdea(uint64_t seed, const std::string& traditionId,
                      const std::string& scaleId, int rootSemitone, int stepCount,
                      IdeaPattern& outIdea, std::string* error = nullptr,
                      float density = -1.0f);

private:
    void close();
    static int sandboxRequire(lua_State* L);

    lua_State* mL = nullptr;
    std::string mScriptsDir;
    bool mReady = false;
};

} // namespace euterpe
