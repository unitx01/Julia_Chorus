# Caesar Chorus VST

A plugin model of the PedalPCB Caesar Chorus (BBD analog chorus/vibrato).
Controls: **Rate**, **Depth**, **Lag**, **Shape** (Triangle/Sine), **Blend**.

Builds VST3 on Windows/Mac/Linux, AU on Mac, and a Standalone app.

## What you need

- **CMake** 3.22 or newer: https://cmake.org/download/
- **Git**: https://git-scm.com/
- A C++ compiler:
  - **Windows:** Visual Studio 2022 (Community is free), with the *Desktop development with C++* workload
  - **Mac:** Xcode (from the App Store), then run `xcode-select --install`

JUCE is downloaded automatically during the first build.

## Build

Open a terminal (on Windows, the *Developer Command Prompt for VS 2022*) in this folder:

```
cmake -B build
cmake --build build --config Release
```

The first run takes a while because it downloads and compiles JUCE.

## Install

The plugin ends up in `build/CaesarChorus_artefacts/Release/`.

Copy the `Caesar Chorus.vst3` folder to:

- **Windows:** `C:\Program Files\Common Files\VST3\`
- **Mac:** `~/Library/Audio/Plug-Ins/VST3/`
  (and on Mac, the `.component` from the `AU` folder to `~/Library/Audio/Plug-Ins/Components/`)

Then rescan plugins in your DAW.

## Controls

| Knob  | What it does |
|-------|--------------|
| Rate  | LFO speed, 0.05–10 Hz |
| Depth | How far the delay time sweeps |
| Lag   | Base delay time (about 1.5–12 ms) – short = shimmery, long = more doubling |
| Shape | Triangle, or a rounded "sine" (the pedal rounds the triangle with LEDs) |
| Blend | Dry → wet. Middle = chorus, fully clockwise = vibrato |

## Tweaking the sound

All the voicing constants are at the top of the `ChorusEngine` class in
`Source/ChorusDSP.h` (delay range, filter cutoffs, modulation range, saturation).
They're approximations of the circuit, so adjust them by ear against the real pedal.

## Notes

This is a model, not a component-level simulation: the filters, LFO, and BBD
delay are approximated from the schematic. It doesn't reproduce BBD hiss or
clock noise. Based on the PedalPCB Caesar Chorus documentation; for personal use.
