# Fidget - Experimental VST Synthesizer

Fidget is a weird and experimental VST/AU synthesizer where each MIDI note has its own unique "personality" that morphs with a single Weirdness knob.

## Features

- **8 Different Weirdness Types** - Each note is permanently assigned one of these behaviors:
  - **Wobbler** - Frequency wobbles with an LFO
  - **Glitcher** - Random glitches and dropouts
  - **Harmonizer** - Adds strange harmonics
  - **Reverser** - Phase reversals
  - **BitCrusher** - Lo-fi bit reduction
  - **RingMod** - Ring modulation effects
  - **Granular** - Micro stutters
  - **FilterSweep** - Resonant filter sweeps

- **Single Weirdness Knob** - Controls the intensity of each note's unique effect
- **Deterministic Behavior** - Each note always has the same weird behavior
- **Visual Feedback** - UI shows which type of weirdness is active with color coding

## Building

### Prerequisites
- macOS (tested on macOS 14)
- CMake 3.16 or higher
- Xcode Command Line Tools

### Build Instructions

1. Clone the repository with submodules:
```bash
git clone --recursive https://github.com/WaxedTangent/fidget.git
cd fidget
```

2. Build the project:
```bash
cmake -S . -B build
cmake --build build
```

The plugin will be automatically installed to:
- VST3: `~/Library/Audio/Plug-Ins/VST3/Fidget.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/Fidget.component`
- Standalone: `build/Fidget_artefacts/Standalone/Fidget.app`

## Usage

1. Load Fidget in your DAW as a VST3 or AU plugin
2. Play different MIDI notes to discover their unique behaviors
3. Turn the Weirdness knob to morph between normal and weird sounds
4. Each note's behavior is consistent - the same note always produces the same type of weirdness

## Technical Details

Built with:
- [JUCE](https://juce.com/) framework
- C++17
- CMake build system

## License

This project is open source. Feel free to use, modify, and distribute as you see fit.

## Author

Created by Luke (WaxedTangent)