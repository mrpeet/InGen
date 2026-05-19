#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace ingen {

class InGenSamplerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    InGenSamplerAudioProcessorEditor (InGenSamplerAudioProcessor&);
    ~InGenSamplerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    InGenSamplerAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InGenSamplerAudioProcessorEditor)
};

} // namespace ingen
