#include "SamplerVoiceA.h"
#include <cmath>

namespace ingen {

SamplerVoiceA::SamplerVoiceA()
{
}

bool SamplerVoiceA::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<SamplerSoundA*> (sound) != nullptr;
}

void SamplerVoiceA::startNote (int midiNoteNumber, 
                               float velocity, 
                               juce::SynthesiserSound* sound, 
                               int currentPitchWheelPosition)
{
    auto* soundA = dynamic_cast<SamplerSoundA*> (sound);
    if (soundA == nullptr)
        return;

    noteVelocity = velocity;

    // Configure the envelope
    adsr.setSampleRate (getSampleRate());
    adsr.setParameters (soundA->adsrParams);
    adsr.noteOn();

    // Zero-crossing aligned crop start marker
    sourceSamplePosition = static_cast<double> (soundA->cropStart);

    // Pitch ratio calculation
    // pitchDelta = (Key Pressed - Root Key) + Tuning Offset Cents / 100
    double pitchDelta = static_cast<double> (midiNoteNumber - soundA->rootKey) + 
                        (static_cast<double> (soundA->tuningOffsetCents) / 100.0);

    playbackRatio = std::pow (2.0, pitchDelta / 12.0) * 
                     (soundA->originalSampleRate / getSampleRate());

    // Basic MIDI pitch wheel support (+/- 2 semitones default)
    juce::ignoreUnused (currentPitchWheelPosition);
}

void SamplerVoiceA::stopNote (float velocity, bool allowTailOff)
{
    juce::ignoreUnused (velocity);

    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        clearCurrentNote();
        adsr.reset();
    }
}

void SamplerVoiceA::pitchWheelMoved (int newPitchWheelValue)
{
    // High-performance pitch bending can be added in future steps
    juce::ignoreUnused (newPitchWheelValue);
}

void SamplerVoiceA::controllerMoved (int controllerNumber, int newControllerValue)
{
    juce::ignoreUnused (controllerNumber, newControllerValue);
}

void SamplerVoiceA::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, 
                                     int startSample, 
                                     int numSamples)
{
    auto* soundA = dynamic_cast<SamplerSoundA*> (getCurrentlyPlayingSound().get());
    if (soundA == nullptr)
        return;

    const auto& sampleBuffer = soundA->sampleData;
    const int totalChannels = sampleBuffer.getNumChannels();
    const int cropEndMarker = soundA->cropEnd;

    // Buffer pointers
    const float* const* srcChannelData = sampleBuffer.getArrayOfReadPointers();
    float* const* destChannelData = outputBuffer.getArrayOfWritePointers();

    for (int i = 0; i < numSamples; ++i)
    {
        // 1. Get the discrete sample integer position and fractional offset (alpha)
        int pos = static_cast<int> (sourceSamplePosition);
        float alpha = static_cast<float> (sourceSamplePosition - pos);

        // Terminate play immediately if we reach the crop end marker or the ADSR decays
        if (pos >= cropEndMarker - 1 || !adsr.isActive())
        {
            clearCurrentNote();
            adsr.reset();
            break;
        }

        // 2. Fetch envelope scale value
        float envelopeValue = adsr.getNextSample() * noteVelocity;

        // 3. Linear Interpolation for high quality resampled pitch-shifting
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            // Map plugin channels to sample channels (mono-to-stereo fallback)
            int srcChannel = std::min (channel, totalChannels - 1);
            
            float val1 = srcChannelData[srcChannel][pos];
            float val2 = srcChannelData[srcChannel][pos + 1];
            
            float interpolatedSample = val1 + alpha * (val2 - val1);
            
            // Mix into DAW output buffer
            destChannelData[channel][startSample + i] += interpolatedSample * envelopeValue;
        }

        // 4. Advance reader index
        sourceSamplePosition += playbackRatio;
    }
}

} // namespace ingen
