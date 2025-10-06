// PitchDetector.cpp

#include "PitchDetector.h"

PitchDetector::PitchDetector() : env(ORT_LOGGING_LEVEL_WARNING, "PitchDetector"),
    memoryInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault))
{}

PitchDetector::~PitchDetector() {}

bool PitchDetector::initialize(const void* modelData, size_t modelDataLength) {
    try {
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(1);
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);
        session = std::make_unique<Ort::Session>(env, modelData, modelDataLength, sessionOptions);
        return true;
    }
    catch (const Ort::Exception& e) {
        DBG("ONNX Runtime error: " + juce::String(e.what()));
        return false;
    }
}

void PitchDetector::processBuffer(const juce::AudioBuffer<float>& buffer) {
    if (buffer.getNumChannels() < 1 || !session) return;
    
    // Handle mono or stereo by averaging channels if stereo
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    std::vector<float> monoData(numSamples);

    if (numChannels == 1) {
        const float* channelData = buffer.getReadPointer(0);
        std::copy(channelData, channelData + numSamples, monoData.begin());
    } else if (numChannels >= 2) {
        const float* left = buffer.getReadPointer(0);
        const float* right = buffer.getReadPointer(1);
        for (int i = 0; i < numSamples; ++i) {
            monoData[i] = (left[i] + right[i]) * 0.5f;
        }
    } else {
        const float* channelData = buffer.getReadPointer(0);
        std::copy(channelData, channelData + numSamples, monoData.begin());
    }

    internalBuffer.insert(internalBuffer.end(), monoData.begin(), monoData.end());

    
    // Process frames when enough samples are available
    while (internalBuffer.size() >= frameSize) {
        std::vector<float> frame(internalBuffer.begin(), internalBuffer.begin() + frameSize);

        // Prepare input tensor
        std::vector<int64_t> inputShape = { 1, static_cast<int64_t>(frameSize) };
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo, frame.data(), frame.size(), inputShape.data(), inputShape.size());

        // Get input/output names
        Ort::AllocatorWithDefaultOptions allocator;
        auto inputName = session->GetInputNameAllocated(0, allocator);
        auto outputName = session->GetOutputNameAllocated(0, allocator);
        const char* inputNames[] = { inputName.get() };
        const char* outputNames[] = { outputName.get() };

        // Run inference
        std::vector<Ort::Value> outputTensors = session->Run(
            Ort::RunOptions{ nullptr }, inputNames, &inputTensor, 1, outputNames, 1);

        if (!outputTensors.empty()) {
            float* outputData = outputTensors[0].GetTensorMutableData<float>();
            int numBins = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape()[1];

            // Find pitch with highest probability
            auto maxIt = std::max_element(outputData, outputData + numBins);
            int maxIndex = std::distance(outputData, maxIt);
            currentConfidence = *maxIt;
            currentFrequency = mapIndexToFrequency(maxIndex);
        }

        // Remove processed samples
        internalBuffer.erase(internalBuffer.begin(), internalBuffer.begin() + frameSize);
    }
}

float PitchDetector::getCurrentFrequency() const { return currentFrequency; }
float PitchDetector::getCurrentConfidence() const { return currentConfidence; }

float PitchDetector::mapIndexToFrequency(int index) const {
    const float f_min = 32.7f; // C1
    const float cents_per_bin = 20.0f; // CREPE uses 20 cents per bin

    // fixing detection distortion
    float correctionOffset = 2.67f;

    return f_min * powf(2.0f, (index * cents_per_bin) / 1200.0f) * correctionOffset;
}

