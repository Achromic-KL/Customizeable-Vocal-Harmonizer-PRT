Vocal Harmonizer

A real-time vocal harmonizer built as a native Windows app with JUCE. Sing or hum into your mic and it detects your pitch, calculates a harmony interval, and pitch-shifts your voice live to produce a second harmonized voice mixed in with your own.

What it does
Captures microphone input and analyzes it in real time using the YIN pitch detection algorithm
Lets you pick a harmony interval (minor third, major third, fourth, fifth, octave, or below) from a dropdown
Pitch-shifts your voice to that interval live using the Rubber Band Library
Mixes your natural voice with the harmonized voice and plays both back together
Includes a built-in audio device picker for tuning latency (buffer size, ASIO drivers, etc.)

Prerequisites:

You'll need these installed before building:

Visual Studio 2022 or 2026 (Community) — free. During install, select the "Desktop development with C++" workload. This is required; without it, CMake can't find a usable compiler.
CMake 3.22+ — from cmake.org. Check "Add CMake to PATH" during install.
Git — from git-scm.com. Also used by CMake to auto-download JUCE.
vcpkg — Microsoft's C++ package manager, used to install the Rubber Band library (see below).
Setting up Rubber Band (required dependency)

Rubber Band doesn't ship prebuilt binaries or official CMake support, so it's installed via vcpkg. Paste this into your powershell:
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install rubberband:x64-windows

This takes a few minutes, it's compiling the library. Use the x64-windows triplet specifically, the static triplet (x64-windows-static) isn't supported by this port.

This repo isn't available to use commercially yet, the app won't work on your machine.
