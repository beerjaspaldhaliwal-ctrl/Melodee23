cmake_minimum_required(VERSION 3.22)

project(Melodee VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static MSVC runtime so the plugin does not need the VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(MELODEE_BUILD_PLUGIN "Build the JUCE audio plugin (VST3 / AU / Standalone)" ON)
option(MELODEE_BUILD_CLI    "Build the melodee command-line MIDI generator"         ON)
option(MELODEE_BUILD_TESTS  "Build the core unit tests"                             ON)
set(MELODEE_JUCE_PATH "" CACHE PATH "Path to a local JUCE checkout. Leave empty to download JUCE automatically.")

add_subdirectory(core)

if(MELODEE_BUILD_CLI)
    add_subdirectory(cli)
endif()

if(MELODEE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(MELODEE_BUILD_PLUGIN)
    add_subdirectory(plugin)
endif()
