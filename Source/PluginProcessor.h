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
    
    // Wave types (one per note in octave)
    enum class WaveType
    {
        Sine,           // C
        Square,         // C#
        Sawtooth,       // D
        Triangle,       // D#
        Pulse25,        // E
        WhiteNoise,     // F
        PinkNoise,      // F#
        Supersaw,       // G
        FM,             // G#
        SquareSub,      // A
        Pulse75,        // A#
        CrackleNoise,   // B
        NUM_WAVE_TYPES
    };
    
    const char* getWaveTypeName(WaveType type) const
    {
        switch(type)
        {
            case WaveType::Sine: return "Sine";
            case WaveType::Square: return "Square";
            case WaveType::Sawtooth: return "Sawtooth";
            case WaveType::Triangle: return "Triangle";
            case WaveType::Pulse25: return "Pulse 25%";
            case WaveType::WhiteNoise: return "White Noise";
            case WaveType::PinkNoise: return "Pink Noise";
            case WaveType::Supersaw: return "Supersaw";
            case WaveType::FM: return "FM";
            case WaveType::SquareSub: return "Square+Sub";
            case WaveType::Pulse75: return "Pulse 75%";
            case WaveType::CrackleNoise: return "Crackle";
            default: return "Unknown";
        }
    }
    
    WaveType getCurrentWaveType() const;
    
    // Filter types (one per note in octave)
    enum class FilterType
    {
        LowPass,        // C
        HighPass,       // C#
        BandPass,       // D
        Notch,          // D#
        Comb,           // E
        FormantA,       // F
        FormantE,       // F#
        FormantI,       // G
        FormantO,       // G#
        FormantU,       // A
        Phaser,         // A#
        RingModFilter,  // B
        NUM_FILTER_TYPES
    };
    
    const char* getFilterTypeName(FilterType type) const
    {
        switch(type)
        {
            case FilterType::LowPass: return "Low Pass";
            case FilterType::HighPass: return "High Pass";
            case FilterType::BandPass: return "Band Pass";
            case FilterType::Notch: return "Notch";
            case FilterType::Comb: return "Comb";
            case FilterType::FormantA: return "Formant A";
            case FilterType::FormantE: return "Formant E";
            case FilterType::FormantI: return "Formant I";
            case FilterType::FormantO: return "Formant O";
            case FilterType::FormantU: return "Formant U";
            case FilterType::Phaser: return "Phaser";
            case FilterType::RingModFilter: return "Ring Mod Filter";
            default: return "Unknown";
        }
    }
    
    FilterType getCurrentFilterType() const;

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
    
    // Additional oscillator state
    float subPhase = 0.0f;    // For sub oscillator
    float fmPhase = 0.0f;     // For FM carrier
    float noiseState = 0.0f;  // For pink noise
    float crackleTimer = 0.0f; // For crackle noise
    std::array<float, 7> sawPhases = {0}; // For supersaw
    
    // Random number generator for consistent randomness
    juce::Random random;
    
    // Filter state variables
    float filterState1 = 0.0f;
    float filterState2 = 0.0f;
    float filterState3 = 0.0f;
    float filterState4 = 0.0f;
    float combDelay[44100] = {0}; // 1 second of delay for comb filter
    int combIndex = 0;
    float phaserPhase = 0.0f;
    std::array<float, 4> phaserStages = {0};
    
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
        WaveType waveType = WaveType::Sine;
        FilterType filterType = FilterType::LowPass;
        
        // Random effect amounts for each knob position (0-127)
        std::array<float, 128> randomAmounts = {0};
        // Random filter parameters for each knob position
        std::array<float, 128> randomCutoffs = {0};
        std::array<float, 128> randomResonances = {0};
    };
    
    std::array<NoteWeirdness, 128> noteWeirdness;
    
    // Helper functions
    float midiNoteToFrequency(int midiNote)
    {
        return 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
    }
    
    WaveType getWaveTypeForNote(int midiNote) const
    {
        int noteInOctave = midiNote % 12;
        return static_cast<WaveType>(noteInOctave);
    }
    
    FilterType getFilterTypeForNote(int midiNote) const
    {
        int noteInOctave = midiNote % 12;
        return static_cast<FilterType>(noteInOctave);
    }
    
    void initializeNoteWeirdness();
    float generateOscillator(WaveType type, float phase, float frequency);
    float processWeirdOscillator(float baseValue, int noteNumber, float weirdnessAmount);
    float processChaosFilter(float input, FilterType type, float cutoff, float resonance);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FidgetAudioProcessor)
};