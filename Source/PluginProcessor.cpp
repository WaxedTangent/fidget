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
       parameters(*this, nullptr, "Parameters", createParameterLayout()),
       random(juce::Time::currentTimeMillis())
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
        
        // Assign wave type based on note in octave
        nw.waveType = getWaveTypeForNote(note);
        
        // Assign filter type based on note in octave
        nw.filterType = getFilterTypeForNote(note);
        
        // Set parameters for each type
        nw.wobbleRate = 0.5f + (seed1 - std::floor(seed1)) * 20.0f;
        nw.glitchChance = (seed2 - std::floor(seed2)) * 0.6f;
        nw.harmonicMix = 2.0f + (seed3 - std::floor(seed3)) * 10.0f;
        nw.ringModFreq = 50.0f + (seed1 - std::floor(seed1)) * 1000.0f;
        nw.filterFreq = 200.0f + (seed2 - std::floor(seed2)) * 4000.0f;
        nw.bitDepth = 2.0f + (seed3 - std::floor(seed3)) * 14.0f;
        nw.grainSize = 0.001f + (seed1 - std::floor(seed1)) * 0.1f;
        
        // Generate random effect amounts for each knob position
        for (int knobPos = 0; knobPos < 128; ++knobPos)
        {
            // Create unique random value for this note + knob position combination
            float knobSeed = std::sin((note * 128 + knobPos) * 0.7654f) * 1000.0f;
            nw.randomAmounts[knobPos] = knobSeed - std::floor(knobSeed);
            
            // Random filter parameters
            float filterSeed1 = std::sin((note * 128 + knobPos) * 0.4321f) * 1000.0f;
            float filterSeed2 = std::cos((note * 128 + knobPos) * 0.8765f) * 1000.0f;
            nw.randomCutoffs[knobPos] = 100.0f + (filterSeed1 - std::floor(filterSeed1)) * 8000.0f; // 100Hz to 8100Hz
            nw.randomResonances[knobPos] = (filterSeed2 - std::floor(filterSeed2)) * 0.95f; // 0 to 0.95
        }
    }
}

FidgetAudioProcessor::WeirdType FidgetAudioProcessor::getCurrentWeirdType() const
{
    if (currentNote >= 0 && currentNote < 128)
        return noteWeirdness[currentNote].type;
    return WeirdType::Wobbler;
}

FidgetAudioProcessor::WaveType FidgetAudioProcessor::getCurrentWaveType() const
{
    if (currentNote >= 0 && currentNote < 128)
        return noteWeirdness[currentNote].waveType;
    return WaveType::Sine;
}

FidgetAudioProcessor::FilterType FidgetAudioProcessor::getCurrentFilterType() const
{
    if (currentNote >= 0 && currentNote < 128)
        return noteWeirdness[currentNote].filterType;
    return FilterType::LowPass;
}

float FidgetAudioProcessor::generateOscillator(WaveType type, float phase, float frequency)
{
    switch (type)
    {
        case WaveType::Sine:
            return std::sin(2.0f * juce::MathConstants<float>::pi * phase);
            
        case WaveType::Square:
            return phase < 0.5f ? 1.0f : -1.0f;
            
        case WaveType::Sawtooth:
            return 2.0f * phase - 1.0f;
            
        case WaveType::Triangle:
            return phase < 0.5f ? 4.0f * phase - 1.0f : 3.0f - 4.0f * phase;
            
        case WaveType::Pulse25:
            return phase < 0.25f ? 1.0f : -1.0f;
            
        case WaveType::WhiteNoise:
            return (random.nextFloat() * 2.0f - 1.0f);
            
        case WaveType::PinkNoise:
        {
            float white = random.nextFloat() * 2.0f - 1.0f;
            noiseState = 0.99f * noiseState + 0.01f * white;
            return noiseState;
        }
            
        case WaveType::Supersaw:
        {
            float output = 0.0f;
            for (int i = 0; i < 7; ++i)
            {
                float detune = 1.0f + (i - 3) * 0.01f;
                output += 2.0f * sawPhases[i] - 1.0f;
            }
            return output / 7.0f;
        }
            
        case WaveType::FM:
        {
            float modulator = std::sin(2.0f * juce::MathConstants<float>::pi * fmPhase);
            return std::sin(2.0f * juce::MathConstants<float>::pi * (phase + 0.5f * modulator));
        }
            
        case WaveType::SquareSub:
        {
            float square = phase < 0.5f ? 1.0f : -1.0f;
            float sub = std::sin(2.0f * juce::MathConstants<float>::pi * subPhase);
            return 0.7f * square + 0.3f * sub;
        }
            
        case WaveType::Pulse75:
            return phase < 0.75f ? 1.0f : -1.0f;
            
        case WaveType::CrackleNoise:
        {
            crackleTimer += 1.0f / currentSampleRate;
            if (crackleTimer > 0.01f * (1.0f + random.nextFloat()))
            {
                crackleTimer = 0.0f;
                return (random.nextFloat() * 2.0f - 1.0f) * 2.0f; // Louder bursts
            }
            return 0.0f;
        }
            
        default:
            return 0.0f;
    }
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
            if (glitchCounter > currentSampleRate / 100)
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
                     harmonic * weirdnessAmount * 1.2f;  // Increased harmonic content
            phase2 += (frequency * nw.harmonicMix) / currentSampleRate;
            if (phase2 > 1.0f) phase2 -= 1.0f;
            break;
        }
        
        case WeirdType::Reverser:
        {
            float reverseAmount = std::sin(phase * juce::MathConstants<float>::pi * 16.0f);  // Doubled frequency
            output = baseValue * (1.0f - weirdnessAmount * 1.5f + reverseAmount * weirdnessAmount * 1.5f);
            break;
        }
        
        case WeirdType::BitCrusher:
        {
            float bitDepth = 16.0f - (15.5f * weirdnessAmount);  // More extreme crushing
            float scale = std::pow(2.0f, bitDepth);
            output = std::round(baseValue * scale) / scale;
            break;
        }
        
        case WeirdType::RingMod:
        {
            float ringMod = std::sin(2.0f * juce::MathConstants<float>::pi * phase2);
            output = baseValue * (1.0f - weirdnessAmount + ringMod * weirdnessAmount * 2.0f);  // Doubled intensity
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
                phase = 0.0f;
            }
            float grainEnv = std::sin(grainPhase * juce::MathConstants<float>::pi);
            output = baseValue * (1.0f - weirdnessAmount + grainEnv * weirdnessAmount * 2.0f);  // Doubled effect
            break;
        }
        
        case WeirdType::FilterSweep:
        {
            float cutoff = nw.filterFreq * (1.0f + std::sin(wobblePhase * 2.0f * juce::MathConstants<float>::pi));
            float resonance = 10.0f * weirdnessAmount;  // Doubled from 5.0f to 10.0f
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

float FidgetAudioProcessor::processChaosFilter(float input, FilterType type, float cutoff, float resonance)
{
    // Normalize cutoff to 0-1 range
    float normalizedCutoff = juce::jlimit(0.0f, 1.0f, cutoff / static_cast<float>(currentSampleRate * 0.5));
    float f = normalizedCutoff * 1.16f;
    float fb = resonance + resonance / (1.0f - f);
    
    switch (type)
    {
        case FilterType::LowPass:
        {
            // 4-pole ladder filter
            filterState1 += f * (input - filterState1 + fb * (filterState1 - filterState2));
            filterState2 += f * (filterState1 - filterState2);
            filterState3 += f * (filterState2 - filterState3);
            filterState4 += f * (filterState3 - filterState4);
            return filterState4;
        }
        
        case FilterType::HighPass:
        {
            // High pass using low pass subtraction
            filterState1 += f * (input - filterState1 + fb * (filterState1 - filterState2));
            filterState2 += f * (filterState1 - filterState2);
            return input - filterState2;
        }
        
        case FilterType::BandPass:
        {
            // Band pass
            filterState1 += f * (input - filterState1 + fb * (filterState1 - filterState2));
            filterState2 += f * (filterState1 - filterState2);
            return filterState1 - filterState2;
        }
        
        case FilterType::Notch:
        {
            // Notch (band reject)
            filterState1 += f * (input - filterState1 + fb * (filterState1 - filterState2));
            filterState2 += f * (filterState1 - filterState2);
            return input - (filterState1 - filterState2);
        }
        
        case FilterType::Comb:
        {
            // Comb filter with feedback
            float delaySamples = juce::jlimit(1.0f, 44100.0f, static_cast<float>(currentSampleRate) / cutoff);
            int delayInt = static_cast<int>(delaySamples);
            int readIndex = combIndex - delayInt;
            if (readIndex < 0) readIndex += 44100;
            
            float delayed = readIndex < 44100 ? combDelay[readIndex] : 0.0f;
            float output = input + delayed * resonance;
            if (combIndex < 44100) combDelay[combIndex] = output;
            combIndex = (combIndex + 1) % 44100;
            return output;
        }
        
        case FilterType::FormantA:
        {
            // Formant filter for 'A' vowel (700Hz, 1220Hz, 2600Hz)
            float f1 = 700.0f / static_cast<float>(currentSampleRate) * 2.0f;
            float f2 = 1220.0f / static_cast<float>(currentSampleRate) * 2.0f;
            filterState1 += f1 * (input - filterState1) * 3.0f;
            filterState2 += f2 * (input - filterState2) * 2.0f;
            return (filterState1 + filterState2) * 0.5f;
        }
        
        case FilterType::FormantE:
        {
            // Formant filter for 'E' vowel (660Hz, 1720Hz)
            float f1 = 660.0f / static_cast<float>(currentSampleRate) * 2.0f;
            float f2 = 1720.0f / static_cast<float>(currentSampleRate) * 2.0f;
            filterState1 += f1 * (input - filterState1) * 3.0f;
            filterState2 += f2 * (input - filterState2) * 2.0f;
            return (filterState1 + filterState2) * 0.5f;
        }
        
        case FilterType::FormantI:
        {
            // Formant filter for 'I' vowel (270Hz, 2290Hz)
            float f1 = 270.0f / static_cast<float>(currentSampleRate) * 2.0f;
            float f2 = 2290.0f / static_cast<float>(currentSampleRate) * 2.0f;
            filterState1 += f1 * (input - filterState1) * 3.0f;
            filterState2 += f2 * (input - filterState2) * 2.0f;
            return (filterState1 + filterState2) * 0.5f;
        }
        
        case FilterType::FormantO:
        {
            // Formant filter for 'O' vowel (730Hz, 1090Hz)
            float f1 = 730.0f / static_cast<float>(currentSampleRate) * 2.0f;
            float f2 = 1090.0f / static_cast<float>(currentSampleRate) * 2.0f;
            filterState1 += f1 * (input - filterState1) * 3.0f;
            filterState2 += f2 * (input - filterState2) * 2.0f;
            return (filterState1 + filterState2) * 0.5f;
        }
        
        case FilterType::FormantU:
        {
            // Formant filter for 'U' vowel (300Hz, 870Hz)
            float f1 = 300.0f / static_cast<float>(currentSampleRate) * 2.0f;
            float f2 = 870.0f / static_cast<float>(currentSampleRate) * 2.0f;
            filterState1 += f1 * (input - filterState1) * 3.0f;
            filterState2 += f2 * (input - filterState2) * 2.0f;
            return (filterState1 + filterState2) * 0.5f;
        }
        
        case FilterType::Phaser:
        {
            // 4-stage phaser
            phaserPhase += 0.5f / static_cast<float>(currentSampleRate);
            if (phaserPhase > 1.0f) phaserPhase -= 1.0f;
            
            float lfo = std::sin(phaserPhase * 2.0f * juce::MathConstants<float>::pi);
            float sweepFreq = cutoff * (1.0f + lfo * 0.5f);
            float allpassFreq = sweepFreq / static_cast<float>(currentSampleRate);
            
            float signal = input;
            for (int i = 0; i < 4; ++i)
            {
                float temp = signal;
                signal = phaserStages[i] + signal * allpassFreq;
                phaserStages[i] = temp - signal * allpassFreq;
            }
            
            return input + signal * resonance;
        }
        
        case FilterType::RingModFilter:
        {
            // Ring modulation with filtered carrier
            float carrier = std::sin(2.0f * juce::MathConstants<float>::pi * filterState3);
            filterState3 += cutoff / static_cast<float>(currentSampleRate);
            if (filterState3 > 1.0f) filterState3 -= 1.0f;
            
            float ringMod = input * carrier;
            filterState1 += f * (ringMod - filterState1);
            return filterState1;
        }
        
        default:
            return input;
    }
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
            subPhase = 0.0f;
            fmPhase = 0.0f;
            wobblePhase = 0.0f;
            grainPhase = 0.0f;
            glitchCounter = 0;
            filterState = 0.0f;
            
            // Reset supersaw phases with slight detuning
            for (int i = 0; i < 7; ++i)
            {
                sawPhases[i] = 0.0f;
            }
            
            // Reset filter states
            filterState1 = 0.0f;
            filterState2 = 0.0f;
            filterState3 = 0.0f;
            filterState4 = 0.0f;
            phaserPhase = 0.0f;
            for (int i = 0; i < 4; ++i)
            {
                phaserStages[i] = 0.0f;
            }
        }
        else if (message.isNoteOff())
        {
            if (message.getNoteNumber() == currentNote)
            {
                noteOn = false;
            }
        }
    }
    
    // Get current weirdness value and convert to knob position (0-127)
    float weirdnessValue = *weirdnessParam;
    int knobPosition = static_cast<int>(weirdnessValue * 127.0f);
    
    // Get the random amount for this knob position
    float randomWeirdnessAmount = 0.0f;
    float randomCutoff = 1000.0f;
    float randomResonance = 0.0f;
    if (currentNote >= 0 && currentNote < 128)
    {
        randomWeirdnessAmount = noteWeirdness[currentNote].randomAmounts[knobPosition];
        randomCutoff = noteWeirdness[currentNote].randomCutoffs[knobPosition];
        randomResonance = noteWeirdness[currentNote].randomResonances[knobPosition];
    }
    
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
    const float subPhaseIncrement = (frequency * 0.5f) / currentSampleRate; // Sub osc at half frequency
    const float fmPhaseIncrement = (frequency * 2.0f) / currentSampleRate; // FM at double frequency
    
    // Get wave type and filter type for current note
    WaveType waveType = currentNote >= 0 ? noteWeirdness[currentNote].waveType : WaveType::Sine;
    FilterType filterType = currentNote >= 0 ? noteWeirdness[currentNote].filterType : FilterType::LowPass;
    
    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            // Update envelope
            envelope = juce::jlimit(0.0f, 1.0f, envelope + envelopeIncrement);
            
            // Generate oscillator based on wave type
            float waveform = generateOscillator(waveType, phase, frequency);
            
            // Apply weird processing with random amount
            float weirdWave = processWeirdOscillator(waveform, currentNote, randomWeirdnessAmount);
            
            // Apply chaos filter
            float filtered = processChaosFilter(weirdWave, filterType, randomCutoff, randomResonance);
            
            // Output with envelope and velocity
            channelData[sample] = amplitude * envelope * velocity * filtered;
            
            // Update phases
            phase += phaseIncrement;
            if (phase > 1.0f) phase -= 1.0f;
            
            subPhase += subPhaseIncrement;
            if (subPhase > 1.0f) subPhase -= 1.0f;
            
            fmPhase += fmPhaseIncrement;
            if (fmPhase > 1.0f) fmPhase -= 1.0f;
            
            // Update supersaw phases
            for (int i = 0; i < 7; ++i)
            {
                float detune = 1.0f + (i - 3) * 0.01f;
                sawPhases[i] += (phaseIncrement * detune);
                if (sawPhases[i] > 1.0f) sawPhases[i] -= 1.0f;
            }
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