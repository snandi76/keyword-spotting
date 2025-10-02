// main.cpp  (updated)
// NOTE: overwrite your existing main.cpp with this file and rebuild.

#include <iostream>
#include <vector>
#include <cstring>
#include <fstream>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <cstdint>

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "edge-impulse-sdk/dsp/numpy.hpp"                 // <- numpy::signal_from_buffer()
#include "model-parameters/model_metadata.h"
#include "model-parameters/model_variables.h"

class KeywordSpottingApp {
private:
    // Model constants (from metadata)
    static constexpr size_t MODEL_SR = EI_CLASSIFIER_FREQUENCY;                     // typically 16000
    static constexpr size_t RAW_SAMPLE_COUNT = EI_CLASSIFIER_RAW_SAMPLE_COUNT;      // typically 16000
    static constexpr size_t SLICE_HOP = (EI_CLASSIFIER_RAW_SAMPLE_COUNT / EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW); // e.g. 4000
    static constexpr float CONFIDENCE_THRESHOLD = 0.6f;

public:
    KeywordSpottingApp() {
        std::cout << "=== Keyword Spotting Application ===" << std::endl;
        std::cout << "Model: " << EI_CLASSIFIER_PROJECT_NAME << std::endl;
        std::cout << "Categories: ";
        for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            std::cout << ei_classifier_inferencing_categories[i];
            if (i < EI_CLASSIFIER_LABEL_COUNT - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        std::cout << "Model raw sample count: " << RAW_SAMPLE_COUNT << std::endl;
        std::cout << "Sampling frequency (model): " << MODEL_SR << " Hz" << std::endl;
        std::cout << "Slices per window: " << EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW << std::endl;
        std::cout << "Slice hop (samples): " << SLICE_HOP << std::endl;
        std::cout << "====================================" << std::endl;
    }

    // Synthetic test (keeps your earlier functionality)
    void testWithSyntheticData() {
        std::cout << "\n--- Testing with Synthetic Audio Data ---" << std::endl;
        std::vector<std::string> testSignals = {
            "sine_wave_440hz",
            "white_noise",
            "silence"
        };

        for (auto & sig : testSignals) {
            auto data = generateTestSignal(sig);
            if (data.size() == RAW_SAMPLE_COUNT) {
                runInferenceSingleWindow(data.data(), RAW_SAMPLE_COUNT, sig);
            }
        }
    }

    // WAV test (sliding windows)
    bool testWithWavFile(const std::string & filename) {
        std::cout << "\n--- Testing with WAV File (sliding windows): " << filename << " ---" << std::endl;
        std::vector<float> audioData;
        if (!loadWavFile(filename, audioData)) {
            std::cerr << "Failed to load WAV file: " << filename << std::endl;
            return false;
        }

        // If file shorter than a single model window -> require longer recording
        if (audioData.size() < RAW_SAMPLE_COUNT) {
            std::cerr << "Recording too short for a single inference window. "
                      << "Need at least " << RAW_SAMPLE_COUNT << " samples ("
                      << (float)RAW_SAMPLE_COUNT / MODEL_SR << " s)." << std::endl;
            return false;
        }

        std::cout << "Loaded WAV: " << filename << " (" << audioData.size() << " samples)" << std::endl;

        // Sliding-window inference: copy each window into a contiguous buffer and call run_classifier
        size_t hop = SLICE_HOP;
        size_t window = RAW_SAMPLE_COUNT;
        size_t total_windows = 0;

        // allocate buffer once
        std::vector<float> window_buf(window);

        for (size_t start = 0; start + window <= audioData.size(); start += hop) {
            std::memcpy(window_buf.data(), audioData.data() + start, window * sizeof(float));
            std::cout << "Running window starting at " << std::fixed << std::setprecision(3)
                      << (double)start / MODEL_SR << " s ..." << std::endl;

            if (!runInferenceSingleWindow(window_buf.data(), window, filename + " (window)")) {
                std::cerr << "Window start: " << (double)start / MODEL_SR << "s -> ERROR processing window" << std::endl;
            } else {
                // success printed inside
            }
            total_windows++;
        }

        std::cout << "Total windows processed: " << total_windows << std::endl;
        return true;
    }

private:
    // Generate synthetic signals
    std::vector<float> generateTestSignal(const std::string & type) {
        std::vector<float> sig(RAW_SAMPLE_COUNT, 0.0f);
        if (type == "sine_wave_440hz") {
            for (size_t i = 0; i < RAW_SAMPLE_COUNT; ++i) {
                sig[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * (double)i / MODEL_SR);
            }
        } else if (type == "white_noise") {
            srand((unsigned)time(nullptr));
            for (size_t i = 0; i < RAW_SAMPLE_COUNT; ++i) {
                sig[i] = (2.0f * rand() / (float)RAND_MAX) - 1.0f;
            }
        } else { /* silence left as zeros */ }
        return sig;
    }

    // Single-window inference helper (returns true on EI_CLASSIFIER_OK)
    bool runInferenceSingleWindow(const float *buffer, size_t length, const std::string &source) {
        if (length != RAW_SAMPLE_COUNT) {
            std::cerr << "runInferenceSingleWindow: expected " << RAW_SAMPLE_COUNT
                      << " samples, got " << length << std::endl;
            return false;
        }

        // Make a contiguous signal_t from the float buffer
        signal_t signal;
        int err = numpy::signal_from_buffer((float*)buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
        if (err != 0) {
            std::cerr << "Failed to create signal from buffer (" << err << ")" << std::endl;
            return false;
        }

        ei_impulse_result_t result = { 0 };
        auto t0 = std::chrono::high_resolution_clock::now();
        EI_IMPULSE_ERROR r = run_classifier(&signal, &result, true); // debug true -> more logs from SDK
        auto t1 = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

        std::cout << "Source: " << source << std::endl;
        std::cout << "Inference time: " << ms << " ms" << std::endl;

        if (r != EI_IMPULSE_OK) {
            std::cerr << "Inference failed with error: " << r << std::endl;
            return false;
        }

        // Print classification results
        for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; ++i) {
            auto c = result.classification[i];
            std::cout << "  " << c.label << ": " << std::fixed << std::setprecision(4) << c.value;
            if (c.value >= CONFIDENCE_THRESHOLD) std::cout << "  ***";
            std::cout << std::endl;
        }

        // best prediction
        int best = 0;
        for (size_t i = 1; i < EI_CLASSIFIER_LABEL_COUNT; ++i) {
            if (result.classification[i].value > result.classification[best].value) best = i;
        }
        std::cout << "Best prediction: " << result.classification[best].label
                  << " (" << result.classification[best].value << ")" << std::endl;

        return true;
    }

    // WAV loader (expects little-endian RIFF 16-bit PCM mono)
    bool loadWavFile(const std::string &filename, std::vector<float> &audioData) {
        struct WavHeader {
            char riff_header[4];
            uint32_t wav_size;
            char wave_header[4];
            char fmt_header[4];
            uint32_t fmt_chunk_size;
            uint16_t audio_format;
            uint16_t num_channels;
            uint32_t sample_rate;
            uint32_t byte_rate;
            uint16_t sample_alignment;
            uint16_t bit_depth;
            char data_header[4];
            uint32_t data_bytes;
        } header;

        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) { std::cerr << "Could not open " << filename << std::endl; return false; }

        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (strncmp(header.riff_header, "RIFF", 4) != 0 ||
            strncmp(header.wave_header, "WAVE", 4) != 0) {
            std::cerr << "Invalid WAV header" << std::endl; return false;
        }

        if (header.audio_format != 1) {
            std::cerr << "Only PCM WAV supported" << std::endl; return false;
        }
        if (header.num_channels != 1) {
            std::cerr << "Only mono WAV supported" << std::endl; return false;
        }

        size_t samples_to_read = header.data_bytes / sizeof(int16_t);
        std::vector<int16_t> pcm(samples_to_read);
        file.read(reinterpret_cast<char*>(pcm.data()), header.data_bytes);
        if (file.gcount() != static_cast<std::streamsize>(header.data_bytes)) {
            std::cerr << "Could not read all audio bytes" << std::endl;
            return false;
        }

        // convert to float -1..1
        std::vector<float> raw(samples_to_read);
        for (size_t i = 0; i < samples_to_read; ++i) raw[i] = (float)pcm[i] / 32768.0f;

        // If not at model sample rate, resample (simple linear)
        if (header.sample_rate != MODEL_SR) {
            std::cout << "Resampling from " << header.sample_rate << " Hz to " << MODEL_SR << " Hz" << std::endl;
            audioData = resampleAudio(raw, (size_t)((double)raw.size() * MODEL_SR / header.sample_rate));
        } else {
            audioData = std::move(raw);
        }

        return true;
    }

    // simple linear resample (input floats)
    std::vector<float> resampleAudio(const std::vector<float> &in, size_t outLen) {
        if (in.empty() || outLen == 0) return {};
        std::vector<float> out(outLen);
        double ratio = (double)in.size() / (double)outLen;
        for (size_t i = 0; i < outLen; ++i) {
            double src = i * ratio;
            size_t i1 = (size_t)src;
            size_t i2 = std::min(i1 + 1, in.size()-1);
            double frac = src - i1;
            out[i] = in[i1] * (1.0 - frac) + in[i2] * frac;
        }
        return out;
    }
};

void printUsage(const char *pname) {
    std::cout << "Usage: " << pname << " --synthetic | --wav <file> | --help\n";
}

int main(int argc, char **argv) {
    KeywordSpottingApp app;

    if (argc < 2) {
        app.testWithSyntheticData();
        return 0;
    }

    std::string arg = argv[1];
    if (arg == "--synthetic") {
        app.testWithSyntheticData();
    } else if (arg == "--wav") {
        if (argc < 3) { printUsage(argv[0]); return 1; }
        app.testWithWavFile(argv[2]);
    } else {
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
