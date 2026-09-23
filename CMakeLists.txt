cmake_minimum_required(VERSION 3.22)

project(luaPlug VERSION 1.0.0)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

# ============================================================================ #

set(JUCE_PATH "/Users/nicorusso/Development/JUCE/"
    CACHE PATH "Path to an external JUCE checkout")

# ============================================================================ #

if(CMAKE_GENERATOR MATCHES "Xcode|Visual Studio")
    message(WARNING
        "Generator '${CMAKE_GENERATOR}' does not support "
        "CMAKE_EXPORT_COMPILE_COMMANDS. Configure a separate build "
        "directory with -G Ninja (or 'Unix Makefiles') if you need "
        "compile_commands.json for clangd.")
endif()

if(JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

if(APPLE)
    set(BUILD_FORMATS AU VST3 Standalone)
else()
    set(BUILD_FORMATS VST3 Standalone)
endif()

# ============================================================================ #

add_subdirectory(vendor/pffft)

set(LUA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/lua)

file(GLOB LUA_SOURCES
    ${LUA_DIR}/src/*.c
)

list(REMOVE_ITEM LUA_SOURCES
    ${LUA_DIR}/src/lua.c
    ${LUA_DIR}/src/luac.c
)

add_library(lua STATIC ${LUA_SOURCES})

target_include_directories(lua PUBLIC
    ${LUA_DIR}/src
)

# ============================================================================ #

juce_add_plugin(luaPlug
    PRODUCT_NAME "[omni] Lua Plug"
    COMPANY_NAME "Omnicorum"

    PLUGIN_MANUFACTURER_CODE Omni
    PLUGIN_CODE luap

    FORMATS ${BUILD_FORMATS}

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE

    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    COPY_PLUGIN_AFTER_BUILD TRUE

    VST3_CATEGORIES Fx
    AU_MAIN_TYPE kAudioUnitType_Effect
)

juce_add_binary_data(Data
    SOURCES
        fonts/JetBrainsMono-Regular.ttf
        fonts/JetBrainsMono-Bold.ttf
)
set_target_properties(Data PROPERTIES EXPORT_COMPILE_COMMANDS OFF)

juce_generate_juce_header(luaPlug)

target_sources(luaPlug
    PRIVATE
        src/processor.cpp
        src/editor.cpp
)

target_include_directories(luaPlug
    PRIVATE
        include/
        src/
)

target_compile_definitions(luaPlug
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(luaPlug
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_devices
        PFFFT::PFFFT
        Data
        lua
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)


