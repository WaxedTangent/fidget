#include "PluginProcessor.h"
#include "PluginEditor.h"

FidgetAudioProcessorEditor::FidgetAudioProcessorEditor (FidgetAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Create the weirdness knob
    weirdnessKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    weirdnessKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    weirdnessKnob.setRange(0.0, 1.0, 0.01);
    addAndMakeVisible(weirdnessKnob);
    
    // Attach to parameter
    weirdnessAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(
        audioProcessor.getParameters(), "weirdness", weirdnessKnob));
    
    setSize (400, 400);
    startTimerHz(30); // Update UI 30 times per second
}

FidgetAudioProcessorEditor::~FidgetAudioProcessorEditor()
{
    stopTimer();
}

void FidgetAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Dark background
    g.fillAll (juce::Colours::black);
    
    // Title
    g.setColour (juce::Colours::cyan);
    g.setFont (28.0f);
    g.drawFittedText ("FIDGET", getLocalBounds().removeFromTop(50), juce::Justification::centred, 1);
    
    // Weird type indicator
    g.setFont (20.0f);
    
    int currentNote = audioProcessor.getCurrentNote();
    if (currentNote >= 0)
    {
        auto weirdType = audioProcessor.getCurrentWeirdType();
        const char* typeName = audioProcessor.getWeirdTypeName(weirdType);
        
        // Color code by type
        switch(weirdType)
        {
            case FidgetAudioProcessor::WeirdType::Wobbler:
                g.setColour(juce::Colours::purple); break;
            case FidgetAudioProcessor::WeirdType::Glitcher:
                g.setColour(juce::Colours::red); break;
            case FidgetAudioProcessor::WeirdType::Harmonizer:
                g.setColour(juce::Colours::orange); break;
            case FidgetAudioProcessor::WeirdType::Reverser:
                g.setColour(juce::Colours::yellow); break;
            case FidgetAudioProcessor::WeirdType::BitCrusher:
                g.setColour(juce::Colours::lime); break;
            case FidgetAudioProcessor::WeirdType::RingMod:
                g.setColour(juce::Colours::magenta); break;
            case FidgetAudioProcessor::WeirdType::Granular:
                g.setColour(juce::Colours::turquoise); break;
            case FidgetAudioProcessor::WeirdType::FilterSweep:
                g.setColour(juce::Colours::gold); break;
            default:
                g.setColour(juce::Colours::grey); break;
        }
        
        g.drawFittedText(typeName, getLocalBounds().removeFromTop(100).withTrimmedTop(50), 
                         juce::Justification::centred, 1);
        
        // Note info
        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        
        const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        int noteName = currentNote % 12;
        int octave = (currentNote / 12) - 2;
        
        juce::String noteText = juce::String(noteNames[noteName]) + juce::String(octave) + 
                               " (MIDI " + juce::String(currentNote) + ")";
        
        g.drawFittedText(noteText, getLocalBounds().removeFromTop(130).withTrimmedTop(100), 
                         juce::Justification::centred, 1);
    }
    else
    {
        g.setColour(juce::Colours::grey);
        g.drawFittedText("Play a note!", getLocalBounds().removeFromTop(100).withTrimmedTop(50), 
                         juce::Justification::centred, 1);
    }
    
    
    // Instructions
    g.setFont(12.0f);
    g.setColour(juce::Colours::grey);
    g.drawFittedText("Each note has its own weird behavior!\nTurn the knob to morph the weirdness.", 
                     getLocalBounds().removeFromBottom(50), 
                     juce::Justification::centred, 2);
}

void FidgetAudioProcessorEditor::resized()
{
    // Position the knob
    int knobSize = 100;
    weirdnessKnob.setBounds((getWidth() - knobSize) / 2, 200, knobSize, knobSize);
}

void FidgetAudioProcessorEditor::timerCallback()
{
    // Only repaint if the note has changed
    int currentNote = audioProcessor.getCurrentNote();
    if (currentNote != lastNote)
    {
        lastNote = currentNote;
        repaint();
    }
}