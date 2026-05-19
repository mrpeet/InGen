#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "SamplerSoundBase.h"
#include "SamplerVoiceA.h"
#include "SamplerVoiceB.h"

namespace ingen {

/**
 * Main coordinator for the dual-layer sampler engine.
 * Hosts two independent JUCE synthesiser instances, routes MIDI messages,
 * and handles dynamic voice assignments.
 */
class SamplerEngine {
public:
    SamplerEngine();
    ~SamplerEngine() = default;

    /**
     * Set up sample rates and buffer parameters.
     */
    void prepareToPlay (double sampleRate, int samplesPerBlock);

    /**
     * Routes MIDI and renders both audio layers in parallel.
     */
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    /**
     * Clears existing sounds and registers a new tonal instrument sample (Layer A).
     */
    void loadTonalSound (const juce::String& name,
                         juce::AudioBuffer<float>&& buffer,
                         double sampleRate,
                         int rootKey,
                         float centsOffset,
                         int cropStart,
                         int cropEnd);

    /**
     * Clears all tonal sounds on Layer A.
     */
    void clearTonalLayer();

    /**
     * Registers a tonal instrument sample with a specific key range (Layer A).
     */
    void addTonalSound (const juce::String& name,
                        juce::AudioBuffer<float>&& buffer,
                        double sampleRate,
                        int rootKey,
                        float centsOffset,
                        int cropStart,
                        int cropEnd,
                        int minNote = 0,
                        int maxNote = 127);

    /**
     * Registers a foley sound variation to Layer B.
     */
    void addFoleySound (const juce::String& name,
                        FoleyTriggerMode mode,
                        juce::AudioBuffer<float>&& buffer,
                        double sampleRate,
                        float minVelocity = 0.0f,
                        float maxVelocity = 1.0f);

    /**
     * Clears all Foley sounds on Layer B to load a fresh mechanical preset profile.
     */
    void clearFoleyLayer();

    /**
     * Link/Decouple presets between Layer A and Layer B.
     */
    void setLayerLinking (bool shouldLink) { isLinked = shouldLink; }
    bool getLayerLinking() const { return isLinked; }

    void setLayerAGain (float gain) { layerAGain = gain; }
    float getLayerAGain() const { return layerAGain; }

    void setLayerBGain (float gain) { layerBGain = gain; }
    float getLayerBGain() const { return layerBGain; }

    /**
     * Get active sounds for GUI visualizers.
     */
    SamplerSoundA* getActiveTonalSound() const;
    SamplerSoundB* getActiveFoleySound() const;

    int getNumTonalSounds() const;
    int getNumFoleySounds() const;

private:
    juce::Synthesiser layerA; // Tonal Layer
    juce::Synthesiser layerB; // Foley / Mechanics Layer

    bool isLinked = true;
    float layerAGain = 1.0f;
    float layerBGain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerEngine)
};

} // namespace ingen
