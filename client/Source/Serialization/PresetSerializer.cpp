#include "PresetSerializer.h"

namespace ingen {

juce::String PresetSerializer::serializeToJson (const PresetData& data)
{
    // Create the root dynamic object
    juce::DynamicObject::Ptr rootObj = new juce::DynamicObject();

    rootObj->setProperty ("isLinked", data.isLinked);

    // 1. Serialize Layer A (Tonal)
    juce::DynamicObject::Ptr layerAObj = new juce::DynamicObject();
    layerAObj->setProperty ("prompt", data.promptA);
    layerAObj->setProperty ("rootKey", data.rootKeyA);
    layerAObj->setProperty ("centsOffset", data.centsOffsetA);
    layerAObj->setProperty ("cropStart", data.cropStartA);
    layerAObj->setProperty ("cropEnd", data.cropEndA);
    layerAObj->setProperty ("attack", data.attackA);
    layerAObj->setProperty ("decay", data.decayA);
    layerAObj->setProperty ("sustain", data.sustainA);
    layerAObj->setProperty ("release", data.releaseA);
    layerAObj->setProperty ("samplePath", data.samplePathA);
    
    rootObj->setProperty ("layerA", juce::var (layerAObj.get()));

    // 2. Serialize Layer B (Foley)
    juce::DynamicObject::Ptr layerBObj = new juce::DynamicObject();
    layerBObj->setProperty ("prompt", data.promptB);
    layerBObj->setProperty ("triggerMode", data.triggerModeB);
    layerBObj->setProperty ("minVelocity", data.minVelocityB);
    layerBObj->setProperty ("maxVelocity", data.maxVelocityB);
    
    // Add alternate variation paths list
    juce::Array<juce::var> variationsArray;
    for (const auto& path : data.samplePathsB)
    {
        variationsArray.add (path);
    }
    layerBObj->setProperty ("samplePaths", variationsArray);

    rootObj->setProperty ("layerB", juce::var (layerBObj.get()));

    // Convert dynamic object to raw formatted JSON string
    return juce::JSON::toString (juce::var (rootObj.get()), false, 4);
}

bool PresetSerializer::deserializeFromJson (const juce::String& jsonString, PresetData& outData)
{
    auto parsedVar = juce::JSON::parse (jsonString);
    auto* rootObj = parsedVar.getDynamicObject();

    if (rootObj == nullptr)
        return false;

    // 1. Deserialize global properties
    outData.isLinked = rootObj->getProperty ("isLinked").getValue();

    // 2. Deserialize Layer A
    if (rootObj->hasProperty ("layerA"))
    {
        if (auto* layerA = rootObj->getProperty ("layerA").getDynamicObject())
        {
            outData.promptA = layerA->getProperty ("prompt").toString();
            outData.rootKeyA = static_cast<int> (layerA->getProperty ("rootKey"));
            outData.centsOffsetA = static_cast<float> (layerA->getProperty ("centsOffset"));
            outData.cropStartA = static_cast<int> (layerA->getProperty ("cropStart"));
            outData.cropEndA = static_cast<int> (layerA->getProperty ("cropEnd"));
            outData.attackA = static_cast<float> (layerA->getProperty ("attack"));
            outData.decayA = static_cast<float> (layerA->getProperty ("decay"));
            outData.sustainA = static_cast<float> (layerA->getProperty ("sustain"));
            outData.releaseA = static_cast<float> (layerA->getProperty ("release"));
            outData.samplePathA = layerA->getProperty ("samplePath").toString();
        }
    }

    // 3. Deserialize Layer B
    if (rootObj->hasProperty ("layerB"))
    {
        if (auto* layerB = rootObj->getProperty ("layerB").getDynamicObject())
        {
            outData.promptB = layerB->getProperty ("prompt").toString();
            outData.triggerModeB = layerB->getProperty ("triggerMode").toString();
            outData.minVelocityB = static_cast<float> (layerB->getProperty ("minVelocity"));
            outData.maxVelocityB = static_cast<float> (layerB->getProperty ("maxVelocity"));

            outData.samplePathsB.clear();
            if (auto* sampleArray = layerB->getProperty ("samplePaths").getArray())
            {
                for (int i = 0; i < sampleArray->size(); ++i)
                {
                    outData.samplePathsB.push_back (sampleArray->getReference (i).toString());
                }
            }
        }
    }

    return true;
}

PresetData PresetSerializer::extractFromEngine (const SamplerEngine& engine, 
                                                 const juce::String& samplePathA,
                                                 const std::vector<juce::String>& samplePathsB)
{
    PresetData data;
    data.isLinked = engine.getLayerLinking();
    data.samplePathA = samplePathA;
    data.samplePathsB = samplePathsB;

    // Load active Layer A values
    if (auto* soundA = engine.getActiveTonalSound())
    {
        data.promptA = soundA->soundName; // Set prompt as sound identifier name
        data.rootKeyA = soundA->rootKey;
        data.centsOffsetA = soundA->tuningOffsetCents;
        data.cropStartA = soundA->cropStart;
        data.cropEndA = soundA->cropEnd;
        data.attackA = soundA->adsrParams.attack;
        data.decayA = soundA->adsrParams.decay;
        data.sustainA = soundA->adsrParams.sustain;
        data.releaseA = soundA->adsrParams.release;
    }

    // Load active Layer B values
    if (auto* soundB = engine.getActiveFoleySound())
    {
        data.promptB = soundB->foleyName;
        data.triggerModeB = (soundB->triggerMode == FoleyTriggerMode::NoteOn) ? "NoteOn" : "NoteOff";
        data.minVelocityB = soundB->velocityRange.getStart();
        data.maxVelocityB = soundB->velocityRange.getEnd();
    }

    return data;
}

} // namespace ingen
