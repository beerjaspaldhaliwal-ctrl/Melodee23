# ---- JUCE ---------------------------------------------------------------------
if(MELODEE_JUCE_PATH)
    add_subdirectory(${MELODEE_JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---- Formats -----------------------------------------------------------------
set(MELODEE_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND MELODEE_FORMATS AU)
elseif(UNIX)
    list(APPEND MELODEE_FORMATS LV2)
endif()

juce_add_plugin(Melodee
    COMPANY_NAME                "Melodee"
    COMPANY_WEBSITE             "https://github.com/beerjaspaldhaliwal-ctrl/Melodee23"
    BUNDLE_ID                   com.melodee.melodee
    PLUGIN_MANUFACTURER_CODE    Mlde
    PLUGIN_CODE                 Mldy
    FORMATS                     ${MELODEE_FORMATS}
    PRODUCT_NAME                "Melodee"
    DESCRIPTION                 "Scales, chord progressions and melody variations"
    ICON_BIG                    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icon.png"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Instrument Generator
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    LV2URI                      "https://github.com/beerjaspaldhaliwal-ctrl/Melodee23"
)

target_sources(Melodee PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Session.cpp
    Source/Sequencer.cpp
    Source/PreviewSynth.cpp
    Source/ui/MelodeeLookAndFeel.cpp
    Source/ui/PianoRoll.cpp
    Source/ui/ProgressionBrowser.cpp
    Source/ui/ChordStrip.cpp
    Source/ui/GeneratorPanel.cpp
    Source/ui/DragExportButton.cpp
)

target_include_directories(Melodee PRIVATE Source)

target_compile_definitions(Melodee PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE=0)

if(MSVC)
    target_compile_options(Melodee PRIVATE /utf-8)
endif()

target_link_libraries(Melodee
    PRIVATE
        melodee_core
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---- Headless processor tests (drive the real plugin with a fake DAW transport) ----
if(MELODEE_BUILD_TESTS)
    juce_add_console_app(MelodeeProcessorTests PRODUCT_NAME "MelodeeProcessorTests")

    target_sources(MelodeeProcessorTests PRIVATE
        tests/ProcessorTests.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Session.cpp
        Source/Sequencer.cpp
        Source/PreviewSynth.cpp
        Source/ui/MelodeeLookAndFeel.cpp
        Source/ui/PianoRoll.cpp
        Source/ui/ProgressionBrowser.cpp
        Source/ui/ChordStrip.cpp
        Source/ui/GeneratorPanel.cpp
        Source/ui/DragExportButton.cpp)

    target_include_directories(MelodeeProcessorTests PRIVATE Source)

    target_compile_definitions(MelodeeProcessorTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1
        JUCE_MODAL_LOOPS_PERMITTED=1)

    if(MSVC)
        target_compile_options(MelodeeProcessorTests PRIVATE /utf-8)
    endif()

    target_link_libraries(MelodeeProcessorTests PRIVATE
        melodee_core
        juce::juce_audio_utils
        juce::juce_recommended_config_flags)

    add_test(NAME melodee_processor_tests COMMAND MelodeeProcessorTests)
endif()
