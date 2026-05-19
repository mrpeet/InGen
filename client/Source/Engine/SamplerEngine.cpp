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
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Allocate temporary buffers for Layer A and Layer B to mix them with independent gains
    juce::AudioBuffer<float> bufferA (numChannels, numSamples);
    juce::AudioBuffer<float> bufferB (numChannels, numSamples);
    
    bufferA.clear();
    bufferB.clear();

    // Render both synthesiser layers in parallel into their respective buffers.
    layerA.renderNextBlock (bufferA, midiMessages, 0, numSamples);
    layerB.renderNextBlock (bufferB, midiMessages, 0, numSamples);

    // Apply the independent volume gains
    bufferA.applyGain (layerAGain);
    bufferB.applyGain (layerBGain);

    // Sum them back into the main DAW output buffer
    buffer.clear();
    for (int ch = 0; ch < numChannels; ++ch)
    {
        buffer.addFrom (ch, 0, bufferA.getReadPointer (ch), numSamples);
        buffer.addFrom (ch, 0, bufferB.getReadPointer (ch), numSamples);
    }
}

void SamplerEngine::loadTonalSound (const juce::String& name,
                                     juce::AudioBuffer<float>&& buffer,
                                     double sampleRate,
                                     int rootKey,
                                     float centsOffset,
                                     int cropStart,
                                     int cropEnd)
{
    clearTonalLayer();
    addTonalSound (name, std::move (buffer), sampleRate, rootKey, centsOffset, cropStart, cropEnd, 0, 127);
}

void SamplerEngine::clearTonalLayer()
{
    layerA.clearSounds();
}

void SamplerEngine::addTonalSound (const juce::String& name,
                                    juce::AudioBuffer<float>&& buffer,
                                    double sampleRate,
                                    int rootKey,
                                    float centsOffset,
                                    int cropStart,
                                    int cropEnd,
                                    int minNote,
                                    int maxNote)
{
    layerA.addSound (new SamplerSoundA (name, 
                                        std::move (buffer), 
                                        sampleRate, 
                                        rootKey, 
                                        centsOffset, 
                                        cropStart, 
                                        cropEnd,
                                        minNote,
                                        maxNote));
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

int SamplerEngine::getNumTonalSounds() const
{
    return layerA.getNumSounds();
}

int SamplerEngine::getNumFoleySounds() const
{
    int totalVariations = 0;
    for (int i = 0; i < layerB.getNumSounds(); ++i)
    {
        if (auto* s = dynamic_cast<SamplerSoundB*> (layerB.getSound (i).get()))
        {
            totalVariations += static_cast<int> (s->getNumVariations());
        }
    }
    return totalVariations;
}

} // namespace ingen
