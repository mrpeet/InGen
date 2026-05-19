#include "TransientDetector.h"
#include <cmath>
#include <algorithm>

namespace ingen {

std::pair<int, int> TransientDetector::detectCropPoints(const juce::AudioBuffer<float>& buffer, 
                                                       float rmsThreshold, 
                                                       float silenceThreshold)
{
    const int totalSamples = buffer.getNumSamples();
    if (totalSamples <= 0)
        return {0, 0};

    // Analyze first channel (left)
    const float* channelData = buffer.getReadPointer(0);

    const int windowSize = 256;
    int startSample = 0;
    int endSample = totalSamples - 1;

    // 1. Scan forward for the initial transient start point (Onset)
    for (int i = 0; i < totalSamples - windowSize; i += 64)
    {
        // Calculate RMS energy in the active window
        float sumOfSquares = 0.0f;
        for (int j = 0; j < windowSize; ++j)
        {
            float val = channelData[i + j];
            sumOfSquares += val * val;
        }
        float rms = std::sqrt(sumOfSquares / static_cast<float>(windowSize));

        if (rms >= rmsThreshold)
        {
            startSample = i;
            break;
        }
    }

    // Snap the start point to the nearest zero-crossing to prevent pop noises
    startSample = findNearestZeroCrossing(channelData, totalSamples, startSample);

    // 2. Scan backward from the end to find where the signal decays into silence
    for (int i = totalSamples - windowSize - 1; i > startSample + windowSize; i -= 128)
    {
        float sumOfSquares = 0.0f;
        for (int j = 0; j < windowSize; ++j)
        {
            float val = channelData[i + j];
            sumOfSquares += val * val;
        }
        float rms = std::sqrt(sumOfSquares / static_cast<float>(windowSize));

        if (rms >= silenceThreshold)
        {
            endSample = i + windowSize;
            break;
        }
    }

    // Snap the decay end point to its nearest zero-crossing
    endSample = findNearestZeroCrossing(channelData, totalSamples, endSample);

    // Sanity boundary bounds check
    if (startSample >= endSample || startSample < 0 || endSample >= totalSamples)
    {
        startSample = 0;
        endSample = totalSamples - 1;
    }

    return {startSample, endSample};
}

int TransientDetector::findNearestZeroCrossing(const float* channelData, int size, int targetIndex, int maxRadius)
{
    if (targetIndex <= 0 || targetIndex >= size)
        return targetIndex;

    int bestIndex = targetIndex;
    float bestValDiff = 1e10f;
    bool foundPerfectZeroCrossing = false;

    // Search outward (both backward and forward in parallel)
    for (int r = 1; r <= maxRadius; ++r)
    {
        // Check backward sample index
        int idxBack = targetIndex - r;
        if (idxBack > 0)
        {
            // Zero-crossing check: sign change between back and previous
            if (channelData[idxBack] * channelData[idxBack - 1] <= 0.0f)
            {
                // Select index with the absolute smallest sample value (closest to real zero)
                float absVal = std::abs(channelData[idxBack]);
                if (absVal < bestValDiff)
                {
                    bestIndex = idxBack;
                    bestValDiff = absVal;
                    foundPerfectZeroCrossing = true;
                }
            }
        }

        // Check forward sample index
        int idxFwd = targetIndex + r;
        if (idxFwd < size)
        {
            // Zero-crossing check: sign change between forward and previous
            if (channelData[idxFwd] * channelData[idxFwd - 1] <= 0.0f)
            {
                float absVal = std::abs(channelData[idxFwd]);
                if (absVal < bestValDiff)
                {
                    bestIndex = idxFwd;
                    bestValDiff = absVal;
                    foundPerfectZeroCrossing = true;
                }
            }
        }

        // Once we have checked both sides at radius r, if we found a perfect crossing,
        // we can terminate search early (since closer is better for crop accuracy)
        if (foundPerfectZeroCrossing)
            break;
    }

    return bestIndex;
}

} // namespace ingen
