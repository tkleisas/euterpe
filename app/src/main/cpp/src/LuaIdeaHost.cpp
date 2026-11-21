#include "LuaIdeaHost.h"
#include "WorldPercussionSynth.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

namespace euterpe {

namespace {
bool readTextFile(const std::filesystem::path& p, std::string& out) {
    std::ifstream ifs(p, std::ios::in | std::ios::binary);
    if (!ifs) return false;
    std::ostringstream ss;
    ss << ifs.rdbuf();
    out = ss.str();
    return true;
}
} // namespace

LuaIdeaHost::LuaIdeaHost() = default;

LuaIdeaHost::~LuaIdeaHost() {
    close();
}

void LuaIdeaHost::close() {
    if (mL) {
        lua_close(mL);
        mL = nullptr;
    }
    mReady = false;
}

int LuaIdeaHost::sandboxRequire(lua_State* L) {
    const char* modName = luaL_checkstring(L, 1);
    auto* self = static_cast<LuaIdeaHost*>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!self || !modName) {
        return luaL_error(L, "invalid require context");
    }

    // Check cache in registry["euterpe_loaded"][modName]
    lua_getfield(L, LUA_REGISTRYINDEX, "euterpe_loaded");
    lua_getfield(L, -1, modName);
    if (!lua_isnil(L, -1)) {
        return 1;
    }
    lua_pop(L, 1);

    // Restrict module name to safe relative path characters
    std::string name(modName);
    if (name.find("..") != std::string::npos || name.find('/') != std::string::npos ||
        name.find('\\') != std::string::npos) {
        return luaL_error(L, "disallowed module name: %s", modName);
    }
    std::replace(name.begin(), name.end(), '.', '/');
    std::filesystem::path filePath = std::filesystem::path(self->mScriptsDir) / (name + ".lua");

    std::string source;
    if (!readTextFile(filePath, source)) {
        return luaL_error(L, "module not found: %s", filePath.c_str());
    }

    if (luaL_loadbuffer(L, source.data(), source.size(), filePath.c_str()) != LUA_OK) {
        return lua_error(L);
    }
    lua_call(L, 0, 1);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pushboolean(L, 1);
    }

    // Store in cache
    lua_pushvalue(L, -1);
    lua_setfield(L, -3, modName);
    return 1;
}

bool LuaIdeaHost::init(const std::string& scriptsDir, std::string* error) {
    close();
    mScriptsDir = scriptsDir;

    mL = luaL_newstate();
    if (!mL) {
        if (error) *error = "luaL_newstate failed";
        return false;
    }

    // Open only safe standard libraries (no io, os, debug, package)
    luaL_requiref(mL, "_G", luaopen_base, 1);
    lua_pop(mL, 1);
    luaL_requiref(mL, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(mL, 1);
    luaL_requiref(mL, LUA_TABLIBNAME, luaopen_table, 1);
    lua_pop(mL, 1);
    luaL_requiref(mL, LUA_STRLIBNAME, luaopen_string, 1);
    lua_pop(mL, 1);

    // Strip file-loading base globals
    lua_pushnil(mL);
    lua_setglobal(mL, "dofile");
    lua_pushnil(mL);
    lua_setglobal(mL, "loadfile");

    // Module cache table in registry
    lua_newtable(mL);
    lua_setfield(mL, LUA_REGISTRYINDEX, "euterpe_loaded");

    // Sandboxed require()
    lua_pushlightuserdata(mL, this);
    lua_pushcclosure(mL, &LuaIdeaHost::sandboxRequire, 1);
    lua_setglobal(mL, "require");

    // Load euterpe_ideas module and keep it in registry["euterpe_ideas"]
    lua_getglobal(mL, "require");
    lua_pushstring(mL, "euterpe_ideas");
    if (lua_pcall(mL, 1, 1, 0) != LUA_OK) {
        if (error) *error = lua_tostring(mL, -1) ? lua_tostring(mL, -1) : "failed to load euterpe_ideas";
        close();
        return false;
    }
    lua_setfield(mL, LUA_REGISTRYINDEX, "euterpe_ideas");
    mReady = true;
    return true;
}

bool LuaIdeaHost::loadScale(const std::string& scaleId, std::vector<float>& outDegrees,
                            std::string* error) {
    if (!mReady || !mL) {
        if (error) *error = "LuaIdeaHost not ready";
        return false;
    }

    lua_getfield(mL, LUA_REGISTRYINDEX, "euterpe_ideas");
    lua_getfield(mL, -1, "getScale");
    lua_pushstring(mL, scaleId.c_str());
    if (lua_pcall(mL, 1, 1, 0) != LUA_OK) {
        if (error) *error = lua_tostring(mL, -1) ? lua_tostring(mL, -1) : "getScale error";
        lua_pop(mL, 2);
        return false;
    }

    outDegrees.clear();
    if (lua_istable(mL, -1)) {
        lua_Integer len = luaL_len(mL, -1);
        for (lua_Integer i = 1; i <= len; ++i) {
            lua_rawgeti(mL, -1, i);
            outDegrees.push_back(static_cast<float>(lua_tonumber(mL, -1)));
            lua_pop(mL, 1);
        }
    }
    lua_pop(mL, 2);
    return !outDegrees.empty();
}

bool LuaIdeaHost::loadRhythm(const std::string& rhythmId, RhythmPattern& outPattern,
                             std::string* error) {
    if (!mReady || !mL) {
        if (error) *error = "LuaIdeaHost not ready";
        return false;
    }

    lua_getfield(mL, LUA_REGISTRYINDEX, "euterpe_ideas");
    lua_getfield(mL, -1, "getRhythm");
    lua_pushstring(mL, rhythmId.c_str());
    if (lua_pcall(mL, 1, 1, 0) != LUA_OK) {
        if (error) *error = lua_tostring(mL, -1) ? lua_tostring(mL, -1) : "getRhythm error";
        lua_pop(mL, 2);
        return false;
    }

    if (!lua_istable(mL, -1)) {
        lua_pop(mL, 2);
        if (error) *error = "getRhythm did not return a table";
        return false;
    }

    RhythmPattern pat;
    lua_getfield(mL, -1, "stepCount");
    pat.stepCount = std::max(1, static_cast<int>(lua_tointeger(mL, -1)));
    lua_pop(mL, 1);

    lua_getfield(mL, -1, "stepsPerBeat");
    pat.stepsPerBeat = std::max(0.5f, static_cast<float>(lua_tonumber(mL, -1)));
    lua_pop(mL, 1);

    lua_getfield(mL, -1, "swing");
    pat.swing = static_cast<float>(lua_tonumber(mL, -1));
    lua_pop(mL, 1);

    pat.steps.assign(static_cast<size_t>(pat.stepCount), {});

    lua_getfield(mL, -1, "hits");
    if (lua_istable(mL, -1)) {
        lua_pushnil(mL);
        while (lua_next(mL, -2) != 0) {
            if (lua_isinteger(mL, -2) && lua_istable(mL, -1)) {
                int stepIdx = static_cast<int>(lua_tointeger(mL, -2));
                if (stepIdx >= 0 && stepIdx < pat.stepCount) {
                    lua_Integer nHits = luaL_len(mL, -1);
                    for (lua_Integer h = 1; h <= nHits; ++h) {
                        lua_rawgeti(mL, -1, h);
                        if (lua_istable(mL, -1)) {
                            lua_rawgeti(mL, -1, 1);
                            const char* vName = lua_tostring(mL, -1);
                            int vId = WorldPercussionSynth::voiceIdFromName(vName);
                            lua_pop(mL, 1);

                            lua_rawgeti(mL, -1, 2);
                            float vel = static_cast<float>(lua_tonumber(mL, -1));
                            lua_pop(mL, 1);

                            if (vId >= 0) {
                                pat.steps[static_cast<size_t>(stepIdx)].push_back({vId, vel});
                            }
                        }
                        lua_pop(mL, 1);
                    }
                }
            }
            lua_pop(mL, 1);
        }
    }
    lua_pop(mL, 3); // hits, rhythm table, module table
    outPattern = std::move(pat);
    return true;
}

bool LuaIdeaHost::generateIdea(uint64_t seed, const std::string& traditionId,
                               const std::string& scaleId, int rootSemitone, int stepCount,
                               IdeaPattern& outIdea, std::string* error, float density) {
    if (!mReady || !mL) {
        if (error) *error = "LuaIdeaHost not ready";
        return false;
    }

    lua_getfield(mL, LUA_REGISTRYINDEX, "euterpe_ideas");
    lua_getfield(mL, -1, "generateIdea");

    lua_newtable(mL);
    lua_pushinteger(mL, static_cast<lua_Integer>(seed));
    lua_setfield(mL, -2, "seed");
    lua_pushstring(mL, traditionId.c_str());
    lua_setfield(mL, -2, "tradition");
    lua_pushstring(mL, scaleId.c_str());
    lua_setfield(mL, -2, "scaleId");
    lua_pushinteger(mL, rootSemitone);
    lua_setfield(mL, -2, "rootSemi");
    lua_pushinteger(mL, stepCount);
    lua_setfield(mL, -2, "stepCount");
    if (density > 0.0f) {
        lua_pushnumber(mL, density);
        lua_setfield(mL, -2, "density");
    }

    if (lua_pcall(mL, 1, 1, 0) != LUA_OK) {
        if (error) *error = lua_tostring(mL, -1) ? lua_tostring(mL, -1) : "generateIdea error";
        lua_pop(mL, 2);
        return false;
    }

    if (!lua_istable(mL, -1)) {
        lua_pop(mL, 2);
        if (error) *error = "generateIdea did not return table";
        return false;
    }

    IdeaPattern idea;
    lua_getfield(mL, -1, "stepCount");
    idea.stepCount = std::max(1, static_cast<int>(lua_tointeger(mL, -1)));
    lua_pop(mL, 1);

    auto readNoteList = [&](const char* fieldName, std::vector<IdeaNote>& dest) {
        lua_getfield(mL, -1, fieldName);
        if (lua_istable(mL, -1)) {
            lua_Integer n = luaL_len(mL, -1);
            for (lua_Integer i = 1; i <= n; ++i) {
                lua_rawgeti(mL, -1, i);
                if (lua_istable(mL, -1)) {
                    IdeaNote note;
                    lua_getfield(mL, -1, "step");
                    note.step = static_cast<int>(lua_tointeger(mL, -1));
                    lua_pop(mL, 1);

                    lua_getfield(mL, -1, "dur");
                    note.durationSteps = std::max(1, static_cast<int>(lua_tointeger(mL, -1)));
                    lua_pop(mL, 1);

                    lua_getfield(mL, -1, "midi");
                    note.midiNote = std::clamp(static_cast<int>(lua_tointeger(mL, -1)), 21, 108);
                    lua_pop(mL, 1);

                    lua_getfield(mL, -1, "cents");
                    note.cents = static_cast<float>(lua_tonumber(mL, -1));
                    lua_pop(mL, 1);

                    lua_getfield(mL, -1, "vel");
                    note.velocity = std::clamp(static_cast<float>(lua_tonumber(mL, -1)), 0.1f, 1.0f);
                    lua_pop(mL, 1);

                    dest.push_back(note);
                }
                lua_pop(mL, 1);
            }
        }
        lua_pop(mL, 1);
    };

    readNoteList("notes", idea.notes);
    readNoteList("bassNotes", idea.bassNotes);

    lua_pop(mL, 2); // idea table, module table
    outIdea = std::move(idea);
    return true;
}

} // namespace euterpe
