cmake_minimum_required(VERSION 3.22)
project(AureaPiano VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

option(AUREA_BUILD_AAX "Build the unsigned AAX Native package for Pro Tools" OFF)
set(AUREA_FORMATS VST3 Standalone)
if(AUREA_BUILD_AAX)
    list(APPEND AUREA_FORMATS AAX)
endif()

juce_add_plugin(AureaPiano
    COMPANY_NAME "Aurea Audio"
    BUNDLE_ID "audio.aurea.piano"
    PLUGIN_MANUFACTURER_CODE Aure
    PLUGIN_CODE Apno
    PRODUCT_NAME "Aurea Piano"
    PLUGIN_NAME "Aurea Piano"
    FORMATS ${AUREA_FORMATS}
    IS_SYNTH TRUE
    AAX_CATEGORY SWGenerators
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(AureaPiano PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_features(AureaPiano PRIVATE cxx_std_17)
if(MSVC)
    target_compile_options(AureaPiano PRIVATE /utf-8)
endif()
target_compile_definitions(AureaPiano PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(AureaPiano PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

include(CTest)
if(BUILD_TESTING)
    add_executable(model_smoke Tests/model_smoke.cpp)
    target_compile_features(model_smoke PRIVATE cxx_std_17)
    add_test(NAME model_smoke COMMAND model_smoke)
    find_program(AUREA_NODE node)
    if(AUREA_NODE)
        add_test(NAME demo_smoke COMMAND "${AUREA_NODE}" "${CMAKE_CURRENT_SOURCE_DIR}/Tests/demo_smoke.cjs")
    endif()
endif()
