#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ingen {

InGenSamplerAudioProcessorEditor::InGenSamplerAudioProcessorEditor (InGenSamplerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Establish standard plugin dimensions
    setSize (800, 500);
}

InGenSamplerAudioProcessorEditor::~InGenSamplerAudioProcessorEditor()
{
}

void InGenSamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Draw an elegant modern background gradient
    g.fillAll (juce::Colours::darkgrey);

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("InGen Multi-Layer Sampler", getLocalBounds(), juce::Justification::centred, 1);
}

void InGenSamplerAudioProcessorEditor::resized()
{
}

} // namespace ingen
