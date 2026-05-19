#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/GlassmorphicLookAndFeel.h"
#include "UI/WaveformVisualizer.h"

namespace ingen {

class InGenSamplerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         public juce::Slider::Listener,
                                         public juce::Button::Listener
{
public:
    InGenSamplerAudioProcessorEditor (InGenSamplerAudioProcessor&);
    ~InGenSamplerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Slider listener callback
    void sliderValueChanged (juce::Slider* slider) override;

    // Button listener callback
    void buttonClicked (juce::Button* button) override;

private:
    void triggerSampleGeneration();
    void updateAdsrFromSliders();

    InGenSamplerAudioProcessor& audioProcessor;

    // Premium LookAndFeel design system
    GlassmorphicLookAndFeel glassLookAndFeel;

    // Interactive Waveform Visualizer
    WaveformVisualizer waveformVisualizer;

    // UI Widgets
    juce::TextEditor promptEditor;
    juce::TextButton generateButton;

    // ADSR Knobs (Tonal Layer A)
    juce::Slider attackKnob;
    juce::Slider decayKnob;
    juce::Slider sustainKnob;
    juce::Slider releaseKnob;

    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;

    // Layer Settings Toggles
    juce::TextButton presetLinkButton;  // Connect/Disconnect Layer A and B
    juce::TextButton foleyModeButton;    // Alternate trigger mode "Note-On" / "Note-Off"

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InGenSamplerAudioProcessorEditor)
};

} // namespace ingen
