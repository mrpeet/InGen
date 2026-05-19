#include "SampleAnalysisJob.h"
#include "PitchDetector.h"
#include "TransientDetector.h"
#include <juce_events/juce_events.h>

namespace ingen {

SampleAnalysisJob::SampleAnalysisJob(const AudioResult& rawResult, CompletionCallback callback)
    : Thread ("SampleAnalysis_" + rawResult.audioFile.getFileNameWithoutExtension()),
      result (rawResult),
      completionCallback (callback)
{
    formatManager.registerBasicFormats(); // Registers WAV and AIFF readers
}

SampleAnalysisJob::~SampleAnalysisJob()
{
    // Wait for thread to finish safely before deletion
    stopThread (5000);
}

void SampleAnalysisJob::startAnalysis()
{
    startThread();
}

void SampleAnalysisJob::run()
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (result.audioFile));
    
    if (reader == nullptr)
    {
        // Fail gracefully
        juce::MessageManager::callAsync ([this]() {
            completionCallback (result, juce::AudioBuffer<float>(), 44100.0, 0, 0);
        });
        return;
    }

    // Allocate buffer size
    const int numSamples = static_cast<int> (reader->lengthInSamples);
    const int numChannels = static_cast<int> (reader->numChannels);
    
    juce::AudioBuffer<float> fullBuffer (numChannels, numSamples);
    reader->read (&fullBuffer, 0, numSamples, 0, true, true);

    // 1. Run transient detection & auto-cropping
    TransientDetector transientDetector;
    auto cropPoints = transientDetector.detectCropPoints (fullBuffer);
    
    int cropStart = cropPoints.first;
    int cropEnd = cropPoints.second;

    // 2. Run pitch detection YIN on a clean sustained window (skipping the noisy transient attack)
    const int analysisWindowSize = 4096;
    int analysisStart = cropStart + static_cast<int> (reader->sampleRate * 0.2); // 200ms after crop start
    
    // Safety check: if the file is too short, start earlier
    if (analysisStart + analysisWindowSize > fullBuffer.getNumSamples())
        analysisStart = cropStart;
        
    juce::AudioBuffer<float> analysisBuffer (1, analysisWindowSize);
    if (analysisStart + analysisWindowSize <= fullBuffer.getNumSamples())
    {
        analysisBuffer.copyFrom (0, 0, fullBuffer, 0, analysisStart, analysisWindowSize);
    }
    else
    {
        // Fallback if the whole sample is shorter than 4096 samples
        analysisBuffer.makeCopyOf (fullBuffer);
    }

    PitchDetector pitchDetector (reader->sampleRate);
    float estimatedF0 = pitchDetector.detectPitch (analysisBuffer);

    if (estimatedF0 > 0.0f)
    {
        // 1. Retain the server's logical MIDI root key to ensure perfect octave mapping
        // 2. Calculate the cents offset relative to the expected frequency of this root key
        double expectedFrequency = 440.0 * std::pow (2.0, (result.midiRootKey - 69.0) / 12.0);
        
        double centsReal = 1200.0 * std::log2 (static_cast<double> (estimatedF0) / expectedFrequency);
        
        // Wrap cents to [-600, 600] range to get the nearest pitch class tuning offset
        while (centsReal > 600.0)  centsReal -= 1200.0;
        while (centsReal < -600.0) centsReal += 1200.0;
        
        // If the offset is within a sensible range of +-150 cents, apply it!
        // Otherwise, the AI likely generated a different pitch class or noise, so we skip cents correction to avoid distortion.
        if (std::abs (centsReal) <= 150.0)
        {
            result.pitchCorrectionCents = static_cast<float> (centsReal);
            DBG ("[InGen DSP] Detected frequency: " << estimatedF0 << " Hz. Expected: " << expectedFrequency << " Hz. Applying cents correction: " << centsReal);
        }
        else
        {
            result.pitchCorrectionCents = 0.0f;
            DBG ("[InGen DSP] Detected frequency: " << estimatedF0 << " Hz. Out of bounds of pitch class. Cents offset ignored.");
        }
    }
    else
    {
        result.pitchCorrectionCents = 0.0f;
        DBG ("[InGen DSP] Signal unpitched. Retaining server default MIDI root key: " << result.midiRootKey);
    }

    // 3. Deliver results safely back to the Message Thread (GUI thread)
    // We capture by value/copy to avoid life-cycle issues since this job will be destroyed.
    auto finalResult = result;
    auto finalCallback = completionCallback;
    double sampleRate = reader->sampleRate;
    
    juce::MessageManager::callAsync ([finalResult, fullBuffer, sampleRate, cropStart, cropEnd, finalCallback]() {
        finalCallback (finalResult, fullBuffer, sampleRate, cropStart, cropEnd);
    });
}

} // namespace ingen
