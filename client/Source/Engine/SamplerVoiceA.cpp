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
    isPlayForward = !soundA->isReversed;

    // Configure the envelope
    adsr.setSampleRate (getSampleRate());
    adsr.setParameters (soundA->adsrParams);
    adsr.noteOn();

    // Zero-crossing aligned crop start/end marker
    sourceSamplePosition = soundA->isReversed ? 
                           static_cast<double> (soundA->cropEnd - 2) : 
                           static_cast<double> (soundA->cropStart);

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
    
    // Bounds boundaries
    const int cropStartMarker = soundA->cropStart;
    const int cropEndMarker = soundA->cropEnd;
    
    const LoopMode loopMode = soundA->loopMode;
    const int loopStartMarker = soundA->loopStart;
    const int loopEndMarker = soundA->loopEnd;
    const float crossfadePercent = soundA->loopCrossfadePercent;

    // Buffer pointers
    const float* const* srcChannelData = sampleBuffer.getArrayOfReadPointers();
    float* const* destChannelData = outputBuffer.getArrayOfWritePointers();

    for (int i = 0; i < numSamples; ++i)
    {
        // 1. Get the discrete sample integer position and fractional offset (alpha)
        int pos = static_cast<int> (sourceSamplePosition);
        float alpha = static_cast<float> (sourceSamplePosition - pos);

        // Terminate play immediately if the ADSR decays
        if (!adsr.isActive())
        {
            clearCurrentNote();
            adsr.reset();
            break;
        }

        // Loop Boundaries check
        if (loopMode == LoopMode::Off)
        {
            // Simple linear playback: terminate if out of bounds
            if (pos >= cropEndMarker - 1 || pos < cropStartMarker)
            {
                clearCurrentNote();
                adsr.reset();
                break;
            }
        }
        else
        {
            // Looping mode boundary logic
            if (loopMode == LoopMode::Forward)
            {
                if (isPlayForward)
                {
                    if (sourceSamplePosition >= static_cast<double>(loopEndMarker))
                    {
                        double overshoot = sourceSamplePosition - static_cast<double>(loopEndMarker);
                        double loopLength = static_cast<double>(loopEndMarker - loopStartMarker);
                        
                        if (loopLength > 0.0)
                        {
                            sourceSamplePosition = static_cast<double>(loopStartMarker) + std::fmod(overshoot, loopLength);
                            pos = static_cast<int>(sourceSamplePosition);
                            alpha = static_cast<float>(sourceSamplePosition - pos);
                        }
                        else
                        {
                            clearCurrentNote();
                            adsr.reset();
                            break;
                        }
                    }
                }
                else
                {
                    if (sourceSamplePosition <= static_cast<double>(loopStartMarker))
                    {
                        double undershoot = static_cast<double>(loopStartMarker) - sourceSamplePosition;
                        double loopLength = static_cast<double>(loopEndMarker - loopStartMarker);
                        
                        if (loopLength > 0.0)
                        {
                            sourceSamplePosition = static_cast<double>(loopEndMarker) - std::fmod(undershoot, loopLength);
                            pos = static_cast<int>(sourceSamplePosition);
                            alpha = static_cast<float>(sourceSamplePosition - pos);
                        }
                        else
                        {
                            clearCurrentNote();
                            adsr.reset();
                            break;
                        }
                    }
                }
            }
            else if (loopMode == LoopMode::PingPong)
            {
                if (isPlayForward)
                {
                    if (sourceSamplePosition >= static_cast<double>(loopEndMarker))
                    {
                        isPlayForward = false;
                        double overshoot = sourceSamplePosition - static_cast<double>(loopEndMarker);
                        sourceSamplePosition = static_cast<double>(loopEndMarker) - overshoot;
                        pos = static_cast<int>(sourceSamplePosition);
                        alpha = static_cast<float>(sourceSamplePosition - pos);
                    }
                }
                else
                {
                    if (sourceSamplePosition <= static_cast<double>(loopStartMarker))
                    {
                        isPlayForward = true;
                        double undershoot = static_cast<double>(loopStartMarker) - sourceSamplePosition;
                        sourceSamplePosition = static_cast<double>(loopStartMarker) + undershoot;
                        pos = static_cast<int>(sourceSamplePosition);
                        alpha = static_cast<float>(sourceSamplePosition - pos);
                    }
                }

                // Final safety clip
                if (pos >= cropEndMarker - 1 || pos < cropStartMarker)
                {
                    clearCurrentNote();
                    adsr.reset();
                    break;
                }
            }
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
            float primarySample = val1 + alpha * (val2 - val1);

            float finalSample = primarySample;

            // Apply Equal-Power Loop Crossfading (Forward loops only)
            if (loopMode == LoopMode::Forward && crossfadePercent > 0.0f)
            {
                int loopLength = loopEndMarker - loopStartMarker;
                int crossfadeLength = static_cast<int>(static_cast<float>(loopLength) * crossfadePercent);
                crossfadeLength = std::min(crossfadeLength, loopLength / 2); // Prevent overlapping

                if (crossfadeLength > 0)
                {
                    // If we are within the crossfade zone [loopEndMarker - crossfadeLength, loopEndMarker]
                    if (pos >= loopEndMarker - crossfadeLength && pos < loopEndMarker)
                    {
                        // Calculate linear progress g from 0.0 to 1.0
                        float g = static_cast<float>(pos - (loopEndMarker - crossfadeLength) + alpha) / static_cast<float>(crossfadeLength);
                        g = std::max(0.0f, std::min(1.0f, g));

                        // Equal-Power cos/sin blend curves
                        float fadeOut = std::cos(g * 1.57079632f); // cos(g * pi/2)
                        float fadeIn = std::sin(g * 1.57079632f);  // sin(g * pi/2)

                        // Sample index in loop start area corresponding to the fade phase
                        double blendSourcePos = sourceSamplePosition - static_cast<double>(loopLength);
                        int blendPos = static_cast<int>(blendSourcePos);
                        float blendAlpha = static_cast<float>(blendSourcePos - blendPos);

                        if (blendPos >= loopStartMarker && blendPos + 1 < cropEndMarker)
                        {
                            float b1 = srcChannelData[srcChannel][blendPos];
                            float b2 = srcChannelData[srcChannel][blendPos + 1];
                            float blendSample = b1 + blendAlpha * (b2 - b1);

                            finalSample = (primarySample * fadeOut) + (blendSample * fadeIn);
                        }
                    }
                }
            }
            
            // Mix into DAW output buffer
            destChannelData[channel][startSample + i] += finalSample * envelopeValue;
        }

        // 4. Advance reader index (supporting bidirectional Ping-Pong)
        if (isPlayForward)
            sourceSamplePosition += playbackRatio;
        else
            sourceSamplePosition -= playbackRatio;
    }
}

} // namespace ingen
