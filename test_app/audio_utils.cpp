#include "audio_utils.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <random>

bool AudioUtils::loadWavFile(const std::string& filename, std::vector<float>& audioData) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return false;
    }
    
    // Read WAV header
    WavHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));
    
    if (!validateWavHeader(header)) {
        std::cerr << "Error: Invalid WAV file format" << std::endl;
        return false;
    }
    
    // Check if it's mono (single channel)
    if (header.num_channels != 1) {
        std::cerr << "Warning: File has " << header.num_channels 
                  << " channels, converting to mono by taking first channel" << std::endl;
    }
    
    // Check bit depth
    if (header.bit_depth != 16) {
        std::cerr << "Error: Only 16-bit PCM files are supported" << std::endl;
        return false;
    }
    
    // Calculate number of samples
    size_t numSamples = header.data_bytes / (header.bit_depth / 8) / header.num_channels;
    
    // Read audio data
    std::vector<int16_t> pcmData(numSamples);
    file.read(reinterpret_cast<char*>(pcmData.data()), header.data_bytes);
    
    if (file.gcount() != static_cast<std::streamsize>(header.data_bytes)) {
        std::cerr << "Error: Could not read all audio data" << std::endl;
        return false;
    }
    
    // Convert to float
    audioData.resize(numSamples);
    for (size_t i = 0; i < numSamples; i++) {
        audioData[i] = pcm16ToFloat(pcmData[i]);
    }
    
    std::cout << "Loaded WAV file: " << filename << std::endl;
    std::cout << "  Sample rate: " << header.sample_rate << " Hz" << std::endl;
    std::cout << "  Channels: " << header.num_channels << std::endl;
    std::cout << "  Bit depth: " << header.bit_depth << " bits" << std::endl;
    std::cout << "  Duration: " << (float)numSamples / header.sample_rate << " seconds" << std::endl;
    std::cout << "  Samples: " << numSamples << std::endl;
    
    return true;
}

bool AudioUtils::saveWavFile(const std::string& filename, const std::vector<float>& audioData, 
                           uint32_t sampleRate, uint16_t bitDepth) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not create file " << filename << std::endl;
        return false;
    }
    
    // Create WAV header
    WavHeader header;
    strncpy(header.riff_header, "RIFF", 4);
    strncpy(header.wave_header, "WAVE", 4);
    strncpy(header.fmt_header, "fmt ", 4);
    strncpy(header.data_header, "data", 4);
    
    header.fmt_chunk_size = 16;
    header.audio_format = 1; // PCM
    header.num_channels = 1;
    header.sample_rate = sampleRate;
    header.bit_depth = bitDepth;
    header.sample_alignment = header.num_channels * (header.bit_depth / 8);
    header.byte_rate = header.sample_rate * header.sample_alignment;
    header.data_bytes = audioData.size() * (header.bit_depth / 8);
    header.wav_size = 36 + header.data_bytes;
    
    // Write header
    file.write(reinterpret_cast<const char*>(&header), sizeof(WavHeader));
    
    // Convert and write audio data
    for (float sample : audioData) {
        int16_t pcm = floatToPcm16(sample);
        file.write(reinterpret_cast<const char*>(&pcm), sizeof(int16_t));
    }
    
    std::cout << "Saved WAV file: " << filename << std::endl;
    std::cout << "  Sample rate: " << sampleRate << " Hz" << std::endl;
    std::cout << "  Bit depth: " << bitDepth << " bits" << std::endl;
    std::cout << "  Duration: " << (float)audioData.size() / sampleRate << " seconds" << std::endl;
    std::cout << "  Samples: " << audioData.size() << std::endl;
    
    return true;
}

std::vector<float> AudioUtils::resampleAudio(const std::vector<float>& input, size_t targetLength) {
    if (input.empty() || targetLength == 0) {
        return std::vector<float>();
    }
    
    if (input.size() == targetLength) {
        return input;
    }
    
    std::vector<float> output(targetLength);
    double ratio = static_cast<double>(input.size()) / targetLength;
    
    for (size_t i = 0; i < targetLength; i++) {
        double sourceIndex = i * ratio;
        size_t index1 = static_cast<size_t>(sourceIndex);
        size_t index2 = std::min(index1 + 1, input.size() - 1);
        
        double fraction = sourceIndex - index1;
        
        // Linear interpolation
        output[i] = input[index1] * (1.0 - fraction) + input[index2] * fraction;
    }
    
    return output;
}

void AudioUtils::normalizeAudio(std::vector<float>& audioData) {
    if (audioData.empty()) {
        return;
    }
    
    // Find maximum absolute value
    float maxVal = 0.0f;
    for (float sample : audioData) {
        maxVal = std::max(maxVal, std::abs(sample));
    }
    
    // Normalize if max value is greater than 1.0
    if (maxVal > 1.0f) {
        float scale = 1.0f / maxVal;
        for (float& sample : audioData) {
            sample *= scale;
        }
    }
}

float AudioUtils::pcm16ToFloat(int16_t pcm) {
    return static_cast<float>(pcm) / 32768.0f;
}

int16_t AudioUtils::floatToPcm16(float sample) {
    // Clamp to [-1.0, 1.0] range
    sample = std::max(-1.0f, std::min(1.0f, sample));
    return static_cast<int16_t>(sample * 32767.0f);
}

std::vector<float> AudioUtils::generateTone(float frequency, float duration, 
                                          uint32_t sampleRate, float amplitude) {
    size_t numSamples = static_cast<size_t>(duration * sampleRate);
    std::vector<float> tone(numSamples);
    
    for (size_t i = 0; i < numSamples; i++) {
        tone[i] = amplitude * sin(2.0f * M_PI * frequency * i / sampleRate);
    }
    
    return tone;
}

std::vector<float> AudioUtils::generateWhiteNoise(float duration, 
                                                uint32_t sampleRate, float amplitude) {
    size_t numSamples = static_cast<size_t>(duration * sampleRate);
    std::vector<float> noise(numSamples);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    
    for (size_t i = 0; i < numSamples; i++) {
        noise[i] = amplitude * dis(gen);
    }
    
    return noise;
}

void AudioUtils::printWavInfo(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }
    
    WavHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));
    
    if (!validateWavHeader(header)) {
        std::cerr << "Error: Invalid WAV file format" << std::endl;
        return;
    }
    
    printHeaderInfo(header);
}

bool AudioUtils::validateWavHeader(const WavHeader& header) {
    // Check RIFF header
    if (strncmp(header.riff_header, "RIFF", 4) != 0) {
        std::cerr << "Error: Not a RIFF file" << std::endl;
        return false;
    }
    
    // Check WAVE header
    if (strncmp(header.wave_header, "WAVE", 4) != 0) {
        std::cerr << "Error: Not a WAVE file" << std::endl;
        return false;
    }
    
    // Check fmt header
    if (strncmp(header.fmt_header, "fmt ", 4) != 0) {
        std::cerr << "Error: Invalid fmt chunk" << std::endl;
        return false;
    }
    
    // Check data header
    if (strncmp(header.data_header, "data", 4) != 0) {
        std::cerr << "Error: Invalid data chunk" << std::endl;
        return false;
    }
    
    // Check audio format (should be PCM = 1)
    if (header.audio_format != 1) {
        std::cerr << "Error: Only PCM format is supported" << std::endl;
        return false;
    }
    
    return true;
}

void AudioUtils::printHeaderInfo(const WavHeader& header) {
    std::cout << "WAV File Information:" << std::endl;
    std::cout << "  File size: " << header.wav_size + 8 << " bytes" << std::endl;
    std::cout << "  Audio format: " << (header.audio_format == 1 ? "PCM" : "Unknown") << std::endl;
    std::cout << "  Channels: " << header.num_channels << std::endl;
    std::cout << "  Sample rate: " << header.sample_rate << " Hz" << std::endl;
    std::cout << "  Byte rate: " << header.byte_rate << " bytes/sec" << std::endl;
    std::cout << "  Block align: " << header.sample_alignment << " bytes" << std::endl;
    std::cout << "  Bit depth: " << header.bit_depth << " bits" << std::endl;
    std::cout << "  Data size: " << header.data_bytes << " bytes" << std::endl;
    
    if (header.sample_rate > 0) {
        size_t numSamples = header.data_bytes / (header.bit_depth / 8) / header.num_channels;
        std::cout << "  Duration: " << (float)numSamples / header.sample_rate << " seconds" << std::endl;
        std::cout << "  Samples: " << numSamples << std::endl;
    }
}
