cmake_minimum_required(VERSION 3.25)

# Universal binary on macOS (Apple Silicon + Intel). Must be set before project().
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(JGMasteringEQ VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(JGEQ_BUILD_TESTS "Build the headless test suite" ON)

# ---- JUCE (pinned) ----------------------------------------------------------
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        9.0.6
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

# ---- Plugin -----------------------------------------------------------------
# Formats: VST3 + AU now. AAX is added once the PACE signing tools arrive
# (the AAX SDK is confidential and must never be committed to a public repo).
set(JGEQ_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND JGEQ_FORMATS AU)
endif()

juce_add_plugin(JGMasteringEQ
    COMPANY_NAME "JG Designs"
    COMPANY_WEBSITE ""
    BUNDLE_ID com.jgdesigns.masteringeq
    PLUGIN_MANUFACTURER_CODE Jgds      # 4 chars, at least one uppercase. Never change after release.
    PLUGIN_CODE Jgme                   # 4 chars, unique per plugin. Never change after release.
    PRODUCT_NAME "JG Mastering EQ"
    FORMATS ${JGEQ_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Fx EQ Mastering
    AU_MAIN_TYPE kAudioUnitType_Effect
    NEEDS_WEB_BROWSER TRUE
    NEEDS_CURL FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(JGMasteringEQ PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/Parameters.cpp
    src/ui/WebResources.cpp)

# The interface (ui/ folder) is compiled into the plugin so it works offline.
juce_add_binary_data(JGUIData
    HEADER_NAME JGUIData.h
    NAMESPACE JGUIData
    SOURCES
        ui/index.html
        ui/app.js
        ui/fonts/Archivo-latin.woff2
        ui/fonts/Michroma-latin.woff2)

target_include_directories(JGMasteringEQ PUBLIC src)

target_compile_definitions(JGMasteringEQ PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(JGMasteringEQ
    PRIVATE
        JGUIData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---- Tests ------------------------------------------------------------------
if(JGEQ_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
