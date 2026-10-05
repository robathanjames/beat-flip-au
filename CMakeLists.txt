cmake_minimum_required(VERSION 3.22)
project(BeatFlip VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BEATFLIP_BUILD_PLUGIN "Build the macOS AU and standalone app" ON)
set(BEATFLIP_JUCE_SOURCE_DIR "" CACHE PATH "Optional existing JUCE 8.0.15 checkout")

add_library(beatflip_core STATIC Source/GlitchEngine.cpp)
target_include_directories(beatflip_core PUBLIC Source)
set_target_properties(beatflip_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(beatflip_core PRIVATE /W4)
else()
    target_compile_options(beatflip_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(beatflip_tests Tests/EngineTests.cpp Tests/AllocationGuard.cpp)
    target_link_libraries(beatflip_tests PRIVATE beatflip_core)
    add_test(NAME audio_engine COMMAND beatflip_tests)
endif()

add_executable(beatflip_demo EXCLUDE_FROM_ALL Tools/RenderDemo.cpp)
target_link_libraries(beatflip_demo PRIVATE beatflip_core)

if(BEATFLIP_BUILD_PLUGIN)
    if(NOT APPLE)
        message(FATAL_ERROR "The Audio Unit needs macOS. For portable DSP tests, configure with -DBEATFLIP_BUILD_PLUGIN=OFF.")
    endif()

    if(BEATFLIP_JUCE_SOURCE_DIR)
        add_subdirectory("${BEATFLIP_JUCE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/juce")
    else()
        include(FetchContent)
        FetchContent_Declare(JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 8.0.15
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(JUCE)
    endif()

    juce_add_plugin(BeatFlip
        COMPANY_NAME "Rob James"
        BUNDLE_ID "com.robjames.beatflip"
        PLUGIN_MANUFACTURER_CODE Rbjm
        PLUGIN_CODE BtFp
        FORMATS AU Standalone
        PRODUCT_NAME "Beat Flip"
        DESCRIPTION "One-button rhythmic drum glitch effect"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        MICROPHONE_PERMISSION_ENABLED TRUE
        MICROPHONE_PERMISSION_TEXT "Beat Flip needs audio input to process your drum audio in the standalone app."
        AU_MAIN_TYPE kAudioUnitType_Effect
        COPY_PLUGIN_AFTER_BUILD FALSE)

    target_sources(BeatFlip PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/FactoryPresets.h)
    target_compile_definitions(BeatFlip PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)
    target_link_libraries(BeatFlip PRIVATE
        beatflip_core
        juce::juce_audio_utils
        PUBLIC juce::juce_recommended_config_flags
               juce::juce_recommended_warning_flags)

    if(BUILD_TESTING)
        juce_add_console_app(beatflip_processor_tests PRODUCT_NAME "Beat Flip integration tests")
        target_sources(beatflip_processor_tests PRIVATE Tests/ProcessorTests.cpp Tests/AllocationGuard.cpp)
        target_include_directories(beatflip_processor_tests PRIVATE Source Tests)
        target_link_libraries(beatflip_processor_tests PRIVATE BeatFlip juce::juce_audio_utils)
        add_test(NAME plugin_state_and_editor COMMAND beatflip_processor_tests)
    endif()
endif()
