// PluginEditor.cpp

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MLPitchDetectorAudioProcessorEditor::MLPitchDetectorAudioProcessorEditor (MLPitchDetectorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Load the background image from BinaryData
    backgroundImage = juce::ImageFileFormat::loadFrom(BinaryData::background_png, BinaryData::background_pngSize);

    // Set up frequency label
    addAndMakeVisible (frequencyLabel);
    frequencyLabel.setFont (juce::Font (22.0f));

    // Set up confidence label
    addAndMakeVisible (confidenceLabel);
    confidenceLabel.setFont (juce::Font (22.0f));

    setSize (400, 300);
    
    startTimerHz(30);
}

MLPitchDetectorAudioProcessorEditor::~MLPitchDetectorAudioProcessorEditor()
{
    
}

void MLPitchDetectorAudioProcessorEditor::timerCallback()
{
    frequencyLabel.setText("Frequency: " + juce::String(audioProcessor.getCurrentFrequency(), 2), juce::dontSendNotification);
    confidenceLabel.setText("Confidence: " + juce::String(audioProcessor.getCurrentConfidence(), 2), juce::dontSendNotification);
}

//==============================================================================
void MLPitchDetectorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImage (backgroundImage, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);

}

void MLPitchDetectorAudioProcessorEditor::resized()
{
    frequencyLabel.setBounds (20, 100, 360, 40);
    confidenceLabel.setBounds (20, 200, 360, 40);
}
