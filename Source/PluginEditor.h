#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class FidgetAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   public juce::Timer
{
public:
    FidgetAudioProcessorEditor (FidgetAudioProcessor&);
    ~FidgetAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    FidgetAudioProcessor& audioProcessor;
    int lastNote = -1;
    
    // UI Components
    juce::Slider weirdnessKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> weirdnessAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FidgetAudioProcessorEditor)
};