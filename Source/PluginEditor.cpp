#include "PluginEditor.h"

using namespace juce;

namespace
{
constexpr int kWidth = 980, kHeight = 640;
// compact view: 9:16 like YouTube Shorts; exactly half of 1080x1920
constexpr int kCompactW = 540, kCompactH = 960;
constexpr float kPx = 3.0f; // chunky pixel size for lines and dots

// vowel chart axes: F2 on a log scale (high on the left), F1 downwards
constexpr float kF2Hi = 2800.0f, kF2Lo = 560.0f, kF1Lo = 180.0f, kF1Hi = 900.0f;
} // namespace

//==============================================================================
void Palette::applyTheme (int id)
{
    struct T { uint32 bg, panel, teal, tealDim, tealBright, cream, orange, blue, blueDim, lavender, pink, dim,
               wood, woodDark, woodLight, star, glow, mouth, grilleDark, grilleLight, pupil, white; };
    static const T themes[] = {
        // Daisy: night sky, teal frames, cream/orange text (matches the reference short)
        { 0xff0a0c18, 0xff0e1324, 0xff2b8c84, 0xff17403f, 0xff4fd1b0, 0xfff6e7b0, 0xfff2a65a, 0xff6ec6ff, 0xff2c5a86,
          0xffb48cff, 0xfff7a8c4, 0xff3a405c, 0xff8a5530, 0xff4a2c18, 0xffb07a45, 0xff8fa3ff, 0xffffd27a, 0xffffe08a,
          0xff1d2a22, 0xff2f6b5a, 0xff10304a, 0xffffffff },
        // Amber Terminal: monochrome amber phosphor CRT, like a 1980s VT220
        { 0xff0c0803, 0xff140d05, 0xff9a5c12, 0xff3d2508, 0xffffb000, 0xffffd88a, 0xffffffff, 0xffffc040, 0xff6a3e0a,
          0xffd08a20, 0xffffa040, 0xff4a3216, 0xff6a4410, 0xff2e1d06, 0xff9a6418, 0xffa06a20, 0xffffc860, 0xffffe0a0,
          0xff1a1105, 0xff4a2e08, 0xff2a1a04, 0xfffff4dc },
        // Pocket Green: the 4-shade handheld LCD look
        { 0xff0f380f, 0xff163f16, 0xff5f8a2a, 0xff1f4a1f, 0xff8bac0f, 0xff9bbc0f, 0xffe0f8d0, 0xffc4e08a, 0xff3f7030,
          0xff8bac0f, 0xffc4e08a, 0xff4f7a3a, 0xff306230, 0xff0f380f, 0xff8bac0f, 0xff306230, 0xffe0f8d0, 0xffe0f8d0,
          0xff0f380f, 0xff306230, 0xff0f380f, 0xffe0f8d0 },
        // Synthwave: deep purple night, magenta frames, cyan and hot pink
        { 0xff12051f, 0xff1b0a2c, 0xffb5179e, 0xff4a1050, 0xff2de2e6, 0xfff7f0ff, 0xffff6ad5, 0xff2de2e6, 0xff3a2a7a,
          0xfffdd23e, 0xffff9e64, 0xff4a2a6a, 0xff3a2a7a, 0xff1f1440, 0xff6a4ac0, 0xffc77dff, 0xffff6ad5, 0xffffd1f4,
          0xff1f0f30, 0xff4a2a8a, 0xff12051f, 0xffffffff },
        // Hazy Vaporwave: 4C265F F20775 0678BF 07598C 079DD9
        { 0xff170b1f, 0xff21102d, 0xff0678bf, 0xff0a2f4a, 0xff079dd9, 0xfff3e6f5, 0xfff20775, 0xff079dd9, 0xff07598c,
          0xff9b6fc0, 0xffff7fb8, 0xff6a4a85, 0xff4c265f, 0xff2b1538, 0xff6e3f8a, 0xff5a7ec9, 0xfff6e7ff, 0xffffd1ea,
          0xff1a0d24, 0xff07598c, 0xff170b1f, 0xffffffff },
        // Neutral Palette 2: 548D95 76C1C3 AE7F21 F0AE44 789342 B4DB6F
        { 0xff111a1b, 0xff172426, 0xff548d95, 0xff2a474b, 0xff76c1c3, 0xffeef3e0, 0xfff0ae44, 0xff76c1c3, 0xff3d6a70,
          0xffb4db6f, 0xffcf9a35, 0xff55706f, 0xff789342, 0xff45552a, 0xff9bb35e, 0xff5f9799, 0xfff3f0d0, 0xffffd27a,
          0xff172426, 0xff548d95, 0xff111a1b, 0xffffffff },
        // Happy Feet: ED1F2F 0DC8DB 52D932 FFC729 FF8008
        { 0xff101418, 0xff171c22, 0xff0a8f9e, 0xff0b4a52, 0xff0dc8db, 0xfffff4d6, 0xffff8008, 0xff0dc8db, 0xff0a6b78,
          0xff52d932, 0xffff5a66, 0xff4d5966, 0xffed1f2f, 0xff8a1019, 0xffff5a66, 0xff5ac8d5, 0xffffc729, 0xffffe08a,
          0xff1a1f26, 0xff2f8a1c, 0xff101418, 0xffffffff },
        // Tam & Zpet: 414142 DDB826 D7D7D7 A4AF84 DDDCBE
        { 0xff1c1c1d, 0xff262627, 0xff8a9370, 0xff4a4f3c, 0xffdddcbe, 0xffdddcbe, 0xffddb826, 0xffa4af84, 0xff55594a,
          0xffd7d7d7, 0xffe6cf6a, 0xff5e5e5f, 0xff5a5a5b, 0xff2e2e2f, 0xff7a7a7b, 0xff8a8a8b, 0xffd7d7d7, 0xffdddcbe,
          0xff1c1c1d, 0xff4a4f3c, 0xff1c1c1d, 0xffffffff },
        // A House of Bees: D6AA58 A9CDD4 E1EBF0 FFFFFF 477A91
        { 0xff0f1a20, 0xff16252d, 0xff477a91, 0xff223c48, 0xffa9cdd4, 0xffe1ebf0, 0xfff0c470, 0xff6fb0cc, 0xff2f5566,
          0xffffffff, 0xffd6aa58, 0xff4f6a77, 0xffd6aa58, 0xff8c6a2c, 0xffeac584, 0xff7fa3ab, 0xffe1ebf0, 0xffffffff,
          0xff16252d, 0xff477a91, 0xff0f1a20, 0xffffffff },
        // Sports Day: 913F4E CEEAE7 6287A2 FEFFF2 170901
        { 0xff170901, 0xff22100a, 0xff6287a2, 0xff2c3d4a, 0xffceeae7, 0xfffefff2, 0xffc85a6c, 0xff8fd6cf, 0xff3e5568,
          0xff8fb0c8, 0xffd9707f, 0xff5a4440, 0xff913f4e, 0xff4f1f29, 0xffb9606f, 0xff6287a2, 0xfffefff2, 0xfffefff2,
          0xff1f0e08, 0xff3e5568, 0xff170901, 0xffffffff },
    };
    const T& t = themes[jlimit (0, (int) (sizeof (themes) / sizeof (themes[0])) - 1, id)];
    bg = Colour (t.bg); panel = Colour (t.panel); teal = Colour (t.teal); tealDim = Colour (t.tealDim);
    tealBright = Colour (t.tealBright); cream = Colour (t.cream); orange = Colour (t.orange); blue = Colour (t.blue);
    blueDim = Colour (t.blueDim); lavender = Colour (t.lavender); pink = Colour (t.pink); dim = Colour (t.dim);
    wood = Colour (t.wood); woodDark = Colour (t.woodDark); woodLight = Colour (t.woodLight); star = Colour (t.star);
    glow = Colour (t.glow); mouth = Colour (t.mouth); grilleDark = Colour (t.grilleDark); grilleLight = Colour (t.grilleLight);
    pupil = Colour (t.pupil); white = Colour (t.white);
}

//==============================================================================
PixelLookAndFeel::PixelLookAndFeel() { updateColours(); }

void PixelLookAndFeel::updateColours()
{
    Palette::applyTheme (themeId);
    setColour (TextEditor::backgroundColourId, Palette::panel);
    setColour (TextEditor::textColourId, Palette::cream);
    setColour (TextEditor::highlightColourId, Palette::teal.withAlpha (0.5f));
    setColour (TextEditor::highlightedTextColourId, Palette::cream);
    setColour (TextEditor::outlineColourId, Palette::teal);
    setColour (TextEditor::focusedOutlineColourId, Palette::tealBright);
    setColour (CaretComponent::caretColourId, Palette::orange);
    setColour (ScrollBar::thumbColourId, Palette::teal);
}

void PixelLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int width, int height, float sliderPos, float, float,
                                         Slider::SliderStyle, Slider& s)
{
    Palette::applyTheme (themeId);
    const int block = 6, gap = 2, n = width / (block + gap);
    const float frac = jlimit (0.0f, 1.0f, (sliderPos - (float) x) / (float) width);
    const int filled = roundToInt (frac * (float) n);
    const bool hot = s.isMouseOverOrDragging();
    for (int i = 0; i < n; ++i)
    {
        Colour c = i < filled ? (hot ? Palette::cream : Palette::orange) : Palette::tealDim;
        g.setColour (c);
        g.fillRect (x + i * (block + gap), y + 1, block, height - 2);
    }
}

void PixelButton::paint (Graphics& g)
{
    if (theme != nullptr) Palette::applyTheme (*theme);
    if (subtle && ! hover && ! down)
    {
        const String t = getText ? getText() : String();
        float ts = 2.0f;
        while (ts > 1.0f && PixelFont::width (t, ts) > (float) getWidth() - 4.0f) ts -= 0.5f;
        PixelFont::drawCentred (g, t, getLocalBounds().toFloat(), ts, Palette::dim.brighter (0.2f));
        return;
    }
    auto r = getLocalBounds().toFloat();
    const bool active = isActive && isActive();
    g.setColour (down ? Palette::teal.withAlpha (0.5f) : Palette::panel);
    g.fillRect (r);
    g.setColour (active ? Palette::orange : (hover ? Palette::tealBright : Palette::teal));
    const float t = 2.0f;
    g.fillRect (r.removeFromTop (t));
    g.fillRect (r.removeFromBottom (t));
    g.fillRect (r.removeFromLeft (t));
    g.fillRect (r.removeFromRight (t));
    const String text = getText ? getText() : String();
    // shrink the text if it would not fit (e.g. "DIV 1/16")
    float sc = 2.0f;
    while (sc > 1.0f && PixelFont::width (text, sc) > (float) getWidth() - 8.0f)
        sc -= 0.5f;
    PixelFont::drawCentred (g, text, getLocalBounds().toFloat(), sc, active ? Palette::orange : Palette::cream);
}

//==============================================================================
FormantDaisyEditor::FormantDaisyEditor (FormantDaisyProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    fftData.assign ((size_t) fftSize * 2, 0.0f);
    recent.assign ((size_t) fftSize, 0.0f);

    Random rnd (1961); // the year an IBM 7090 sang Daisy Bell
    for (int i = 0; i < 220; ++i)
        stars.push_back ({ rnd.nextFloat() * kWidth, rnd.nextFloat() * (float) kCompactH, rnd.nextFloat() * MathConstants<float>::twoPi,
                           rnd.nextFloat() < 0.15f ? 3 : 2 });

    // --- lyrics box ---
    lyricsBox.setMultiLine (true, true);
    lyricsBox.setReturnKeyStartsNewLine (true);
#if JUCE_MAJOR_VERSION >= 8
    lyricsBox.setFont (Font (FontOptions (Font::getDefaultMonospacedFontName(), 15.0f, Font::bold)));
#else
    lyricsBox.setFont (Font (Font::getDefaultMonospacedFontName(), 15.0f, Font::bold));
#endif
    lyricsBox.setIndents (8, 8);
    lyricsBox.setText (proc.getLyricsText(), dontSendNotification);
    lyricsBox.addListener (this);
    addAndMakeVisible (lyricsBox);
    lastLyricsVersion = proc.getLyricsVersion();

    // --- buttons ---
    auto& apvts = proc.apvts;
    auto attach = [&apvts] (const char* id, float& store, Component& c) {
        auto a = std::make_unique<ParameterAttachment> (*apvts.getParameter (id), [&store, &c] (float v) {
            store = v;
            c.repaint();
        });
        a->sendInitialUpdate();
        return a;
    };

    daisyBtn.getText = [] { return String ("DAISY"); };
    daisyBtn.onClick = [this] {
        lyricsBox.setText (FormantDaisyProcessor::defaultLyrics(), dontSendNotification);
        proc.setLyricsText (lyricsBox.getText());
        lastLyricsVersion = proc.getLyricsVersion();
    };
    restartBtn.getText = [] { return String ("RESTART"); };
    restartBtn.onClick = [this] { proc.requestRestart(); };

    loopAtt = attach ("loop", loopValue, loopBtn);
    loopBtn.getText = [] { return String ("LOOP"); };
    loopBtn.isActive = [this] { return loopValue > 0.5f; };
    loopBtn.onClick = [this] { loopAtt->setValueAsCompleteGesture (loopValue > 0.5f ? 0.0f : 1.0f); };

    helpBtn.getText = [] { return String ("?"); };
    helpBtn.isActive = [this] { return showHelp; };
    helpBtn.onClick = [this] { showHelp = ! showHelp; repaint(); };

    // R: put every parameter back to its default (lyrics are left alone)
    resetBtn.getText = [] { return String ("R"); };
    resetBtn.onClick = [this] {
        for (auto* prm : proc.getParameters())
            if (auto* rp = dynamic_cast<RangedAudioParameter*> (prm))
            {
                rp->beginChangeGesture();
                rp->setValueNotifyingHost (rp->getDefaultValue());
                rp->endChangeGesture();
            }
        repaint();
    };

    modeAtt = attach ("mode", modeValue, modeBtn);
    modeBtn.getText = [this] { return String (modeValue > 0.5f ? "MODE: TEMPO" : "MODE: NOTES"); };
    modeBtn.onClick = [this] { modeAtt->setValueAsCompleteGesture (modeValue > 0.5f ? 0.0f : 1.0f); };

    divAtt = attach ("division", divValue, divBtn);
    divBtn.getText = [this] {
        static const char* names[] = { "1/4", "1/8", "1/8T", "1/16" };
        return String ("DIV ") + names[jlimit (0, 3, roundToInt (divValue))];
    };
    divBtn.isActive = [this] { return modeValue > 0.5f; };
    divBtn.onClick = [this] { divAtt->setValueAsCompleteGesture ((float) ((roundToInt (divValue) + 1) % 4)); };

    syncAtt = attach ("restartOnPlay", syncValue, syncBtn);
    syncBtn.getText = [] { return String ("PLAY>1"); };
    syncBtn.isActive = [this] { return syncValue > 0.5f; };
    syncBtn.onClick = [this] { syncAtt->setValueAsCompleteGesture (syncValue > 0.5f ? 0.0f : 1.0f); };

    // CARRIER: what the formants shape (AUTO = audio input when present, else the buzz)
    carrierAtt = attach ("carrier", carrierValue, carrierBtn);
    carrierBtn.getText = [this] {
        static const char* n[] = { "AUTO", "BUZZ", "INPUT" };
        return String ("CARRIER: ") + n[jlimit (0, 2, roundToInt (carrierValue))];
    };
    carrierBtn.isActive = [this] { return proc.uiCarrierActive.load(); };
    carrierBtn.onClick = [this] { carrierAtt->setValueAsCompleteGesture ((float) ((roundToInt (carrierValue) + 1) % 3)); };

    // ABC: spell all-caps acronyms (PHP -> P H P) on/off
    spellAtt = attach ("spellAcronyms", spellValue, spellBtn);
    spellBtn.getText = [] { return String ("ABC"); };
    spellBtn.isActive = [this] { return spellValue > 0.5f; };
    spellBtn.onClick = [this] { spellAtt->setValueAsCompleteGesture (spellValue > 0.5f ? 0.0f : 1.0f); };

    viewBtn.getText = [this] { return String (compact ? "FULL" : "9:16"); };
    viewBtn.onClick = [this] { setCompact (! compact); };
    viewBtn.subtle = true;
    addAndMakeVisible (viewBtn);

    static const char* themeNames[] = { "DAISY", "AMBER", "POCKET", "SYNTH", "HAZY", "NEUTRAL", "HAPPY", "TAM", "BEES", "SPORTS" };
    themeBtn.getText = [this] { return String ("COLOR: ") + themeNames[jlimit (0, numThemes - 1, themeId)]; };
    themeBtn.onClick = [this] { setTheme ((themeId + 1) % numThemes); };
    themeBtn.subtle = true;
    addAndMakeVisible (themeBtn);

    for (auto* b : { &daisyBtn, &restartBtn, &loopBtn, &helpBtn, &resetBtn, &modeBtn, &divBtn, &syncBtn, &spellBtn, &carrierBtn })
        addAndMakeVisible (*b);

    // --- sliders ---
    const std::pair<const char*, const char*> list[] = {
        { "source", "VOICE" }, { "breath", "BREATH" }, { "formant", "FORMANT" }, { "speed", "CONS LEN" },
        { "glide", "GLIDE" }, { "vibDepth", "VIBRATO" }, { "vibRate", "VIB RATE" }, { "gate", "GATE" },
        { "lead", "LEAD" }, { "lofi", "LO-FI" }, { "volume", "VOLUME" },
    };
    for (auto& [id, label] : list)
    {
        auto k = std::make_unique<Knob>();
        k->id = id;
        k->label = label;
        k->slider.setSliderStyle (Slider::LinearHorizontal);
        k->slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        k->slider.onValueChange = [this] { repaint (560, 280, 420, 320); };
        addAndMakeVisible (k->slider);
        k->att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (apvts, id, k->slider);
        if (auto* rp = dynamic_cast<RangedAudioParameter*> (apvts.getParameter (id)))
            k->slider.setDoubleClickReturnValue (true, rp->convertFrom0to1 (rp->getDefaultValue()));
        knobs.push_back (std::move (k));
    }

    setSize (kWidth, kHeight);
    for (auto* b : { &daisyBtn, &restartBtn, &loopBtn, &helpBtn, &resetBtn, &modeBtn, &divBtn, &syncBtn, &spellBtn, &viewBtn, &themeBtn, &carrierBtn })
        b->theme = &themeId;
    robotStyle = jlimit (0, numRobots - 1, (int) proc.apvts.state.getProperty ("robot", 0));
    setTheme ((int) proc.apvts.state.getProperty ("theme", 0));
    setCompact ((bool) proc.apvts.state.getProperty ("compactView", false));
    startTimerHz (30);
}

FormantDaisyEditor::~FormantDaisyEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void FormantDaisyEditor::setTheme (int id)
{
    themeId = jlimit (0, numThemes - 1, id);
    proc.apvts.state.setProperty ("theme", themeId, nullptr); // saved with the project
    lnf.themeId = themeId;
    lnf.updateColours();
    Palette::applyTheme (themeId);
    for (auto id2 : { TextEditor::backgroundColourId, TextEditor::textColourId, TextEditor::outlineColourId,
                      TextEditor::focusedOutlineColourId, TextEditor::highlightColourId })
        lyricsBox.setColour (id2, lnf.findColour (id2));
    lyricsBox.applyColourToAllText (Palette::cream);
    repaint();
}

void FormantDaisyEditor::setRobot (int style)
{
    robotStyle = ((style % numRobots) + numRobots) % numRobots;
    proc.apvts.state.setProperty ("robot", robotStyle, nullptr);
    repaint();
}

Rectangle<float> FormantDaisyEditor::robotHitArea() const
{
    const float P = robotScale;
    return { robotPos.x - 4 * P, robotPos.y - 8 * P, 30 * P, 40 * P };
}

void FormantDaisyEditor::mouseDown (const MouseEvent& e)
{
    if (robotHitArea().contains (e.position))
        setRobot (robotStyle + 1); // click Daisy for the next robot
}

void FormantDaisyEditor::mouseMove (const MouseEvent& e)
{
    setMouseCursor (robotHitArea().contains (e.position) ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
}

void FormantDaisyEditor::setCompact (bool shouldBeCompact)
{
    compact = shouldBeCompact;
    proc.apvts.state.setProperty ("compactView", compact, nullptr); // saved with the project

    juce::Component* controls[] = { &lyricsBox, &daisyBtn, &restartBtn, &loopBtn, &helpBtn, &resetBtn,
                                    &modeBtn, &divBtn, &syncBtn, &spellBtn, &carrierBtn };
    for (auto* c : controls) c->setVisible (! compact);
    for (auto& k : knobs) k->slider.setVisible (! compact);
    if (compact) showHelp = false;

    if (compact) setSize (kCompactW, kCompactH);
    else setSize (kWidth, kHeight);
    resized();
    repaint();
}

void FormantDaisyEditor::resized()
{
    if (compact)
    {
        // 540 x 960: title, chart, then Daisy cabled into the spectrum and scope
        titleArea = { 20, 30, 500, 136 };
        chartArea = { 20, 184, 500, 344 };
        specArea = { 200, 562, 320, 160 };
        waveArea = { 200, 740, 320, 120 };
        robotScale = 6.0f;
        robotPos = { 26, 604 };
        viewBtn.setBounds (kCompactW - 64, kCompactH - 34, 52, 24);
        themeBtn.setBounds (kCompactW - 64 - 4 - 92, kCompactH - 34, 92, 24);
        themeBtn.getText = [this] { static const char* n[] = { "DAISY", "AMBER", "POCKET", "SYNTH", "HAZY", "NEUTRAL", "HAPPY", "TAM", "BEES", "SPORTS" };
                                    return String (n[jlimit (0, numThemes - 1, themeId)]); };
        return;
    }

    titleArea = { 24, 14, 520, 122 };
    chartArea = { 34, 146, 500, 236 };
    specArea = { 150, 398, 384, 84 };
    waveArea = { 150, 492, 384, 60 };
    robotScale = 4.0f;
    robotPos = { 34, 416 };
    viewBtn.setBounds (568, kHeight - 30, 56, 24);
    themeBtn.setBounds (630, kHeight - 30, 170, 24);
    themeBtn.getText = [this] { static const char* n[] = { "DAISY", "AMBER", "POCKET", "SYNTH", "HAZY", "NEUTRAL", "HAPPY", "TAM", "BEES", "SPORTS" };
                                return String ("COLOR: ") + n[jlimit (0, numThemes - 1, themeId)]; };

    const int x = 568, w = 388;
    lyricsBox.setBounds (x, 40, w, 150);
    const int bw = (w - 18) / 4;
    daisyBtn.setBounds (x, 200, bw, 30);
    restartBtn.setBounds (x + (bw + 6), 200, bw, 30);
    loopBtn.setBounds (x + 2 * (bw + 6), 200, bw, 30);
    {
        const int sx = x + 3 * (bw + 6), sw = w - 3 * (bw + 6), half = (sw - 6) / 2;
        helpBtn.setBounds (sx, 200, half, 30);
        resetBtn.setBounds (sx + half + 6, 200, sw - half - 6, 30);
    }
    {
        const int modeW = 142, divW = 92, syncW = 84, gap = 6;
        modeBtn.setBounds (x, 238, modeW, 30);
        divBtn.setBounds (x + modeW + gap, 238, divW, 30);
        syncBtn.setBounds (x + modeW + divW + 2 * gap, 238, syncW, 30);
        const int sx = x + modeW + divW + syncW + 3 * gap;
        spellBtn.setBounds (sx, 238, x + w - sx, 30);
    }

    carrierBtn.setBounds (x, 578, 196, 24);

    int y = 284;
    for (auto& k : knobs)
    {
        k->slider.setBounds (x + 122, y, 196, 14);
        y += 27;
    }
}

//==============================================================================
void FormantDaisyEditor::textEditorTextChanged (TextEditor&)
{
    textDebounce = 8; // commit ~250 ms after typing stops
}

void FormantDaisyEditor::timerCallback()
{
    ++frame;
    auto& s = proc.getSinger();

    if (textDebounce > 0 && --textDebounce == 0)
    {
        proc.setLyricsText (lyricsBox.getText());
        lastLyricsVersion = proc.getLyricsVersion();
        textDebounce = -1;
    }
    if (proc.getLyricsVersion() != lastLyricsVersion && ! lyricsBox.hasKeyboardFocus (true))
    {
        lyricsBox.setText (proc.getLyricsText(), dontSendNotification);
        lastLyricsVersion = proc.getLyricsVersion();
    }
    lyrics = proc.getLyricsForUI();

    // trail on the vowel chart
    const float v = s.vizVoicing.load();
    // divide out the Formant knob so a smaller/larger voice still lines up with the vowel map
    const float shift = jmax (0.1f, proc.apvts.getRawParameterValue ("formant")->load());
    trail.push_back ({ s.vizF1.load() / shift, s.vizF2.load() / shift, s.vizF3.load() / shift, v });
    while (trail.size() > 75) trail.pop_front();

    // robot blinks once per rest
    {
        // queue one blink per rest so "...." gives four distinct blinks
        const int rc = s.vizRestCount.load();
        pendingBlinks = jmin (8, pendingBlinks + jmax (0, rc - lastRestCount));
        lastRestCount = rc;
        if (blinkFrames > 0)
            --blinkFrames;
        else if (pendingBlinks > 0)
        {
            --pendingBlinks;
            blinkFrames = 4; // 3 frames closed + 1 open (~130 ms per blink)
        }
    }

    // robot mouth: F2 sets the width, F1 sets the height
    {
        const bool open = s.vizLevel.load() > 0.02f;
        const float f1n = jlimit (0.0f, 1.0f, (s.vizF1.load() / shift - 200.0f) / 700.0f);
        const float f2n = jlimit (0.0f, 1.0f, std::log (jlimit (600.0f, 2800.0f, s.vizF2.load() / shift) / 600.0f) / std::log (2800.0f / 600.0f));
        const float tw = open ? 1.0f + 4.2f * f2n : 0.0f;
        const float th = open ? 1.0f + 4.2f * f1n : 0.0f;
        mouthW += (tw - mouthW) * 0.45f;
        mouthH += (th - mouthH) * 0.45f;
    }

    // phoneme ticker
    const int ph = s.vizPhoneme.load();
    if (ph != lastPhoneme)
    {
        lastPhoneme = ph;
        if (ph > 0)
        {
            ticker.push_back (String (fd::phoneme (ph).name).toUpperCase());
            while (ticker.size() > 9) ticker.pop_front();
        }
    }

    // spectrum
    proc.copyRecentAudio (recent.data(), fftSize);
    std::fill (fftData.begin(), fftData.end(), 0.0f);
    std::copy (recent.begin(), recent.end(), fftData.begin());
    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());
    const double sr = proc.getSampleRateForUI();
    const float maxHz = 5000.0f, binHz = (float) (sr / fftSize);
    for (int b = 0; b < numBars; ++b)
    {
        const int lo = (int) ((float) b * maxHz / numBars / binHz);
        const int hi = jmax (lo + 1, (int) ((float) (b + 1) * maxHz / numBars / binHz));
        float m = 0;
        for (int i = lo; i < hi && i < fftSize / 2; ++i) m = jmax (m, fftData[(size_t) i]);
        const float db = Decibels::gainToDecibels (m / (fftSize * 0.25f), -90.0f);
        const float val = jlimit (0.0f, 1.0f, (db + 72.0f) / 66.0f);
        bars[(size_t) b] = jmax (val, bars[(size_t) b] * 0.82f);
    }

    repaint();
}

//==============================================================================
void FormantDaisyEditor::pixelLine (Graphics& g, Point<float> a, Point<float> b, float size)
{
    const float len = a.getDistanceFrom (b);
    const int steps = jmax (1, (int) (len / size));
    for (int i = 0; i <= steps; ++i)
    {
        auto p = a + (b - a) * ((float) i / (float) steps);
        g.fillRect (std::floor (p.x / size) * size, std::floor (p.y / size) * size, size, size);
    }
}

void FormantDaisyEditor::drawFrame (Graphics& g, Rectangle<float> r, Colour c, bool brackets)
{
    g.setColour (c.withAlpha (0.55f));
    g.drawRect (r, 2.0f);
    if (! brackets) return;
    g.setColour (c.brighter (0.3f));
    const float L = 12, T = 3;
    auto corner = [&] (float x, float y, float dx, float dy) {
        g.fillRect (dx > 0 ? x : x - L, y - T * 0.5f, L, T);
        g.fillRect (x - T * 0.5f, dy > 0 ? y : y - L, T, L);
    };
    const auto e = r.expanded (3);
    corner (e.getX(), e.getY(), 1, 1);
    corner (e.getRight(), e.getY(), -1, 1);
    corner (e.getX(), e.getBottom(), 1, -1);
    corner (e.getRight(), e.getBottom(), -1, -1);
}

Point<float> FormantDaisyEditor::chartPoint (float f1, float f2, Rectangle<float> a) const
{
    const auto inner = a.reduced (34, 26);
    const float x = (std::log (kF2Hi) - std::log (jlimit (kF2Lo, kF2Hi, f2))) / (std::log (kF2Hi) - std::log (kF2Lo));
    // log F1 spreads out the low-F1 consonants along the top
    const float y = std::log (jlimit (kF1Lo, kF1Hi, f1) / kF1Lo) / std::log (kF1Hi / kF1Lo);
    return { inner.getX() + x * inner.getWidth(), inner.getY() + y * inner.getHeight() };
}

void FormantDaisyEditor::paint (Graphics& g)
{
    Palette::applyTheme (themeId); // palette is shared, so each editor sets its own first
    drawBackground (g);
    drawTitle (g);
    if (showHelp)
        drawHelp (g, chartArea);
    else
        drawVowelChart (g, chartArea);
    drawSpectrum (g, specArea);
    drawWaveform (g, waveArea);
    drawRobot (g, robotPos.x, robotPos.y);
    if (compact)
    {
        const String name = "FORMANT DAISY";
        PixelFont::draw (g, name, 20, (float) kCompactH - 28, 2.0f, Palette::teal);
        PixelFont::draw (g, "PROJECT MAMMOTH", 20 + PixelFont::width (name + " ", 2.0f) + 6, (float) kCompactH - 28, 2.0f,
                         Palette::dim.brighter (0.3f));
        return;
    }
    drawControlsText (g);
}

void FormantDaisyEditor::drawBackground (Graphics& g)
{
    g.fillAll (Palette::bg);
    const float t = (float) frame / 30.0f;
    for (auto& s : stars)
    {
        const float a = 0.25f + 0.35f * (0.5f + 0.5f * std::sin (t * 0.8f + s.phase));
        g.setColour (Palette::star.withAlpha (a));
        g.fillRect (std::round (s.x), std::round (s.y), (float) s.size - 1.0f, (float) s.size - 1.0f);
    }
    if (compact) return;
    // divider between the show and the controls
    g.setColour (Palette::tealDim.withAlpha (0.6f));
    for (int y = 16; y < kHeight - 16; y += 6)
        g.fillRect (552, y, 2, 3);
}

void FormantDaisyEditor::drawTitle (Graphics& g)
{
    auto& s = proc.getSinger();
    const auto area = titleArea;

    // top-left: phonemes of the current lyric line, sung ones lit up
    drawLinePhonemes (g, area, s.vizSyllable.load());

    const int posIdx = s.vizSyllable.load(); // may point at a punctuation rest
    int sylIdx = posIdx;                      // the syllable whose word we show
    if (lyrics && sylIdx >= 0 && sylIdx < (int) lyrics->syllables.size())
        while (sylIdx >= 0 && lyrics->syllables[(size_t) sylIdx].rest)
            --sylIdx; // during a rest keep showing the word that just finished
    const bool haveSyl = lyrics && sylIdx >= 0 && sylIdx < (int) lyrics->syllables.size()
                         && ! lyrics->syllables[(size_t) sylIdx].rest;

    if (! haveSyl)
    {
        PixelFont::drawCentred (g, "THE PEAKS ARE FORMANTS", { area.getX(), area.getY() + 24, area.getWidth(), 16 }, 2.0f,
                                Palette::cream);
        const String a = "FORMANT ", b = "DAISY";
        const float sc = 6.0f, w = PixelFont::width (a + b, sc);
        const float x0 = area.getCentreX() - w * 0.5f, y0 = area.getY() + 50;
        PixelFont::draw (g, a, x0, y0, sc, Palette::cream);
        PixelFont::draw (g, b, x0 + PixelFont::width (a, sc) + sc, y0, sc, Palette::orange);
        const String hint = modeValue > 0.5f ? "HOLD A NOTE - SPEAKS IN TEMPO" : "PLAY A NOTE - SINGS A SYLLABLE";
        PixelFont::drawCentred (g, hint, { area.getX(), area.getBottom() - 14, area.getWidth(), 14 }, 2.0f,
                                Palette::dim.brighter (0.4f));
        return;
    }

    const auto& syls = lyrics->syllables;
    const int wordIdx = syls[(size_t) sylIdx].word;

    // gather this word's syllables
    std::vector<int> mine;
    for (int i = 0; i < (int) syls.size(); ++i)
        if (syls[(size_t) i].word == wordIdx) mine.push_back (i);

    String whole;
    for (int i : mine) whole << String (syls[(size_t) i].label).toUpperCase();
    if (whole.isEmpty()) whole = "?";

    // punctuation rests count as steps, like the notes that play them
    int singable = 0, position = 0;
    for (int i = 0; i < (int) syls.size(); ++i)
        if (! syls[(size_t) i].rest || syls[(size_t) i].noteRest)
        {
            ++singable;
            if (i <= posIdx) position = singable;
        }

    PixelFont::drawCentred (g, "NOW SINGING  " + String (position) + "/" + String (singable),
                            { area.getX(), area.getY() + 24, area.getWidth(), 16 }, 2.0f, Palette::orange);

    const float sc = jlimit (3.0f, 7.0f, std::floor (area.getWidth() / (6.0f * (float) whole.length() + 2.0f)));
    float x = area.getCentreX() - PixelFont::width (whole, sc) * 0.5f;
    const float y = area.getY() + 48;
    for (int i : mine)
    {
        const String piece = String (syls[(size_t) i].label).toUpperCase();
        Colour c = i < sylIdx ? Palette::orange : (i == sylIdx ? Palette::cream : Palette::dim);
        if (i == sylIdx && s.vizLevel.load() > 0.02f)
            c = Palette::orange.interpolatedWith (Palette::cream, 0.5f + 0.5f * std::sin ((float) frame * 0.4f));
        PixelFont::draw (g, piece, x, y, sc, c);
        x += piece.length() * 6.0f * sc;
    }

    // upcoming words
    String next;
    int added = 0, lastWord = wordIdx;
    for (int i = sylIdx + 1; i < (int) syls.size() && added < 5; ++i)
    {
        const auto& sy = syls[(size_t) i];
        if (sy.rest || sy.word == lastWord) continue;
        lastWord = sy.word;
        if (sy.word >= 0 && sy.word < (int) lyrics->words.size())
        {
            next << String (lyrics->words[(size_t) sy.word]).toUpperCase() << " ";
            ++added;
        }
    }
    PixelFont::drawCentred (g, next.trim(), { area.getX(), area.getBottom() - 14, area.getWidth(), 14 }, 2.0f,
                            Palette::dim.brighter (0.4f));
}

void FormantDaisyEditor::drawLinePhonemes (Graphics& g, Rectangle<float> area, int posIdx)
{
    const float sc = 2.0f;
    const String prefix = ">> ";
    PixelFont::draw (g, prefix, area.getX(), area.getY(), sc, Palette::cream.withAlpha (0.85f));
    if (! lyrics || lyrics->syllables.empty())
        return;
    const auto& syls = lyrics->syllables;
    const int n = (int) syls.size();

    // the syllable being sung (or just finished, during a rest) and its line
    int cur = jlimit (-1, n - 1, posIdx);
    while (cur >= 0 && syls[(size_t) cur].rest) --cur;
    const int line = posIdx >= 0 && posIdx < n ? syls[(size_t) posIdx].line : syls[0].line;

    struct Tok { String text; int syl; };
    std::vector<Tok> toks;
    int prevWord = -9;
    for (int i = 0; i < n; ++i)
    {
        const auto& sy = syls[(size_t) i];
        if (sy.line != line || sy.rest) continue;
        String t;
        for (int ph : sy.onset) t << fd::phoneme (ph).name;
        t << fd::phoneme (sy.nucleus).name;
        for (int ph : sy.coda) t << fd::phoneme (ph).name;
        t = t.toUpperCase();
        if (! toks.empty()) t = (sy.word == prevWord ? "-" : " ") + t;
        prevWord = sy.word;
        toks.push_back ({ t, i });
    }
    if (toks.empty()) return;

    // scroll long lines so the current syllable stays in view
    const float x0 = area.getX() + PixelFont::width (prefix, sc) + sc;
    const float avail = area.getRight() - x0;
    int curTok = 0;
    for (int k = 0; k < (int) toks.size(); ++k)
        if (toks[(size_t) k].syl <= cur) curTok = k;
    int first = 0;
    auto widthFrom = [&] (int a, int b) {
        float w = 0;
        for (int k = a; k <= b; ++k) w += (float) toks[(size_t) k].text.length() * 6.0f * sc;
        return w;
    };
    // only scroll when the whole line doesn't fit
    if (widthFrom (0, (int) toks.size() - 1) > avail)
        while (first < curTok && widthFrom (first, curTok) > avail * 0.4f) ++first;

    float x = x0;
    for (int k = first; k < (int) toks.size(); ++k)
    {
        String t = toks[(size_t) k].text;
        if (k == first) t = t.trimStart().trimCharactersAtStart ("-");
        const float w = (float) t.length() * 6.0f * sc;
        if (x + w - sc > area.getRight()) break;
        const int syl = toks[(size_t) k].syl;
        const Colour c = syl < cur ? Palette::orange : (syl == cur ? Palette::cream : Palette::dim.brighter (0.4f));
        PixelFont::draw (g, t, x, area.getY(), sc, c);
        x += w;
    }
}

// F3 colour for the compact view: low F3 (R, ER) = pink, normal = warm glow, high = blue
Colour FormantDaisyEditor::f3Colour (float f3)
{
    const float t = jlimit (0.0f, 1.0f, (f3 - 1400.0f) / (3100.0f - 1400.0f));
    return t < 0.55f ? Palette::pink.interpolatedWith (Palette::glow, t / 0.55f)
                     : Palette::glow.interpolatedWith (Palette::blue, (t - 0.55f) / 0.45f);
}

// Noise static: consonant hiss drawn as a cloud of sparkles around the moving dot.
// Size = how spread out the noise is (S tight, F and TH wide), density = loudness,
// colour = noise pitch (high hiss whiter, low hiss pinker). H is a faint lavender haze.
void FormantDaisyEditor::drawNoiseCloud (Graphics& g, Point<float> centre)
{
    auto& s = proc.getSinger();
    const float fric = s.vizFric.load(), asp = s.vizAsp.load();
    if (fric < 0.03f && asp < 0.03f) return;
    Random r ((int64) frame * 7919);
    auto spray = [&] (float radius, float level, Colour c)
    {
        const int count = (int) (level * 60.0f);
        for (int i = 0; i < count; ++i)
        {
            // roughly normal spread around the dot
            const float gx = (r.nextFloat() + r.nextFloat() + r.nextFloat() - 1.5f) / 1.5f;
            const float gy = (r.nextFloat() + r.nextFloat() + r.nextFloat() - 1.5f) / 1.5f;
            const float sz = r.nextFloat() < 0.25f ? 3.0f : 2.0f;
            g.setColour (c.withAlpha (jlimit (0.2f, 1.0f, level * (0.6f + 0.4f * r.nextFloat()))));
            g.fillRect (std::round (centre.x + gx * radius), std::round (centre.y + gy * radius), sz, sz);
        }
    };
    if (asp > 0.03f)
        spray (22.0f, asp * 0.7f, Palette::lavender);
    if (fric > 0.03f)
    {
        const float hz = s.vizFricHz.load();
        const float t = jlimit (0.0f, 1.0f, std::log (hz / 1500.0f) / std::log (7000.0f / 1500.0f));
        const float radius = 8.0f + 12.0f * s.vizFricSpread.load();
        spray (radius, fric, Palette::pink.interpolatedWith (Palette::white, t * 0.7f));
        // name the hiss while it sounds
        const int ph = s.vizPhoneme.load();
        const auto kind = fd::phoneme (ph).kind;
        const bool noisy = kind != fd::Kind::Silence && kind != fd::Kind::Vowel && kind != fd::Kind::Diphthong
                           && kind != fd::Kind::Nasal && kind != fd::Kind::Liquid && kind != fd::Kind::Glide;
        const String name = String (fd::phoneme (ph).name).toUpperCase();
        if (noisy && name.isNotEmpty())
            PixelFont::draw (g, name, std::round (centre.x - PixelFont::width (name, 2.0f) * 0.5f),
                             std::round (centre.y - radius - 22), 2.0f, Palette::white);
    }
}

void FormantDaisyEditor::drawVowelChart (Graphics& g, Rectangle<float> a)
{
    g.setColour (Palette::bg.withAlpha (0.85f));
    g.fillRect (a);

    // faint dotted grid
    g.setColour (Palette::tealDim.withAlpha (0.45f));
    for (float gx = a.getX() + 25; gx < a.getRight(); gx += 50)
        for (float gy = a.getY() + 6; gy < a.getBottom(); gy += 8)
            g.fillRect (std::round (gx), std::round (gy), 2.0f, 2.0f);
    for (float gy = a.getY() + 30; gy < a.getBottom(); gy += 50)
        for (float gx = a.getX() + 6; gx < a.getRight(); gx += 8)
            g.fillRect (std::round (gx), std::round (gy), 2.0f, 2.0f);

    drawFrame (g, a, Palette::teal, true);

    auto P = [&] (const char* ph) {
        const auto& d = fd::phoneme (fd::findPhoneme (ph));
        return chartPoint (d.f1, d.f2, a);
    };

    // every vowel the singer uses, labelled with the same codes as the phoneme line
    // (top left). Offsets keep labels apart where targets crowd together.
    struct Mark { const char* ph; float lx, ly; };
    const Mark mono[] = {
        { "iy", -28, -7 }, { "ih", 6, -7 }, { "eh", 6, -7 }, { "ae", 6, -7 }, { "aa", 6, -7 }, { "ao", 6, -7 },
        { "ah", 6, -7 }, { "uh", 6, -7 }, { "uw", 6, -7 }, { "er", 6, -7 }, { "ax", -28, -7 },
    };
    const Mark diph[] = { { "ey", -28, -7 }, { "ay", -28, 4 }, { "aw", -28, 18 }, { "oy", -11, 7 }, { "ow", -28, -7 } };
    auto codeOf = [] (const char* ph) { return String (ph).toUpperCase(); };

    // consonants: small pink dots. Sounds that share a target are grouped.
    struct Cons { const char* label; const char* ph; float lx, ly; };
    const Cons cons[] = {
        { "P B", "p", 5, -3 },  { "T D", "t", 5, -3 },  { "K G", "k", 5, -3 },  { "F", "f", -10, -3 },
        { "V", "v", 5, -3 },    { "TH", "th", 5, -3 },  { "DH", "dh", 5, -3 },  { "S", "s", 5, -3 },
        { "Z", "z", 5, -3 },    { "SH ZH CH", "sh", 5, -3 }, { "JH", "jh", 5, -3 }, { "M", "m", 5, -3 },
        { "N", "n", 5, -3 },    { "NG", "ng", 5, -3 },  { "L", "l", 5, -3 },    { "R", "r", 5, -5 },
        { "W", "w", 5, -3 },    { "Y", "y", -10, -3 },
    };
    for (auto& c : cons)
    {
        const auto p = P (c.ph);
        g.setColour (Palette::pink.withAlpha (0.75f));
        g.fillRect (std::round (p.x) - 2, std::round (p.y) - 2, 4.0f, 4.0f);
        const float lx = c.lx < 0 ? -4.0f - PixelFont::width (c.label, 1.0f) : c.lx;
        PixelFont::draw (g, c.label, std::round (p.x + lx), std::round (p.y + c.ly), 1.0f, Palette::pink.withAlpha (0.8f));
    }

    // diphthongs: hollow marker at the start
    for (auto& m : diph)
    {
        const auto p = P (m.ph);
        g.setColour (Palette::teal);
        g.drawRect (std::round (p.x) - 3, std::round (p.y) - 3, 6.0f, 6.0f, 1.5f);
        PixelFont::draw (g, codeOf (m.ph), std::round (p.x + m.lx), std::round (p.y + m.ly), 2.0f, Palette::teal);
    }
    for (auto& m : mono)
    {
        const auto p = P (m.ph);
        g.setColour (Palette::teal);
        g.fillRect (std::round (p.x) - 3, std::round (p.y) - 3, 6.0f, 6.0f);
        PixelFont::draw (g, codeOf (m.ph), std::round (p.x + m.lx), std::round (p.y + m.ly), 2.0f, Palette::teal.brighter (0.2f));
    }

    // trail
    const int n = (int) trail.size();
    for (int i = 1; i < n; ++i)
    {
        const auto& p0 = trail[(size_t) i - 1];
        const auto& p1 = trail[(size_t) i];
        if (p0.v < 0.05f || p1.v < 0.05f) continue;
        const float age = (float) i / (float) n;
        const Colour tc = f3Colour (p1.f3);
        g.setColour (tc.withAlpha (0.15f + 0.85f * age * age));
        pixelLine (g, chartPoint (p0.f1, p0.f2, a), chartPoint (p1.f1, p1.f2, a), kPx);
    }

    // current position: a glowing pixel blob
    auto& s = proc.getSinger();
    const float v = s.vizVoicing.load();
    const float shift = jmax (0.1f, proc.apvts.getRawParameterValue ("formant")->load());
    const auto pt = chartPoint (s.vizF1.load() / shift, s.vizF2.load() / shift, a);
    if (v > 0.05f)
    {
        const float pulse = 0.85f + 0.15f * std::sin ((float) frame * 0.5f);
        const float cx = std::floor (pt.x / kPx) * kPx, cy = std::floor (pt.y / kPx) * kPx;
        for (int ring = 4; ring >= 1; --ring)
        {
            const float r = (float) ring * kPx * 1.6f * pulse;
            const Colour glowC = f3Colour (s.vizF3.load() / shift);
            g.setColour (glowC.withAlpha (0.16f * (5 - ring) * jmin (1.0f, v * 1.5f)));
            g.fillRect (cx - r, cy - r, r * 2 + kPx, r * 2 + kPx);
        }
        g.setColour (Palette::white.withAlpha (jmin (1.0f, v * 1.5f)));
        g.fillRect (cx - kPx, cy - kPx, kPx * 3, kPx * 3);
    }
    drawNoiseCloud (g, pt);

    // bar / beat indicator (top right): one box per beat, the current one lit.
    // While a line break waits for the next bar it shows WAIT.
    {
        const int num = jlimit (1, 16, proc.uiBeatsPerBar.load());
        const int beat = proc.uiBeat.load();
        const bool playing = proc.uiPlaying.load();
        const bool waiting = proc.getSinger().vizWaiting.load();
        const float box = num > 8 ? 6.0f : 8.0f, gap = 3.0f;
        const float boxesW = (float) num * (box + gap) - gap;
        const float bx = a.getRight() - 12 - boxesW, by = a.getY() + 10;
        const float alpha = playing ? 1.0f : 0.45f;
        for (int i = 0; i < num; ++i)
        {
            const Rectangle<float> r { bx + (float) i * (box + gap), by, box, box };
            if (i == beat)
                g.setColour ((i == 0 ? Palette::cream : Palette::orange).withAlpha (alpha));
            else
                g.setColour ((i == 0 ? Palette::teal : Palette::tealDim).withAlpha (alpha));
            g.fillRect (r);
            if (waiting) { g.setColour (Palette::pink.withAlpha (alpha)); g.drawRect (r.expanded (1), 1.0f); }
        }
        const String barText = "BAR " + String (proc.uiBar.load());
        float tx = bx - 8 - PixelFont::width (barText, 1.0f);
        PixelFont::draw (g, barText, tx, by, 1.0f, Palette::cream.withAlpha (0.8f * alpha));
        if (waiting)
        {
            tx -= PixelFont::width ("WAIT ", 1.0f) + 4;
            PixelFont::draw (g, "WAIT", tx, by, 1.0f, Palette::pink);
        }
    }

    // readout

    const String rd = "F1 " + String (roundToInt (s.vizF1.load())) + "  F2 " + String (roundToInt (s.vizF2.load()))
                      + "  F3 " + String (roundToInt (s.vizF3.load()));
    PixelFont::draw (g, rd, a.getRight() - PixelFont::width (rd, 1.0f) - 8, a.getBottom() - 12, 1.0f, Palette::teal);
}

void FormantDaisyEditor::drawHelp (Graphics& g, Rectangle<float> a)
{
    g.setColour (Palette::bg.withAlpha (0.92f));
    g.fillRect (a);
    drawFrame (g, a, Palette::orange, true);
    const char* lines[] = {
        "HOW TO SING",
        "NOTES: ONE SYLLABLE PER NOTE, HOLD IT",
        "TEMPO: HOLD A NOTE, ONE SYLLABLE/DIV",
        "DAI-SY        SPLIT SYLLABLES YOURSELF",
        "[D EY Z IY]   TYPE RAW PHONEMES",
        "CEILING = S IY | L IH NG  ON ITS OWN",
        "  LINE SETS HOW A WORD IS SAID",
        ", . ! ? = ONE REST NOTE EACH",
        "NEW LINE = WAIT FOR THE NEXT BAR",
        "ABC = SPELL CAPS ACRONYMS (PHP)",
        "LEAD = CONSONANTS START EARLY SO",
        "  VOWELS LAND ON THE BEAT",
    };
    const float step = jmin (18.0f, (a.getHeight() - 22.0f) / (float) std::size (lines));
    float y = a.getY() + 12;
    for (auto* l : lines)
    {
        PixelFont::draw (g, l, a.getX() + 16, y, 2.0f, String (l) == "HOW TO SING" ? Palette::orange : Palette::cream);
        y += step;
    }
}

void FormantDaisyEditor::drawSpectrum (Graphics& g, Rectangle<float> a)
{
    g.setColour (Palette::bg.withAlpha (0.85f));
    g.fillRect (a);
    drawFrame (g, a, Palette::teal, false);

    auto& s = proc.getSinger();
    const float f1 = s.vizF1.load(), f2 = s.vizF2.load(), f3 = s.vizF3.load();
    const auto inner = a.reduced (6, 6).withTrimmedTop (14);
    const float bw = inner.getWidth() / numBars;
    const float maxHz = 5000.0f;

    for (int b = 0; b < numBars; ++b)
    {
        const float hz = (b + 0.5f) * maxHz / numBars;
        Colour c = Palette::blueDim;
        if (std::abs (hz - f1) < jmax (130.0f, f1 * 0.3f)) c = Palette::orange;
        else if (std::abs (hz - f2) < 220.0f) c = Palette::blue;
        else if (std::abs (hz - f3) < 220.0f) c = Palette::lavender;
        const float h = std::floor (bars[(size_t) b] * inner.getHeight() / kPx) * kPx;
        const float x = inner.getX() + b * bw;
        for (float y = 0; y < h; y += kPx)
        {
            g.setColour (c.withAlpha (0.55f + 0.45f * (y / jmax (1.0f, inner.getHeight()))));
            g.fillRect (std::round (x), std::round (inner.getBottom() - y - kPx), jmax (2.0f, std::round (bw) - 2.0f), kPx - 1.0f);
        }
        // little peak cap
        if (h > 0)
        {
            g.setColour (c.brighter (0.4f));
            g.fillRect (std::round (x), std::round (inner.getBottom() - h - kPx - 1), jmax (2.0f, std::round (bw) - 2.0f), 2.0f);
        }
    }


    // legend
    const String sq = String::charToString ((juce_wchar) 3);
    const std::pair<const char*, Colour> keys[] = { { "F1", Palette::orange }, { "F2", Palette::blue }, { "F3", Palette::lavender } };
    float kx = a.getRight() - 92;
    for (auto& [txt, col] : keys)
    {
        PixelFont::draw (g, sq, kx, a.getY() + 6, 1.0f, col);
        PixelFont::draw (g, txt, kx + 8, a.getY() + 6, 1.0f, Palette::cream);
        kx += 28;
    }
}

void FormantDaisyEditor::drawWaveform (Graphics& g, Rectangle<float> a)
{
    g.setColour (Palette::bg.withAlpha (0.85f));
    g.fillRect (a);
    drawFrame (g, a, Palette::teal, false);

    // stabilise on a rising zero crossing
    const int span = 900;
    int start = fftSize - span - 1;
    for (int i = fftSize - span - 1; i > fftSize / 4; --i)
        if (recent[(size_t) i - 1] < 0.0f && recent[(size_t) i] >= 0.0f) { start = i; break; }

    const auto inner = a.reduced (6, 6);
    const int cols = (int) (inner.getWidth() / kPx);
    float peak = 0.05f;
    for (int i = 0; i < span; ++i) peak = jmax (peak, std::abs (recent[(size_t) (start + i)]));
    Point<float> prev;
    g.setColour (Palette::tealBright);
    for (int c = 0; c < cols; ++c)
    {
        const float v = recent[(size_t) (start + c * span / cols)] / peak;
        Point<float> p { inner.getX() + c * kPx, inner.getCentreY() - v * inner.getHeight() * 0.45f };
        if (c > 0) pixelLine (g, prev, p, kPx);
        prev = p;
    }
}

void FormantDaisyEditor::drawRobot (Graphics& g, float x, float y)
{
    auto& s = proc.getSinger();
    const float level = jlimit (0.0f, 1.0f, s.vizLevel.load() * 2.5f);
    const float P = robotScale;
    auto cell = [&] (float cx, float cy, float w, float h, Colour c) {
        g.setColour (c);
        g.fillRect (x + cx * P, y + cy * P, w * P, h * P);
    };

    // cables into the spectrum box
    {
        const Point<float> o1 { x + 22 * P, y + 13 * P }, o2 { x + 22 * P, y + 16 * P };
        const Point<float> t1 { specArea.getX(), specArea.getY() + specArea.getHeight() * 0.26f },
                           t2 { specArea.getX(), specArea.getY() + specArea.getHeight() * 0.67f };
        auto cable = [&] (Point<float> a, Point<float> b, Colour c) {
            g.setColour (c);
            Point<float> prev = a;
            for (int i = 1; i <= 24; ++i)
            {
                const float t = (float) i / 24.0f;
                const Point<float> c1 { a.x + 18, a.y }, c2 { b.x - 18, b.y };
                const float u = 1 - t;
                Point<float> p = a * (u * u * u) + c1 * (3 * u * u * t) + c2 * (3 * u * t * t) + b * (t * t * t);
                pixelLine (g, prev, p, 3.0f);
                prev = p;
            }
        };
        cable (o1, t1, Palette::orange);
        cable (o2, t2, Palette::blue);
    }

    // ---- body colours: each robot gets its own tint taken from the theme ----
    Colour base = Palette::wood, dark = Palette::woodDark, light = Palette::woodLight;
    auto tint = [&] (Colour c) { base = c; dark = c.darker (0.9f); light = c.brighter (0.35f); };
    switch (robotStyle)
    {
        case 1: tint (Palette::blueDim); break;
        case 2: tint (Palette::teal.darker (0.4f)); break;
        case 3: tint (Palette::pink.darker (1.0f)); break;
        case 4: tint (Palette::orange.darker (0.6f)); break;
        case 6: tint (Palette::lavender.darker (0.9f)); break;
        case 7: tint (Palette::dim.brighter (0.5f)); break;
        case 8: tint (Palette::blue.darker (1.1f)); break;
        case 9: tint (Palette::tealBright.darker (1.2f)); break;
        case 10: tint (Palette::glow.darker (1.3f)); break;
        default: break; // 0 and 5 keep the wood
    }
    const Colour bulb = Palette::glow.withAlpha (0.25f + 0.75f * jmin (1.0f, s.vizVoicing.load() * 1.5f));
    auto line = [&] (float x0, float y0, float x1, float y1, Colour c) {
        const int n = jmax (1, roundToInt (jmax (std::abs (x1 - x0), std::abs (y1 - y0))));
        for (int i = 0; i <= n; ++i)
            cell (std::round (x0 + (x1 - x0) * (float) i / (float) n), std::round (y0 + (y1 - y0) * (float) i / (float) n), 1, 1, c);
    };

    // ---- head pieces drawn behind the body ----
    int corner = 0; // body corner rounding, in cells
    switch (robotStyle)
    {
        case 1: // TV: rabbit-ear antennas
            line (7, 2, 3, -3, dark);  line (15, 2, 19, -3, dark);
            cell (2, -4, 2, 2, bulb);  cell (19, -4, 2, 2, bulb);
            corner = 2;
            break;
        case 2: // dome with a beacon
            for (int cy = -5; cy < 3; ++cy)
                for (int cx = 0; cx < 22; ++cx)
                {
                    const float dx = (float) cx + 0.5f - 11.0f, dy = (float) cy + 0.5f - 3.0f;
                    const float d = std::sqrt (dx * dx + dy * dy);
                    if (d <= 8.0f) cell ((float) cx, (float) cy, 1, 1, d > 7.0f ? dark : (cy < -2 ? light : base));
                }
            cell (10, -7, 2, 2, bulb);
            break;
        case 3: // boombox: carry handle and knobs
            cell (4, -2, 14, 1, dark); cell (4, -2, 1, 5, dark); cell (17, -2, 1, 5, dark);
            cell (1, 1, 2, 2, Palette::grilleLight); cell (19, 1, 2, 2, bulb);
            corner = 1;
            break;
        case 4: // cat ears
            for (int k = 0; k < 6; ++k)
            {
                const float w = (float) jmax (1, 8 - (k * 8) / 6), y0 = (float) (2 - k);
                cell (0, y0, w, 1, dark);                 // left ear leans out
                cell (22 - w, y0, w, 1, dark);            // right ear
                if (w > 3 && k < 4) { cell (1, y0, w - 3, 1, Palette::pink.withAlpha (0.75f));
                                      cell (22 - w + 2, y0, w - 3, 1, Palette::pink.withAlpha (0.75f)); }
            }
            break;
        case 5: // cathedral radio: arched top with a trim band
            for (int cy = -3; cy < 3; ++cy)
                for (int cx = 0; cx < 22; ++cx)
                {
                    const float dx = (float) cx + 0.5f - 11.0f, dy = (float) cy + 0.5f - 9.0f;
                    const float d = std::sqrt (dx * dx + dy * dy);
                    if (d <= 12.0f) cell ((float) cx, (float) cy, 1, 1, d > 11.0f ? dark : (cy < 0 ? light : base));
                }
            break;
        case 6: // crown of three spikes with glowing tips
            for (int sp = 0; sp < 3; ++sp)
            {
                const float sx = 2.0f + 7.0f * (float) sp;
                for (int k = 0; k < 5; ++k) cell (sx + (float) (k / 2), (float) (2 - k), (float) (5 - 2 * (k / 2)), 1, dark);
                cell (sx + 2, -3, 1, 1, bulb);
            }
            break;
        case 7: // bolted: side bolts and a short stub antenna
            cell (-2, 7, 2, 5, dark); cell (22, 7, 2, 5, dark);
            cell (-3, 8, 1, 3, light); cell (24, 8, 1, 3, light);
            cell (10.5f, 1, 1, 2, dark); cell (10, 0, 2, 1, bulb);
            break;
        case 8: // headphones: band over the top, cups at the sides
            for (int cy = -4; cy < 9; ++cy)
                for (int cx = -2; cx < 24; ++cx)
                {
                    const float dx = (float) cx + 0.5f - 11.0f, dy = (float) cy + 0.5f - 9.0f;
                    const float d = std::sqrt (dx * dx + dy * dy);
                    if (d >= 11.6f && d <= 13.0f) cell ((float) cx, (float) cy, 1, 1, Palette::dim.brighter (0.6f));
                }
            corner = 1;
            break;
        case 9: // propeller beanie (the propeller spins with the voice)
        {
            const float spin = std::abs (std::sin ((float) frame * 0.6f * (0.3f + 2.0f * level)));
            const float bw = 1.0f + 6.0f * spin;
            // striped beanie dome sitting on the head
            for (int cy = -2; cy < 4; ++cy)
                for (int cx = 0; cx < 22; ++cx)
                {
                    const float dx = (float) cx + 0.5f - 11.0f, dy = ((float) cy + 0.5f - 4.0f) * 1.6f;
                    if (dx * dx + dy * dy <= 121.0f)
                        cell ((float) cx, (float) cy, 1, 1, ((cx / 3) % 2) == 0 ? Palette::orange.darker (0.3f) : Palette::blue.darker (0.4f));
                }
            cell (10.5f, -5, 1, 3, dark);
            cell (11.0f - bw, -6, bw, 2, Palette::orange); cell (12.0f, -6, bw, 2, Palette::blue);
            cell (10, -6, 3, 2, dark);
            break;
        }
        case 10: // bug: curly antennae with bulbs
            line (6, 2, 4, -1, dark); line (4, -1, 2, -2, dark);
            line (16, 2, 18, -1, dark); line (18, -1, 20, -2, dark);
            cell (0, -4, 2, 2, bulb); cell (20, -4, 2, 2, bulb);
            corner = 3;
            break;
        default: // classic: single antenna with a bulb that lights when voiced
            cell (10.5f, 0, 1, 3, dark);
            cell (10, -1, 2, 1, bulb);
            break;
    }

    // ---- body (same footprint for every robot, optional rounded corners) ----
    for (int j = 0; j < 24; ++j)
    {
        const int edge = jmin (j, 23 - j);
        const int inset = edge < corner ? corner - edge : 0;
        const float x0 = (float) inset, w = 22.0f - 2.0f * (float) inset;
        cell (x0, (float) (3 + j), w, 1, dark);
        if (j > 0 && j < 23 && w > 2)
            cell (x0 + 1, (float) (3 + j), w - 2, 1, j <= 2 ? light : base);
        if (j > 2 && j < 23)
            cell (x0 + 1, (float) (3 + j), 1, 1, light.withAlpha (0.6f));
    }

    // ---- extra details over the body ----
    switch (robotStyle)
    {
        case 3: for (int k = 0; k < 4; ++k) cell (6.0f + 3.0f * (float) k, 4, 2, 1, dark); break;     // vent slots
        case 5: cell (1, 5, 20, 1, Palette::glow.withAlpha (0.8f)); break;                         // gold trim
        case 7: for (int k = 0; k < 5; ++k) cell (4.0f + 3.0f * (float) k, 4, 2, 1, dark); break;     // vents
        case 8: cell (-3, 6, 3, 7, Palette::dim.brighter (0.4f)); cell (22, 6, 3, 7, Palette::dim.brighter (0.4f));
                cell (-2, 7, 1, 5, Palette::teal); cell (23, 7, 1, 5, Palette::teal); break;          // ear cups
        case 9: cell (0, 5, 22, 1, dark); break; // beanie brim
        default: break;
    }

    // eyes: blink on each rest
    const bool blink = blinkFrames > 1;
    for (float ex : { 4.0f, 14.0f })
    {
        if (blink)
            cell (ex, 9, 4, 1, Palette::blue);
        else
        {
            cell (ex, 7, 4, 4, Palette::blue);
            cell (ex + 1, 8, 2, 2, Palette::pupil);
            cell (ex, 7, 1, 1, Palette::white.withAlpha (0.8f));
        }
    }

    // speaker grille: a round grid of holes that glows with the voice
    const float gcx = 11, gcy = 18.5f, R = 6.2f;
    for (int iy = -7; iy <= 7; ++iy)
        for (int ix = -7; ix <= 7; ++ix)
        {
            const float d = std::sqrt ((float) (ix * ix + iy * iy));
            if (d > R) continue;
            Colour c = Palette::grilleDark;
            if (d > R - 1.0f) c = dark;
            else if (((ix + iy) & 1) == 0) c = Palette::grilleLight;
            // the mouth: an ellipse, F2 = width, F1 = height, brightness = loudness
            if (mouthW > 0.4f && mouthH > 0.4f && d <= R - 1.0f)
            {
                const float ex = (float) ix / mouthW, ey = (float) iy / mouthH;
                const float e = ex * ex + ey * ey;
                if (e <= 1.0f)
                    c = Palette::mouth.withAlpha (jlimit (0.35f, 1.0f, 0.4f + 0.6f * level) * (1.0f - 0.35f * e));
            }
            cell (gcx + (float) ix - 0.5f, gcy + (float) iy - 0.5f, 1, 1, c);
        }

    // feet
    cell (3, 27, 4, 2, dark);
    cell (15, 27, 4, 2, dark);

    // name tag
    const Rectangle<float> tag { x - 6, y + 30 * P, 22 * P + 12, P >= 6.0f ? 30.0f : 22.0f };
    g.setColour (Palette::orange);
    g.fillRect (tag);
    g.setColour (Palette::woodDark);
    g.drawRect (tag, 2.0f);
    const String name = "DAISY-" + String (robotStyle + 1);
    float ts = P >= 6.0f ? 3.0f : 2.0f;
    while (ts > 1.0f && PixelFont::width (name, ts) > tag.getWidth() - 8.0f) ts -= 0.5f;
    PixelFont::drawCentred (g, name, tag, ts, Palette::woodDark);
}

void FormantDaisyEditor::drawControlsText (Graphics& g)
{
    const float x = 568;
    PixelFont::draw (g, "LYRICS", x, 20, 2.0f, Palette::cream);
    if (lyrics)
    {
        // length in notes (Notes mode) or steps (Tempo mode), and the bars that takes
        // at the DIV note value in the host's time signature
        const bool tempo = modeValue > 0.5f;
        static const double divQuarters[] = { 1.0, 0.5, 1.0 / 3.0, 0.25 };
        const double qpb = proc.uiBeatsPerBar.load() * 4.0 / jmax (1, proc.uiBeatUnit.load());
        const double stepsPerBar = qpb / divQuarters[jlimit (0, 3, roundToInt (divValue))];
        const int bars = lyrics->barLength (stepsPerBar, tempo);
        const String count = "LENGTH: " + String (lyrics->stepLength (tempo)) + " (" + String (bars)
                             + (bars == 1 ? " BAR)" : " BARS)");
        PixelFont::draw (g, count, x + 388 - PixelFont::width (count, 2.0f), 20, 2.0f, Palette::teal);
    }

    for (auto& k : knobs)
    {
        const auto b = k->slider.getBounds().toFloat();
        PixelFont::draw (g, k->label, x, b.getY(), 2.0f, Palette::cream);
        String val;
        if (auto* p = proc.apvts.getParameter (k->id))
        {
            if (k->id == "source")
            {
                const float v = (float) k->slider.getValue();
                val = v < 0.15f ? "BUZZ" : (v > 0.85f ? "GLOT" : String (roundToInt (v * 100)) + "%");
            }
            else
            {
                val = p->getCurrentValueAsText().toUpperCase().removeCharacters (" ");
                if (auto* rp = dynamic_cast<RangedAudioParameter*> (p))
                {
                    const String lab = rp->getLabel().toUpperCase();
                    if (lab.isNotEmpty() && ! val.endsWith (lab)) val << lab;
                }
            }
        }
        PixelFont::draw (g, val.substring (0, 7), b.getRight() + 10, b.getY(), 2.0f, Palette::orange);
    }

    // audio input meter beside the CARRIER button
    {
        const auto b = carrierBtn.getBounds().toFloat();
        const float lvl = jlimit (0.0f, 1.0f, std::sqrt (proc.uiInputLevel.load()));
        const float mx = b.getRight() + 12, my = b.getY() + 6;
        PixelFont::draw (g, "IN", mx, my + 1, 2.0f, Palette::cream);
        const int n = 12, filled = roundToInt (lvl * (float) n);
        for (int i = 0; i < n; ++i)
        {
            g.setColour (i < filled ? (proc.uiCarrierActive.load() ? Palette::orange : Palette::teal) : Palette::tealDim);
            g.fillRect (mx + 30 + (float) i * 8.0f, my, 6.0f, 14.0f);
        }
    }

    // footer
    PixelFont::draw (g, "FORMANT DAISY", 24, kHeight - 22, 2.0f, Palette::teal);
    PixelFont::draw (g, "V" JucePlugin_VersionString "  FORMANT SYNTHESIS", 24 + PixelFont::width ("FORMANT DAISY ", 2.0f) + 6, kHeight - 22, 2.0f,
                     Palette::dim.brighter (0.3f));
    const String latency = "LATENCY " + String (roundToInt (proc.getLatencySamples() * 1000.0 / proc.getSampleRateForUI())) + "MS";
    PixelFont::draw (g, latency, 956 - PixelFont::width (latency, 2.0f), kHeight - 22, 2.0f, Palette::dim.brighter (0.3f));
}
