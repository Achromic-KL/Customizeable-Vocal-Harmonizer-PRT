# Vocal Harmonizer

A real-time vocal harmonizer built as a native Windows app with [JUCE](https://juce.com/). Sing or hum into your mic and it detects your pitch, calculates a harmony interval, and pitch-shifts your voice live to produce a second harmonized voice mixed in with your own.

<img width="1904" height="990" alt="untitled" src="https://github.com/user-attachments/assets/534c6d6e-5f9d-4ea7-bc13-171ea26ba1b6" />


## What it does

- Captures microphone input and analyzes it in real time using the **YIN pitch detection algorithm**
- Lets you pick a harmony interval (minor third, major third, fourth, fifth, octave, or below) from a dropdown
- Pitch-shifts your voice to that interval live using the [Rubber Band Library](https://breakfastquay.com/rubberband/)
- Mixes your natural voice with the harmonized voice and plays both back together
- Includes a built-in audio device picker for tuning latency (buffer size, ASIO drivers, etc.)

## Prerequisites

You'll need these installed before building:

1. **Visual Studio 2022 or 2026 (Community)** : free. During install, select the **"Desktop development with C++"** workload. This is required; without it, CMake can't find a usable compiler.
2. **CMake** 3.22+ : from [cmake.org](https://cmake.org/download/). Check "Add CMake to PATH" during install.
3. **Git** : from [git-scm.com](https://git-scm.com/). Also used by CMake to auto-download JUCE.
4. **vcpkg** : Microsoft's C++ package manager, used to install the Rubber Band library (see below).

## Setting up Rubber Band (required dependency)

Rubber Band doesn't ship prebuilt binaries or official CMake support, so it's installed via vcpkg:

```powershell
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install rubberband:x64-windows
```

This takes a few minutes it's compiling the library. Use the `x64-windows` triplet specifically.

`CMakeLists.txt` needs to know where vcpkg installed Rubber Band. It resolves this automatically from a `VCPKG_ROOT` environment variable — set one (once) pointing at wherever you cloned vcpkg:

```powershell
setx VCPKG_ROOT "C:\path\to\vcpkg"
```

Open a **new** terminal window after running that (environment variable changes don't apply to already-open terminals), then proceed to the build step below.

If you'd rather not set an environment variable, you can instead pass the path directly at configure time:

```powershell
cmake -B build -G "Visual Studio 18 2026" -A x64 -DRUBBERBAND_ROOT="C:/path/to/vcpkg/installed/x64-windows"
```

If neither is set, CMake will fail with a clear error explaining what to do rather than a confusing linker error later.

## Building

From the project root:

```powershell
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

(If you're on Visual Studio 2022 instead of 2026, use `-G "Visual Studio 17 2022"`.)

First configure will take a few minutes, it downloads the full JUCE framework source via `FetchContent`. Rebuilds after that are much faster.

The executable, along with the Rubber Band runtime DLLs it needs (copied automatically by the build), lands at:

```
build\VocalHarmonizer_artefacts\Release\Vocal Harmonizer.exe
```

Run it directly, or open `build\VocalHarmonizer.sln` in Visual Studio for debugging.

## Using it

1. Launch the app and allow microphone access if prompted.
2. Use headphones, the app plays your harmonized voice back live, and speakers will cause feedback into the mic.
3. Sing or hum a sustained note. The detected pitch and note name appear on screen.
4. Pick a harmony interval from the dropdown — the target harmony note updates live.
5. If the delay feels too long during live use, click **Audio Settings...** in the app and lower the buffer size, or switch to an ASIO driver if you have one (e.g. via the free [ASIO4ALL](https://asio4all.org/)).

## Routing into Discord / other voice apps

The app outputs to a normal playback device by default, which other apps (like Discord) can't pick up as a microphone input. To route it:

1. Install [VB-Audio Virtual Cable](https://vb-audio.com/Cable/) (free).
2. In the app's Audio Settings, set the output device to **CABLE Input**.
3. In Discord (or whatever app you're using), set the input/microphone device to **CABLE Output**.
4. To still hear yourself while performing, enable **"Listen to this device"** on CABLE Output in Windows' Sound settings (Recording tab → CABLE Output → Properties → Listen), routed to your headphones.

## Project structure

```
VocalHarmonizer/
├── CMakeLists.txt          # Build config; fetches JUCE, links Rubber Band
├── Source/
│   ├── Main.cpp             # App entry point / window setup
│   ├── MainComponent.h/.cpp # Audio I/O, pitch detection, harmony logic, GUI
│   └── PitchDetector.h       # YIN pitch detection implementation
```

## Known limitations / roadmap

- Dry/harmony mix levels are currently fixed in code, not adjustable from the GUI
- Only one harmony voice at a time (no multi-voice stacking yet)
- No preset saving/loading
- Harmony interval is fixed-semitone, not key/scale-aware (so it can sound "off" against certain notes depending on the key you're singing in)

## License note

Rubber Band Library is GPL-licensed. It's used here as an external dependency (installed via vcpkg, not bundled in this repo), so this doesn't affect the license of this project's own source code. Worth reading [Rubber Band's licensing page](https://breakfastquay.com/rubberband/license.html) if you want to use it commercially.
