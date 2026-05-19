#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace ingen {

/**
 * High-performance, self-contained YIN pitch detection engine.
 * Used to analyze generated tonal samples (.wav) to detect their fundamental frequency (f0),
 * map them to MIDI root notes, and compute cent-tuning offsets.
 */
class PitchDetector {
public:
    PitchDetector(double sampleRate);
    ~PitchDetector() = default;

    /**
     * Estimates the fundamental frequency (f0) of a monophonic audio buffer.
     * @param buffer Raw audio buffer (typically monophonic).
     * @param threshold YIN absolute threshold (typically 0.1 to 0.15).
     * @return Estimated frequency in Hz. Returns -1.0 if the signal is unpitched/silent.
     */
    float detectPitch(const juce::AudioBuffer<float>& buffer, float threshold = 0.15f);

    /**
     * Helper to convert a frequency to its closest MIDI note and tuning offset in cents.
     * @param frequency Fundamental frequency in Hz.
     * @param outMidiNote Will be set to the closest MIDI note number.
     * @param outCentsOffset Will be set to the fine-tuning offset in cents.
     */
    static void getMidiNoteAndOffset(float frequency, int& outMidiNote, float& outCentsOffset);

private:
    double sampleRate;
    int bufferSize;

    // YIN internal steps
    void difference(const float* signal, int size, float* yinBuffer);
    void cumulativeMeanNormalizedDifference(float* yinBuffer, int size);
    int absoluteThreshold(const float* yinBuffer, int size, float threshold);
    float parabolicInterpolation(const float* yinBuffer, int tau);
};

} // namespace ingen
