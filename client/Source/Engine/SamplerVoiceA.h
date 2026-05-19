#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "SamplerSoundBase.h"

namespace ingen {

/**
 * Custom SynthesiserVoice for Layer A (Tonal playback).
 * Handles pitch-shifting, linear resampling interpolation, ADSR modulation,
 * and strict zero-crossing crop marker reading.
 */
class SamplerVoiceA : public juce::SynthesiserVoice {
public:
    SamplerVoiceA();
    ~SamplerVoiceA() override = default;

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    
    void startNote (int midiNoteNumber, 
                    float velocity, 
                    juce::SynthesiserSound* sound, 
                    int currentPitchWheelPosition) override;
                    
    void stopNote (float velocity, bool allowTailOff) override;
    
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;
    
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, 
                          int startSample, 
                          int numSamples) override;

private:
    double sourceSamplePosition = 0.0;
    double playbackRatio = 1.0;
    float noteVelocity = 0.0f;

    juce::ADSR adsr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerVoiceA)
};

} // namespace ingen
