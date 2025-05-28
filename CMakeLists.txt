cmake_minimum_required(VERSION 3.16)
project(Fidget VERSION 0.0.1)

# Include JUCE
add_subdirectory(JUCE)

# Create our plugin target
juce_add_plugin(Fidget
    PLUGIN_MANUFACTURER_CODE Luke
    PLUGIN_CODE Fidg
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Fidget"
    COMPANY_NAME "Luke"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_NAME "Fidget"
)

# Generate JUCE header
juce_generate_juce_header(Fidget)

# Source files
target_sources(Fidget
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

# Link required JUCE modules
target_link_libraries(Fidget
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Set C++ standard
target_compile_features(Fidget PRIVATE cxx_std_17)