// PluginProcessor.cpp

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MLPitchDetectorAudioProcessor::MLPitchDetectorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    pitchDetector(std::make_unique<PitchDetector>()),
    pitchThread(std::make_unique<PitchDetectionThread>(*pitchDetector))
#endif
{
    
    if (!pitchDetector->initialize(BinaryData::crepe_medium_onnx, BinaryData::crepe_medium_onnxSize))
    {
        DBG("Failed to initialize CREPE model");
        pitchDetectorReady.store(false);
    }
    else
    {
        pitchDetectorReady.store(true);
        DBG("CREPE model loaded successfully");
    }

    pitchThread->startThread();
    
}

MLPitchDetectorAudioProcessor::~MLPitchDetectorAudioProcessor()
{
    pitchThread->stopThread(1000);
}

//==============================================================================
const juce::String MLPitchDetectorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MLPitchDetectorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MLPitchDetectorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MLPitchDetectorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MLPitchDetectorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MLPitchDetectorAudioProcessor::getNumPrograms()
{
    return 1;
}

int MLPitchDetectorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MLPitchDetectorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MLPitchDetectorAudioProcessor::getProgramName (int index)
{
    return {};
}

void MLPitchDetectorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void MLPitchDetectorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{

}

void MLPitchDetectorAudioProcessor::releaseResources()
{

}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MLPitchDetectorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void MLPitchDetectorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    if (pitchThread)
        pitchThread->processAudio(buffer);
    

}

void MLPitchDetectorAudioProcessor::PitchDetectionThread::run()
{
    while (!threadShouldExit()) {
        juce::AudioBuffer<float> processingBuffer;

        {
            juce::ScopedLock lock(bufferLock);
            if (writePosition > 0) {
                // Copy accumulated data for processing
                processingBuffer.setSize(accumulatedBuffer.getNumChannels(), writePosition);
                for (int ch = 0; ch < accumulatedBuffer.getNumChannels(); ++ch) {
                    processingBuffer.copyFrom(ch, 0, accumulatedBuffer, ch, 0, writePosition);
                }
                writePosition = 0; // Reset write position
            }
        }

        // Process outside the lock to avoid blocking audio thread
        if (processingBuffer.getNumSamples() > 0) {
            pitchDetector.processBuffer(processingBuffer);
        }

        wait(10);
    }
}

void MLPitchDetectorAudioProcessor::PitchDetectionThread::processAudio(const juce::AudioBuffer<float>& buffer)
{
    juce::ScopedLock lock(bufferLock);

    // Initialize accumulated buffer if needed
    if (accumulatedBuffer.getNumChannels() != buffer.getNumChannels()) {
        accumulatedBuffer.setSize(buffer.getNumChannels(), 8192); // Reasonable size
        writePosition = 0;  // Track where to write next
    }

    // Check if we have enough space
    int samplesNeeded = buffer.getNumSamples();
    if (writePosition + samplesNeeded > accumulatedBuffer.getNumSamples()) {
        // If not enough space, process what we have and start fresh
        writePosition = 0;
    }

    // Append new samples
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        accumulatedBuffer.copyFrom(ch, writePosition, buffer, ch, 0, samplesNeeded);
    }
    writePosition += samplesNeeded;
}

float MLPitchDetectorAudioProcessor::getCurrentFrequency() const
{
    return pitchDetector ? pitchDetector->getCurrentFrequency() : 0.0f;
}

float MLPitchDetectorAudioProcessor::getCurrentConfidence() const
{
    return pitchDetector ? pitchDetector->getCurrentConfidence() : 0.0f;
}

//==============================================================================
bool MLPitchDetectorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MLPitchDetectorAudioProcessor::createEditor()
{
    return new MLPitchDetectorAudioProcessorEditor (*this);
}

//==============================================================================
void MLPitchDetectorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void MLPitchDetectorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MLPitchDetectorAudioProcessor();
}
