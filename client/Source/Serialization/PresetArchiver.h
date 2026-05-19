#pragma once

#include <juce_core/juce_core.h>
#include "PresetSerializer.h"

namespace ingen {

/**
 * Handles exporting and importing self-contained InGen preset packages (.ingsam).
 * Combines WAV samples and JSON metadata into a compressed ZIP archive.
 */
class PresetArchiver {
public:
    PresetArchiver() = default;
    ~PresetArchiver() = default;

    /**
     * Exports a preset to a compressed .ingsam file.
     * @param targetFile Target destination path (e.g. C:/Presets/MySynth.ingsam).
     * @param presetMetadata Struct containing presets, ADSR values, etc.
     * @param sampleA Real WAV file for Layer A.
     * @param samplesB List of variation WAV files for Layer B.
     * @return True if export succeeded, false on file/compression errors.
     */
    static bool exportPreset (const juce::File& targetFile,
                              PresetData& presetMetadata,
                              const juce::File& sampleA,
                              const std::vector<juce::File>& samplesB);

    /**
     * Imports and decompresses a self-contained .ingsam package.
     * @param sourceZipFile Path to the .ingsam file.
     * @param extractDir Directory where samples and JSON will be unpacked.
     * @param outMetadata Will be populated with parsed preset settings.
     * @param outSampleA Will be set to the decompressed Layer A WAV file.
     * @param outSamplesB Will be populated with the decompressed Layer B WAV variations.
     * @return True if import was 100% successful.
     */
    static bool importPreset (const juce::File& sourceZipFile,
                              const juce::File& extractDir,
                              PresetData& outMetadata,
                              juce::File& outSampleA,
                              std::vector<juce::File>& outSamplesB);
};

} // namespace ingen
