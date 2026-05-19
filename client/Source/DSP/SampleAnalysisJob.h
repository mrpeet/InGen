#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <functional>
#include "../Network/AudioGeneratorAPI.h"

namespace ingen {

/**
 * Background thread worker that loads a raw WAV file, runs transient and pitch detection,
 * and passes the calculated root key and crop boundary metadata back to the host.
 */
class SampleAnalysisJob : private juce::Thread {
public:
    using CompletionCallback = std::function<void(const AudioResult& updatedResult, 
                                                  const juce::AudioBuffer<float>& loadedBuffer,
                                                  double sampleRate,
                                                  int cropStartSample, 
                                                  int cropEndSample)>;

    SampleAnalysisJob(const AudioResult& rawResult, CompletionCallback callback);
    ~SampleAnalysisJob() override;

    /**
     * Spawns the background thread and begins the analysis.
     */
    void startAnalysis();

private:
    void run() override;

    AudioResult result;
    CompletionCallback completionCallback;
    
    juce::AudioFormatManager formatManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleAnalysisJob)
};

} // namespace ingen
