#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <functional>

namespace ingen {

/**
 * Configuration parameters for the generative AI models.
 */
struct PromptConfig {
    juce::String prompt;
    int noteCount = 5;
    int octaves = 2;
    int foleyCount = 5;
    float temperature = 1.0f;
};

/**
 * Holds the resulting audio file path and associated metadata returned by the generation server.
 */
struct AudioResult {
    juce::File audioFile;
    juce::String rawPrompt;
    int midiRootKey = 60; // Estimated root note (middle C by default)
    float pitchCorrectionCents = 0.0f; // Initial pitch correction calculated offline
};

/**
 * Pure abstract class defining the interface for generating audio samples.
 * This decouples the sampler engine from the underlying HTTP client or local Python server.
 */
class AudioGeneratorAPI {
public:
    virtual ~AudioGeneratorAPI() = default;

    using TonalCallback = std::function<void(const std::vector<AudioResult>& samples, const juce::String& errorMessage)>;
    using FoleyCallback = std::function<void(const std::vector<AudioResult>& samples, const juce::String& errorMessage)>;

    /**
     * Triggers the generation of tonal/melodic samples (Layer A).
     * @param config Generation settings and text prompt.
     * @param callback Callback executed on the message thread when generation completes.
     */
    virtual void generateTonalSamples(const PromptConfig& config, TonalCallback callback) = 0;

    /**
     * Triggers the generation of foley/mechanical samples (Layer B).
     * @param config Generation settings and text prompt.
     * @param callback Callback executed on the message thread when generation completes.
     */
    virtual void generateFoleySamples(const PromptConfig& config, FoleyCallback callback) = 0;
};

} // namespace ingen
