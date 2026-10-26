# Side Fregna

Side Fregna is a JUCE VST3/Standalone sidechain ducking effect. The project is in active development and is not yet a complete implementation of the supplied product brief.

## Included now

- Separate stereo bass and kick input buses.
- Host-tempo cycle selector (1/16 through one bar), depth, click control, and a 10 ms DAW-compensated lookahead that places the fade before the kick.
- FULL, SUB, and BAND routing modes.
- Thirteen starting curve shapes, a draggable 33-point curve, and saved curve data in plugin state.
- A 10 ms DAW-compensated lookahead for the pre-kick click fade.
- Manual low-band phase rotation and a kick-gated narrow dynamic bell in SPACE.
- WAV/AIFF kick sample loading by button or drag-and-drop; the sample is saved with the plug-in state and can drive the detector when no routed kick is present.
- Three tabbed pages and a dark interface based on the supplied references.
- Windows GitHub Actions workflow that builds and packages a VST3 artifact.

## Still needed for the full brief

The current DSP/editor are a prototype. The graph currently edits a fixed 33-point table; step/segment dragging, handles, free point insertion/removal, preset management/order, and undo are not implemented. The loaded kick is not yet shown as a waveform, and exact host-grid phase following and full last-kick render matching still need work. SUB/BAND use simple non-linear-phase filters, BAND does not yet expose two draggable crossover handles, PHASE is manual and lacks automatic turn detection, and SPACE currently has one bell rather than an adaptive bank. These features need implementation and DAW validation before this should be called release-ready.

## Build on Windows

Install CMake 3.22+ and Visual Studio 2022 with the Desktop development with C++ workload. The first configure fetches JUCE 8.0.8.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target SideFregna_VST3
```

The plugin bundle is under `build/SideFregna_artefacts/Release/VST3`. GitHub Actions builds the same artifact on pushes, pull requests, and manual runs.

## Install

Download and extract the `Side-Fregna-VST3-Windows` artifact from a successful GitHub Actions run, then run `Install.ps1` from the extracted folder. It installs to the per-user VST3 folder under `%LOCALAPPDATA%`; rescan VST3 plug-ins in the DAW afterward.

## License

Project source: MIT. JUCE is fetched separately and remains under its own license terms.
