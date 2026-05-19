#include "SamplerVoiceB.h"
#include <cmath>

namespace ingen {

SamplerVoiceB::SamplerVoiceB()
{
}

bool SamplerVoiceB::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<SamplerSoundB*> (sound) != nullptr;
}

void SamplerVoiceB::startNote (int midiNoteNumber, 
                               float velocity, 
                               juce::SynthesiserSound* sound, 
                               int currentPitchWheelPosition)
{
    juce::ignoreUnused (midiNoteNumber, currentPitchWheelPosition);

    auto* soundB = dynamic_cast<SamplerSoundB*> (sound);
    if (soundB == nullptr)
        return;

    // Check if the trigger velocity falls within this sound's velocity zone
    if (!soundB->velocityRange.contains (velocity))
    {
        clearCurrentNote();
        return;
    }

    if (soundB->triggerMode == FoleyTriggerMode::NoteOn)
    {
        triggerVelocity = velocity;
        triggerSound (velocity);
    }
    else if (soundB->triggerMode == FoleyTriggerMode::NoteOff)
    {
        // Standby phase. Wait for stopNote release trigger
        isWaitingForRelease = true;
        isPlaying = false;
        triggerVelocity = velocity;
    }
}

void SamplerVoiceB::triggerSound (float velocity)
{
    auto* soundB = dynamic_cast<SamplerSoundB*> (getCurrentlyPlayingSound().get());
    if (soundB == nullptr)
    {
        clearCurrentNote();
        return;
    }

    int variationIndex = 0;
    activeVariationBuffer = soundB->getNextVariation (variationIndex);

    if (activeVariationBuffer == nullptr || activeVariationBuffer->getNumSamples() <= 0)
    {
        clearCurrentNote();
        return;
    }

    sourceSamplePosition = 0.0;
    playbackRatio = soundB->getVariationSampleRate (variationIndex) / getSampleRate();
    
    noteVelocity = velocity;
    isPlaying = true;
}

void SamplerVoiceB::stopNote (float velocity, bool allowTailOff)
{
    juce::ignoreUnused (velocity, allowTailOff);

    if (isWaitingForRelease)
    {
        // Trigger Note-Off Foley noise now!
        isWaitingForRelease = false;
        triggerSound (triggerVelocity);
    }
    else if (isPlaying)
    {
        // For Note-On Foley clicks, let the sound naturally play to completion.
        // Offloading foley release stops ensures we don't cut off key click releases.
    }
    else
    {
        clearCurrentNote();
    }
}

void SamplerVoiceB::pitchWheelMoved (int newPitchWheelValue)
{
    juce::ignoreUnused (newPitchWheelValue);
}

void SamplerVoiceB::controllerMoved (int controllerNumber, int newControllerValue)
{
    juce::ignoreUnused (controllerNumber, newControllerValue);
}

void SamplerVoiceB::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, 
                                     int startSample, 
                                     int numSamples)
{
    if (isWaitingForRelease)
        return; // Silent standby phase

    if (!isPlaying || activeVariationBuffer == nullptr)
        return;

    const int totalChannels = activeVariationBuffer->getNumChannels();
    const int totalSamples = activeVariationBuffer->getNumSamples();

    const float* const* srcChannelData = activeVariationBuffer->getArrayOfReadPointers();
    float* const* destChannelData = outputBuffer.getArrayOfWritePointers();

    for (int i = 0; i < numSamples; ++i)
    {
        int pos = static_cast<int> (sourceSamplePosition);
        float alpha = static_cast<float> (sourceSamplePosition - pos);

        // Terminate play immediately if we reach the end of the one-shot sample
        if (pos >= totalSamples - 1)
        {
            clearCurrentNote();
            isPlaying = false;
            activeVariationBuffer = nullptr;
            break;
        }

        float volumeMultiplier = noteVelocity;

        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            int srcChannel = std::min (channel, totalChannels - 1);
            
            float val1 = srcChannelData[srcChannel][pos];
            float val2 = srcChannelData[srcChannel][pos + 1];
            
            float interpolatedSample = val1 + alpha * (val2 - val1);
            
            destChannelData[channel][startSample + i] += interpolatedSample * volumeMultiplier;
        }

        sourceSamplePosition += playbackRatio;
    }
}

} // namespace ingen
