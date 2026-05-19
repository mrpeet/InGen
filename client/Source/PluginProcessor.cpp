#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ingen {

InGenSamplerAudioProcessor::InGenSamplerAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

InGenSamplerAudioProcessor::~InGenSamplerAudioProcessor()
{
}

const juce::String InGenSamplerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool InGenSamplerAudioProcessor::acceptsMidi() const
{
    return true;
}

bool InGenSamplerAudioProcessor::producesMidi() const
{
    return false;
}

bool InGenSamplerAudioProcessor::isMidiEffect() const
{
    return false;
}

double InGenSamplerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int InGenSamplerAudioProcessor::getNumPrograms()
{
    return 1;
}

int InGenSamplerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void InGenSamplerAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String InGenSamplerAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void InGenSamplerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void InGenSamplerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    samplerEngine.prepareToPlay (sampleRate, samplesPerBlock);
}

void InGenSamplerAudioProcessor::releaseResources()
{
}

bool InGenSamplerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void InGenSamplerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear output channels that have no input data to prevent feedback noise
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Render Layer A & Layer B synthesiser engines in parallel
    samplerEngine.processBlock (buffer, midiMessages);
}

bool InGenSamplerAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* InGenSamplerAudioProcessor::createEditor()
{
    return new InGenSamplerAudioProcessorEditor (*this);
}

void InGenSamplerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void InGenSamplerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

} // namespace ingen

// Entry point creator required by JUCE
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ingen::InGenSamplerAudioProcessor();
}
