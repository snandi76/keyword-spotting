#ifndef AUDIO_UTILS_H
#define AUDIO_UTILS_H

#include <vector>
#include <string>
#include <cstdint>

// WAV file header structure
#pragma pack(push, 1)
struct WavHeader {
    char riff_header[4];        // "RIFF"
    uint32_t wav_size;          // File size - 8
    char wave_header[4];        // "WAVE"
    char fmt_header[4];         // "fmt "
    uint32_t fmt_chunk_size;    // Format chunk size (usually 16)
    uint16_t audio_format;      // Audio format (1 = PCM)
    uint16_t num_channels;      // Number of channels
    uint32_t sample_rate;       // Sample rate
    uint32_t byte_rate;         // Byte rate
    uint16_t sample_alignment;  // Sample alignment
    uint16_t bit_depth;         // Bit depth
    char data_header[4];        // "data"
    uint32_t data_bytes;        // Data size in bytes
};
#pragma pack(pop)

class AudioUtils {
public:
    // Load a WAV file and return audio data as float values (-1.0 to 1.0)
    static bool loadWavFile(const std::string& filename, std::vector<float>& audioData);
    
    // Save audio data as a WAV file
    static bool saveWavFile(const std::string& filename, const std::vector<float>& audioData, 
                           uint32_t sampleRate = 16000, uint16_t bitDepth = 16);
    
    // Resample audio data to a different length (simple linear interpolation)
    static std::vector<float> resampleAudio(const std::vector<float>& input, size_t targetLength);
    
    // Normalize audio data to [-1.0, 1.0] range
    static void normalizeAudio(std::vector<float>& audioData);
    
    // Convert 16-bit PCM to float
    static float pcm16ToFloat(int16_t pcm);
    
    // Convert float to 16-bit PCM
    static int16_t floatToPcm16(float sample);
    
    // Generate a test tone (sine wave)
    static std::vector<float> generateTone(float frequency, float duration, 
                                          uint32_t sampleRate = 16000, float amplitude = 0.5f);
    
    // Generate white noise
    static std::vector<float> generateWhiteNoise(float duration, 
                                                uint32_t sampleRate = 16000, float amplitude = 0.1f);
    
    // Print audio file information
    static void printWavInfo(const std::string& filename);

private:
    static bool validateWavHeader(const WavHeader& header);
    static void printHeaderInfo(const WavHeader& header);
};

#endif // AUDIO_UTILS_H
