#include "WaveformVisualizer.h"
#include "GlassmorphicLookAndFeel.h"
#include <cmath>

namespace ingen {

WaveformVisualizer::WaveformVisualizer()
{
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void WaveformVisualizer::setSound (SamplerSoundA* newSound)
{
    activeSound = newSound;
    repaint();
}

void WaveformVisualizer::clear()
{
    activeSound = nullptr;
    repaint();
}

float WaveformVisualizer::sampleToX (int sampleIndex) const
{
    if (activeSound == nullptr || activeSound->sampleData.getNumSamples() <= 0)
        return 0.0f;

    float ratio = static_cast<float> (sampleIndex) / static_cast<float> (activeSound->sampleData.getNumSamples());
    return ratio * static_cast<float> (getWidth());
}

int WaveformVisualizer::xToSample (float x) const
{
    if (activeSound == nullptr || activeSound->sampleData.getNumSamples() <= 0)
        return 0;

    float ratio = x / static_cast<float> (getWidth());
    int sample = static_cast<int> (ratio * activeSound->sampleData.getNumSamples());
    
    return juce::jlimit (0, activeSound->sampleData.getNumSamples() - 1, sample);
}

int WaveformVisualizer::snapToZeroCrossing (int sampleIndex) const
{
    if (activeSound == nullptr)
        return sampleIndex;

    const auto& buffer = activeSound->sampleData;
    const int totalSamples = buffer.getNumSamples();
    const float* channelData = buffer.getReadPointer (0);

    // Search bidirectionally up to 256 samples in each direction
    int bestSample = sampleIndex;
    
    for (int offset = 0; offset < 256; ++offset)
    {
        // Search forward
        int forwardIndex = sampleIndex + offset;
        if (forwardIndex > 0 && forwardIndex < totalSamples)
        {
            if (channelData[forwardIndex] * channelData[forwardIndex - 1] <= 0.0f)
            {
                bestSample = forwardIndex;
                break;
            }
        }

        // Search backward
        int backwardIndex = sampleIndex - offset;
        if (backwardIndex > 0 && backwardIndex < totalSamples)
        {
            if (channelData[backwardIndex] * channelData[backwardIndex - 1] <= 0.0f)
            {
                bestSample = backwardIndex;
                break;
            }
        }
    }

    return bestSample;
}

void WaveformVisualizer::paint (juce::Graphics& g)
{
    // Draw Glassmorphic Background Panel
    g.setColour (GlassmorphicLookAndFeel::bgGlass);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
    
    g.setColour (GlassmorphicLookAndFeel::borderGlass);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.0f, 1.0f);

    if (activeSound == nullptr)
    {
        g.setColour (GlassmorphicLookAndFeel::textMuted);
        g.setFont (juce::Font (13.0f, juce::Font::italic));
        g.drawFittedText (TRANS ("Lade Wellenform... prompten Sie ein Sound-Lead!"),
                          getLocalBounds(),
                          juce::Justification::centred, 1);
        return;
    }

    const auto& buffer = activeSound->sampleData;
    const int totalSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (totalSamples <= 0)
        return;

    const float* const* channelPointers = buffer.getArrayOfReadPointers();
    const float centerY = getHeight() * 0.5f;
    const float heightScale = getHeight() * 0.45f;

    // 1. Draw Waveform Peaks
    g.setColour (GlassmorphicLookAndFeel::neonCyan.withAlpha (0.65f));

    for (int x = 0; x < getWidth(); ++x)
    {
        int startSample = xToSample (static_cast<float> (x));
        int endSample = xToSample (static_cast<float> (x + 1));
        
        // Find min/max peaks within this pixel's horizontal range
        float minVal = 0.0f;
        float maxVal = 0.0f;

        for (int sample = startSample; sample < endSample; ++sample)
        {
            for (int channel = 0; channel < numChannels; ++channel)
            {
                float val = channelPointers[channel][sample];
                
                if (val > maxVal) maxVal = val;
                if (val < minVal) minVal = val;
            }
        }

        float yMin = centerY + minVal * heightScale;
        float yMax = centerY + maxVal * heightScale;

        // Draw vertical pixel column
        g.drawVerticalLine (x, yMin, yMax);
    }

    // 2. Draw Crop Boundaries and Darkened Out-Of-Bounds Masks
    float startX = sampleToX (activeSound->cropStart);
    float endX = sampleToX (activeSound->cropEnd);

    // Left Out-Of-Bounds Mask (Crop Start)
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRect (0.0f, 0.0f, startX, static_cast<float> (getHeight()));

    // Right Out-Of-Bounds Mask (Crop End)
    g.fillRect (endX, 0.0f, static_cast<float> (getWidth()) - endX, static_cast<float> (getHeight()));

    // 3. Draw Crop Start Marker line (Neon Cyan)
    g.setColour (GlassmorphicLookAndFeel::neonCyan);
    g.drawVerticalLine (static_cast<int> (startX), 0.0f, static_cast<float> (getHeight()));
    
    // Crop Start Handle circle indicator
    g.fillEllipse (startX - 4.0f, 2.0f, 8.0f, 8.0f);
    g.setColour (juce::Colours::white);
    g.drawEllipse (startX - 4.0f, 2.0f, 8.0f, 8.0f, 1.0f);

    // 4. Draw Crop End Marker line
    g.setColour (GlassmorphicLookAndFeel::neonCyan);
    g.drawVerticalLine (static_cast<int> (endX), 0.0f, static_cast<float> (getHeight()));
    
    // Crop End Handle circle indicator
    g.fillEllipse (endX - 4.0f, getHeight() - 10.0f, 8.0f, 8.0f);
    g.setColour (juce::Colours::white);
    g.drawEllipse (endX - 4.0f, getHeight() - 10.0f, 8.0f, 8.0f, 1.0f);

    // 5. Interactively Change Mouse Cursor based on Hover
    // (Handled beautifully inside mouseDown/mouseDrag)
}

void WaveformVisualizer::resized()
{
}

void WaveformVisualizer::mouseDown (const juce::MouseEvent& event)
{
    if (activeSound == nullptr)
        return;

    float startX = sampleToX (activeSound->cropStart);
    float endX = sampleToX (activeSound->cropEnd);

    // Dynamic mouse boundary checks (within 8-pixel horizontal range)
    if (std::abs (event.position.x - startX) < 8.0f)
    {
        activeDragState = StartMarker;
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    }
    else if (std::abs (event.position.x - endX) < 8.0f)
    {
        activeDragState = EndMarker;
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    }
    else
    {
        activeDragState = None;
    }
}

void WaveformVisualizer::mouseDrag (const juce::MouseEvent& event)
{
    if (activeSound == nullptr || activeDragState == None)
        return;

    int requestedSample = xToSample (event.position.x);
    int snappedSample = snapToZeroCrossing (requestedSample);

    if (activeDragState == StartMarker)
    {
        // Safe check: Prevent StartMarker from overlapping with EndMarker (min 128 samples apart)
        if (snappedSample < activeSound->cropEnd - 128)
        {
            activeSound->cropStart = snappedSample;
            repaint();
        }
    }
    else if (activeDragState == EndMarker)
    {
        // Safe check: Prevent EndMarker from crossing StartMarker
        if (snappedSample > activeSound->cropStart + 128)
        {
            activeSound->cropEnd = snappedSample;
            repaint();
        }
    }
}

void WaveformVisualizer::mouseUp (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);

    setMouseCursor (juce::MouseCursor::NormalCursor);

    if (activeSound != nullptr && activeDragState != None)
    {
        if (onCropChanged)
        {
            onCropChanged (activeSound->cropStart, activeSound->cropEnd);
        }
    }

    activeDragState = None;
}

} // namespace ingen
