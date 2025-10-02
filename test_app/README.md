# Keyword Spotting Test Application

This is a CMake-based test application for the Edge Impulse keyword spotting library. It allows you to test the keyword spotting model with synthetic audio data, WAV files, or microphone input.

## Model Information

- **Project**: keyword-spotting
- **Categories**: hey_sun, noise, unknown
- **Sample Rate**: 16,000 Hz
- **Duration**: 1 second (16,000 samples)
- **Confidence Threshold**: 0.6

## Building the Application

### Prerequisites

- CMake 3.13.1 or higher
- C++17 compatible compiler (GCC, Clang, or MSVC)
- Make or Ninja build system

### Build Steps

1. Navigate to the test_app directory:
   ```bash
   cd test_app
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

3. The executable will be created as `build/keyword_spotting_test`

## Usage

### Command Line Options

```bash
./keyword_spotting_test [options]
```

**Options:**
- `--synthetic` - Test with synthetic audio data (default if no options provided)
- `--wav <file>` - Test with a WAV file
- `--mic` - Test with microphone input (not implemented)
- `--help` - Show help message

### Examples

1. **Test with synthetic data:**
   ```bash
   ./keyword_spotting_test --synthetic
   ```
   This will test the model with:
   - 440Hz sine wave (should be classified as "unknown")
   - White noise (should be classified as "noise")
   - Silence (should be classified as "noise")

2. **Test with a WAV file:**
   ```bash
   ./keyword_spotting_test --wav test_audio.wav
   ```

3. **Show help:**
   ```bash
   ./keyword_spotting_test --help
   ```

## WAV File Requirements

For testing with WAV files, the audio should meet these requirements:

- **Format**: 16-bit PCM WAV
- **Channels**: Mono (single channel)
- **Sample Rate**: 16,000 Hz (will be resampled if different)
- **Duration**: 1 second (16,000 samples)
- **File Size**: ~32 KB for 1 second of audio

### Creating Test WAV Files

You can create test WAV files using various tools:

1. **Using Audacity:**
   - Create a new project
   - Set project rate to 16,000 Hz
   - Generate → Tone (440 Hz, 1 second)
   - File → Export → Export Audio
   - Choose WAV format, 16-bit PCM

2. **Using FFmpeg:**
   ```bash
   # Generate a 440Hz tone
   ffmpeg -f lavfi -i "sine=frequency=440:duration=1" -ar 16000 -ac 1 test_tone.wav
   
   # Generate white noise
   ffmpeg -f lavfi -i "anoisesrc=duration=1" -ar 16000 -ac 1 test_noise.wav
   ```

3. **Using SoX:**
   ```bash
   # Generate a 440Hz tone
   sox -n -r 16000 -c 1 test_tone.wav synth 1 sine 440
   
   # Generate white noise
   sox -n -r 16000 -c 1 test_noise.wav synth 1 whitenoise
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

## File Structure

```
test_app/
├── main.cpp              # Main application
├── audio_utils.h         # Audio utilities header
├── audio_utils.cpp       # Audio utilities implementation
├── CMakeLists.txt        # CMake configuration
├── build.sh              # Build script
├── README.md             # This file
└── build/                # Build directory (created during build)
    └── keyword_spotting_test  # Executable
```

## License

This test application is provided as-is for testing the Edge Impulse keyword spotting library. Please refer to the Edge Impulse license terms for the underlying model and SDK.
