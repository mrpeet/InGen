#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ingen {

InGenSamplerAudioProcessorEditor::InGenSamplerAudioProcessorEditor (InGenSamplerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), waveformVisualizer()
{
    // 1. Activate custom glassmorphic LookAndFeel
    setLookAndFeel (&glassLookAndFeel);

    // 2. Setup the Interactive Waveform Visualizer
    addAndMakeVisible (waveformVisualizer);
    
    // Bind active sound if already available in engine
    if (auto* soundA = audioProcessor.getSamplerEngine().getActiveTonalSound())
    {
        waveformVisualizer.setSound (soundA);
    }
    
    // Repaint on manual crop changes
    waveformVisualizer.onCropChanged = [this] (int start, int end) {
        juce::ignoreUnused (start, end);
        updateAdsrFromSliders();
    };

    // 3. Configure Prompt Text Editor
    addAndMakeVisible (promptEditor);
    promptEditor.setTextToShowWhenEmpty (TRANS ("Beschreibe deinen Sound... (z.B. 'Warm 80s Analog Lead')"),
                                         GlassmorphicLookAndFeel::textMuted);
    
    // 4. Configure Generate Button
    addAndMakeVisible (generateButton);
    generateButton.setButtonText (TRANS ("GENERATE"));
    generateButton.addListener (this);

    // 5. Configure ADSR Envelope Knobs (Tonal Layer A)
    // Attack
    attackKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    attackKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 15);
    attackKnob.setRange (0.001, 5.0, 0.001); // 1ms to 5s
    attackKnob.setValue (0.01);
    attackKnob.setName ("layerA_attack");
    attackKnob.addListener (this);
    addAndMakeVisible (attackKnob);
    
    attackLabel.setText (TRANS ("ATTACK"), juce::dontSendNotification);
    attackLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (attackLabel);

    // Decay
    decayKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    decayKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 15);
    decayKnob.setRange (0.01, 10.0, 0.01); // 10ms to 10s
    decayKnob.setValue (0.2);
    decayKnob.setName ("layerA_decay");
    decayKnob.addListener (this);
    addAndMakeVisible (decayKnob);

    decayLabel.setText (TRANS ("DECAY"), juce::dontSendNotification);
    decayLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (decayLabel);

    // Sustain
    sustainKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    sustainKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 15);
    sustainKnob.setRange (0.0, 1.0, 0.01); // 0% to 100%
    sustainKnob.setValue (0.8);
    sustainKnob.setName ("layerA_sustain");
    sustainKnob.addListener (this);
    addAndMakeVisible (sustainKnob);

    sustainLabel.setText (TRANS ("SUSTAIN"), juce::dontSendNotification);
    sustainLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (sustainLabel);

    // Release
    releaseKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    releaseKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 15);
    releaseKnob.setRange (0.01, 10.0, 0.01); // 10ms to 10s
    releaseKnob.setValue (0.5);
    releaseKnob.setName ("layerA_release");
    releaseKnob.addListener (this);
    addAndMakeVisible (releaseKnob);

    releaseLabel.setText (TRANS ("RELEASE"), juce::dontSendNotification);
    releaseLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (releaseLabel);

    // 6. Configure Layer Toggle Buttons
    addAndMakeVisible (presetLinkButton);
    presetLinkButton.setButtonText (TRANS ("LINKED LAYERS"));
    presetLinkButton.addListener (this);

    addAndMakeVisible (foleyModeButton);
    foleyModeButton.setButtonText (TRANS ("FOLEY: NOTE-ON"));
    foleyModeButton.addListener (this);

    // Establish standard premium plugin dimension ratio
    setSize (850, 480);
}

InGenSamplerAudioProcessorEditor::~InGenSamplerAudioProcessorEditor()
{
    // De-register custom LookAndFeel on destruction
    setLookAndFeel (nullptr);
}

void InGenSamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Draw base Obsidian space background
    g.fillAll (GlassmorphicLookAndFeel::bgBase);

    // Draw header glowing typography
    g.setColour (GlassmorphicLookAndFeel::textLight);
    g.setFont (juce::Font (20.0f, juce::Font::bold));
    g.drawText ("InGen Sampler", 20, 15, 200, 30, juce::Justification::left);

    g.setColour (GlassmorphicLookAndFeel::textMuted);
    g.setFont (juce::Font (12.0f, juce::Font::plain));
    g.drawText ("AI-POWERED MULTI-LAYER SAMPLING ENGINE", 165, 21, 300, 20, juce::Justification::left);
}

void InGenSamplerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    
    // 1. Margins reduction
    area.removeFromTop (50); // Header gap
    area.reduce (20, 15);

    // 2. Search / Generation bar
    auto searchArea = area.removeFromTop (45);
    promptEditor.setBounds (searchArea.removeFromLeft (static_cast<int> (area.getWidth() * 0.78f)).reduced (0, 4));
    generateButton.setBounds (searchArea.reduced (5, 4));

    area.removeFromTop (10); // Spacing gap

    // 3. Main Waveform Screen
    waveformVisualizer.setBounds (area.removeFromTop (170));

    area.removeFromTop (15); // Spacing gap

    // 4. Envelope & Mechanical Controls Screen
    auto controlsArea = area;
    
    // Allocate ADSR Left Panel
    auto adsrArea = controlsArea.removeFromLeft (static_cast<int> (controlsArea.getWidth() * 0.65f));
    int knobW = adsrArea.getWidth() / 4;
    
    // Layout 4 knobs sequentially
    auto attackArea = adsrArea.removeFromLeft (knobW);
    attackLabel.setBounds (attackArea.removeFromTop (15));
    attackKnob.setBounds (attackArea.reduced (5));

    auto decayArea = adsrArea.removeFromLeft (knobW);
    decayLabel.setBounds (decayArea.removeFromTop (15));
    decayKnob.setBounds (decayArea.reduced (5));

    auto sustainArea = adsrArea.removeFromLeft (knobW);
    sustainLabel.setBounds (sustainArea.removeFromTop (15));
    sustainKnob.setBounds (sustainArea.reduced (5));

    auto releaseArea = adsrArea;
    releaseLabel.setBounds (releaseArea.removeFromTop (15));
    releaseKnob.setBounds (releaseArea.reduced (5));

    // Allocate Mechanical/Decoupling Right Panel
    auto togglesArea = controlsArea;
    togglesArea.removeFromLeft (15); // Pad gap
    
    int toggleH = togglesArea.getHeight() / 2;
    presetLinkButton.setBounds (togglesArea.removeFromTop (toggleH).reduced (0, 8));
    foleyModeButton.setBounds (togglesArea.reduced (0, 8));
}

void InGenSamplerAudioProcessorEditor::sliderValueChanged (juce::Slider* slider)
{
    juce::ignoreUnused (slider);
    updateAdsrFromSliders();
}

void InGenSamplerAudioProcessorEditor::buttonClicked (juce::Button* button)
{
    if (button == &generateButton)
    {
        triggerSampleGeneration();
    }
    else if (button == &presetLinkButton)
    {
        bool newLinkState = !audioProcessor.getSamplerEngine().getLayerLinking();
        audioProcessor.getSamplerEngine().setLayerLinking (newLinkState);
        
        presetLinkButton.setButtonText (newLinkState ? TRANS ("LINKED LAYERS") : TRANS ("DECOUPLED LAYERS"));
    }
    else if (button == &foleyModeButton)
    {
        if (auto* soundB = audioProcessor.getSamplerEngine().getActiveFoleySound())
        {
            if (soundB->triggerMode == FoleyTriggerMode::NoteOn)
            {
                soundB->triggerMode = FoleyTriggerMode::NoteOff;
                foleyModeButton.setButtonText (TRANS ("FOLEY: NOTE-OFF"));
            }
            else
            {
                soundB->triggerMode = FoleyTriggerMode::NoteOn;
                foleyModeButton.setButtonText (TRANS ("FOLEY: NOTE-ON"));
            }
        }
    }
}

void InGenSamplerAudioProcessorEditor::updateAdsrFromSliders()
{
    if (auto* soundA = audioProcessor.getSamplerEngine().getActiveTonalSound())
    {
        soundA->adsrParams.attack = static_cast<float> (attackKnob.getValue());
        soundA->adsrParams.decay = static_cast<float> (decayKnob.getValue());
        soundA->adsrParams.sustain = static_cast<float> (sustainKnob.getValue());
        soundA->adsrParams.release = static_cast<float> (releaseKnob.getValue());
    }
}

void InGenSamplerAudioProcessorEditor::triggerSampleGeneration()
{
    juce::String promptText = promptEditor.getText();
    if (promptText.isEmpty())
        return;

    // GUI Visualizer visual state trigger loading fallback
    generateButton.setButtonText (TRANS ("CREATING..."));
    generateButton.setEnabled (false);
    
    // In Phase 6, we trigger the network client callback.
    // For premium GUI demonstration, we simulate loading a synthetic wave buffer 
    // to instantly showcase the high-fidelity 60fps waveform rendering in JUCE!
    
    juce::MessageManager::callAsync ([this]() {
        const int sampleRate = 44100;
        const int numSamples = sampleRate * 3; // 3 seconds sample
        
        juce::AudioBuffer<float> mockBuffer (2, numSamples);
        
        // Generate an elegant synthetic warm wave pluck with decay harmonics
        float* left = mockBuffer.getWritePointer (0);
        float* right = mockBuffer.getWritePointer (1);
        
        for (int i = 0; i < numSamples; ++i)
        {
            float t = static_cast<float> (i) / sampleRate;
            float freq = 220.0f; // A3 root pitch
            
            // Fundamental sine + triangle harmonics
            float pluckDecay = std::exp (-2.5f * t);
            float osc = 0.6f * std::sin (2.0f * juce::MathConstants<float>::pi * freq * t) +
                        0.3f * std::sin (2.0f * juce::MathConstants<float>::pi * freq * 2.0f * t);
            
            float val = osc * pluckDecay;
            
            left[i] = val;
            right[i] = val;
        }

        // Load generated WAV sound directly into SamplerEngine
        audioProcessor.getSamplerEngine().loadTonalSound (
            "GeneratedWarmPluck", 
            std::move (mockBuffer), 
            sampleRate, 
            57,       // Root Key A3 (MIDI 57)
            0.0f,     // Tuning Offset Cents
            256,      // Crop Start Snapped
            numSamples - 512 // Crop End Snapped
        );

        // Bind active sound to the visualizer
        if (auto* soundA = audioProcessor.getSamplerEngine().getActiveTonalSound())
        {
            waveformVisualizer.setSound (soundA);
        }

        generateButton.setButtonText (TRANS ("GENERATE"));
        generateButton.setEnabled (true);
        repaint();
    });
}

} // namespace ingen
