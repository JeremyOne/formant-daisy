#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ids
{
static const juce::String mode = "mode", division = "division", gate = "gate", glide = "glide",
                          vibDepth = "vibDepth", vibRate = "vibRate", source = "source", breath = "breath",
                          formant = "formant", speed = "speed", lofi = "lofi", lead = "lead", volume = "volume",
                          loop = "loop", restartOnPlay = "restartOnPlay", restart = "restart",
                          spellAcronyms = "spellAcronyms", carrier = "carrier";
}

juce::String FormantDaisyProcessor::defaultLyrics()
{
    return "Daisy Daisy give me your answer do\n"
           "I'm half crazy all for the love of you\n"
           "It won't be a stylish marriage\n"
           "I can't afford a carriage\n"
           "But you'll look sweet upon the seat\n"
           "Of a bicycle built for two";
}

juce::AudioProcessorValueTreeState::ParameterLayout FormantDaisyProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto pct = [] (float v, int) { return String (juce::roundToInt (v * 100.0f)) + "%"; };
    auto whole = [] (float v, int) { return String (juce::roundToInt (v)); };
    auto oneDp = [] (float v, int) { return String (v, 1); };

    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::mode, 1 }, "Mode", StringArray { "Notes", "Tempo" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::division, 1 }, "Division",
                                                   StringArray { "1/4", "1/8", "1/8T", "1/16" }, 1));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::gate, 1 }, "Gate", NormalisableRange<float> (0.1f, 1.0f), 0.8f,
                                                  AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::glide, 1 }, "Glide", NormalisableRange<float> (0.0f, 400.0f, 0.0f, 0.5f), 60.0f,
                                                  AudioParameterFloatAttributes().withLabel ("ms").withStringFromValueFunction (whole)));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::vibDepth, 1 }, "Vibrato", NormalisableRange<float> (0.0f, 100.0f), 25.0f,
                                                  AudioParameterFloatAttributes().withLabel ("ct").withStringFromValueFunction (whole)));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::vibRate, 1 }, "Vib Rate", NormalisableRange<float> (2.0f, 9.0f), 5.5f,
                                                  AudioParameterFloatAttributes().withLabel ("Hz").withStringFromValueFunction (oneDp)));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::source, 1 }, "Voice (Buzz-Glottal)", NormalisableRange<float> (0.0f, 1.0f), 0.5f,
                                                  AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::breath, 1 }, "Breath", NormalisableRange<float> (0.0f, 1.0f), 0.15f,
                                                  AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::formant, 1 }, "Formant", NormalisableRange<float> (0.7f, 1.5f), 1.0f,
                                                  AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return "x" + String (v, 2); })));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::speed, 1 }, "Consonant Length", NormalisableRange<float> (0.5f, 2.0f), 1.0f,
                                                  AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return "x" + String (v, 2); })));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::lofi, 1 }, "Lo-Fi", NormalisableRange<float> (0.0f, 1.0f), 0.0f,
                                                  AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::lead, 1 }, "Consonant Lead", NormalisableRange<float> (0.0f, 150.0f, 1.0f), 60.0f,
                                                  AudioParameterFloatAttributes().withLabel ("ms").withStringFromValueFunction (whole)));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ids::volume, 1 }, "Volume", NormalisableRange<float> (-36.0f, 6.0f), -6.0f,
                                                  AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (oneDp)));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { ids::loop, 1 }, "Loop Lyrics", true));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { ids::restartOnPlay, 1 }, "Restart On Play", true));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { ids::spellAcronyms, 1 }, "Spell Acronyms", true));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ids::carrier, 1 }, "Carrier",
                                                   StringArray { "Auto", "Buzz", "Input" }, 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { ids::restart, 1 }, "Restart Lyrics", false));
    return l;
}

FormantDaisyProcessor::FormantDaisyProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          // optional audio inputs for vocoder-style use; both off by default.
                          // Hosts like Ableton show the second one as the plugin's Sidechain.
                          .withInput ("Input", juce::AudioChannelSet::stereo(), false)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pMode = apvts.getRawParameterValue (ids::mode);
    pDivision = apvts.getRawParameterValue (ids::division);
    pGate = apvts.getRawParameterValue (ids::gate);
    pGlide = apvts.getRawParameterValue (ids::glide);
    pVibDepth = apvts.getRawParameterValue (ids::vibDepth);
    pVibRate = apvts.getRawParameterValue (ids::vibRate);
    pSource = apvts.getRawParameterValue (ids::source);
    pBreath = apvts.getRawParameterValue (ids::breath);
    pFormant = apvts.getRawParameterValue (ids::formant);
    pSpeed = apvts.getRawParameterValue (ids::speed);
    pLofi = apvts.getRawParameterValue (ids::lofi);
    pLead = apvts.getRawParameterValue (ids::lead);
    pVolume = apvts.getRawParameterValue (ids::volume);
    pLoop = apvts.getRawParameterValue (ids::loop);
    pRestartOnPlay = apvts.getRawParameterValue (ids::restartOnPlay);
    pRestart = apvts.getRawParameterValue (ids::restart);
    pSpell = apvts.getRawParameterValue (ids::spellAcronyms);
    pCarrier = apvts.getRawParameterValue (ids::carrier);
    lastSpell = pSpell->load() > 0.5f;

    apvts.addParameterListener (ids::lead, this);
    apvts.addParameterListener (ids::mode, this);
    apvts.addParameterListener (ids::spellAcronyms, this);

    vizBuffer.assign ((size_t) vizSize, 0.0f);
    setLyricsText (defaultLyrics());
}

FormantDaisyProcessor::~FormantDaisyProcessor()
{
    apvts.removeParameterListener (ids::lead, this);
    apvts.removeParameterListener (ids::mode, this);
    apvts.removeParameterListener (ids::spellAcronyms, this);
    cancelPendingUpdate();
}

bool FormantDaisyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    for (const auto& in : layouts.inputBuses)
        if (! in.isDisabled() && in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo())
            return false;
    return true;
}

void FormantDaisyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate.store (sampleRate);
    singer.prepare (sampleRate);
    mono.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
    carrierBuf.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
    inputHoldSamples = 0;
    {
        const juce::SpinLock::ScopedLockType sl (lyricsLock);
        if (uiLyrics) singer.setLyrics (uiLyrics);
        pendingLyrics.reset();
    }
    setLatencySamples (juce::roundToInt (pLead->load() * 0.001 * sampleRate));
    tempoHeldCount = 0;
    freePpq = 0.0;
}

void FormantDaisyProcessor::parameterChanged (const juce::String& id, float)
{
    juce::ignoreUnused (id);
    triggerAsyncUpdate(); // latency changes and mode switches are handled off the audio thread
}

void FormantDaisyProcessor::handleAsyncUpdate()
{
    const int lat = juce::roundToInt (pLead->load() * 0.001 * currentSampleRate.load());
    if (lat != getLatencySamples())
        setLatencySamples (lat);

    // re-read the lyrics when the acronym toggle changes
    const bool spell = pSpell->load() > 0.5f;
    if (spell != lastSpell)
        setLyricsText (lyricsText);
}

//==============================================================================
void FormantDaisyProcessor::setLyricsText (const juce::String& text)
{
    lyricsText = text;
    lyricsVersion.fetch_add (1);
    lastSpell = pSpell == nullptr || pSpell->load() > 0.5f;
    auto parsed = std::make_shared<const fd::Lyrics> (fd::parseLyrics (text.toStdString(), lastSpell));
    const juce::SpinLock::ScopedLockType sl (lyricsLock);
    pendingLyrics = parsed;
    uiLyrics = parsed;
}

juce::String FormantDaisyProcessor::getLyricsText() const { return lyricsText; }

std::shared_ptr<const fd::Lyrics> FormantDaisyProcessor::getLyricsForUI() const
{
    const juce::SpinLock::ScopedLockType sl (lyricsLock);
    return uiLyrics;
}

void FormantDaisyProcessor::copyRecentAudio (float* dest, int numSamples) const
{
    numSamples = juce::jmin (numSamples, vizSize);
    const int w = vizWrite.load (std::memory_order_acquire);
    for (int i = 0; i < numSamples; ++i)
        dest[i] = vizBuffer[(size_t) ((w - numSamples + i + vizSize) % vizSize)];
}

//==============================================================================
static double divisionToPpq (int index)
{
    switch (index)
    {
        case 0: return 1.0;       // 1/4
        case 1: return 0.5;       // 1/8
        case 2: return 1.0 / 3.0; // 1/8 triplet
        default: return 0.25;     // 1/16
    }
}

void FormantDaisyProcessor::handleTempoMode (juce::MidiBuffer& midi, int numSamples)
{
    const double sr = currentSampleRate.load();
    const int lead = juce::roundToInt (pLead->load() * 0.001 * sr);

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            int i = 0;
            for (; i < tempoHeldCount && tempoHeld[(size_t) i] != m.getNoteNumber(); ++i) {}
            if (i == tempoHeldCount && tempoHeldCount < (int) tempoHeld.size())
                tempoHeld[(size_t) tempoHeldCount++] = m.getNoteNumber();
            tempoVelocity = m.getFloatVelocity();
        }
        else if (m.isNoteOff())
        {
            for (int i = 0; i < tempoHeldCount; ++i)
                if (tempoHeld[(size_t) i] == m.getNoteNumber())
                {
                    for (int j = i; j < tempoHeldCount - 1; ++j) tempoHeld[(size_t) j] = tempoHeld[(size_t) j + 1];
                    --tempoHeldCount;
                    break;
                }
            if (tempoHeldCount == 0)
                singer.releaseAll (meta.samplePosition + lead);
        }
        else if (m.isPitchWheel())
            singer.setPitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.0f * 2.0f);
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            tempoHeldCount = 0;
            singer.releaseAll (meta.samplePosition);
        }
    }

    // musical clock (set up in updateClock)
    const double ppqStart = clkPpqStart, ppqPerSample = clkPpqPerSample;
    const double ppqEnd = ppqStart + numSamples * ppqPerSample;

    if (tempoHeldCount <= 0)
        return;

    const double step = divisionToPpq (juce::roundToInt (pDivision->load()));
    const int gateSamples = juce::roundToInt (pGate->load() * step / ppqPerSample);
    double b = std::ceil (ppqStart / step - 1.0e-7) * step;
    while (b < ppqEnd)
    {
        const int offset = juce::jlimit (0, numSamples - 1, (int) std::floor ((b - ppqStart) / ppqPerSample));
        singer.triggerStep (tempoHeld[(size_t) tempoHeldCount - 1], tempoVelocity, offset, gateSamples, barAt (offset));
        b += step;
    }
}

// Musical clock: the host's position, bar and time signature while it plays;
// otherwise free-running at the host tempo so Tempo mode and bar waits still work.
void FormantDaisyProcessor::updateClock (int numSamples)
{
    const double sr = currentSampleRate.load();
    double bpm = 120.0, ppq = freePpq, barStart = -1.0;
    bool playing = false;
    int64_t barCount = -1;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto ts = pos->getTimeSignature())
                if (ts->numerator > 0 && ts->denominator > 0) { clkNum = ts->numerator; clkDen = ts->denominator; }
            playing = pos->getIsPlaying();
            if (playing)
            {
                if (auto p = pos->getPpqPosition()) ppq = *p;
                if (auto bs = pos->getPpqPositionOfLastBarStart()) barStart = *bs;
                if (auto bc = pos->getBarCount()) barCount = *bc;
            }
        }
    clkQpb = juce::jmax (0.25, clkNum * 4.0 / clkDen);
    clkPpqStart = ppq;
    clkPpqPerSample = bpm / 60.0 / sr;
    if (barStart >= 0.0 && barStart <= ppq + 1.0e-6)
    {
        clkOrigin = barStart;
        clkBarBase = barCount >= 0 ? barCount : (int64_t) std::llround (barStart / clkQpb);
    }
    else
    {
        clkOrigin = 0.0;
        clkBarBase = 0;
    }
    freePpq = ppq + numSamples * clkPpqPerSample;

    // display: bar and beat at the start of this block
    const double rel = ppq - clkOrigin;
    const double inBar = rel - std::floor (rel / clkQpb) * clkQpb;
    uiBar.store ((int) (barAt (0) + 1));
    uiBeat.store (juce::jlimit (0, clkNum - 1, (int) std::floor (inBar / (4.0 / clkDen) + 1.0e-6)));
    uiBeatsPerBar.store (clkNum);
    uiBeatUnit.store (clkDen);
    uiPlaying.store (playing);
}

int64_t FormantDaisyProcessor::barAt (int sampleOffset) const
{
    // a note up to 1/40 of a whole note early still counts as the next bar (humanised timing)
    constexpr double tolerance = 0.1;
    const double rel = clkPpqStart + sampleOffset * clkPpqPerSample - clkOrigin;
    return clkBarBase + (int64_t) std::floor ((rel + tolerance) / clkQpb);
}

void FormantDaisyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    if ((int) mono.size() < numSamples)
        mono.resize ((size_t) numSamples); // only if the host exceeds the announced block size

    // lyrics hand-over
    {
        const juce::SpinLock::ScopedTryLockType tl (lyricsLock);
        if (tl.isLocked() && pendingLyrics)
        {
            singer.setLyrics (pendingLyrics);
            pendingLyrics.reset();
            lyricsChanged = true;
        }
    }

    // parameters
    fd::SingerParams sp;
    sp.source = pSource->load();
    sp.breath = pBreath->load();
    sp.formantShift = pFormant->load();
    sp.speed = pSpeed->load();
    sp.glideMs = pGlide->load();
    sp.vibDepthCents = pVibDepth->load();
    sp.vibRateHz = pVibRate->load();
    sp.lofi = pLofi->load();
    sp.gain = juce::Decibels::decibelsToGain (pVolume->load()) * 1.4f;
    sp.loop = pLoop->load() > 0.5f;
    singer.setParams (sp);
    singer.setLeadSamples (juce::roundToInt (pLead->load() * 0.001 * currentSampleRate.load()));

    // restarts: UI button, automatable parameter (rising edge), transport start
    const bool restartParam = pRestart->load() > 0.5f;
    bool doRestart = restartRequested.exchange (false) || (restartParam && ! lastRestartParam);
    lastRestartParam = restartParam;
    bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            playing = pos->getIsPlaying();
    const bool started = playing && ! wasPlaying;
    if (started && pRestartOnPlay->load() > 0.5f)
        doRestart = true;
    if (! playing && wasPlaying)
        singer.releaseAll (0);
    wasPlaying = playing;
    if (doRestart)
        singer.restart();

    updateClock (numSamples);

    // bar markers: follow the song position. On play, a jump or a loop, find the
    // section for this bar (silent until the next marker if we're mid-section).
    {
        const bool markers = playing && singer.hasSections();
        singer.setSectionSync (markers);
        const bool jumped = playing && ! started && expectedPpq >= 0.0 && std::abs (clkPpqStart - expectedPpq) > 0.05;
        if (markers && (started || jumped || lyricsChanged || doRestart))
        {
            const double rel = clkPpqStart - clkOrigin;
            const double inBar = rel - std::floor (rel / clkQpb) * clkQpb;
            const bool atBarStart = inBar < 0.1 || inBar > clkQpb - 0.1;
            if (jumped) singer.releaseAll (0);
            singer.locate (barAt (0), atBarStart);
        }
        lyricsChanged = false;
        expectedPpq = playing ? clkPpqStart + numSamples * clkPpqPerSample : -1.0;
    }

    const bool tempoMode = pMode->load() > 0.5f;
    if (tempoMode)
    {
        handleTempoMode (midi, numSamples);
    }
    else
    {
        tempoHeldCount = 0;
        for (const auto meta : midi)
        {
            const auto m = meta.getMessage();
            if (m.isNoteOn())
                singer.noteOn (m.getNoteNumber(), m.getFloatVelocity(), meta.samplePosition, barAt (meta.samplePosition));
            else if (m.isNoteOff())
                singer.noteOff (m.getNoteNumber(), meta.samplePosition);
            else if (m.isPitchWheel())
                singer.setPitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.0f * 2.0f);
            else if (m.isAllNotesOff() || m.isAllSoundOff())
                singer.releaseAll (meta.samplePosition);
        }
    }

    // ---- optional audio input (carrier) ----
    // Copy it out first: JUCE shares the main input's channels with the output.
    const float* carrierPtr = nullptr;
    float inPeak = 0.0f;
    {
        if ((int) carrierBuf.size() < numSamples) carrierBuf.resize ((size_t) numSamples);
        std::fill (carrierBuf.begin(), carrierBuf.begin() + numSamples, 0.0f);
        int used = 0;
        for (int busIdx : { 1, 0 }) // sidechain first, then the main input
        {
            auto* bus = getBus (true, busIdx);
            if (bus == nullptr || ! bus->isEnabled() || used > 0) continue;
            auto in = getBusBuffer (buffer, true, busIdx);
            const int nCh = in.getNumChannels();
            if (nCh <= 0) continue;
            for (int ch = 0; ch < nCh; ++ch)
            {
                const float* src = in.getReadPointer (ch);
                for (int i = 0; i < numSamples; ++i) carrierBuf[(size_t) i] += src[i] / (float) nCh;
            }
            used = nCh;
        }
        for (int i = 0; i < numSamples; ++i) inPeak = juce::jmax (inPeak, std::abs (carrierBuf[(size_t) i]));
        if (used > 0) carrierPtr = carrierBuf.data();
    }
    // AUTO: use the input while audio has arrived in the last 1.5 s, else the buzz
    const double sr = currentSampleRate.load();
    if (inPeak > 0.001f) inputHoldSamples = (int) (1.5 * sr);
    else inputHoldSamples = juce::jmax (0, inputHoldSamples - numSamples);
    const int carrierMode = juce::roundToInt (pCarrier->load()); // 0 auto, 1 buzz, 2 input
    const bool useInput = carrierPtr != nullptr && (carrierMode == 2 || (carrierMode == 0 && inputHoldSamples > 0));
    singer.setCarrierMix (useInput ? 1.0f : 0.0f);
    uiCarrierActive.store (useInput);
    uiInputLevel.store (juce::jmax (inPeak, uiInputLevel.load() * 0.9f));

    singer.process (mono.data(), numSamples, carrierPtr);

    {
        auto out = getBusBuffer (buffer, false, 0);
        for (int ch = 0; ch < out.getNumChannels(); ++ch)
            out.copyFrom (ch, 0, mono.data(), numSamples);
    }

    // viz ring buffer
    int w = vizWrite.load (std::memory_order_relaxed);
    for (int i = 0; i < numSamples; ++i)
    {
        vizBuffer[(size_t) w] = mono[(size_t) i];
        w = (w + 1) % vizSize;
    }
    vizWrite.store (w, std::memory_order_release);
}

//==============================================================================
void FormantDaisyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("lyrics", lyricsText, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void FormantDaisyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        if (tree.isValid() && tree.hasType (apvts.state.getType()))
        {
            const juce::String text = tree.getProperty ("lyrics", defaultLyrics()).toString();
            apvts.replaceState (tree);
            setLyricsText (text); // the editor notices via getLyricsVersion()
        }
    }
}

juce::AudioProcessorEditor* FormantDaisyProcessor::createEditor() { return new FormantDaisyEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FormantDaisyProcessor(); }
