#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Engine/SamplerEngine.h"

namespace ingen {

class InGenSamplerAudioProcessor : public juce::AudioProcessor
{
public:
    InGenSamplerAudioProcessor();
    ~InGenSamplerAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Public access to the dual-layer sampler engine
    SamplerEngine& getSamplerEngine() { return samplerEngine; }

    enum class ServerStatus
    {
        offline,
        launching,
        online
    };

    ServerStatus getServerStatus() const { return serverStatus; }
    bool getIsGenerating() const { return isGenerating; }
    void setIsGenerating (bool generating) { isGenerating = generating; }
    
    void launchServerAsync();
    void stopServer();
    void checkServerHealth();

private:
    class HealthPollerThread : public juce::Thread
    {
    public:
        HealthPollerThread (InGenSamplerAudioProcessor& p)
            : juce::Thread ("ServerHealthPoller"), processor (p)
        {}

        void run() override
        {
            while (!threadShouldExit())
            {
                processor.checkServerHealth();
                wait (2000); // Query health endpoint every 2 seconds
            }
        }

    private:
        InGenSamplerAudioProcessor& processor;
    };

    SamplerEngine samplerEngine;

    // Server process and background polling control
    std::atomic<ServerStatus> serverStatus { ServerStatus::offline };
    std::atomic<bool> isGenerating { false };
    juce::ChildProcess serverProcess;
    HealthPollerThread pollerThread { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InGenSamplerAudioProcessor)
};

} // namespace ingen
