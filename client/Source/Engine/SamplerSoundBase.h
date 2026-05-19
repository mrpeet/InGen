#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

namespace ingen {

/**
 * Trigger policy for Layer B Foley sounds.
 */
enum class FoleyTriggerMode {
    NoteOn,  // Trigger sound when key is pressed (e.g. hammer attack click)
    NoteOff  // Trigger sound when key is released (e.g. damper landing noise)
};

/**
 * Loop Playback Mode for Sampler Layer A.
 */
enum class LoopMode {
    Off = 0,
    Forward,
    PingPong
};

/**
 * Base JUCE Sound mapping class for InGen Sampler.
 */
class SamplerSoundBase : public juce::SynthesiserSound {
public:
    SamplerSoundBase() = default;
    ~SamplerSoundBase() override = default;

    bool appliesToChannel (int /*midiChannel*/) override { return true; }
};

/**
 * Layer A (Tonal) Instrument Sound Definition.
 */
class SamplerSoundA : public SamplerSoundBase {
public:
    SamplerSoundA(const juce::String& name,
                  juce::AudioBuffer<float>&& audioBuffer,
                  double sampleRate,
                  int midiRootKey,
                  float centsOffset,
                  int cropStartSample,
                  int cropEndSample,
                  int minMidiNote = 0,
                  int maxMidiNote = 127)
        : soundName(name),
          sampleData(std::move(audioBuffer)),
          originalSampleRate(sampleRate),
          rootKey(midiRootKey),
          tuningOffsetCents(centsOffset),
          cropStart(cropStartSample),
          cropEnd(cropEndSample),
          minNote(minMidiNote),
          maxNote(maxMidiNote)
    {
        // Default standard ADSR parameters
        adsrParams.attack  = 0.01f; // 10ms
        adsrParams.decay   = 0.1f;  // 100ms
        adsrParams.sustain = 1.0f;  // full volume
        adsrParams.release = 0.5f;  // 500ms decay

        // Default loop parameters
        loopMode = LoopMode::Off;
        loopStart = cropStartSample;
        loopEnd = cropEndSample;
        loopCrossfadePercent = 0.0f;
        isReversed = false;
    }

    bool appliesToNote (int midiNoteNumber) override
    {
        return midiNoteNumber >= minNote && midiNoteNumber <= maxNote;
    }

    const juce::String soundName;
    juce::AudioBuffer<float> sampleData;
    double originalSampleRate;
    int rootKey;
    float tuningOffsetCents;
    int cropStart;
    int cropEnd;

    // Loop Configuration
    LoopMode loopMode;
    int loopStart;
    int loopEnd;
    float loopCrossfadePercent;
    bool isReversed;

    juce::ADSR::Parameters adsrParams;
    
    int minNote = 0;
    int maxNote = 127;

    JUCE_DECLARE_WEAK_REFERENCEABLE (SamplerSoundA)
};

/**
 * Layer B (Foley/Mechanics) Organic Noise Sound Definition.
 */
class SamplerSoundB : public SamplerSoundBase {
public:
    SamplerSoundB(const juce::String& name,
                  FoleyTriggerMode mode,
                  float minVelocity = 0.0f,
                  float maxVelocity = 1.0f)
        : foleyName(name),
          triggerMode(mode),
          velocityRange(minVelocity, maxVelocity)
    {
        isReversed = false;
    }

    bool appliesToNote (int midiNoteNumber) override
    {
        // Foley noises are triggered in parallel with tonal keys.
        // By default, they apply to all notes unless customized.
        juce::ignoreUnused (midiNoteNumber);
        return true;
    }

    /**
     * Adds an alternate variation buffer for Round-Robin selection.
     */
    void addVariation(juce::AudioBuffer<float>&& buffer, double sampleRate)
    {
        variations.push_back(std::move(buffer));
        variationSampleRates.push_back(sampleRate);
    }

    const juce::AudioBuffer<float>* getNextVariation(int& indexToUse) const
    {
        if (variations.empty())
            return nullptr;

        indexToUse = static_cast<int> (currentRRIndex % variations.size());
        currentRRIndex++;
        return &variations[indexToUse];
    }

    double getVariationSampleRate(int index) const
    {
        if (index >= 0 && index < static_cast<int>(variationSampleRates.size()))
            return variationSampleRates[index];
        return 44100.0;
    }

    size_t getNumVariations() const { return variations.size(); }

    const juce::String foleyName;
    FoleyTriggerMode triggerMode;
    juce::Range<float> velocityRange;
    bool isReversed;

private:
    std::vector<juce::AudioBuffer<float>> variations;
    std::vector<double> variationSampleRates;
    
    // Mutable index so we can cycle round-robin even in const getter contexts
    mutable size_t currentRRIndex = 0;
};

} // namespace ingen
