#include "GlassmorphicLookAndFeel.h"

namespace ingen {

// Define modern cyber-neon color constants
const juce::Colour GlassmorphicLookAndFeel::bgBase       { 0xFF0B0B10 }; // Deep Obsidian
const juce::Colour GlassmorphicLookAndFeel::bgGlass      { 0x991E1E2C }; // Frosted dark glass
const juce::Colour GlassmorphicLookAndFeel::neonCyan     { 0xFF00F0FF }; // Electric Cyan
const juce::Colour GlassmorphicLookAndFeel::neonMagenta  { 0xFFFF007F }; // Hot Pink
const juce::Colour GlassmorphicLookAndFeel::textLight    { 0xFFF5F5FA }; // Ice White
const juce::Colour GlassmorphicLookAndFeel::textMuted    { 0xFF7E7E94 }; // Steel Grey
const juce::Colour GlassmorphicLookAndFeel::borderGlass  { 0x22FFFFFF }; // Soft glass frosted border

GlassmorphicLookAndFeel::GlassmorphicLookAndFeel()
{
    // Global standard styling definitions
    setColour (juce::Label::textColourId, textLight);
    setColour (juce::TextButton::textColourOffId, textLight);
    setColour (juce::TextButton::textColourOnId, textLight);
    
    // Sliders
    setColour (juce::Slider::thumbColourId, neonCyan);
    setColour (juce::Slider::rotarySliderFillColourId, neonCyan);
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0x1CFFFFFF));
    setColour (juce::Slider::textBoxTextColourId, textLight);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    // Text editors
    setColour (juce::TextEditor::backgroundColourId, bgGlass);
    setColour (juce::TextEditor::textColourId, textLight);
    setColour (juce::TextEditor::highlightColourId, juce::Colour (0x4000F0FF));
    setColour (juce::TextEditor::focusedOutlineColourId, neonCyan);
    setColour (juce::TextEditor::outlineColourId, borderGlass);
}

void GlassmorphicLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                                float sliderPosProportionString, float rotaryStartAngle, float rotaryEndAngle,
                                                juce::Slider& slider)
{
    // High-precision geometric layouts
    const float radius = juce::jmin (width / 2.0f, height / 2.0f) - 6.0f;
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = rotaryStartAngle + sliderPosProportionString * (rotaryEndAngle - rotaryStartAngle);

    // 1. Draw static background circular arc track
    g.setColour (slider.findColour (juce::Slider::rotarySliderOutlineColourId));
    juce::Path backgroundTrack;
    backgroundTrack.addCentredArc (centreX, centreY, radius - 2.0f, radius - 2.0f,
                                   0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.strokePath (backgroundTrack, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 2. Draw active glowing slider value arc
    // We alternate colors between Neon Cyan and Neon Magenta depending on the slider ID/Name
    bool isFoleySlider = slider.getName().containsIgnoreCase ("foley") || slider.getName().containsIgnoreCase ("layerB");
    juce::Colour activeColor = isFoleySlider ? neonMagenta : neonCyan;

    g.setColour (activeColor);
    juce::Path activeTrack;
    activeTrack.addCentredArc (centreX, centreY, radius - 2.0f, radius - 2.0f,
                               0.0f, rotaryStartAngle, angle, true);
    
    // Add micro-glow by drawing a wider translucent stroke underneath
    g.setColour (activeColor.withAlpha (0.15f));
    g.strokePath (activeTrack, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    
    g.setColour (activeColor);
    g.strokePath (activeTrack, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 3. Draw frosted dial thumb wheel
    g.setColour (bgGlass);
    g.fillEllipse (rx + 4.0f, ry + 4.0f, rw - 8.0f, rw - 8.0f);
    
    g.setColour (borderGlass);
    g.drawEllipse (rx + 4.0f, ry + 4.0f, rw - 8.0f, rw - 8.0f, 1.0f);

    // 4. Draw pointer line (neon dot representing value dial angle)
    juce::Path pointer;
    const float pointerLength = radius - 8.0f;
    const float pointerThickness = 3.0f;
    
    pointer.addRectangle (-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength * 0.6f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
    
    g.setColour (activeColor);
    g.fillPath (pointer);
}

void GlassmorphicLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                    const juce::Colour& backgroundColour,
                                                    bool shouldDrawButtonAsHighlighted,
                                                    bool shouldDrawButtonAsDown)
{
    const auto cornerSize = 6.0f;
    const auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);

    // Anti-aliasing is enabled by default in JUCE graphics contexts

    // Determine custom neon background colors based on hover states
    juce::Colour fillColour = bgGlass;
    juce::Colour borderColour = borderGlass;

    if (shouldDrawButtonAsDown)
    {
        fillColour = neonCyan.withAlpha (0.4f);
        borderColour = neonCyan;
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        fillColour = bgGlass.withAlpha (0.9f);
        borderColour = neonCyan.withAlpha (0.8f);
    }
    else if (backgroundColour != juce::Colours::transparentBlack)
    {
        fillColour = backgroundColour.withAlpha (0.5f);
        borderColour = backgroundColour;
    }

    // Draw main button chassis
    g.setColour (fillColour);
    g.fillRoundedRectangle (bounds, cornerSize);

    // Draw high quality frosted neon border
    g.setColour (borderColour);
    g.drawRoundedRectangle (bounds, cornerSize, 1.2f);
}

void GlassmorphicLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    g.setFont (juce::Font (13.0f, juce::Font::bold));
    
    // Glowing text depending on state
    if (shouldDrawButtonAsDown)
        g.setColour (textLight);
    else if (shouldDrawButtonAsHighlighted)
        g.setColour (neonCyan);
    else
        g.setColour (textLight.withAlpha (0.9f));

    const int yIndent = button.getProperties().contains ("yIndent") ? static_cast<int> (button.getProperties()["yIndent"]) : 0;
    
    g.drawFittedText (button.getButtonText(),
                      button.getLocalBounds().withY (button.getLocalBounds().getY() + yIndent),
                      juce::Justification::centred, 2);
}

void GlassmorphicLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int width, int height,
                                                    juce::TextEditor& textEditor)
{
    if (textEditor.hasKeyboardFocus (true))
    {
        g.setColour (neonCyan);
        g.drawRoundedRectangle (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height), 6.0f, 1.5f);
    }
    else
    {
        g.setColour (borderGlass);
        g.drawRoundedRectangle (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height), 6.0f, 1.0f);
    }
}

void GlassmorphicLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int width, int height,
                                                        juce::TextEditor& textEditor)
{
    juce::ignoreUnused (textEditor);
    g.setColour (bgGlass);
    g.fillRoundedRectangle (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height), 6.0f);
}

} // namespace ingen
