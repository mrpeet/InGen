#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace ingen {

/**
 * Custom modern LookAndFeel targeting glassmorphism, dynamic gradients,
 * neon highlights, and premium aesthetic vector widgets.
 */
class GlassmorphicLookAndFeel : public juce::LookAndFeel_V4 {
public:
    GlassmorphicLookAndFeel();
    ~GlassmorphicLookAndFeel() override = default;

    // Custom UI Color Constants (Neon Cyber Palette)
    static const juce::Colour bgBase;       // Deep Indigo-charcoal (#0F0F16)
    static const juce::Colour bgGlass;      // Translucent Frost (#1C1C28BF)
    static const juce::Colour neonCyan;     // Active Primary (#00F0FF)
    static const juce::Colour neonMagenta;  // Secondary Accents (#FF007F)
    static const juce::Colour textLight;    // Crisp Typography (#F0F0F8)
    static const juce::Colour textMuted;    // Secondary Details (#8A8A9E)
    static const juce::Colour borderGlass;  // Frosted edge glow (#30FFFFFF)

    // Override dial rendering for premium neon circular envelopes
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportionString, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;

    // Override standard button background
    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    // Override button text colors
    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    // Override TextEditor (Prompt search bars)
    void drawTextEditorOutline (juce::Graphics& g, int width, int height,
                                juce::TextEditor& textEditor) override;

    void fillTextEditorBackground (juce::Graphics& g, int width, int height,
                                   juce::TextEditor& textEditor) override;
};

} // namespace ingen
