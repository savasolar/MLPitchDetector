// PitchDetector.h

#pragma once
#include <JuceHeader.h>
#include <onnxruntime_cxx_api.h>
#include <vector>

class PitchDetector {

public:
    PitchDetector();
    ~PitchDetector();
    bool initialize(const void* modelData, size_t modelDataLength);
    void processBuffer(const juce::AudioBuffer<float>& buffer);
    float getCurrentFrequency() const;
    float getCurrentConfidence() const;

private:
    Ort::Env env;
    std::unique_ptr<Ort::Session> session;
    Ort::MemoryInfo memoryInfo;
    std::vector<float> internalBuffer;
    float currentFrequency = 0.0f;
    float currentConfidence = 0.0f;
    size_t frameSize = 1024;
    float mapIndexToFrequency(int index) const;
};

