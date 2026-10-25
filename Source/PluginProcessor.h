#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "engine/FormantSinger.h"

class FormantDaisyProcessor : public juce::AudioProcessor,
                              private juce::AudioProcessorValueTreeState::Listener,
                              private juce::AsyncUpdater
{
public:
    FormantDaisyProcessor();
    ~FormantDaisyProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- lyrics (message thread) --------------------------------------------
    void setLyricsText (const juce::String& text);
    juce::String getLyricsText() const;
    std::shared_ptr<const fd::Lyrics> getLyricsForUI() const;
    void requestRestart() { restartRequested.store (true); }
    int getLyricsVersion() const { return lyricsVersion.load(); }

    static juce::String defaultLyrics();

    // --- visualisation --------------------------------------------------------
    static constexpr int vizSize = 8192;
    void copyRecentAudio (float* dest, int numSamples) const; // newest last
    fd::FormantSinger& getSinger() { return singer; }
    double getSampleRateForUI() const { return currentSampleRate.load(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged (const juce::String& id, float newValue) override;
    void handleAsyncUpdate() override;
    void handleTempoMode (juce::MidiBuffer& midi, int numSamples);
    void updateClock (int numSamples);
    int64_t barAt (int sampleOffset) const; // which bar a sample in this block falls in

public:
    // bar/beat display (read by the editor)
    std::atomic<int> uiBar { 1 }, uiBeat { 0 }, uiBeatsPerBar { 4 }, uiBeatUnit { 4 };
    std::atomic<bool> uiPlaying { false };
    std::atomic<bool> uiCarrierActive { false }; // the audio input is shaping the sound
    std::atomic<float> uiInputLevel { 0.0f };

private:

    fd::FormantSinger singer;

    // lyrics hand-over: message thread -> audio thread
    juce::SpinLock lyricsLock;
    std::shared_ptr<const fd::Lyrics> pendingLyrics;
    std::shared_ptr<const fd::Lyrics> uiLyrics; // message-thread copy
    juce::String lyricsText;
    std::atomic<int> lyricsVersion { 0 };

    std::atomic<bool> restartRequested { false };
    std::atomic<double> currentSampleRate { 48000.0 };

    // tempo mode
    std::array<int, 16> tempoHeld {};
    int tempoHeldCount = 0;
    float tempoVelocity = 0.8f;
    double freePpq = 0.0;
    // musical clock for the current block
    double clkPpqStart = 0.0, clkPpqPerSample = 0.0, clkQpb = 4.0, clkOrigin = 0.0;
    int64_t clkBarBase = 0;
    int clkNum = 4, clkDen = 4;
    bool wasPlaying = false;
    bool lastRestartParam = false;

    // viz ring buffer
    std::vector<float> vizBuffer;
    std::atomic<int> vizWrite { 0 };
    std::vector<float> mono;

    // cached raw parameter pointers
    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pDivision = nullptr;
    std::atomic<float>* pGate = nullptr;
    std::atomic<float>* pGlide = nullptr;
    std::atomic<float>* pVibDepth = nullptr;
    std::atomic<float>* pVibRate = nullptr;
    std::atomic<float>* pSource = nullptr;
    std::atomic<float>* pBreath = nullptr;
    std::atomic<float>* pFormant = nullptr;
    std::atomic<float>* pSpeed = nullptr;
    std::atomic<float>* pLofi = nullptr;
    std::atomic<float>* pLead = nullptr;
    std::atomic<float>* pVolume = nullptr;
    std::atomic<float>* pLoop = nullptr;
    std::atomic<float>* pRestartOnPlay = nullptr;
    std::atomic<float>* pRestart = nullptr;
    std::atomic<float>* pSpell = nullptr;
    std::atomic<float>* pCarrier = nullptr;
    std::vector<float> carrierBuf;
    int inputHoldSamples = 0;
    bool lastSpell = true; // message thread

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FormantDaisyProcessor)
};
