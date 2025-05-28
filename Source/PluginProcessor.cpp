#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "weirdness", "Weirdness", 0.0f, 1.0f, 0.5f));
    
    return { params.begin(), params.end() };
}

FidgetAudioProcessor::FidgetAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       parameters(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    weirdnessParam = parameters.getRawParameterValue("weirdness");
    initializeNoteWeirdness();
}

FidgetAudioProcessor::~FidgetAudioProcessor()
{
}

void FidgetAudioProcessor::initializeNoteWeirdness()
{
    // Use deterministic "randomness" based on note number
    for (int note = 0; note < 128; ++note)
    {
        // Create unique values for each note using hash-like operations
        float seed1 = std::sin(note * 0.1234f) * 1000.0f;
        float seed2 = std::cos(note * 0.5678f) * 1000.0f;
        float seed3 = std::sin(note * 0.9876f) * 1000.0f;
        
        auto& nw = noteWeirdness[note];
        
        // Assign weird type based on note
        int typeIndex = std::abs(static_cast<int>(seed1)) % static_cast<int>(WeirdType::NUM_TYPES);
        nw.type = static_cast<WeirdType>(typeIndex);
        
        // Set parameters for each type
        nw.wobbleRate = 0.5f + (seed1 - std::floor(seed1)) * 20.0f;  // Increased from 10.0f to 20.0f
        nw.glitchChance = (seed2 - std::floor(seed2)) * 0.6f;  // Increased from 0.3f to 0.6f
        nw.harmonicMix = 2.0f + (seed3 - std::floor(seed3)) * 10.0f;  // Increased from 5.0f to 10.0f
        nw.ringModFreq = 50.0f + (seed1 - std::floor(seed1)) * 1000.0f;  // Increased from 500.0f to 1000.0f
        nw.filterFreq = 200.0f + (seed2 - std::floor(seed2)) * 4000.0f;  // Increased from 2000.0f to 4000.0f
        nw.bitDepth = 2.0f + (seed3 - std::floor(seed3)) * 14.0f;
        nw.grainSize = 0.001f + (seed1 - std::floor(seed1)) * 0.1f;  // Increased from 0.05f to 0.1f
    }
}

FidgetAudioProcessor::WeirdType FidgetAudioProcessor::getCurrentWeirdType() const
{
    if (currentNote >= 0 && currentNote < 128)
        return noteWeirdness[currentNote].type;
    return WeirdType::Wobbler;
}

const juce::String FidgetAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FidgetAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FidgetAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FidgetAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FidgetAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FidgetAudioProcessor::getNumPrograms()
{
    return 1;
}

int FidgetAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FidgetAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String FidgetAudioProcessor::getProgramName (int index)
{
    return {};
}

void FidgetAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void FidgetAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
}

void FidgetAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FidgetAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

float FidgetAudioProcessor::processWeirdOscillator(float baseValue, int noteNumber, float weirdnessAmount)
{
    if (noteNumber < 0 || noteNumber >= 128) return baseValue;
    
    const auto& nw = noteWeirdness[noteNumber];
    float output = baseValue;
    
    switch (nw.type)
    {
        case WeirdType::Wobbler:
        {
            float wobble = std::sin(wobblePhase * 2.0f * juce::MathConstants<float>::pi);
            float freqMod = 1.0f + (wobble * 0.8f * weirdnessAmount);  // Increased from 0.2f to 0.8f
            output = baseValue * freqMod;
            wobblePhase += nw.wobbleRate / currentSampleRate;
            if (wobblePhase > 1.0f) wobblePhase -= 1.0f;
            break;
        }
        
        case WeirdType::Glitcher:
        {
            glitchCounter++;
            if (glitchCounter > currentSampleRate / 100) // Check every 10ms
            {
                glitchCounter = 0;
                float random = std::sin(phase * 12345.6789f) * 1000.0f;
                random = random - std::floor(random);
                if (random < nw.glitchChance * weirdnessAmount)
                {
                    output = baseValue * ((random < 0.5f) ? -2.0f : 4.0f);  // More extreme glitches
                }
            }
            break;
        }
        
        case WeirdType::Harmonizer:
        {
            float harmonic = std::sin(2.0f * juce::MathConstants<float>::pi * phase2);
            output = baseValue * (1.0f - weirdnessAmount * 0.8f) + 
                     harmonic * weirdnessAmount * 1.2f;  // More harmonic content
            phase2 += (frequency * nw.harmonicMix) / currentSampleRate;
            if (phase2 > 1.0f) phase2 -= 1.0f;
            break;
        }
        
        case WeirdType::Reverser:
        {
            float reverseAmount = std::sin(phase * juce::MathConstants<float>::pi * 16.0f);  // More intense reversing
            output = baseValue * (1.0f - weirdnessAmount * 1.5f + reverseAmount * weirdnessAmount * 2.0f);
            break;
        }
        
        case WeirdType::BitCrusher:
        {
            float bitDepth = 16.0f - (15.5f * weirdnessAmount);  // More extreme bit crushing
            float scale = std::pow(2.0f, bitDepth);
            output = std::round(baseValue * scale) / scale;
            break;
        }
        
        case WeirdType::RingMod:
        {
            float ringMod = std::sin(2.0f * juce::MathConstants<float>::pi * phase2);
            output = baseValue * (1.0f - weirdnessAmount * 1.5f + ringMod * weirdnessAmount * 2.0f);  // More ring mod intensity
            phase2 += nw.ringModFreq / currentSampleRate;
            if (phase2 > 1.0f) phase2 -= 1.0f;
            break;
        }
        
        case WeirdType::Granular:
        {
            grainPhase += 1.0f / (nw.grainSize * currentSampleRate);
            if (grainPhase > 1.0f)
            {
                grainPhase = 0.0f;
                phase = 0.0f; // Reset phase for stutter effect
            }
            float grainEnv = std::sin(grainPhase * juce::MathConstants<float>::pi);
            output = baseValue * (1.0f - weirdnessAmount * 1.5f + grainEnv * weirdnessAmount * 2.0f);  // More dramatic granular effect
            break;
        }
        
        case WeirdType::FilterSweep:
        {
            // Simple resonant filter simulation
            float cutoff = nw.filterFreq * (1.0f + std::sin(wobblePhase * 2.0f * juce::MathConstants<float>::pi));
            float resonance = 10.0f * weirdnessAmount;  // Double the resonance for more dramatic filter sweeps
            float filterFreq = cutoff / currentSampleRate;
            filterState += (baseValue - filterState) * filterFreq;
            float highpass = baseValue - filterState;
            output = filterState + highpass * resonance;
            wobblePhase += 0.5f / currentSampleRate;
            if (wobblePhase > 1.0f) wobblePhase -= 1.0f;
            break;
        }
        
        default:
            break;
    }
    
    return output;
}

void FidgetAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Process MIDI messages
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        
        if (message.isNoteOn())
        {
            currentNote = message.getNoteNumber();
            velocity = message.getFloatVelocity();
            frequency = midiNoteToFrequency(currentNote);
            noteOn = true;
            
            // Reset oscillator states for consistent sound
            phase = 0.0f;
            phase2 = 0.0f;
            wobblePhase = 0.0f;
            grainPhase = 0.0f;
            glitchCounter = 0;
            filterState = 0.0f;
        }
        else if (message.isNoteOff())
        {
            if (message.getNoteNumber() == currentNote)
            {
                noteOn = false;
            }
        }
    }
    
    // Get current weirdness value
    float weirdness = *weirdnessParam;
    
    // Calculate envelope
    float envelopeIncrement = 0.0f;
    if (noteOn && envelope < 1.0f)
    {
        envelopeIncrement = 1.0f / (attackTime * currentSampleRate);
    }
    else if (!noteOn && envelope > 0.0f)
    {
        envelopeIncrement = -1.0f / (releaseTime * currentSampleRate);
    }
    
    // Generate audio
    const float phaseIncrement = frequency / currentSampleRate;
    
    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            // Update envelope
            envelope = juce::jlimit(0.0f, 1.0f, envelope + envelopeIncrement);
            
            // Generate base sine wave
            float sineWave = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
            
            // Apply weird processing
            float weirdWave = processWeirdOscillator(sineWave, currentNote, weirdness);
            
            // Output with envelope and velocity
            channelData[sample] = amplitude * envelope * velocity * weirdWave;
            
            phase += phaseIncrement;
            if (phase > 1.0f)
                phase -= 1.0f;
        }
    }
}

bool FidgetAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* FidgetAudioProcessor::createEditor()
{
    return new FidgetAudioProcessorEditor (*this);
}

void FidgetAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void FidgetAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FidgetAudioProcessor();
}