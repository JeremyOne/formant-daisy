// A tiny hand-drawn 5x7 bitmap font for the retro look. Each glyph pixel is
// drawn as a square of `scale` screen pixels. Lowercase maps to uppercase.
// Special characters: \x01 = left arrow, \x02 = down arrow, \x03 = solid square.
#pragma once
#include <juce_graphics/juce_graphics.h>

namespace PixelFont
{
void draw (juce::Graphics& g, const juce::String& text, float x, float y, float scale, juce::Colour colour);
void drawCentred (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, float scale, juce::Colour colour);
float width (const juce::String& text, float scale);
inline float height (float scale) { return 7.0f * scale; }
} // namespace PixelFont
