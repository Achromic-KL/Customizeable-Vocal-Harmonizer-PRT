#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <rubberband/RubberBandStretcher.h>
#include <atomic>
#include <deque>
#include "PitchDetector.h"

class MainComponent : public juce::AudioAppComponent,
                       private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    static juce::String frequencyToNoteName (float freqHz);
    static float semitonesToRatio (int semitones);

    static constexpr int analysisSize = 2048;
    static constexpr int hopSize = 512;

    std::unique_ptr<PitchDetector> pitchDetector;
    double currentSampleRate = 44100.0;

    std::vector<float> ringBuffer;
    int ringWritePos = 0;
    int samplesSinceLastAnalysis = 0;

    std::atomic<float> latestFrequency { -1.0f };

    // Written on the message thread (combo box callback), read on the
    // audio thread (every block, to drive the stretcher's pitch scale).
    // That cross-thread read is new as of Stage 3, hence atomic now.
    std::atomic<int> harmonySemitones { 4 }; // default: major third up

    // --- Stage 3: Rubber Band real-time pitch shifting ---
    std::unique_ptr<RubberBand::RubberBandStretcher> harmonizer;

    // Rubber Band doesn't hand back exactly one block's worth of samples
    // per block fed in (it buffers internally for its windowed analysis),
    // so we push whatever it gives us into this FIFO and pop fixed-size
    // chunks back out to match JUCE's audio callback size.
    std::deque<float> harmonyFifo;

    // Mix levels. Fixed for now - Stage 4 adds proper level controls.
    static constexpr float dryGain = 0.8f;
    static constexpr float harmonyGain = 0.6f;

    juce::Label statusLabel;
    juce::Label pitchLabel;
    juce::Label noteLabel;

    juce::Label intervalCaption;
    juce::ComboBox intervalSelector;

    juce::Label harmonyCaption;
    juce::Label harmonyNoteLabel;
    juce::Label harmonyFreqLabel;

    // Stage 6 (pulled forward): lets you pick a lower-latency audio driver
    // (e.g. ASIO, if installed) and buffer size without recompiling.
    juce::TextButton audioSettingsButton { "Audio Settings..." };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
