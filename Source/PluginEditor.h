// PluginEditor.h

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class MLPitchDetectorAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    MLPitchDetectorAudioProcessorEditor (MLPitchDetectorAudioProcessor&);
    ~MLPitchDetectorAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    MLPitchDetectorAudioProcessor& audioProcessor;
    
    juce::Image backgroundImage;
    juce::Label frequencyLabel;
    juce::Label confidenceLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MLPitchDetectorAudioProcessorEditor)
};
