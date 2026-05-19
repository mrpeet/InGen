#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Engine/SamplerSoundBase.h"

namespace ingen {

/**
 * Interactive Waveform Visualizer featuring cached peak drawings,
 * draggable crop markers, zero-crossing snapping, and transient onset guides.
 */
class WaveformVisualizer : public juce::Component {
public:
    WaveformVisualizer();
    ~WaveformVisualizer() override = default;

    /**
     * Bind active tonal sound reference for visualization.
     */
    void setSound (SamplerSoundA* newSound);

    /**
     * Clear active visualizer buffers.
     */
    void clear();

    void paint (juce::Graphics& g) override;
    void resized() override;

    // Mouse overrides for dragging crop lines
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

    // Custom callbacks to notify the parent editor on crop edits
    std::function<void(int newStart, int newEnd)> onCropChanged;

private:
    // Convert crop sample indices to visual X pixels
    float sampleToX (int sampleIndex) const;
    
    // Convert visual X pixels to sample indices
    int xToSample (float x) const;

    // Zero-crossing locking utility
    int snapToZeroCrossing (int sampleIndex) const;

    // Visual boundary checks
    enum DraggedMarker {
        None,
        StartMarker,
        EndMarker
    };

    DraggedMarker activeDragState = None;

    juce::WeakReference<SamplerSoundA> activeSound;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformVisualizer)
};

} // namespace ingen
