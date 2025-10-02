#include <iostream>
#include <vector>
#include <cstring>
#include <fstream>
#include <chrono>
#include <cmath>
#include <iomanip>

// Edge Impulse includes
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "model-parameters/model_metadata.h"
#include "model-parameters/model_variables.h"

// Audio utilities
#include "audio_utils.h"

class KeywordSpottingTester {
private:
    static constexpr size_t SAMPLE_RATE = 16000;
    static constexpr size_t SAMPLE_COUNT = 16000; // 1 second of audio
    static constexpr float CONFIDENCE_THRESHOLD = 0.6f;
    
public:
    KeywordSpottingTester() {
        std::cout << "=== Keyword Spotting Test Application ===" << std::endl;
        std::cout << "Model: " << EI_CLASSIFIER_PROJECT_NAME << std::endl;
        std::cout << "Categories: ";
        for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            std::cout << ei_classifier_inferencing_categories[i];
            if (i < EI_CLASSIFIER_LABEL_COUNT - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        std::cout << "Sample rate: " << SAMPLE_RATE << " Hz" << std::endl;
        std::cout << "Sample duration: " << (float)SAMPLE_COUNT / SAMPLE_RATE << " seconds" << std::endl;
        std::cout << "Confidence threshold: " << CONFIDENCE_THRESHOLD << std::endl;
        std::cout << "=========================================" << std::endl;
    }
    
    // Test with synthetic audio data
    void testWithSyntheticData() {
        std::cout << "\n--- Testing with Synthetic Audio Data ---" << std::endl;
        
        // Generate test signals for each category
        std::vector<std::string> testSignals = {
            "sine_wave_440hz",    // Pure tone - should be classified as "unknown"
            "white_noise",        // Random noise - should be classified as "noise"  
            "silence"             // Silence - should be classified as "noise"
        };
        
        for (const auto& signalType : testSignals) {
            std::cout << "\nTesting signal: " << signalType << std::endl;
            
            std::vector<float> audioData = generateTestSignal(signalType);
            runInference(audioData, signalType);
        }
    }
    
    // Test with WAV file
    bool testWithWavFile(const std::string& filename) {
        std::cout << "\n--- Testing with WAV File: " << filename << " ---" << std::endl;
        
        std::vector<float> audioData;
        if (!loadWavFile(filename, audioData)) {
            std::cerr << "Failed to load WAV file: " << filename << std::endl;
            return false;
        }
        
        // Resample if necessary
        if (audioData.size() != SAMPLE_COUNT) {
            std::cout << "Resampling from " << audioData.size() << " to " << SAMPLE_COUNT << " samples" << std::endl;
            audioData = resampleAudio(audioData, SAMPLE_COUNT);
        }
        
        runInference(audioData, filename);
        return true;
    }
    
    // Test with microphone input (if available)
    void testWithMicrophone() {
        std::cout << "\n--- Testing with Microphone Input ---" << std::endl;
        std::cout << "This would require additional audio capture libraries." << std::endl;
        std::cout << "For now, please use WAV files or synthetic data." << std::endl;
    }
    
private:
    std::vector<float> generateTestSignal(const std::string& type) {
        std::vector<float> signal(SAMPLE_COUNT);
        
        if (type == "sine_wave_440hz") {
            // Generate a 440Hz sine wave
            for (size_t i = 0; i < SAMPLE_COUNT; i++) {
                signal[i] = 0.5f * sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE);
            }
        }
        else if (type == "white_noise") {
            // Generate white noise
            srand(static_cast<unsigned int>(time(nullptr)));
            for (size_t i = 0; i < SAMPLE_COUNT; i++) {
                signal[i] = (2.0f * rand() / RAND_MAX) - 1.0f;
            }
        }
        else if (type == "silence") {
            // Generate silence
            std::fill(signal.begin(), signal.end(), 0.0f);
        }
        else {
            std::cerr << "Unknown signal type: " << type << std::endl;
            std::fill(signal.begin(), signal.end(), 0.0f);
        }
        
        return signal;
    }
    
    void runInference(const std::vector<float>& audioData, const std::string& source) {
        if (audioData.size() != SAMPLE_COUNT) {
            std::cerr << "Error: Audio data size mismatch. Expected " << SAMPLE_COUNT 
                      << ", got " << audioData.size() << std::endl;
            return;
        }
        
        // Create signal structure
        signal_t signal;
        signal.total_length = SAMPLE_COUNT;
        signal.get_data = [&audioData](size_t offset, size_t length, float *out_ptr) {
            size_t copy_length = std::min(length, audioData.size() - offset);
            memcpy(out_ptr, audioData.data() + offset, copy_length * sizeof(float));
            return copy_length;
        };
        
        // Run inference
        ei_impulse_result_t result = { 0 };
        
        auto start_time = std::chrono::high_resolution_clock::now();
        EI_IMPULSE_ERROR error = run_classifier(&signal, &result, false);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Print results
        std::cout << "Source: " << source << std::endl;
        std::cout << "Inference time: " << duration.count() << " ms" << std::endl;
        
        if (error != EI_IMPULSE_OK) {
            std::cerr << "Inference failed with error: " << error << std::endl;
            return;
        }
        
        std::cout << "Results:" << std::endl;
        for (size_t i = 0; i < result.classification_count; i++) {
            ei_classifier_result_t classification = result.classification[i];
            std::cout << "  " << classification.label << ": " 
                      << std::fixed << std::setprecision(4) << classification.value;
            
            if (classification.value >= CONFIDENCE_THRESHOLD) {
                std::cout << " *** HIGH CONFIDENCE ***";
            }
            std::cout << std::endl;
        }
        
        // Find the best prediction
        if (result.classification_count > 0) {
            int best_index = 0;
            for (size_t i = 1; i < result.classification_count; i++) {
                if (result.classification[i].value > result.classification[best_index].value) {
                    best_index = i;
                }
            }
            
            std::cout << "Best prediction: " << result.classification[best_index].label 
                      << " (confidence: " << std::fixed << std::setprecision(4) 
                      << result.classification[best_index].value << ")" << std::endl;
        }
        
        // Print timing information
        if (result.timing.dsp_us > 0) {
            std::cout << "DSP time: " << result.timing.dsp_us << " μs" << std::endl;
        }
        if (result.timing.classification_us > 0) {
            std::cout << "Classification time: " << result.timing.classification_us << " μs" << std::endl;
        }
        if (result.timing.anomaly_us > 0) {
            std::cout << "Anomaly time: " << result.timing.anomaly_us << " μs" << std::endl;
        }
    }
};

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --synthetic    Test with synthetic audio data" << std::endl;
    std::cout << "  --wav <file>   Test with WAV file" << std::endl;
    std::cout << "  --mic          Test with microphone (not implemented)" << std::endl;
    std::cout << "  --help         Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " --synthetic" << std::endl;
    std::cout << "  " << programName << " --wav test_audio.wav" << std::endl;
}

int main(int argc, char* argv[]) {
    KeywordSpottingTester tester;
    
    if (argc < 2) {
        // Default: run synthetic tests
        tester.testWithSyntheticData();
        return 0;
    }
    
    std::string arg1 = argv[1];
    
    if (arg1 == "--help") {
        printUsage(argv[0]);
        return 0;
    }
    else if (arg1 == "--synthetic") {
        tester.testWithSyntheticData();
    }
    else if (arg1 == "--wav") {
        if (argc < 3) {
            std::cerr << "Error: --wav requires a filename argument" << std::endl;
            printUsage(argv[0]);
            return 1;
        }
        tester.testWithWavFile(argv[2]);
    }
    else if (arg1 == "--mic") {
        tester.testWithMicrophone();
    }
    else {
        std::cerr << "Error: Unknown option " << arg1 << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    return 0;
}
