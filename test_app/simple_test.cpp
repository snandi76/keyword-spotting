#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// Simple test to demonstrate the structure without full Edge Impulse SDK
// This shows how the application would work once the SDK is properly integrated

class SimpleKeywordSpottingTest {
private:
    static constexpr size_t SAMPLE_RATE = 16000;
    static constexpr size_t SAMPLE_COUNT = 16000; // 1 second of audio
    static constexpr float CONFIDENCE_THRESHOLD = 0.6f;
    
    // Mock categories (from the actual model)
    std::vector<std::string> categories = {"hey_sun", "noise", "unknown"};
    
public:
    SimpleKeywordSpottingTest() {
        std::cout << "=== Simple Keyword Spotting Test ===" << std::endl;
        std::cout << "Model: keyword-spotting" << std::endl;
        std::cout << "Categories: ";
        for (size_t i = 0; i < categories.size(); i++) {
            std::cout << categories[i];
            if (i < categories.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        std::cout << "Sample rate: " << SAMPLE_RATE << " Hz" << std::endl;
        std::cout << "Sample duration: " << (float)SAMPLE_COUNT / SAMPLE_RATE << " seconds" << std::endl;
        std::cout << "Confidence threshold: " << CONFIDENCE_THRESHOLD << std::endl;
        std::cout << "====================================" << std::endl;
    }
    
    // Mock inference function (simulates what Edge Impulse would do)
    struct MockResult {
        std::string label;
        float confidence;
    };
    
    MockResult mockInference(const std::vector<float>& audioData, const std::string& signalType) {
        MockResult result;
        
        // Simple heuristic-based classification for demonstration
        if (signalType == "sine_wave_440hz") {
            result.label = "unknown";
            result.confidence = 0.85f;
        }
        else if (signalType == "white_noise" || signalType == "silence") {
            result.label = "noise";
            result.confidence = 0.92f;
        }
        else {
            // Analyze audio characteristics
            float rms = calculateRMS(audioData);
            float zeroCrossings = calculateZeroCrossings(audioData);
            
            if (rms < 0.01f) {
                result.label = "noise";
                result.confidence = 0.88f;
            }
            else if (zeroCrossings > 0.1f) {
                result.label = "unknown";
                result.confidence = 0.75f;
            }
            else {
                result.label = "noise";
                result.confidence = 0.70f;
            }
        }
        
        return result;
    }
    
    void testWithSyntheticData() {
        std::cout << "\n--- Testing with Synthetic Audio Data ---" << std::endl;
        
        std::vector<std::string> testSignals = {
            "sine_wave_440hz",
            "white_noise", 
            "silence"
        };
        
        for (const auto& signalType : testSignals) {
            std::cout << "\nTesting signal: " << signalType << std::endl;
            
            std::vector<float> audioData = generateTestSignal(signalType);
            runMockInference(audioData, signalType);
        }
    }
    
private:
    std::vector<float> generateTestSignal(const std::string& type) {
        std::vector<float> signal(SAMPLE_COUNT);
        
        if (type == "sine_wave_440hz") {
            for (size_t i = 0; i < SAMPLE_COUNT; i++) {
                signal[i] = 0.5f * sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE);
            }
        }
        else if (type == "white_noise") {
            srand(static_cast<unsigned int>(time(nullptr)));
            for (size_t i = 0; i < SAMPLE_COUNT; i++) {
                signal[i] = (2.0f * rand() / RAND_MAX) - 1.0f;
            }
        }
        else if (type == "silence") {
            std::fill(signal.begin(), signal.end(), 0.0f);
        }
        
        return signal;
    }
    
    void runMockInference(const std::vector<float>& audioData, const std::string& source) {
        if (audioData.size() != SAMPLE_COUNT) {
            std::cerr << "Error: Audio data size mismatch" << std::endl;
            return;
        }
        
        // Simulate processing time
        auto start_time = std::chrono::high_resolution_clock::now();
        
        MockResult result = mockInference(audioData, source);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Print results
        std::cout << "Source: " << source << std::endl;
        std::cout << "Processing time: " << duration.count() << " ms" << std::endl;
        std::cout << "Result: " << result.label << " (confidence: " 
                  << std::fixed << std::setprecision(4) << result.confidence << ")";
        
        if (result.confidence >= CONFIDENCE_THRESHOLD) {
            std::cout << " *** HIGH CONFIDENCE ***";
        }
        std::cout << std::endl;
    }
    
    float calculateRMS(const std::vector<float>& signal) {
        float sum = 0.0f;
        for (float sample : signal) {
            sum += sample * sample;
        }
        return sqrt(sum / signal.size());
    }
    
    float calculateZeroCrossings(const std::vector<float>& signal) {
        int crossings = 0;
        for (size_t i = 1; i < signal.size(); i++) {
            if ((signal[i-1] >= 0 && signal[i] < 0) || (signal[i-1] < 0 && signal[i] >= 0)) {
                crossings++;
            }
        }
        return static_cast<float>(crossings) / signal.size();
    }
};

int main() {
    std::cout << "Simple Keyword Spotting Test Application" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    std::cout << "This is a simplified test that demonstrates the structure" << std::endl;
    std::cout << "of the keyword spotting application without the full" << std::endl;
    std::cout << "Edge Impulse SDK integration." << std::endl;
    std::cout << std::endl;
    std::cout << "To build the full application with Edge Impulse SDK:" << std::endl;
    std::cout << "1. Ensure you have the Edge Impulse SDK properly set up" << std::endl;
    std::cout << "2. Use the main.cpp and CMakeLists.txt files" << std::endl;
    std::cout << "3. Run: ./build.sh" << std::endl;
    std::cout << std::endl;
    
    SimpleKeywordSpottingTest tester;
    tester.testWithSyntheticData();
    
    std::cout << std::endl;
    std::cout << "=== Test Complete ===" << std::endl;
    std::cout << "This demonstrates the expected behavior of the keyword spotting model." << std::endl;
    std::cout << "The actual Edge Impulse integration would provide real inference results." << std::endl;
    
    return 0;
}
