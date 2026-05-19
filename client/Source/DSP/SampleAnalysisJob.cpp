#include "SampleAnalysisJob.h"
#include "PitchDetector.h"
#include "TransientDetector.h"

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
            completionCallback (result, juce::AudioBuffer<float>(), 0, 0);
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

    // 2. Run pitch detection YIN
    PitchDetector pitchDetector (reader->sampleRate);
    float estimatedF0 = pitchDetector.detectPitch (fullBuffer);

    if (estimatedF0 > 0.0f)
    {
        int detectedMidiNote = 60;
        float centsOffset = 0.0f;
        PitchDetector::getMidiNoteAndOffset (estimatedF0, detectedMidiNote, centsOffset);
        
        result.midiRootKey = detectedMidiNote;
        result.pitchCorrectionCents = centsOffset;
        
        DBG ("[InGen DSP] Detected fundamental frequency: " << estimatedF0 << " Hz (MIDI: " << detectedMidiNote << ", cents: " << centsOffset << ")");
    }
    else
    {
        DBG ("[InGen DSP] Signal unpitched. Retaining server default MIDI root key: " << result.midiRootKey);
    }

    // 3. Deliver results safely back to the Message Thread (GUI thread)
    // We capture by value/copy to avoid life-cycle issues since this job will be destroyed.
    auto finalResult = result;
    auto finalCallback = completionCallback;
    
    juce::MessageManager::callAsync ([finalResult, fullBuffer, cropStart, cropEnd, finalCallback]() {
        finalCallback (finalResult, fullBuffer, cropStart, cropEnd);
    });
}

} // namespace ingen
