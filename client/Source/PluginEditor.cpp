#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ingen {

InGenSamplerAudioProcessorEditor::InGenSamplerAudioProcessorEditor (InGenSamplerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), waveformVisualizer()
{
    // Register basic audio formats (WAV, AIFF)
    formatManager.registerBasicFormats();

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

    // Configure Launch Server Button
    addAndMakeVisible (launchServerButton);
    launchServerButton.setButtonText (TRANS ("LAUNCH AI SERVER"));
    launchServerButton.addListener (this);

    // 5. Configure Header Import/Export Buttons
    addAndMakeVisible (importButton);
    importButton.setButtonText (TRANS ("IMPORT"));
    importButton.addListener (this);

    addAndMakeVisible (exportButton);
    exportButton.setButtonText (TRANS ("EXPORT .INGSAM"));
    exportButton.addListener (this);

    // 6. Configure Parameter Sliders
    noteCountSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    noteCountSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    noteCountSlider.setRange (1.0, 12.0, 1.0);
    noteCountSlider.setValue (5.0);
    noteCountSlider.setName ("note_count");
    noteCountSlider.addListener (this);
    addAndMakeVisible (noteCountSlider);

    noteCountLabel.setText (TRANS ("NOTES"), juce::dontSendNotification);
    noteCountLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (noteCountLabel);

    octavesSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    octavesSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    octavesSlider.setRange (1.0, 4.0, 1.0);
    octavesSlider.setValue (2.0);
    octavesSlider.setName ("octaves");
    octavesSlider.addListener (this);
    addAndMakeVisible (octavesSlider);

    octavesLabel.setText (TRANS ("OCTAVES"), juce::dontSendNotification);
    octavesLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (octavesLabel);

    // 7. Configure ADSR Envelope Knobs (Tonal Layer A)
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

    // 8. Configure Layer Toggle Buttons
    addAndMakeVisible (presetLinkButton);
    presetLinkButton.setButtonText (TRANS ("LINKED LAYERS"));
    presetLinkButton.addListener (this);

    addAndMakeVisible (foleyModeButton);
    foleyModeButton.setButtonText (TRANS ("FOLEY: NOTE-ON"));
    foleyModeButton.addListener (this);

    // 9. Configure Model Selectors
    addAndMakeVisible (tonalModelSelector);
    tonalModelSelector.addItem (TRANS ("AudioGen Medium"), 1);
    tonalModelSelector.addItem (TRANS ("Stable Audio Open"), 2);
    tonalModelSelector.addItem (TRANS ("Audio-MAGNeT Small"), 3);
    tonalModelSelector.addItem (TRANS ("Sony Woosh DFlow"), 4);
    tonalModelSelector.setSelectedId (1, juce::dontSendNotification);

    tonalModelLabel.setText (TRANS ("TONAL A MODEL"), juce::dontSendNotification);
    tonalModelLabel.setFont (juce::Font (10.0f, juce::Font::bold));
    tonalModelLabel.setColour (juce::Label::textColourId, GlassmorphicLookAndFeel::textMuted);
    tonalModelLabel.setJustificationType (juce::Justification::left);
    addAndMakeVisible (tonalModelLabel);

    addAndMakeVisible (foleyModelSelector);
    foleyModelSelector.addItem (TRANS ("AudioGen Medium"), 1);
    foleyModelSelector.addItem (TRANS ("Stable Audio Open"), 2);
    foleyModelSelector.addItem (TRANS ("Audio-MAGNeT Small"), 3);
    foleyModelSelector.addItem (TRANS ("Sony Woosh DFlow"), 4);
    foleyModelSelector.setSelectedId (1, juce::dontSendNotification);

    foleyModelLabel.setText (TRANS ("FOLEY B MODEL"), juce::dontSendNotification);
    foleyModelLabel.setFont (juce::Font (10.0f, juce::Font::bold));
    foleyModelLabel.setColour (juce::Label::textColourId, GlassmorphicLookAndFeel::textMuted);
    foleyModelLabel.setJustificationType (juce::Justification::left);
    addAndMakeVisible (foleyModelLabel);

    // 10. Configure Layer Volume Faders
    layerAVolumeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    layerAVolumeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    layerAVolumeSlider.setRange (0.0, 1.0, 0.01);
    layerAVolumeSlider.setValue (audioProcessor.getSamplerEngine().getLayerAGain());
    layerAVolumeSlider.setName ("layerA_volume");
    layerAVolumeSlider.addListener (this);
    addAndMakeVisible (layerAVolumeSlider);

    layerAVolumeLabel.setText (TRANS ("VOLUME A"), juce::dontSendNotification);
    layerAVolumeLabel.setFont (juce::Font (10.0f, juce::Font::bold));
    layerAVolumeLabel.setColour (juce::Label::textColourId, GlassmorphicLookAndFeel::textMuted);
    layerAVolumeLabel.setJustificationType (juce::Justification::left);
    addAndMakeVisible (layerAVolumeLabel);

    layerBVolumeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    layerBVolumeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    layerBVolumeSlider.setRange (0.0, 1.0, 0.01);
    layerBVolumeSlider.setValue (audioProcessor.getSamplerEngine().getLayerBGain());
    layerBVolumeSlider.setName ("layerB_volume");
    layerBVolumeSlider.addListener (this);
    addAndMakeVisible (layerBVolumeSlider);

    layerBVolumeLabel.setText (TRANS ("VOLUME B"), juce::dontSendNotification);
    layerBVolumeLabel.setFont (juce::Font (10.0f, juce::Font::bold));
    layerBVolumeLabel.setColour (juce::Label::textColourId, GlassmorphicLookAndFeel::textMuted);
    layerBVolumeLabel.setJustificationType (juce::Justification::left);
    addAndMakeVisible (layerBVolumeLabel);

    // 11. Configure Loop Controls
    loopModeButton.setButtonText (TRANS ("LOOP: OFF"));
    loopModeButton.addListener (this);
    addAndMakeVisible (loopModeButton);

    reverseButton.setButtonText (TRANS ("REVERSE: OFF"));
    reverseButton.addListener (this);
    addAndMakeVisible (reverseButton);

    loopCrossfadeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    loopCrossfadeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    loopCrossfadeSlider.setRange (0.0, 100.0, 0.1); // 0% to 100% loop crossfade
    loopCrossfadeSlider.setValue (0.0);
    loopCrossfadeSlider.setName ("layerA_loop_xfade");
    loopCrossfadeSlider.addListener (this);
    addAndMakeVisible (loopCrossfadeSlider);

    loopCrossfadeLabel.setText (TRANS ("LOOP XFADE"), juce::dontSendNotification);
    loopCrossfadeLabel.setFont (juce::Font (10.0f, juce::Font::bold));
    loopCrossfadeLabel.setColour (juce::Label::textColourId, GlassmorphicLookAndFeel::textMuted);
    loopCrossfadeLabel.setJustificationType (juce::Justification::left);
    addAndMakeVisible (loopCrossfadeLabel);

    // Initialize real-time status parameters
    generationProgress = 0.0f;
    statusLogMessage = TRANS ("STATUS: NO SAMPLES LOADED. DESCRIBE A SOUND AND PRESS GENERATE!");
    progressColour = GlassmorphicLookAndFeel::textMuted;

    // Start UI Polling Timer (100ms for smooth LED breathing/pulsing animation)
    startTimer (100);

    // Establish standard premium plugin dimension ratio (wide & spacious)
    setSize (920, 550);
}

InGenSamplerAudioProcessorEditor::~InGenSamplerAudioProcessorEditor()
{
    // 1. Stop the UI timer immediately to prevent callbacks during destruction
    stopTimer();

    // Gracefully shutdown the server and free GPU resources
    generatorAPI.shutdownServer();

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
    g.drawText ("InGen Sampler", 20, 15, 130, 30, juce::Justification::left);

    // Draw Server Status Glowing LED
    g.setColour (progressColour);
    g.fillEllipse (153.0f, 26.0f, 8.0f, 8.0f);
    
    // Draw a subtle outer glow for the LED
    g.setColour (progressColour.withAlpha (0.25f));
    g.fillEllipse (150.0f, 23.0f, 14.0f, 14.0f);

    g.setColour (GlassmorphicLookAndFeel::textMuted);
    g.setFont (juce::Font (12.0f, juce::Font::plain));
    g.drawText ("AI-POWERED MULTI-LAYER SAMPLING ENGINE", 172, 21, 300, 20, juce::Justification::left);

    // Draw Divider / Progress Bar at y = 114 (10px gap between inputs and waveform)
    float progressStartX = 20.0f;
    float progressWidth = static_cast<float> (getWidth()) - 40.0f;
    
    // Base divider line (subtle dark glassmorphic boundary)
    g.setColour (juce::Colour (0x1fffffff)); // 12% transparent white
    g.fillRoundedRectangle (progressStartX, 114.0f, progressWidth, 3.0f, 1.5f);
    
    if (generationProgress > 0.0f)
    {
        float fillWidth = progressWidth * generationProgress;
        
        // Vibrant glowing gradient from Neon Cyan to Electric Violet
        juce::ColourGradient grad (juce::Colour (0xff00d2ff), progressStartX, 114.0f,
                                   juce::Colour (0xffa800ff), progressStartX + progressWidth, 114.0f, false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (progressStartX, 114.0f, fillWidth, 3.0f, 1.5f);
    }

    // Draw Dynamic real-time Status Log below waveform (in the 15px gap, y = 311)
    g.setColour (progressColour);
    g.setFont (juce::Font (10.5f, juce::Font::bold));
    g.drawText (statusLogMessage, 22, 311, getWidth() - 44, 14, juce::Justification::left);
}

void InGenSamplerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    
    // Layout Header buttons in the top-right
    auto headerArea = area.removeFromTop (50);
    headerArea.reduce (20, 10);
    exportButton.setBounds (headerArea.removeFromRight (130).reduced (0, 2));
    headerArea.removeFromRight (10);
    importButton.setBounds (headerArea.removeFromRight (100).reduced (0, 2));
    headerArea.removeFromRight (10);
    launchServerButton.setBounds (headerArea.removeFromRight (140).reduced (0, 2));

    area.reduce (20, 15);

    // 2. Search / Generation bar with Settings Sliders
    auto searchArea = area.removeFromTop (45);
    promptEditor.setBounds (searchArea.removeFromLeft (static_cast<int> (area.getWidth() * 0.52f)).reduced (0, 4));
    searchArea.removeFromLeft (15);

    auto variationsArea = searchArea.removeFromLeft (80);
    noteCountLabel.setBounds (variationsArea.removeFromTop (12));
    noteCountSlider.setBounds (variationsArea.reduced (0, 2));
    
    searchArea.removeFromLeft (10);

    auto octavesArea = searchArea.removeFromLeft (80);
    octavesLabel.setBounds (octavesArea.removeFromTop (12));
    octavesSlider.setBounds (octavesArea.reduced (0, 2));

    searchArea.removeFromLeft (15);
    generateButton.setBounds (searchArea.reduced (0, 4));

    area.removeFromTop (10); // Spacing gap

    // 3. Main Waveform Screen
    waveformVisualizer.setBounds (area.removeFromTop (190));

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
    auto rightPanelArea = controlsArea;
    rightPanelArea.removeFromLeft (15); // Pad gap
    
    // Split Right Panel into three columns: Selectors, Volume Faders, and Toggles
    int totalW = rightPanelArea.getWidth();
    auto selectorsArea = rightPanelArea.removeFromLeft (static_cast<int> (totalW * 0.45f));
    auto volumesArea = rightPanelArea.removeFromLeft (static_cast<int> (totalW * 0.28f));
    auto buttonsArea = rightPanelArea;
    
    volumesArea.removeFromLeft (10); // Spacing gap
    buttonsArea.removeFromLeft (10); // Spacing gap
    
    // Row height
    int rowH = selectorsArea.getHeight() / 3;
    
    // Row 1: Tonal selector, Tonal Volume, Preset Link Button
    auto tonalSelectorArea = selectorsArea.removeFromTop (rowH).reduced (0, 4);
    tonalModelLabel.setBounds (tonalSelectorArea.removeFromTop (15));
    tonalModelSelector.setBounds (tonalSelectorArea.reduced (0, 2));
    
    auto tonalVolumeArea = volumesArea.removeFromTop (rowH).reduced (0, 4);
    layerAVolumeLabel.setBounds (tonalVolumeArea.removeFromTop (15));
    layerAVolumeSlider.setBounds (tonalVolumeArea.reduced (0, 4));
    
    auto tonalButtonArea = buttonsArea.removeFromTop (rowH).reduced (0, 8);
    presetLinkButton.setBounds (tonalButtonArea);
    
    // Row 2: Foley selector, Foley Volume, Foley Mode Button
    auto foleySelectorArea = selectorsArea.removeFromTop (rowH).reduced (0, 4);
    foleyModelLabel.setBounds (foleySelectorArea.removeFromTop (15));
    foleyModelSelector.setBounds (foleySelectorArea.reduced (0, 2));
    
    auto foleyVolumeArea = volumesArea.removeFromTop (rowH).reduced (0, 4);
    layerBVolumeLabel.setBounds (foleyVolumeArea.removeFromTop (15));
    layerBVolumeSlider.setBounds (foleyVolumeArea.reduced (0, 4));
    
    auto foleyButtonArea = buttonsArea.removeFromTop (rowH).reduced (0, 8);
    foleyModeButton.setBounds (foleyButtonArea);

    // Row 3: Loop Controls (Label, Crossfade Slider, Loop Mode Button)
    auto loopSelectorArea = selectorsArea.reduced (0, 4);
    loopCrossfadeLabel.setBounds (loopSelectorArea.reduced (0, 4));

    auto loopVolumeArea = volumesArea.reduced (0, 4);
    loopCrossfadeSlider.setBounds (loopVolumeArea.reduced (0, 4));

    auto loopButtonArea = buttonsArea.reduced (0, 8);
    int halfW = loopButtonArea.getWidth() / 2;
    loopModeButton.setBounds (loopButtonArea.removeFromLeft (halfW - 4));
    loopButtonArea.removeFromLeft (8); // Space gap
    reverseButton.setBounds (loopButtonArea);
}

void InGenSamplerAudioProcessorEditor::timerCallback()
{
    auto status = audioProcessor.getServerStatus();
    
    // Sync loop parameters with UI controls
    if (auto* soundA = audioProcessor.getSamplerEngine().getActiveTonalSound())
    {
        if (soundA->loopMode == LoopMode::Off && loopModeButton.getButtonText() != TRANS ("LOOP: OFF"))
            loopModeButton.setButtonText (TRANS ("LOOP: OFF"));
        else if (soundA->loopMode == LoopMode::Forward && loopModeButton.getButtonText() != TRANS ("LOOP: FORWARD"))
            loopModeButton.setButtonText (TRANS ("LOOP: FORWARD"));
        else if (soundA->loopMode == LoopMode::PingPong && loopModeButton.getButtonText() != TRANS ("LOOP: PING-PONG"))
            loopModeButton.setButtonText (TRANS ("LOOP: PING-PONG"));

        if (!loopCrossfadeSlider.isMouseButtonDown())
        {
            double val = soundA->loopCrossfadePercent * 100.0;
            if (std::abs (loopCrossfadeSlider.getValue() - val) > 0.01)
                loopCrossfadeSlider.setValue (val, juce::dontSendNotification);
        }

        if (soundA->isReversed && reverseButton.getButtonText() != TRANS ("REVERSE: ON"))
            reverseButton.setButtonText (TRANS ("REVERSE: ON"));
        else if (!soundA->isReversed && reverseButton.getButtonText() != TRANS ("REVERSE: OFF"))
            reverseButton.setButtonText (TRANS ("REVERSE: OFF"));
    }

    // Update button visibility based on server status
    launchServerButton.setVisible (status == InGenSamplerAudioProcessor::ServerStatus::offline);
    
    // Smoothly update the status bar text and progress colors if NOT currently generating a sample
    if (generationProgress == 0.0f)
    {
        if (status == InGenSamplerAudioProcessor::ServerStatus::online)
        {
            statusLogMessage = TRANS ("STATUS: AI SERVER ONLINE. DESCRIBE A SOUND AND PRESS GENERATE!");
            progressColour = juce::Colour (0xff2ecc71); // Emerald Green
        }
        else if (status == InGenSamplerAudioProcessor::ServerStatus::launching)
        {
            // Breathing glow pulse (sinusoidal alpha variation)
            float alpha = 0.4f + 0.6f * std::sin (static_cast<float> (juce::Time::getMillisecondCounter()) * 0.005f);
            statusLogMessage = TRANS ("STATUS: LAUNCHING AI SERVER... PLEASE WAIT.");
            progressColour = juce::Colour (0xfff1c40f).withAlpha (alpha); // Pulsing Gold
        }
        else
        {
            statusLogMessage = TRANS ("STATUS: AI SERVER OFFLINE. CLICK 'LAUNCH AI SERVER' IN THE HEADER TO START.");
            progressColour = juce::Colour (0xffe74c3c); // Red
        }
        repaint();
    }
}

void InGenSamplerAudioProcessorEditor::sliderValueChanged (juce::Slider* slider)
{
    if (slider == &layerAVolumeSlider)
    {
        audioProcessor.getSamplerEngine().setLayerAGain (static_cast<float> (layerAVolumeSlider.getValue()));
    }
    else if (slider == &layerBVolumeSlider)
    {
        audioProcessor.getSamplerEngine().setLayerBGain (static_cast<float> (layerBVolumeSlider.getValue()));
    }
    else if (slider == &loopCrossfadeSlider)
    {
        updateLoopFromSliders();
    }
    else
    {
        updateAdsrFromSliders();
    }
}

void InGenSamplerAudioProcessorEditor::buttonClicked (juce::Button* button)
{
    if (button == &launchServerButton)
    {
        audioProcessor.launchServerAsync();
    }
    else if (button == &generateButton)
    {
        triggerSampleGeneration();
    }
    else if (button == &importButton)
    {
        fileChooser = std::make_unique<juce::FileChooser> (TRANS ("Import Preset Archive (.ingsam)"),
                                                           juce::File::getSpecialLocation (juce::File::SpecialLocationType::userDocumentsDirectory),
                                                           "*.ingsam");
        fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this] (const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file != juce::File())
            {
                PresetData presetMetadata;
                juce::File extractDir = juce::File::getSpecialLocation (juce::File::SpecialLocationType::tempDirectory)
                                                 .getChildFile ("InGenImport_" + juce::String (juce::Random::getSystemRandom().nextInt()));
                
                bool success = PresetArchiver::importPreset (file, extractDir, presetMetadata, activeSampleA, activeSamplesB);
                if (success)
                {
                    // Read sample A into buffer
                    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (activeSampleA));
                    if (reader != nullptr)
                    {
                        juce::AudioBuffer<float> buffer (static_cast<int>(reader->numChannels), static_cast<int>(reader->lengthInSamples));
                        reader->read (&buffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
                        
                        audioProcessor.getSamplerEngine().loadTonalSound (
                            presetMetadata.promptA,
                            std::move (buffer),
                            reader->sampleRate,
                            presetMetadata.rootKeyA,
                            presetMetadata.centsOffsetA,
                            presetMetadata.cropStartA,
                            presetMetadata.cropEndA
                        );
                        
                        if (auto* soundA = audioProcessor.getSamplerEngine().getActiveTonalSound())
                        {
                            soundA->adsrParams.attack = presetMetadata.attackA;
                            soundA->adsrParams.decay = presetMetadata.decayA;
                            soundA->adsrParams.sustain = presetMetadata.sustainA;
                            soundA->adsrParams.release = presetMetadata.releaseA;
                            
                            // Update sliders
                            attackKnob.setValue (presetMetadata.attackA, juce::dontSendNotification);
                            decayKnob.setValue (presetMetadata.decayA, juce::dontSendNotification);
                            sustainKnob.setValue (presetMetadata.sustainA, juce::dontSendNotification);
                            releaseKnob.setValue (presetMetadata.releaseA, juce::dontSendNotification);
                            
                            waveformVisualizer.setSound (soundA);
                        }
                    }

                    // Load Layer B
                    audioProcessor.getSamplerEngine().clearFoleyLayer();
                    FoleyTriggerMode triggerMode = (presetMetadata.triggerModeB == "NoteOff") ? FoleyTriggerMode::NoteOff : FoleyTriggerMode::NoteOn;
                    
                    for (auto& varFile : activeSamplesB)
                    {
                        std::unique_ptr<juce::AudioFormatReader> varReader (formatManager.createReaderFor (varFile));
                        if (varReader != nullptr)
                        {
                            juce::AudioBuffer<float> varBuffer (static_cast<int>(varReader->numChannels), static_cast<int>(varReader->lengthInSamples));
                            varReader->read (&varBuffer, 0, static_cast<int>(varReader->lengthInSamples), 0, true, true);
                            
                            audioProcessor.getSamplerEngine().addFoleySound (
                                presetMetadata.promptB,
                                triggerMode,
                                std::move (varBuffer),
                                varReader->sampleRate,
                                presetMetadata.minVelocityB,
                                presetMetadata.maxVelocityB
                            );
                        }
                    }
                    
                    repaint();
                    juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
                                                             TRANS ("Import Successful"),
                                                             TRANS ("Preset unpacked and loaded successfully!"));
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                             TRANS ("Import Failed"),
                                                             TRANS ("Could not extract preset archive."));
                }
            }
        });
    }
    else if (button == &exportButton)
    {
        if (!activeSampleA.existsAsFile())
        {
            juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                     TRANS ("Export Failed"),
                                                     TRANS ("Please generate or import a sound first before exporting!"));
            return;
        }

        fileChooser = std::make_unique<juce::FileChooser> (TRANS ("Export Preset Archive (.ingsam)"),
                                                           juce::File::getSpecialLocation (juce::File::SpecialLocationType::userDocumentsDirectory),
                                                           "*.ingsam");
        fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this] (const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file != juce::File())
            {
                juce::String relA = activeSampleA.getFileName();
                std::vector<juce::String> relsB;
                for (auto& f : activeSamplesB)
                {
                    relsB.push_back (f.getFileName());
                }
                
                PresetData metadata = PresetSerializer::extractFromEngine (
                    audioProcessor.getSamplerEngine(),
                    relA,
                    relsB
                );
                
                // Ensure prompts are stored
                metadata.promptA = promptEditor.getText();
                metadata.promptB = promptEditor.getText();

                bool success = PresetArchiver::exportPreset (file, metadata, activeSampleA, activeSamplesB);
                if (success)
                {
                    juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
                                                             TRANS ("Export Successful"),
                                                             TRANS ("Preset packed and saved successfully to ") + file.getFullPathName());
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                                                             TRANS ("Export Failed"),
                                                             TRANS ("Could not create preset archive."));
                }
            }
        });
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
    else if (button == &loopModeButton)
    {
        if (auto* soundA = audioProcessor.getSamplerEngine().getActiveTonalSound())
        {
            if (soundA->loopMode == LoopMode::Off)
            {
                soundA->loopMode = LoopMode::Forward;
                loopModeButton.setButtonText (TRANS ("LOOP: FORWARD"));
            }
            else if (soundA->loopMode == LoopMode::Forward)
            {
                soundA->loopMode = LoopMode::PingPong;
                loopModeButton.setButtonText (TRANS ("LOOP: PING-PONG"));
            }
            else
            {
                soundA->loopMode = LoopMode::Off;
                loopModeButton.setButtonText (TRANS ("LOOP: OFF"));
            }
        }
    }
    else if (button == &reverseButton)
    {
        if (auto* soundA = audioProcessor.getSamplerEngine().getActiveTonalSound())
        {
            soundA->isReversed = !soundA->isReversed;
            reverseButton.setButtonText (soundA->isReversed ? TRANS ("REVERSE: ON") : TRANS ("REVERSE: OFF"));
        }
        if (auto* soundB = audioProcessor.getSamplerEngine().getActiveFoleySound())
        {
            soundB->isReversed = !soundB->isReversed;
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

void InGenSamplerAudioProcessorEditor::updateLoopFromSliders()
{
    if (auto* soundA = audioProcessor.getSamplerEngine().getActiveTonalSound())
    {
        soundA->loopCrossfadePercent = static_cast<float> (loopCrossfadeSlider.getValue()) / 100.0f;
    }
}

void InGenSamplerAudioProcessorEditor::triggerSampleGeneration()
{
    juce::String promptText = promptEditor.getText();
    if (promptText.isEmpty())
        return;

    generateButton.setButtonText (TRANS ("CREATING..."));
    generateButton.setEnabled (false);
    
    // Set initial loading state
    generationProgress = 0.1f;
    statusLogMessage = TRANS ("STATUS: GENERATING TONAL SAMPLES VIA LOCAL AI...");
    progressColour = juce::Colour (0xff00d2ff); // Neon Cyan
    repaint();

    audioProcessor.setIsGenerating (true);

    PromptConfig config;
    config.prompt = promptText;
    config.tonalModel = getModelStringFromId (tonalModelSelector.getSelectedId());
    config.foleyModel = getModelStringFromId (foleyModelSelector.getSelectedId());
    config.noteCount = static_cast<int> (noteCountSlider.getValue());
    config.octaves = static_cast<int> (octavesSlider.getValue());
    config.foleyCount = config.noteCount;
    config.temperature = 1.0f;

    // Coordinator to synchronize parallel async HTTP/processing tasks (Tonal analysis & Foley generation)
    struct GenerationCoordinator {
        bool tonalFinished = false;
        bool foleyFinished = false;
    };
    auto coordinator = std::make_shared<GenerationCoordinator>();

    generatorAPI.generateTonalSamples (config, [this, config, coordinator](const std::vector<AudioResult>& results, const juce::String& error) {
        if (!error.isEmpty())
        {
            audioProcessor.setIsGenerating (false);
            generationProgress = 0.0f;
            statusLogMessage = TRANS ("STATUS ERROR (TONAL): ") + error;
            progressColour = juce::Colour (0xffe74c3c); // Red
            generateButton.setButtonText (TRANS ("GENERATE"));
            generateButton.setEnabled (true);
            repaint();
            
            juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon, 
                                                     TRANS ("Generation Error"), 
                                                     error);
            return;
        }

        if (results.empty())
        {
            audioProcessor.setIsGenerating (false);
            generationProgress = 0.0f;
            statusLogMessage = TRANS ("STATUS: NO SAMPLES RETURNED BY AI SERVER.");
            progressColour = juce::Colour (0xfff1c40f); // Gold
            generateButton.setButtonText (TRANS ("GENERATE"));
            generateButton.setEnabled (true);
            repaint();
            return;
        }

        // Update progress state
        generationProgress = 0.35f;
        statusLogMessage = TRANS ("STATUS: PROCESSING TONAL ANALYSIS & GENERATING FOLEY...");
        progressColour = juce::Colour (0xff9b59b6); // Amethyst Purple
        repaint();

        // Cache the first Layer A WAV file path for legacy preset export
        activeSampleA = results[0].audioFile;
        
        // We use a shared pointer to keep track of state across multiple asynchronous threads
        struct AnalysisState {
            std::vector<AudioResult> resultsToAnalyze;
            size_t currentIndex = 0;
            
            struct SoundData {
                AudioResult result;
                juce::AudioBuffer<float> buffer;
                double sampleRate;
                int cropStart;
                int cropEnd;
            };
            
            std::vector<SoundData> analyzedSounds;
            std::vector<std::unique_ptr<SampleAnalysisJob>> activeJobs;
            std::function<void()> runNextAnalysis;
        };

        auto state = std::make_shared<AnalysisState>();
        state->resultsToAnalyze = results;

        state->runNextAnalysis = [this, state, coordinator]() {
            if (state->currentIndex >= state->resultsToAnalyze.size())
            {
                // All samples analyzed! Sort them in ascending order of detected MIDI root key
                std::sort (state->analyzedSounds.begin(), state->analyzedSounds.end(), 
                           [](const typename AnalysisState::SoundData& a, const typename AnalysisState::SoundData& b) {
                               return a.result.midiRootKey < b.result.midiRootKey;
                           });

                // Load all of them into the engine with mid-point key ranges
                auto& engine = audioProcessor.getSamplerEngine();
                engine.clearTonalLayer();

                const size_t numSounds = state->analyzedSounds.size();
                for (size_t i = 0; i < numSounds; ++i)
                {
                    int minNote = 0;
                    int maxNote = 127;

                    if (numSounds > 1)
                    {
                        if (i == 0)
                        {
                            minNote = 0;
                            maxNote = (state->analyzedSounds[i].result.midiRootKey + state->analyzedSounds[i + 1].result.midiRootKey) / 2;
                        }
                        else if (i == numSounds - 1)
                        {
                            minNote = (state->analyzedSounds[i - 1].result.midiRootKey + state->analyzedSounds[i].result.midiRootKey) / 2 + 1;
                            maxNote = 127;
                        }
                        else
                        {
                            minNote = (state->analyzedSounds[i - 1].result.midiRootKey + state->analyzedSounds[i].result.midiRootKey) / 2 + 1;
                            maxNote = (state->analyzedSounds[i].result.midiRootKey + state->analyzedSounds[i + 1].result.midiRootKey) / 2;
                        }
                    }

                    engine.addTonalSound (
                        state->analyzedSounds[i].result.rawPrompt,
                        std::move (state->analyzedSounds[i].buffer),
                        state->analyzedSounds[i].sampleRate,
                        state->analyzedSounds[i].result.midiRootKey,
                        state->analyzedSounds[i].result.pitchCorrectionCents,
                        state->analyzedSounds[i].cropStart,
                        state->analyzedSounds[i].cropEnd,
                        minNote,
                        maxNote
                    );
                }

                // Update visualizer to display the first mapped sound
                if (auto* soundA = engine.getActiveTonalSound())
                {
                    waveformVisualizer.setSound (soundA);
                }

                // Clean up all the job threads safely on the GUI thread
                state->activeJobs.clear();

                // Signal tonal analysis completion
                coordinator->tonalFinished = true;
                if (coordinator->foleyFinished)
                {
                    audioProcessor.setIsGenerating (false);
                    generationProgress = 1.0f;
                    statusLogMessage = juce::String::formatted (
                        TRANS ("STATUS: SUCCESS! %d TONAL SAMPLES MAPPED | %d FOLEY VARIATIONS LOADED"),
                        static_cast<int>(engine.getNumTonalSounds()), 
                        static_cast<int>(engine.getNumFoleySounds())
                    );
                    progressColour = juce::Colour (0xff2ecc71); // Emerald Green
                    generateButton.setButtonText (TRANS ("GENERATE"));
                    generateButton.setEnabled (true);
                    
                    // Smoothly reset the progress bar after 5 seconds
                    juce::Timer::callAfterDelay (5000, [this]() {
                        if (generationProgress == 1.0f) {
                            generationProgress = 0.0f;
                            repaint();
                        }
                    });
                }
                else
                {
                    generationProgress = 0.75f;
                    statusLogMessage = TRANS ("STATUS: TONAL MAPPED. COMPLETING FOLEY GENERATION...");
                    progressColour = juce::Colour (0xffe67e22); // Orange
                }
                repaint();
                return;
            }

            // Launch background thread job for the current sample
            auto currentResult = state->resultsToAnalyze[state->currentIndex];
            auto* job = new SampleAnalysisJob (currentResult, [this, state, coordinator](const AudioResult& updatedResult, 
                                                                                         const juce::AudioBuffer<float>& loadedBuffer,
                                                                                         double sampleRate,
                                                                                         int cropStart, 
                                                                                         int cropEnd) {
                typename AnalysisState::SoundData data;
                data.result = updatedResult;
                data.buffer = loadedBuffer;
                data.sampleRate = sampleRate;
                data.cropStart = cropStart;
                data.cropEnd = cropEnd;
                state->analyzedSounds.push_back (std::move (data));

                state->currentIndex++;
                
                // Scale progress smoothly across tonal analysis
                float baseProgress = 0.35f;
                float step = 0.3f / static_cast<float> (state->resultsToAnalyze.size());
                generationProgress = baseProgress + (static_cast<float> (state->currentIndex) * step);
                repaint();

                state->runNextAnalysis();
            });

            state->activeJobs.emplace_back (job);
            job->startAnalysis();
        };

        // Trigger the recursive chain
        state->runNextAnalysis();
        
        // Trigger Foley Generation for Layer B
        generatorAPI.generateFoleySamples (config, [this, coordinator](const std::vector<AudioResult>& foleyResults, const juce::String& foleyError) {
            if (!foleyError.isEmpty())
            {
                audioProcessor.setIsGenerating (false);
                generationProgress = 0.0f;
                statusLogMessage = TRANS ("STATUS ERROR (FOLEY): ") + foleyError;
                progressColour = juce::Colour (0xffe74c3c); // Red
                generateButton.setButtonText (TRANS ("GENERATE"));
                generateButton.setEnabled (true);
                repaint();
                return;
            }

            if (!foleyResults.empty())
            {
                activeSamplesB.clear();
                audioProcessor.getSamplerEngine().clearFoleyLayer();

                for (auto& fRes : foleyResults)
                {
                    activeSamplesB.push_back (fRes.audioFile);

                    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (fRes.audioFile));
                    if (reader != nullptr)
                    {
                        juce::AudioBuffer<float> fBuffer (static_cast<int>(reader->numChannels), static_cast<int>(reader->lengthInSamples));
                        reader->read (&fBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);

                        FoleyTriggerMode mode = FoleyTriggerMode::NoteOn;
                        if (fRes.audioFile.getFileName().containsIgnoreCase ("note_off"))
                        {
                            mode = FoleyTriggerMode::NoteOff;
                        }

                        audioProcessor.getSamplerEngine().addFoleySound (
                            fRes.rawPrompt,
                            mode,
                            std::move (fBuffer),
                            reader->sampleRate,
                            0.0f,
                            1.0f
                        );
                    }
                }
            }

            // Signal foley completion
            coordinator->foleyFinished = true;
            if (coordinator->tonalFinished)
            {
                audioProcessor.setIsGenerating (false);
                auto& engine = audioProcessor.getSamplerEngine();
                generationProgress = 1.0f;
                statusLogMessage = juce::String::formatted (
                    TRANS ("STATUS: SUCCESS! %d TONAL SAMPLES MAPPED | %d FOLEY VARIATIONS LOADED"),
                    static_cast<int>(engine.getNumTonalSounds()), 
                    static_cast<int>(engine.getNumFoleySounds())
                );
                progressColour = juce::Colour (0xff2ecc71); // Emerald Green
                generateButton.setButtonText (TRANS ("GENERATE"));
                generateButton.setEnabled (true);
                
                // Smoothly reset the progress bar after 5 seconds
                juce::Timer::callAfterDelay (5000, [this]() {
                    if (generationProgress == 1.0f) {
                        generationProgress = 0.0f;
                        repaint();
                    }
                });
            }
            else
            {
                generationProgress = 0.75f;
                statusLogMessage = TRANS ("STATUS: FOLEY LOADED. COMPLETING TONAL ANALYSIS...");
                progressColour = juce::Colour (0xffe67e22); // Orange
            }
            repaint();
        });
    });
}

} // namespace ingen
