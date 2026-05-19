#include "SamplerEngine.h"

namespace ingen {

SamplerEngine::SamplerEngine()
{
    // Allocate 16 voices of polyphony for tonal synthesiser (Layer A)
    for (int i = 0; i < 16; ++i)
    {
        layerA.addVoice (new SamplerVoiceA());
    }

    // Allocate 16 voices of polyphony for foley synthesiser (Layer B)
    for (int i = 0; i < 16; ++i)
    {
        layerB.addVoice (new SamplerVoiceB());
    }
}

void SamplerEngine::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    layerA.setCurrentPlaybackSampleRate (sampleRate);
    layerB.setCurrentPlaybackSampleRate (sampleRate);
}

void SamplerEngine::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Render both synthesiser layers in parallel into the target DAW output buffer.
    // Audio is automatically mixed (summed) in-place.
    layerA.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
    layerB.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
}

void SamplerEngine::loadTonalSound (const juce::String& name,
                                     juce::AudioBuffer<float>&& buffer,
                                     double sampleRate,
                                     int rootKey,
                                     float centsOffset,
                                     int cropStart,
                                     int cropEnd)
{
    // Layer A supports one active preset sound mapped across all keys.
    // We clear old sounds before adding the newly generated wave.
    layerA.clearSounds();
    
    layerA.addSound (new SamplerSoundA (name, 
                                        std::move (buffer), 
                                        sampleRate, 
                                        rootKey, 
                                        centsOffset, 
                                        cropStart, 
                                        cropEnd));
}

void SamplerEngine::addFoleySound (const juce::String& name,
                                    FoleyTriggerMode mode,
                                    juce::AudioBuffer<float>&& buffer,
                                    double sampleRate,
                                    float minVelocity,
                                    float maxVelocity)
{
    // Round-Robin Mapping:
    // If a Foley sound with the same preset name already exists, 
    // we append the new wav file as an alternate variation.
    SamplerSoundB* existingSound = nullptr;
    
    for (int i = 0; i < layerB.getNumSounds(); ++i)
    {
        if (auto* s = dynamic_cast<SamplerSoundB*> (layerB.getSound (i).get()))
        {
            if (s->foleyName == name)
            {
                existingSound = s;
                break;
            }
        }
    }

    if (existingSound != nullptr)
    {
        existingSound->addVariation (std::move (buffer), sampleRate);
    }
    else
    {
        auto* s = new SamplerSoundB (name, mode, minVelocity, maxVelocity);
        s->addVariation (std::move (buffer), sampleRate);
        layerB.addSound (s);
    }
}

void SamplerEngine::clearFoleyLayer()
{
    layerB.clearSounds();
}

SamplerSoundA* SamplerEngine::getActiveTonalSound() const
{
    if (layerA.getNumSounds() > 0)
        return dynamic_cast<SamplerSoundA*> (layerA.getSound (0).get());
    return nullptr;
}

SamplerSoundB* SamplerEngine::getActiveFoleySound() const
{
    if (layerB.getNumSounds() > 0)
        return dynamic_cast<SamplerSoundB*> (layerB.getSound (0).get());
    return nullptr;
}

} // namespace ingen
