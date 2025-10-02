# Keyword Spotting Application

This is a CMake-based application that tests the Edge Impulse keyword spotting library. The application creates an executable named `app` and includes the keyword-spotting library from the `keyword-spotting-cpp-mcu-v5` folder.

## Model Information

- **Project**: keyword-spotting
- **Categories**: hey_sun, noise, unknown
- **Sample Rate**: 16,000 Hz
- **Duration**: 1 second (16,000 samples)
- **Confidence Threshold**: 0.6

## Project Structure

```
keyword-spotting/
├── CMakeLists.txt                    # Root CMake configuration
├── main.cpp                          # Main application source
├── build.sh                          # Build script
├── README.md                         # This file
├── keyword-spotting-cpp-mcu-v5/      # Edge Impulse library
│   ├── CMakeLists.txt               # Library CMake configuration
│   ├── edge-impulse-sdk/            # Edge Impulse SDK
│   ├── model-parameters/            # Model metadata and variables
│   └── tflite-model/                # Compiled TensorFlow Lite model
└── test_app/                        # Additional test utilities
    ├── main.cpp                     # Alternative test implementation
    ├── audio_utils.h/.cpp           # Audio utilities
    └── generate_test_data.py        # Test data generator
```

## Building the Application

### Prerequisites

- CMake 3.13.1 or higher
- C++17 compatible compiler (GCC, Clang, or MSVC)
- Make or Ninja build system

### Build Steps

1. Navigate to the project root directory:
   ```bash
   cd keyword-spotting
   ```

2. Run the build script:
   ```bash
   ./build.sh
   ```

   Or build manually:
   ```bash
   mkdir build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j4
   ```

3. The executable will be created as `build/app`

## Usage

### Command Line Options

```bash
./app [options]
```

**Options:**
- `--synthetic` - Test with synthetic audio data (default if no options provided)
- `--wav <file>` - Test with a WAV file
- `--mic` - Test with microphone input (not implemented)
- `--help` - Show help message

### Examples

1. **Test with synthetic data:**
   ```bash
   ./app --synthetic
   ```
   This will test the model with:
   - 440Hz sine wave (should be classified as "unknown")
   - White noise (should be classified as "noise")
   - Silence (should be classified as "noise")

2. **Test with a WAV file:**
   ```bash
   ./app --wav test_audio.wav
   ```

3. **Show help:**
   ```bash
   ./app --help
   ```

## WAV File Requirements

For testing with WAV files, the audio should meet these requirements:

- **Format**: 16-bit PCM WAV
- **Channels**: Mono (single channel)
- **Sample Rate**: 16,000 Hz (will be resampled if different)
- **Duration**: 1 second (16,000 samples)

### Creating Test WAV Files

You can create test WAV files using various tools:

1. **Using FFmpeg:**
   ```bash
   # Generate a 440Hz tone
   ffmpeg -f lavfi -i "sine=frequency=440:duration=1" -ar 16000 -ac 1 test_tone.wav
   
   # Generate white noise
   ffmpeg -f lavfi -i "anoisesrc=duration=1" -ar 16000 -ac 1 test_noise.wav
   ```

2. **Using the test data generator:**
   ```bash
   cd test_app
   python3 generate_test_data.py
   ```

## Expected Results

### Synthetic Data Tests

1. **440Hz Sine Wave**: Should be classified as "unknown" with high confidence
2. **White Noise**: Should be classified as "noise" with high confidence  
3. **Silence**: Should be classified as "noise" with high confidence

### Real Audio Tests

- **"hey_sun" keyword**: Should be classified as "hey_sun" with confidence > 0.6
- **Background noise**: Should be classified as "noise"
- **Other speech**: Should be classified as "unknown"

## API Usage

The application demonstrates how to use the Edge Impulse API:

```cpp
// Include the necessary headers
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "model-parameters/model_metadata.h"
#include "model-parameters/model_variables.h"

// Create signal structure
signal_t signal;
signal.total_length = 16000; // 1 second of audio
signal.get_data = [&audioData](size_t offset, size_t length, float *out_ptr) {
    size_t copy_length = std::min(length, audioData.size() - offset);
    memcpy(out_ptr, audioData.data() + offset, copy_length * sizeof(float));
    return copy_length;
};

// Run inference
ei_impulse_result_t result = { 0 };
EI_IMPULSE_ERROR error = run_classifier(&signal, &result, false);

// Process results
if (error == EI_IMPULSE_OK) {
    for (size_t i = 0; i < result.classification_count; i++) {
        ei_classifier_result_t classification = result.classification[i];
        std::cout << classification.label << ": " << classification.value << std::endl;
    }
}
```

## Troubleshooting

### Build Issues

1. **CMake version too old:**
   ```bash
   # On macOS with Homebrew
   brew install cmake
   
   # On Ubuntu/Debian
   sudo apt-get install cmake
   ```

2. **Compiler not found:**
   ```bash
   # On macOS
   xcode-select --install
   
   # On Ubuntu/Debian
   sudo apt-get install build-essential
   ```

### Runtime Issues

1. **WAV file not found:**
   - Ensure the file path is correct
   - Check file permissions

2. **Invalid WAV format:**
   - Ensure the file is 16-bit PCM WAV
   - Check that the file is not corrupted

3. **Poor classification results:**
   - Ensure audio is 1 second duration
   - Check that sample rate is 16,000 Hz
   - Verify audio quality (not too quiet/loud)

## Performance

The application will display timing information for:
- DSP processing time
- Classification time
- Total inference time

Typical performance on modern hardware:
- DSP: ~1-5 ms
- Classification: ~10-50 ms
- Total: ~15-60 ms

## License

This application is provided as-is for testing the Edge Impulse keyword spotting library. Please refer to the Edge Impulse license terms for the underlying model and SDK.
