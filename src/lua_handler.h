#pragma once

#include "juce/juce.h"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include <array>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>

class LuaHandler {
  private:
    struct Slot {
        lua_State *L     = nullptr;
        bool       valid = false;

        const std::atomic<double> *fs = nullptr;
        std::array<double, 8>      macros{};
        int                        channel = 0;
        std::string                last_error;

        Slot()                        = default;
        Slot(const Slot &)            = delete;
        Slot &operator=(const Slot &) = delete;

        ~Slot() {
            if (L)
                lua_close(L);
        }

        void setMacro(int idx, double value) {
            if (idx >= 0 && idx < (int)macros.size())
                macros[(size_t)idx] = value;
        }

        void setChannel(int ch) { channel = ch; }

        // Only ever called on a slot the audio thread is NOT using.
        bool compile(const char *str) {
            if (L)
                lua_close(L);

            L = luaL_newstate();
            luaL_openlibs(L);

            lua_pushlightuserdata(L, this);
            lua_pushcclosure(L, &Slot::luaGetSampleRate, 1);
            lua_setglobal(L, "sampleRate");

            lua_pushlightuserdata(L, this);
            lua_pushcclosure(L, &Slot::luaGetChannel, 1);
            lua_setglobal(L, "getChannel");

            lua_pushlightuserdata(L, this);
            lua_pushcclosure(L, &Slot::luaGetMacro, 1);
            lua_setglobal(L, "macro");

            luaL_dostring(L, prelude);

            if (luaL_dostring(L, str) != LUA_OK) {
                const char *msg = lua_tostring(L, -1);
                last_error      = msg ? msg : "Unknown compile error";
                lua_pop(L, 1);
                valid = false;
                return false;
            }

            lua_getglobal(L, "process");
            const bool has_process = lua_isfunction(L, -1);
            lua_pop(L, 1);

            if (!has_process) {
                last_error = "Script must define: function process(input)";
                valid      = false;
                return false;
            }

            last_error.clear();
            valid = true;
            return true;
        }

        double process(double xn) {
            if (!valid)
                return 0;

            lua_getglobal(L, "process");
            lua_pushnumber(L, xn);

            if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
                lua_pop(L, 1);
                return 0;
            }

            double result = lua_tonumber(L, -1);
            lua_pop(L, 1);
            return result;
        }

        static int luaGetSampleRate(lua_State *L) {
            auto *self =
                static_cast<Slot *>(lua_touserdata(L, lua_upvalueindex(1)));
            lua_pushnumber(L, self->fs ? self->fs->load() : 0.0);
            return 1;
        }

        static int luaGetChannel(lua_State *L) {
            auto *self =
                static_cast<Slot *>(lua_touserdata(L, lua_upvalueindex(1)));
            lua_pushinteger(L, self->fs ? self->channel : 0);
            return 1;
        }

        static int luaGetMacro(lua_State *L) {
            auto *self =
                static_cast<Slot *>(lua_touserdata(L, lua_upvalueindex(1)));
            int idx = (int)lua_tointeger(L, 1) - 1;
            if (idx < 0 || idx >= (int)self->macros.size()) {
                lua_pushnumber(L, 0.0);
                return 1;
            }
            lua_pushnumber(L, self->macros[(size_t)idx]);
            return 1;
        }
    };

  public:
    static constexpr const char *baseline = R"(
function process(input, channel)
    return input
end
)";

    LuaHandler() {
        for (auto &s : slots)
            s.fs = &fs;

        slots[0].compile(baseline);
        active.store(0);
    }

    // Any thread, not concurrently with processBlock (JUCE guarantees this).
    void prepare(double sample_rate) { fs.store(sample_rate); }

    // NON-AUDIO THREADS (message thread, setStateInformation, prepareToPlay)
    // Compiles into the spare slot and swaps it in on success.
    // On failure the currently running script is left untouched.
    // Returns an empty string on success, the error otherwise.
    juce::String compile(const juce::String &code) {
        std::lock_guard<std::mutex> lock(compile_mutex);

        const int spare = 1 - active.load();

        if (!waitUntilNotInUse(spare))
            return "Audio thread busy, try again.";

        auto &slot = slots[(size_t)spare];

        if (!slot.compile(code.toRawUTF8()))
            return juce::String(slot.last_error);

        active.store(spare);
        return {};
    }

    // AUDIO THREAD
    // Create one of these at the top of processBlock. It reserves the
    // active slot for the whole block so compile() can't close it.
    class ScopedBlock {
      public:
        explicit ScopedBlock(LuaHandler &h) : handler(h), slot(h.pin()) {}
        ~ScopedBlock() { handler.unpin(); }

        void   setMacro(int idx, double v) { slot->setMacro(idx, v); }
        void   setChannel(int ch) { slot->setChannel(ch); }
        double process(double xn) { return slot->process(xn); }

      private:
        LuaHandler &handler;
        Slot       *slot;

        JUCE_DECLARE_NON_COPYABLE(ScopedBlock)
    };

  private:
    // Hazard-pointer style handoff: publish which slot we're about to use,
    // then re-check it's still the active one. If compile() swapped in
    // between, retry with the new one.
    Slot *pin() {
        int idx = active.load();
        for (;;) {
            in_use.store(idx);
            const int now = active.load();
            if (now == idx)
                return &slots[(size_t)idx];
            idx = now;
        }
    }

    void unpin() { in_use.store(-1); }

    bool waitUntilNotInUse(int idx) {
        using namespace std::chrono_literals;
        for (int i = 0; i < 1000; ++i) {
            if (in_use.load() != idx)
                return true;
            std::this_thread::sleep_for(1ms);
        }
        return false;
    }

    std::array<Slot, 2> slots;
    std::atomic<int>    active{0};
    std::atomic<int>    in_use{-1};
    std::atomic<double> fs{0.0};
    std::mutex          compile_mutex;

    static constexpr const char *prelude = R"(
    function map(value, min, max)
        return min + value * (max - min)
    end

    function toDb(mag)
        return 20 * math.log(math.abs(mag), 10)
    end

    function toMag(dB)
        return 10 ^ (dB / 20)
    end
    )";
};
