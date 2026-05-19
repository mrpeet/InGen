#pragma once

#include <juce_core/juce_core.h>
#include "../Engine/SamplerEngine.h"

namespace ingen {

/**
 * Structure containing all serializable properties of a sampler preset.
 */
struct PresetData {
    // General Engine State
    bool isLinked = true;

    // Layer A Tonal Settings
    juce::String promptA;
    int rootKeyA = 60;
    float centsOffsetA = 0.0f;
    int cropStartA = 0;
    int cropEndA = 0;
    float attackA = 0.01f;
    float decayA = 0.1f;
    float sustainA = 1.0f;
    float releaseA = 0.5f;
    juce::String samplePathA; // Relative path inside preset ZIP

    // Layer B Foley Settings
    juce::String promptB;
    juce::String triggerModeB = "NoteOn"; // "NoteOn" or "NoteOff"
    float minVelocityB = 0.0f;
    float maxVelocityB = 1.0f;
    std::vector<juce::String> samplePathsB; // Alternate variation paths
};

/**
 * Handles reading and writing PresetData to and from JSON formats.
 */
class PresetSerializer {
public:
    PresetSerializer() = default;
    ~PresetSerializer() = default;

    /**
     * Converts a PresetData structure into a formatted JSON string.
     */
    static juce::String serializeToJson (const PresetData& data);

    /**
     * Parses a JSON string and populates a PresetData structure.
     * @return True if parsing succeeded, false if invalid schema.
     */
    static bool deserializeFromJson (const juce::String& jsonString, PresetData& outData);

    /**
     * Convenience helper to extract preset data directly from the active SamplerEngine.
     */
    static PresetData extractFromEngine (const SamplerEngine& engine, 
                                         const juce::String& samplePathA,
                                         const std::vector<juce::String>& samplePathsB);
};

} // namespace ingen
