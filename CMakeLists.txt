cmake_minimum_required(VERSION 3.22)
project(SideFregna VERSION 0.1.0)

include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.8 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(SideFregna
    COMPANY_NAME "Side Fregna"
    BUNDLE_ID "audio.sidefregna.plugin"
    PLUGIN_MANUFACTURER_CODE SdFg
    PLUGIN_CODE Sfg1
    FORMATS VST3 Standalone
    PRODUCT_NAME "Side Fregna"
    IS_SYNTH FALSE IS_MIDI_EFFECT FALSE NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(SideFregna)
target_sources(SideFregna PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_features(SideFregna PRIVATE cxx_std_17)
target_compile_definitions(SideFregna PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(SideFregna PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
