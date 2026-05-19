#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/GlassmorphicLookAndFeel.h"
#include "UI/WaveformVisualizer.h"
#include "Network/LocalPythonServerClient.h"
#include "Serialization/PresetArchiver.h"
#include "Serialization/PresetSerializer.h"
#include "DSP/SampleAnalysisJob.h"
#include <juce_audio_formats/juce_audio_formats.h>

namespace ingen {

class InGenSamplerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         public juce::Slider::Listener,
                                         public juce::Button::Listener,
                                         public juce::Timer
{
public:
    InGenSamplerAudioProcessorEditor (InGenSamplerAudioProcessor&);
    ~InGenSamplerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Timer callback for background health and LED polling
    void timerCallback() override;

    // Slider listener callback
    void sliderValueChanged (juce::Slider* slider) override;

    // Button listener callback
    void buttonClicked (juce::Button* button) override;

private:
    void triggerSampleGeneration();
    void updateAdsrFromSliders();
    void updateLoopFromSliders();

    InGenSamplerAudioProcessor& audioProcessor;

    // Premium LookAndFeel design system
    GlassmorphicLookAndFeel glassLookAndFeel;

    // Interactive Waveform Visualizer
    WaveformVisualizer waveformVisualizer;

    // Real API client and format helper
    LocalPythonServerClient generatorAPI;
    juce::AudioFormatManager formatManager;

    // Cached generation files
    juce::File activeSampleA;
    std::vector<juce::File> activeSamplesB;
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Real-time Dynamic Status Log & Progress Bar
    float generationProgress = 0.0f;
    juce::String statusLogMessage;
    juce::Colour progressColour;

    // UI Widgets
    juce::TextEditor promptEditor;
    juce::TextButton generateButton;
    juce::TextButton launchServerButton;

    // Header Import/Export Buttons
    juce::TextButton importButton;
    juce::TextButton exportButton;

    // Generation Parameter Settings
    juce::Slider noteCountSlider;
    juce::Slider octavesSlider;
    juce::Label noteCountLabel;
    juce::Label octavesLabel;

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
    juce::TextButton loopModeButton;     // Loop mode toggle: Off, Forward, PingPong
    juce::TextButton reverseButton;      // Reverse playback toggle: On/Off
    juce::Slider loopCrossfadeSlider;
    juce::Label loopCrossfadeLabel;

    // Model Selectors
    juce::ComboBox tonalModelSelector;
    juce::ComboBox foleyModelSelector;
    juce::Label tonalModelLabel;
    juce::Label foleyModelLabel;

    // Layer Volume Controls
    juce::Slider layerAVolumeSlider;
    juce::Slider layerBVolumeSlider;
    juce::Label layerAVolumeLabel;
    juce::Label layerBVolumeLabel;

    juce::String getModelStringFromId (int id)
    {
        switch (id)
        {
            case 2:  return "stable-audio-open";
            case 3:  return "magnet-small";
            case 4:  return "woosh-dflow";
            default: return "audiogen-medium";
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InGenSamplerAudioProcessorEditor)
};

} // namespace ingen
