#include "MainComponent.h"

using namespace RubberBand;

MainComponent::MainComponent()
{
    setAudioChannels (1, 2);

    ringBuffer.resize (analysisSize, 0.0f);

    statusLabel.setText ("Listening... sing or hum into your mic.", juce::dontSendNotification);
    statusLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (statusLabel);

    pitchLabel.setText ("-- Hz", juce::dontSendNotification);
    pitchLabel.setJustificationType (juce::Justification::centred);
    pitchLabel.setFont (juce::Font (24.0f, juce::Font::bold));
    addAndMakeVisible (pitchLabel);

    noteLabel.setText ("--", juce::dontSendNotification);
    noteLabel.setJustificationType (juce::Justification::centred);
    noteLabel.setFont (juce::Font (40.0f, juce::Font::bold));
    addAndMakeVisible (noteLabel);

    intervalCaption.setText ("Harmony interval:", juce::dontSendNotification);
    intervalCaption.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (intervalCaption);

    intervalSelector.addItem ("Minor Third (+3)", 103);
    intervalSelector.addItem ("Major Third (+4)", 104);
    intervalSelector.addItem ("Perfect Fourth (+5)", 105);
    intervalSelector.addItem ("Perfect Fifth (+7)", 107);
    intervalSelector.addItem ("Octave (+12)", 112);
    intervalSelector.addItem ("Third Below (-8)", 92);
    intervalSelector.addItem ("Fifth Below (-5)", 95);
    intervalSelector.setSelectedId (104, juce::dontSendNotification);

    intervalSelector.onChange = [this]
    {
        harmonySemitones.store (intervalSelector.getSelectedId() - 100);
    };
    addAndMakeVisible (intervalSelector);

    harmonyCaption.setText ("Harmony target:", juce::dontSendNotification);
    harmonyCaption.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (harmonyCaption);

    harmonyNoteLabel.setText ("--", juce::dontSendNotification);
    harmonyNoteLabel.setJustificationType (juce::Justification::centred);
    harmonyNoteLabel.setFont (juce::Font (40.0f, juce::Font::bold));
    harmonyNoteLabel.setColour (juce::Label::textColourId, juce::Colours::lightblue);
    addAndMakeVisible (harmonyNoteLabel);

    harmonyFreqLabel.setText ("-- Hz", juce::dontSendNotification);
    harmonyFreqLabel.setJustificationType (juce::Justification::centred);
    harmonyFreqLabel.setFont (juce::Font (24.0f, juce::Font::bold));
    harmonyFreqLabel.setColour (juce::Label::textColourId, juce::Colours::lightblue);
    addAndMakeVisible (harmonyFreqLabel);

    audioSettingsButton.onClick = [this]
    {
        auto* selector = new juce::AudioDeviceSelectorComponent (
            deviceManager,
            1, 1,   // min/max input channels (mono mic)
            2, 2,   // min/max output channels (stereo out)
            false,  // no MIDI input needed
            false,  // no MIDI output needed
            true,   // show channels as stereo pairs
            false); // show all options directly, nothing hidden behind a button

        selector->setSize (500, 450);

        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned (selector);
        options.dialogTitle = "Audio Settings";
        options.dialogBackgroundColour = getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId);
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = true;
        options.launchAsync();
    };
    addAndMakeVisible (audioSettingsButton);

    setSize (500, 460);

    startTimerHz (20);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    pitchDetector = std::make_unique<PitchDetector> (sampleRate, analysisSize);

    std::fill (ringBuffer.begin(), ringBuffer.end(), 0.0f);
    ringWritePos = 0;
    samplesSinceLastAnalysis = 0;

    // OptionProcessRealTime: tells Rubber Band we're feeding it a live
    // stream rather than a whole file, so it can start producing output
    // before it's seen the entire signal.
    // OptionPitchHighConsistency: keeps timbre/quality stable when the
    // pitch scale changes mid-stream (exactly what happens every time you
    // switch the harmony interval dropdown while singing).
    // OptionEngineFaster: the older, lighter-weight R2 engine - noticeably
    // lower latency than the default R3 "Finer" engine, at some cost to
    // shift quality. Worth it for a live performance tool.
    // OptionWindowShort: smaller analysis window = less buffering before
    // the first output sample = lower algorithmic latency. Trade-off is
    // slightly less smooth results on sustained notes, but for live use
    // this matters more than studio-grade smoothness.
    auto options = RubberBandStretcher::OptionProcessRealTime
                  | RubberBandStretcher::OptionPitchHighConsistency
                  | RubberBandStretcher::OptionEngineFaster
                  | RubberBandStretcher::OptionWindowShort;

    harmonizer = std::make_unique<RubberBandStretcher> ((size_t) sampleRate, 1, options);
    harmonizer->setMaxProcessSize ((size_t) samplesPerBlockExpected);
    harmonizer->setPitchScale (semitonesToRatio (harmonySemitones.load()));

    harmonyFifo.clear();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* buffer = bufferToFill.buffer;
    const int numSamples = bufferToFill.numSamples;
    const float* inputData = buffer->getReadPointer (0);

    // --- Pitch detection (unchanged from Stage 1/2) ---
    for (int i = 0; i < numSamples; ++i)
    {
        ringBuffer[(size_t) ringWritePos] = inputData[i];
        ringWritePos = (ringWritePos + 1) % analysisSize;
        ++samplesSinceLastAnalysis;
    }

    if (samplesSinceLastAnalysis >= hopSize && pitchDetector != nullptr)
    {
        samplesSinceLastAnalysis = 0;

        std::vector<float> linear (analysisSize);
        for (int i = 0; i < analysisSize; ++i)
            linear[(size_t) i] = ringBuffer[(size_t) ((ringWritePos + i) % analysisSize)];

        float freq = pitchDetector->detectPitch (linear.data());
        latestFrequency.store (freq);
    }

    // --- Stage 3: pitch-shift the dry signal into the harmony voice ---
    std::vector<float> harmonyBlock (static_cast<size_t> (numSamples), 0.0f);

    if (harmonizer != nullptr)
    {
        harmonizer->setPitchScale (semitonesToRatio (harmonySemitones.load()));

        const float* inPtrs[1] = { inputData };
        harmonizer->process (inPtrs, (size_t) numSamples, false);

        int available = harmonizer->available();
        if (available > 0)
        {
            std::vector<float> retrieved ((size_t) available);
            float* outPtrs[1] = { retrieved.data() };
            int retrievedCount = static_cast<int> (harmonizer->retrieve (outPtrs, available));

            for (int i = 0; i < retrievedCount; ++i)
                harmonyFifo.push_back (retrieved[(size_t) i]);
        }

        // Pop exactly one block's worth out of the FIFO. Early on (first
        // ~20-50ms depending on window size) the FIFO won't have enough
        // samples yet - that startup latency is normal for phase-vocoder
        // style pitch shifting, not a bug. We pad with silence until it
        // catches up.
        for (int i = 0; i < numSamples; ++i)
        {
            if (! harmonyFifo.empty())
            {
                harmonyBlock[(size_t) i] = harmonyFifo.front();
                harmonyFifo.pop_front();
            }
        }
    }

    // --- Mix dry + harmony into every output channel ---
    for (int ch = 0; ch < buffer->getNumChannels(); ++ch)
    {
        auto* out = buffer->getWritePointer (ch, bufferToFill.startSample);
        for (int i = 0; i < numSamples; ++i)
            out[i] = (inputData[i] * dryGain) + (harmonyBlock[(size_t) i] * harmonyGain);
    }
}

void MainComponent::releaseResources()
{
    pitchDetector = nullptr;
    harmonizer = nullptr;
    harmonyFifo.clear();
}

void MainComponent::timerCallback()
{
    float freq = latestFrequency.load();

    if (freq > 0.0f)
    {
        pitchLabel.setText (juce::String (freq, 1) + " Hz", juce::dontSendNotification);
        noteLabel.setText (frequencyToNoteName (freq), juce::dontSendNotification);

        float harmonyFreq = freq * semitonesToRatio (harmonySemitones.load());

        harmonyFreqLabel.setText (juce::String (harmonyFreq, 1) + " Hz", juce::dontSendNotification);
        harmonyNoteLabel.setText (frequencyToNoteName (harmonyFreq), juce::dontSendNotification);
    }
    else
    {
        pitchLabel.setText ("-- Hz", juce::dontSendNotification);
        noteLabel.setText ("--", juce::dontSendNotification);
        harmonyFreqLabel.setText ("-- Hz", juce::dontSendNotification);
        harmonyNoteLabel.setText ("--", juce::dontSendNotification);
    }
}

float MainComponent::semitonesToRatio (int semitones)
{
    return std::pow (2.0f, (float) semitones / 12.0f);
}

juce::String MainComponent::frequencyToNoteName (float freqHz)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    float midiFloat = 69.0f + 12.0f * std::log2 (freqHz / 440.0f);
    int midiNote = (int) std::round (midiFloat);

    int octave = (midiNote / 12) - 1;
    int noteIndex = ((midiNote % 12) + 12) % 12;

    return juce::String (names[noteIndex]) + juce::String (octave);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced (20);

    statusLabel.setBounds (area.removeFromTop (30));
    area.removeFromTop (10);

    noteLabel.setBounds (area.removeFromTop (60));
    pitchLabel.setBounds (area.removeFromTop (35));

    area.removeFromTop (15);

    auto intervalRow = area.removeFromTop (30);
    intervalCaption.setBounds (intervalRow.removeFromLeft (150));
    intervalSelector.setBounds (intervalRow);

    area.removeFromTop (20);

    harmonyCaption.setBounds (area.removeFromTop (25));
    harmonyNoteLabel.setBounds (area.removeFromTop (60));
    harmonyFreqLabel.setBounds (area.removeFromTop (35));

    area.removeFromTop (15);
    audioSettingsButton.setBounds (area.removeFromTop (30).reduced (100, 0));
}
