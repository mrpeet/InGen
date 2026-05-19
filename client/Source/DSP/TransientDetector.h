#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <utility>

namespace ingen {

/**
 * Handles automatic sound cropping using transient detection
 * and zero-crossing alignment to ensure crackle-free sample triggers.
 */
class TransientDetector {
public:
    TransientDetector() = default;
    ~TransientDetector() = default;

    /**
     * Scans an audio buffer to find the best crop start and end points.
     * @param buffer Raw generated audio buffer.
     * @param rmsThreshold Threshold for sound start detection (RMS energy).
     * @param silenceThreshold Threshold for sound decay end detection (silence noise floor).
     * @return Pair containing {startSampleIndex, endSampleIndex}.
     */
    std::pair<int, int> detectCropPoints(const juce::AudioBuffer<float>& buffer, 
                                         float rmsThreshold = 0.015f, 
                                         float silenceThreshold = 0.002f);

    /**
     * Searches around an index to find the exact zero-crossing point.
     * Snaps to the point where data[i] * data[i-1] <= 0.
     * @param channelData Raw floating-point audio channel.
     * @param size Size of the audio buffer.
     * @param targetIndex Index around which to search.
     * @param maxRadius Maximum search radius in samples.
     * @return Snapped sample index.
     */
    static int findNearestZeroCrossing(const float* channelData, int size, int targetIndex, int maxRadius = 500);
};

} // namespace ingen
