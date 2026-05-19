cmake_minimum_required(VERSION 3.15)

project(InGenSampler VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------------
# Fetch JUCE from GitHub (Portable & Self-Contained Build)
# -------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
)

FetchContent_Declare(
    signalsmith-stretch
    GIT_REPOSITORY https://github.com/Signalsmith-Audio/signalsmith-stretch.git
    GIT_TAG main
)

# Avoid building unnecessary JUCE examples and extra utilities
set(JUCE_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(JUCE_BUILD_EXTRAS OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(juce signalsmith-stretch)

# -------------------------------------------------------------
# Add InGen Plugin Target
# -------------------------------------------------------------
juce_add_plugin(InGenSampler
    VERSION "1.0.0"
    COMPANY_NAME "InGen"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_NAME "InGen Sampler"
    PLUGIN_DESC "AI-Powered Multi-Layer Instrument Sampler"
    FORMATS VST3 AU Standalone
)

target_sources(InGenSampler PRIVATE
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
    Source/Network/AudioGeneratorAPI.h
    Source/DSP/PitchDetector.h
    Source/DSP/PitchDetector.cpp
    Source/DSP/TransientDetector.h
    Source/DSP/TransientDetector.cpp
    Source/DSP/SampleAnalysisJob.h
    Source/DSP/SampleAnalysisJob.cpp
    Source/Engine/SamplerSoundBase.h
    Source/Engine/SamplerVoiceA.h
    Source/Engine/SamplerVoiceA.cpp
    Source/Engine/SamplerVoiceB.h
    Source/Engine/SamplerVoiceB.cpp
    Source/Engine/SamplerEngine.h
    Source/Engine/SamplerEngine.cpp
    Source/Serialization/PresetSerializer.h
    Source/Serialization/PresetSerializer.cpp
    Source/Serialization/PresetArchiver.h
    Source/Serialization/PresetArchiver.cpp
)

target_link_libraries(InGenSampler PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_gui_extra
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_events
    juce::juce_core
)

target_include_directories(InGenSampler PRIVATE
    ${signalsmith-stretch_SOURCE_DIR}
)

# Standard C++ Compiler Settings
if (MSVC)
    target_compile_options(InGenSampler PRIVATE /W4)
else()
    target_compile_options(InGenSampler PRIVATE -Wall -Wextra)
endif()
