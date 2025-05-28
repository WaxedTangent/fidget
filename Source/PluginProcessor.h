#pragma once

#include <JuceHeader.h>

class FidgetAudioProcessor : public juce::AudioProcessor
{
public:
    FidgetAudioProcessor();
    ~FidgetAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    // Public getter for UI
    int getCurrentNote() const { return currentNote; }
    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }
    
    // Weird behavior types
    enum class WeirdType
    {
        Wobbler,        // Frequency wobbles
        Glitcher,       // Random glitches
        Harmonizer,     // Strange harmonics
        Reverser,       // Phase reversal
        BitCrusher,    // Bit reduction
        RingMod,       // Ring modulation
        Granular,      // Micro stutters
        FilterSweep,   // Resonant filter
        NUM_TYPES
    };
    
    const char* getWeirdTypeName(WeirdType type) const
    {
        switch(type)
        {
            case WeirdType::Wobbler: return "Wobbler";
            case WeirdType::Glitcher: return "Glitcher";
            case WeirdType::Harmonizer: return "Harmonizer";
            case WeirdType::Reverser: return "Reverser";
            case WeirdType::BitCrusher: return "BitCrusher";
            case WeirdType::RingMod: return "RingMod";
            case WeirdType::Granular: return "Granular";
            case WeirdType::FilterSweep: return "FilterSweep";
            default: return "Unknown";
        }
    }
    
    WeirdType getCurrentWeirdType() const;

private:
    // Parameters
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* weirdnessParam = nullptr;
    
    double currentSampleRate = 44100.0;
    float phase = 0.0f;
    float frequency = 440.0f;
    float amplitude = 0.1f;
    
    // MIDI handling
    int currentNote = -1;  // -1 means no note playing
    float velocity = 0.0f;
    
    // Simple envelope
    float envelope = 0.0f;
    float attackTime = 0.01f;  // 10ms attack
    float releaseTime = 0.1f;  // 100ms release
    bool noteOn = false;
    
    // Weird synthesis state
    float phase2 = 0.0f;      // Secondary oscillator
    float filterState = 0.0f; // For filter sweep
    float bitCrushHold = 0.0f; // For bit crusher
    int glitchCounter = 0;    // For glitcher
    float wobblePhase = 0.0f; // For wobbler LFO
    float grainPhase = 0.0f;  // For granular
    
    // Per-note deterministic weirdness
    struct NoteWeirdness
    {
        float wobbleRate = 0.0f;
        float glitchChance = 0.0f;
        float harmonicMix = 0.0f;
        float ringModFreq = 0.0f;
        float filterFreq = 0.0f;
        float bitDepth = 0.0f;
        float grainSize = 0.0f;
        WeirdType type = WeirdType::Wobbler;
    };
    
    std::array<NoteWeirdness, 128> noteWeirdness;
    
    // Helper functions
    float midiNoteToFrequency(int midiNote)
    {
        return 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
    }
    
    void initializeNoteWeirdness();
    float processWeirdOscillator(float baseValue, int noteNumber, float weirdnessAmount);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FidgetAudioProcessor)
};