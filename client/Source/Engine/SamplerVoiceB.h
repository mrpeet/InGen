#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "SamplerSoundBase.h"

namespace ingen {

/**
 * Custom SynthesiserVoice for Layer B (Foley/Mechanics).
 * Implements Round-Robin, velocity zone triggers, and release-trigger (Note-Off) capabilities.
 */
class SamplerVoiceB : public juce::SynthesiserVoice {
public:
    SamplerVoiceB();
    ~SamplerVoiceB() override = default;

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
    void triggerSound(float velocity);

    const juce::AudioBuffer<float>* activeVariationBuffer = nullptr;
    double sourceSamplePosition = 0.0;
    double playbackRatio = 1.0;
    float triggerVelocity = 0.0f;

    bool isWaitingForRelease = false;
    bool isPlaying = false;
    bool isPlayForward = true;
    std::vector<float> filterState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerVoiceB)
};

} // namespace ingen
