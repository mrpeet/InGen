#include "PresetArchiver.h"

namespace ingen {

bool PresetArchiver::exportPreset (const juce::File& targetFile,
                                  PresetData& presetMetadata,
                                  const juce::File& sampleA,
                                  const std::vector<juce::File>& samplesB)
{
    // 1. Establish temporary staging directory inside system temp directory
    juce::File tempDir = juce::File::getSpecialLocation (juce::File::SpecialLocationType::tempDirectory)
                             .getChildFile ("InGenExport_" + juce::String (juce::Random::getSystemRandom().nextInt()));
    
    if (!tempDir.createDirectory())
        return false;

    // 2. Stage Layer A (Tonal WAV)
    juce::File stagedSampleA = tempDir.getChildFile ("layerA_sample.wav");
    if (!sampleA.copyFileTo (stagedSampleA))
    {
        tempDir.deleteRecursively();
        return false;
    }
    presetMetadata.samplePathA = "layerA_sample.wav";

    // 3. Stage Layer B (Foley alternate WAV variations)
    presetMetadata.samplePathsB.clear();
    std::vector<juce::File> stagedSamplesB;
    
    for (size_t i = 0; i < samplesB.size(); ++i)
    {
        juce::String relPath = "layerB_variation_" + juce::String (i) + ".wav";
        juce::File stagedSampleB = tempDir.getChildFile (relPath);
        
        if (samplesB[i].copyFileTo (stagedSampleB))
        {
            stagedSamplesB.push_back (stagedSampleB);
            presetMetadata.samplePathsB.push_back (relPath);
        }
    }

    // 4. Stage preset.json
    juce::String jsonString = PresetSerializer::serializeToJson (presetMetadata);
    juce::File stagedJson = tempDir.getChildFile ("preset.json");
    if (!stagedJson.replaceWithText (jsonString))
    {
        tempDir.deleteRecursively();
        return false;
    }

    // 5. Compress staging directory into target destination .ingsam ZIP archive
    if (targetFile.exists())
    {
        targetFile.deleteFile();
    }

    bool compressionSucceeded = false;
    
    {
        juce::ZipFile::Builder builder;
        
        // Add JSON metadata entry
        builder.addFile (stagedJson, 9, "preset.json");
        
        // Add Layer A sound entry
        builder.addFile (stagedSampleA, 9, "layerA_sample.wav");
        
        // Add Layer B variations sound entries
        for (size_t i = 0; i < stagedSamplesB.size(); ++i)
        {
            builder.addFile (stagedSamplesB[i], 9, presetMetadata.samplePathsB[i]);
        }
        
        juce::FileOutputStream outStream (targetFile);
        if (outStream.openedOk())
        {
            outStream.setPosition (0);
            outStream.truncate();
            compressionSucceeded = builder.writeToStream (outStream, nullptr);
        }
    }

    // 6. Housekeeping: Recursively clean up temp workspace
    tempDir.deleteRecursively();

    return compressionSucceeded;
}

bool PresetArchiver::importPreset (const juce::File& sourceZipFile,
                                  const juce::File& extractDir,
                                  PresetData& outMetadata,
                                  juce::File& outSampleA,
                                  std::vector<juce::File>& outSamplesB)
{
    if (!sourceZipFile.exists())
        return false;

    if (!extractDir.exists())
    {
        extractDir.createDirectory();
    }

    // 1. Open ZIP and uncompress to extraction workspace
    juce::ZipFile zip (sourceZipFile);
    auto uncompressResult = zip.uncompressTo (extractDir);
    
    if (uncompressResult.failed())
        return false;

    // 2. Read preset.json metadata
    juce::File jsonFile = extractDir.getChildFile ("preset.json");
    if (!jsonFile.exists())
        return false;

    juce::String jsonString = jsonFile.loadFileAsString();
    if (!PresetSerializer::deserializeFromJson (jsonString, outMetadata))
        return false;

    // 3. Map unpacked wav files
    outSampleA = extractDir.getChildFile (outMetadata.samplePathA);
    if (!outSampleA.exists())
        return false;

    outSamplesB.clear();
    for (const auto& path : outMetadata.samplePathsB)
    {
        juce::File varFile = extractDir.getChildFile (path);
        if (varFile.exists())
        {
            outSamplesB.push_back (varFile);
        }
    }

    return true;
}

} // namespace ingen
