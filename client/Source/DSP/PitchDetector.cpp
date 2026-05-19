#include "PitchDetector.h"
#include <cmath>
#include <vector>

namespace ingen {

PitchDetector::PitchDetector(double sr)
    : sampleRate(sr), bufferSize(0)
{
}

float PitchDetector::detectPitch(const juce::AudioBuffer<float>& buffer, float threshold)
{
    // YIN works best on monophonic inputs. We use the first channel (left).
    const float* signal = buffer.getReadPointer(0);
    const int totalSamples = buffer.getNumSamples();

    // Set analysis window. Typically 2048 samples at 44.1kHz is perfect (covers down to ~40Hz).
    // The search range is half the window size.
    int yinHalfSize = totalSamples / 2;
    if (yinHalfSize <= 0)
        return -1.0f;

    std::vector<float> yinBuffer(yinHalfSize, 0.0f);

    // Step 1: Compute difference function
    difference(signal, totalSamples, yinBuffer.data());

    // Step 2: Compute cumulative mean normalized difference
    cumulativeMeanNormalizedDifference(yinBuffer.data(), yinHalfSize);

    // Step 3: Select absolute threshold
    int tau = absoluteThreshold(yinBuffer.data(), yinHalfSize, threshold);

    if (tau != -1)
    {
        // Step 4: Refine pitch with parabolic interpolation
        float refinedTau = parabolicInterpolation(yinBuffer.data(), tau);
        
        // Step 5: Convert lag (tau) to frequency (Hz)
        if (refinedTau > 0.0f)
        {
            float pitchHz = static_cast<float>(sampleRate) / refinedTau;
            // Filter out unrealistic musical notes (e.g. > 10000Hz or < 20Hz)
            if (pitchHz >= 20.0f && pitchHz <= 10000.0f)
                return pitchHz;
        }
    }

    return -1.0f; // Signal is unpitched or below threshold
}

void PitchDetector::difference(const float* signal, int size, float* yinBuffer)
{
    int halfSize = size / 2;
    
    for (int tau = 0; tau < halfSize; ++tau)
    {
        float diff = 0.0f;
        for (int i = 0; i < halfSize; ++i)
        {
            float delta = signal[i] - signal[i + tau];
            diff += delta * delta;
        }
        yinBuffer[tau] = diff;
    }
}

void PitchDetector::cumulativeMeanNormalizedDifference(float* yinBuffer, int size)
{
    yinBuffer[0] = 1.0f;
    float runningSum = 0.0f;
    
    for (int tau = 1; tau < size; ++tau)
    {
        runningSum += yinBuffer[tau];
        if (runningSum > 0.0f)
        {
            yinBuffer[tau] = yinBuffer[tau] / (runningSum / static_cast<float>(tau));
        }
        else
        {
            yinBuffer[tau] = 1.0f;
        }
    }
}

int PitchDetector::absoluteThreshold(const float* yinBuffer, int size, float threshold)
{
    // Search for first local minimum that is below the threshold
    int firstLocalMinimum = -1;
    float globalMinimumValue = 1e10f;
    int globalMinimumIndex = -1;

    for (int tau = 2; tau < size - 1; ++tau)
    {
        // Check if we have a local minimum
        if (yinBuffer[tau] < yinBuffer[tau - 1] && yinBuffer[tau] < yinBuffer[tau + 1])
        {
            if (yinBuffer[tau] < threshold)
            {
                // Found first local minimum below threshold - return immediately (YIN primary choice)
                return tau;
            }
            
            // Keep track of the global minimum in case no local minimum drops below threshold
            if (yinBuffer[tau] < globalMinimumValue)
            {
                globalMinimumValue = yinBuffer[tau];
                globalMinimumIndex = tau;
            }
        }
    }

    // Fallback to the absolute best local minimum found if nothing was below threshold
    return globalMinimumIndex;
}

float PitchDetector::parabolicInterpolation(const float* yinBuffer, int tau)
{
    // Parabolic interpolation fits a parabola through y(tau-1), y(tau), y(tau+1)
    // to find the exact fractional minimum.
    float alpha = yinBuffer[tau - 1];
    float beta = yinBuffer[tau];
    float gamma = yinBuffer[tau + 1];

    float denominator = 2.0f * (alpha - 2.0f * beta + gamma);
    
    if (std::abs(denominator) > 1e-5f)
    {
        float offset = (alpha - gamma) / denominator;
        return static_cast<float>(tau) + offset;
    }
    
    return static_cast<float>(tau);
}

void PitchDetector::getMidiNoteAndOffset(float frequency, int& outMidiNote, float& outCentsOffset)
{
    if (frequency <= 0.0f)
    {
        outMidiNote = 60;
        outCentsOffset = 0.0f;
        return;
    }

    // Formula: MIDI = 12 * log2(f / 440) + 69
    double midiReal = 12.0 * std::log2(static_cast<double>(frequency) / 440.0) + 69.0;
    
    outMidiNote = static_cast<int>(std::round(midiReal));
    
    // Cents offset = 1200 * log2(f_actual / f_target)
    double targetFrequency = 440.0 * std::pow(2.0, (static_cast<double>(outMidiNote) - 69.0) / 12.0);
    outCentsOffset = static_cast<float>(1200.0 * std::log2(static_cast<double>(frequency) / targetFrequency));
}

} // namespace ingen
