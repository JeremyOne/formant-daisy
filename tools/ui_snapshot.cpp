// Dev tool: runs the plugin without a host, plays a few notes and saves
// editor screenshots to PNG. Usage: ui_snapshot outdir
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? argv[1] : "/tmp");
    auto proc = std::make_unique<FormantDaisyProcessor>();
    proc->setPlayConfigDetails (0, 2, 48000.0, 512);
    proc->prepareToPlay (48000.0, 512);
    if (auto* md = std::getenv ("FD_MODE"))
        if (auto* prm = proc->apvts.getParameter ("mode")) prm->setValueNotifyingHost ((float) std::atoi (md));
    if (auto* env = std::getenv ("FD_FORMANT"))
        if (auto* prm = proc->apvts.getParameter ("formant"))
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) std::atof (env)));
    if (auto* lyr = std::getenv ("FD_LYRICS"))
        proc->setLyricsText (juce::String (lyr));
    if (auto* th = std::getenv ("FD_THEME"))
        Palette::applyTheme (std::atoi (th));
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc->createEditor());
    ed->setVisible (true);
    if (auto* fe = dynamic_cast<FormantDaisyEditor*> (ed.get()))
    {
        if (std::getenv ("FD_COMPACT") != nullptr) fe->setCompact (true);
        if (auto* th = std::getenv ("FD_THEME")) fe->setTheme (std::atoi (th));
        if (auto* rb = std::getenv ("FD_ROBOT")) fe->setRobot (std::atoi (rb));
        if (std::getenv ("FD_HELP") != nullptr) fe->setShowHelp (true);
    }

    const int notes[] = { 72, 69, 65, 60, 62, 64, 65 };
    std::atomic<bool> stop { false };
    std::thread audio ([&] {
        juce::AudioBuffer<float> buf (2, 512);
        int64_t sample = 0; int idx = 0;
        const int noteLen = 48000 * 6 / 10;
        while (! stop)
        {
            juce::MidiBuffer midi;
            const int64_t pos = sample % noteLen;
            if (pos < 512) { midi.addEvent (juce::MidiMessage::noteOn (1, notes[idx % 7], 0.8f), (int) pos); }
            if (pos + 512 > noteLen - 2400 && pos <= noteLen - 2400) { midi.addEvent (juce::MidiMessage::noteOff (1, notes[idx % 7]), (int) (noteLen - 2400 - pos)); ++idx; }
            proc->processBlock (buf, midi);
            sample += 512;
            std::this_thread::sleep_for (std::chrono::microseconds (10600));
        }
    });

    std::vector<int> shots { 400, 1500, 2600, 4200 };
    if (auto* env = std::getenv ("FD_SHOTS"))
    {
        shots.clear();
        for (auto& tok : juce::StringArray::fromTokens (env, ",", ""))
            shots.push_back (tok.getIntValue());
    }
    int last = 0, n = 0;
    for (int t : shots)
    {
        juce::MessageManager::getInstance()->runDispatchLoopUntil (t - last);
        last = t;
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f = outDir.getChildFile ("ui_" + juce::String (++n) + ".png");
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat().writeImageToStream (img, os);
        std::printf ("wrote %s\n", f.getFullPathName().toRawUTF8());
    }
    stop = true;
    audio.join();
    ed.reset();
    return 0;
}
