#pragma once

#include "PluginProcessor.h"
#include "PixelFont.h"
#include <deque>

namespace Palette
{
// Mutable so a colour theme can be applied (see applyTheme). Default = "Daisy" theme.
inline juce::Colour bg { 0xff0a0c18 };
inline juce::Colour panel { 0xff0e1324 };
inline juce::Colour teal { 0xff2b8c84 };
inline juce::Colour tealDim { 0xff17403f };
inline juce::Colour tealBright { 0xff4fd1b0 };
inline juce::Colour cream { 0xfff6e7b0 };
inline juce::Colour orange { 0xfff2a65a };
inline juce::Colour blue { 0xff6ec6ff };
inline juce::Colour blueDim { 0xff2c5a86 };
inline juce::Colour lavender { 0xffb48cff };
inline juce::Colour pink { 0xfff7a8c4 };
inline juce::Colour dim { 0xff3a405c };
inline juce::Colour wood { 0xff8a5530 };
inline juce::Colour woodDark { 0xff4a2c18 };
inline juce::Colour woodLight { 0xffb07a45 };
inline juce::Colour star { 0xff8fa3ff };
inline juce::Colour glow { 0xffffd27a };
inline juce::Colour mouth { 0xffffe08a };
inline juce::Colour grilleDark { 0xff1d2a22 };
inline juce::Colour grilleLight { 0xff2f6b5a };
inline juce::Colour pupil { 0xff10304a };
inline juce::Colour white { 0xffffffff };

// 0 = Daisy (default), 1 = Amber Terminal, 2 = Pocket Green, 3 = Synthwave
void applyTheme (int id);
} // namespace Palette

class PixelLookAndFeel : public juce::LookAndFeel_V4
{
public:
    int themeId = 0;
    void updateColours();
    PixelLookAndFeel();
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos, float minSliderPos,
                           float maxSliderPos, juce::Slider::SliderStyle, juce::Slider&) override;
};

class PixelButton : public juce::Component
{
public:
    std::function<juce::String()> getText;
    std::function<bool()> isActive;
    std::function<void()> onClick;
    bool subtle = false; // dim until hovered (used for the view toggle)
    const int* theme = nullptr; // owning editor's colour theme

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override { down = true; repaint(); }
    void mouseUp (const juce::MouseEvent& e) override
    {
        down = false;
        repaint();
        if (contains (e.getPosition()) && onClick) onClick();
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }

private:
    bool down = false, hover = false;
};

class FormantDaisyEditor : public juce::AudioProcessorEditor,
                           private juce::Timer,
                           private juce::TextEditor::Listener
{
public:
    explicit FormantDaisyEditor (FormantDaisyProcessor&);
    ~FormantDaisyEditor() override;

    void paint (juce::Graphics&) override;
    void setCompact (bool shouldBeCompact); // 9:16 view with just the graphs and Daisy
    void setTheme (int id);
    void setRobot (int style);
    void setShowHelp (bool show) { showHelp = show; repaint(); }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    static constexpr int numRobots = 11, numThemes = 4;
    void resized() override;

private:
    void timerCallback() override;
    void textEditorTextChanged (juce::TextEditor&) override;

    // drawing
    void drawBackground (juce::Graphics&);
    void drawTitle (juce::Graphics&);
    void drawVowelChart (juce::Graphics&, juce::Rectangle<float>);
    void drawLinePhonemes (juce::Graphics&, juce::Rectangle<float>, int posIdx);
    void drawSpectrum (juce::Graphics&, juce::Rectangle<float>);
    void drawWaveform (juce::Graphics&, juce::Rectangle<float>);
    void drawRobot (juce::Graphics&, float x, float y);
    void drawControlsText (juce::Graphics&);
    void drawHelp (juce::Graphics&, juce::Rectangle<float>);
    void drawFrame (juce::Graphics&, juce::Rectangle<float>, juce::Colour, bool brackets);
    void pixelLine (juce::Graphics&, juce::Point<float>, juce::Point<float>, float size);
    juce::Point<float> chartPoint (float f1, float f2, juce::Rectangle<float> area) const;

    FormantDaisyProcessor& proc;
    PixelLookAndFeel lnf;

    juce::TextEditor lyricsBox;
    PixelButton daisyBtn, restartBtn, loopBtn, helpBtn, resetBtn, modeBtn, divBtn, syncBtn, spellBtn, viewBtn, themeBtn, carrierBtn;
    std::unique_ptr<juce::ParameterAttachment> modeAtt, divAtt, loopAtt, syncAtt, spellAtt, carrierAtt;
    float modeValue = 0, divValue = 1, loopValue = 1, syncValue = 1, spellValue = 1, carrierValue = 0;

    struct Knob
    {
        juce::String id, label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    std::vector<std::unique_ptr<Knob>> knobs;

    // visual state
    struct TrailPt { float f1, f2, v; };
    std::deque<TrailPt> trail;
    std::deque<juce::String> ticker;
    int lastRestCount = 0, blinkFrames = 0, pendingBlinks = 0; // robot blinks once per rest
    float mouthW = 0.0f, mouthH = 0.0f; // robot mouth half-size in grille cells (F2 = width, F1 = height)
    int lastPhoneme = -1;
    std::shared_ptr<const fd::Lyrics> lyrics;
    int lastLyricsVersion = -1;
    int textDebounce = -1;
    bool showHelp = false;
    bool compact = false;
    int themeId = 0, robotStyle = 0;
    juce::Rectangle<float> robotHitArea() const;
    float robotScale = 4.0f;
    int frame = 0;

    static constexpr int fftOrder = 11, fftSize = 1 << fftOrder, numBars = 56;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> fftData, recent;
    std::array<float, numBars> bars {};

    struct Star { float x, y, phase; int size; };
    std::vector<Star> stars;

    // layout
    juce::Rectangle<float> titleArea, chartArea, specArea, waveArea;
    juce::Point<float> robotPos;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FormantDaisyEditor)
};
