// main.cpp  (fixed: public mic helpers, removed unused param warning, safe
// result init)

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "edge-impulse-sdk/dsp/numpy.hpp" // numpy::signal_from_buffer()
#include "model-parameters/model_metadata.h"
#include "model-parameters/model_variables.h"

#include <portaudio.h>

class KeywordSpottingApp {
private:
  // Model constants (from metadata)
  static constexpr size_t MODEL_SR = EI_CLASSIFIER_FREQUENCY; // typically 16000
  static constexpr size_t RAW_SAMPLE_COUNT =
      EI_CLASSIFIER_RAW_SAMPLE_COUNT; // typically 16000
  static constexpr size_t SLICE_HOP =
      (EI_CLASSIFIER_RAW_SAMPLE_COUNT /
       EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW); // e.g. 4000
  static constexpr float DEFAULT_CONFIDENCE_THRESHOLD = 0.6f;

  // Detection/postprocessing parameters (tweak these)
  const int AGG_WINDOWS =
      3; // number of consecutive windows to aggregate (moving average length)
  const float AVG_THRESHOLD =
      0.5f;                     // moving-average threshold to trigger detection
  const int COOLDOWN_MS = 1500; // milliseconds of cooldown after a detection
  const std::string KEYWORD_LABEL =
      "hey_sun"; // label name for keyword (adjust if different)

  // runtime state for postprocessing
  std::deque<float> score_history;
  std::chrono::steady_clock::time_point last_trigger;

  // cached index of the keyword label in model labels (computed on
  // construction)
  int keyword_label_index = -1;

public:
  KeywordSpottingApp() {
    std::cout << "=== Keyword Spotting Application ===" << std::endl;
#ifdef EI_CLASSIFIER_PROJECT_NAME
    std::cout << "Model: " << EI_CLASSIFIER_PROJECT_NAME << std::endl;
#endif
    std::cout << "Categories: ";
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
#ifdef ei_classifier_inferencing_categories
      std::cout << ei_classifier_inferencing_categories[i];
      if (i < EI_CLASSIFIER_LABEL_COUNT - 1)
        std::cout << ", ";
#endif
    }
    std::cout << std::endl;

    std::cout << "Model raw sample count: " << RAW_SAMPLE_COUNT << std::endl;
    std::cout << "Sampling frequency (model): " << MODEL_SR << " Hz"
              << std::endl;
    std::cout << "Slices per window: " << EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW
              << std::endl;
    std::cout << "Slice hop (samples): " << SLICE_HOP << std::endl;
    std::cout << "====================================" << std::endl;

    // find keyword label index (robust to label order)
    keyword_label_index = findLabelIndex();
    if (keyword_label_index < 0) {
      // fallback: set to 0 and print a warning
      std::cerr << "[WARN] Could not find label '" << KEYWORD_LABEL
                << "' among model labels. "
                << "Falling back to index 0. Please update KEYWORD_LABEL if "
                   "necessary."
                << std::endl;
      keyword_label_index = 0;
    }

    last_trigger = std::chrono::steady_clock::now() -
                   std::chrono::milliseconds(COOLDOWN_MS);
  }

  // Synthetic test (keeps your earlier functionality)
  void testWithSyntheticData() {
    std::cout << "\n--- Testing with Synthetic Audio Data ---" << std::endl;
    std::vector<std::string> testSignals = {"sine_wave_440hz", "white_noise",
                                            "silence"};

    for (auto &sig : testSignals) {
      auto data = generateTestSignal(sig);
      if (data.size() == RAW_SAMPLE_COUNT) {
        runInferenceSingleWindow(data.data(), RAW_SAMPLE_COUNT, sig);
      }
    }
  }

  // WAV test (sliding windows)
  // --- Insert helper functions somewhere in the class (private) ---

  // Peak-normalize audio in-place to target peak (0.95 max)
  void normalize_audio_inplace(std::vector<float> &audio,
                               float target_peak = 0.95f) {
    float peak = 1e-9f;
    for (float v : audio)
      peak = std::max(peak, std::abs(v));
    if (peak < 1e-8f)
      return; // avoid dividing by zero (silent file)
    float gain = target_peak / peak;
    for (auto &v : audio)
      v *= gain;
  }

  // Find best 1-second window start (by RMS) with a given step (250ms default)
  size_t find_best_window_start_by_rms(const std::vector<float> &audio,
                                       size_t window_samples,
                                       size_t step_samples) {
    size_t best_start = 0;
    double best_rms = -1.0;
    if (audio.size() < window_samples)
      return 0;
    for (size_t start = 0; start + window_samples <= audio.size();
         start += step_samples) {
      double sumsq = 0.0;
      // compute RMS
      for (size_t i = start; i < start + window_samples; ++i) {
        double s = audio[i];
        sumsq += s * s;
      }
      double rms = std::sqrt(sumsq / window_samples);
      if (rms > best_rms) {
        best_rms = rms;
        best_start = start;
      }
    }
    return best_start;
  }

  // --- Replace your existing testWithWavFile(...) with this improved version
  // ---

  bool testWithWavFile(const std::string &filename) {
    std::cout << "\n--- Testing with WAV File (sliding windows): " << filename
              << " ---" << std::endl;

    std::vector<float> audioData;
    if (!loadWavFile(filename, audioData)) {
      std::cerr << "Failed to load WAV file: " << filename << std::endl;
      return false;
    }

    // Ensure we have enough samples for at least one model window
    if (audioData.size() < RAW_SAMPLE_COUNT) {
      std::cerr
          << "Recording too short for a single inference window. Need at least "
          << RAW_SAMPLE_COUNT << " samples ("
          << (float)RAW_SAMPLE_COUNT / MODEL_SR << " s)." << std::endl;
      return false;
    }

    std::cout << "Loaded WAV: " << filename << " (" << audioData.size()
              << " samples)" << std::endl;

    // Normalize audio to boost low-volume TTS/human recordings
    normalize_audio_inplace(audioData, 0.95f);

    // Find best 1s region by RMS (search step = 250 ms)
    size_t window_samples = RAW_SAMPLE_COUNT;
    size_t step_samples = MODEL_SR / 4; // 250 ms hop for searching
    size_t best_start =
        find_best_window_start_by_rms(audioData, window_samples, step_samples);
    double best_time_s = (double)best_start / MODEL_SR;

    // Report RMS of best window for debugging
    {
      double sumsq = 0.0;
      for (size_t i = best_start; i < best_start + window_samples; ++i) {
        double s = audioData[i];
        sumsq += s * s;
      }
      double best_rms = std::sqrt(sumsq / window_samples);
      std::cout << "[INFO] Best-energy window start: " << best_time_s
                << " s (RMS=" << best_rms << ")" << std::endl;
    }

    // Run sliding-window inference but start at best_start so we capture the
    // spoken region
    size_t hop = SLICE_HOP; // keep model hop (e.g. 4000)
    size_t window = RAW_SAMPLE_COUNT;
    size_t total_windows = 0;

    // We'll allow windows from best_start - (window-hop) up to end, but clamp
    // Start as early as possible so the model can use earlier overlap if
    // available.
    size_t start0 = 0;
    if (best_start > (window - hop)) {
      start0 = best_start - (window - hop);
    } else {
      start0 = 0;
    }

    // Clamp start0 to valid range
    if (start0 + window > audioData.size()) {
      // If weird, fallback to beginning
      start0 = 0;
    }

    std::vector<float> window_buf(window);
    for (size_t start = start0; start + window <= audioData.size();
         start += hop) {
      std::memcpy(window_buf.data(), audioData.data() + start,
                  window * sizeof(float));
      std::cout << "Running window starting at " << std::fixed
                << std::setprecision(3) << (double)start / MODEL_SR << " s ..."
                << std::endl;

      if (!runInferenceSingleWindow(window_buf.data(), window,
                                    filename + " (window)")) {
        std::cerr << "Window start: " << (double)start / MODEL_SR
                  << "s -> ERROR processing window" << std::endl;
      }
      total_windows++;
    }

    std::cout << "Total windows processed: " << total_windows << std::endl;
    return true;
  }

  // --- PUBLIC mic helpers (moved to public to be callable from main) ---
  // Capture ~N samples from microphone at MODEL_SR
  bool recordFromMic(std::vector<float> &audioData,
                     size_t num_samples = EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
      std::cerr << "PortAudio init error: " << Pa_GetErrorText(err)
                << std::endl;
      return false;
    }

    PaStream *stream;
    PaStreamParameters inputParameters;
    inputParameters.device = Pa_GetDefaultInputDevice();
    if (inputParameters.device == paNoDevice) {
      std::cerr << "No default input device." << std::endl;
      Pa_Terminate();
      return false;
    }
    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency =
        Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;

    err = Pa_OpenStream(&stream, &inputParameters,
                        nullptr, // no output
                        EI_CLASSIFIER_FREQUENCY,
                        256, // frames per buffer
                        paClipOff, nullptr, nullptr);
    if (err != paNoError) {
      std::cerr << "PortAudio open error: " << Pa_GetErrorText(err)
                << std::endl;
      Pa_Terminate();
      return false;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
      std::cerr << "PortAudio start error: " << Pa_GetErrorText(err)
                << std::endl;
      Pa_CloseStream(stream);
      Pa_Terminate();
      return false;
    }

    audioData.resize(num_samples);
    std::vector<int16_t> buffer(num_samples);

    size_t samplesRead = 0;
    while (samplesRead < num_samples) {
      size_t framesToRead = std::min<size_t>(256, num_samples - samplesRead);
      err = Pa_ReadStream(stream, buffer.data() + samplesRead, framesToRead);
      if (err != paNoError) {
        std::cerr << "PortAudio read error: " << Pa_GetErrorText(err)
                  << std::endl;
        break;
      }
      samplesRead += framesToRead;
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    // Convert int16 -> float [-1,1]
    for (size_t i = 0; i < num_samples; i++) {
      audioData[i] = static_cast<float>(buffer[i]) / 32768.0f;
    }

    return samplesRead == num_samples;
  }

  // run inference on a raw float buffer
  void testWithBuffer(const std::vector<float> &audio,
                      const std::string &source) {
    if (audio.size() < RAW_SAMPLE_COUNT) {
      std::cerr << "Buffer too short for one window." << std::endl;
      return;
    }
    runInferenceSingleWindow(audio.data(), RAW_SAMPLE_COUNT, source);
  }

private:
  // find label index by matching KEYWORD_LABEL in compiled labels (returns -1
  // if not found)
  int findLabelIndex() {
#ifdef ei_classifier_inferencing_categories
    for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; ++i) {
      if (KEYWORD_LABEL == std::string(ei_classifier_inferencing_categories[i]))
        return i;
    }
#endif
    return -1;
  }

  // Generate synthetic signals
  std::vector<float> generateTestSignal(const std::string &type) {
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
    } else { /* silence left as zeros */
    }
    return sig;
  }

  // Single-window inference helper (returns true on success)
  bool runInferenceSingleWindow(const float *buffer, size_t length,
                                const std::string &source) {
    if (length != RAW_SAMPLE_COUNT) {
      std::cerr << "runInferenceSingleWindow: expected " << RAW_SAMPLE_COUNT
                << " samples, got " << length << std::endl;
      return false;
    }

    // Make a contiguous signal_t from the float buffer
    signal_t signal;
    int err = numpy::signal_from_buffer(
        (float *)buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
    if (err != 0) {
      std::cerr << "Failed to create signal from buffer (" << err << ")"
                << std::endl;
      return false;
    }

    ei_impulse_result_t result;
    std::memset(&result, 0, sizeof(result)); // safer zero-init

    auto t0 = std::chrono::high_resolution_clock::now();
    EI_IMPULSE_ERROR r = run_classifier(
        &signal, &result, true); // debug true -> more logs from SDK
    auto t1 = std::chrono::high_resolution_clock::now();
    auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    std::cout << "Source: " << source << std::endl;
    std::cout << "Inference time: " << ms << " ms" << std::endl;

    if (r != EI_IMPULSE_OK) {
      std::cerr << "Inference failed with error: " << r << std::endl;
      return false;
    }

    // Print classification results
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; ++i) {
      auto c = result.classification[i];
      std::cout << "  " << c.label << ": " << std::fixed << std::setprecision(4)
                << c.value;
      if (c.value >= DEFAULT_CONFIDENCE_THRESHOLD)
        std::cout << "  ***";
      std::cout << std::endl;
    }

    // best prediction
    int best = 0;
    for (size_t i = 1; i < EI_CLASSIFIER_LABEL_COUNT; ++i) {
      if (result.classification[i].value > result.classification[best].value)
        best = (int)i;
    }
    std::cout << "Best prediction: " << result.classification[best].label
              << " (" << result.classification[best].value << ")" << std::endl;

    // --- POSTPROCESSING: aggregate and debounce detection ---
    float keyword_score = 0.0f;
    if (keyword_label_index >= 0 &&
        keyword_label_index < (int)EI_CLASSIFIER_LABEL_COUNT) {
      keyword_score = result.classification[keyword_label_index].value;
    } else {
      // fallback: use best-score if label index unknown
      keyword_score = result.classification[best].value;
    }

    // push to history
    score_history.push_back(keyword_score);
    if ((int)score_history.size() > AGG_WINDOWS)
      score_history.pop_front();

    // compute average
    float sum = 0.0f;
    for (float v : score_history)
      sum += v;
    float avg = sum / static_cast<float>(score_history.size());

    // cooldown check
    auto now = std::chrono::steady_clock::now();
    int ms_since_last =
        static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
                             now - last_trigger)
                             .count());

    std::cout << "[POST] keyword_score=" << std::fixed << std::setprecision(4)
              << keyword_score << " avg(" << score_history.size() << ")=" << avg
              << " ms_since_last=" << ms_since_last << std::endl;

    if (avg >= AVG_THRESHOLD && ms_since_last >= COOLDOWN_MS) {
      std::cout << "[DETECT] Keyword detected! avg=" << avg
                << " (threshold=" << AVG_THRESHOLD << ")" << std::endl;
      last_trigger = now;
    }

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
    if (!file.is_open()) {
      std::cerr << "Could not open " << filename << std::endl;
      return false;
    }

    file.read(reinterpret_cast<char *>(&header), sizeof(header));
    if (strncmp(header.riff_header, "RIFF", 4) != 0 ||
        strncmp(header.wave_header, "WAVE", 4) != 0) {
      std::cerr << "Invalid WAV header" << std::endl;
      return false;
    }

    if (header.audio_format != 1) {
      std::cerr << "Only PCM WAV supported" << std::endl;
      return false;
    }
    if (header.num_channels != 1) {
      std::cerr << "Only mono WAV supported" << std::endl;
      return false;
    }

    size_t samples_to_read = header.data_bytes / sizeof(int16_t);
    std::vector<int16_t> pcm(samples_to_read);
    file.read(reinterpret_cast<char *>(pcm.data()), header.data_bytes);
    if (file.gcount() != static_cast<std::streamsize>(header.data_bytes)) {
      std::cerr << "Could not read all audio bytes" << std::endl;
      return false;
    }

    // convert to float -1..1
    std::vector<float> raw(samples_to_read);
    for (size_t i = 0; i < samples_to_read; ++i)
      raw[i] = (float)pcm[i] / 32768.0f;

    // If not at model sample rate, resample (simple linear)
    if (header.sample_rate != MODEL_SR) {
      std::cout << "Resampling from " << header.sample_rate << " Hz to "
                << MODEL_SR << " Hz" << std::endl;
      audioData = resampleAudio(
          raw, (size_t)((double)raw.size() * MODEL_SR / header.sample_rate));
    } else {
      audioData = std::move(raw);
    }

    return true;
  }

  // simple linear resample (input floats)
  std::vector<float> resampleAudio(const std::vector<float> &in,
                                   size_t outLen) {
    if (in.empty() || outLen == 0)
      return {};
    std::vector<float> out(outLen);
    double ratio = (double)in.size() / (double)outLen;
    for (size_t i = 0; i < outLen; ++i) {
      double src = i * ratio;
      size_t i1 = (size_t)src;
      size_t i2 = std::min(i1 + 1, in.size() - 1);
      double frac = src - i1;
      out[i] = in[i1] * (1.0 - frac) + in[i2] * frac;
    }
    return out;
  }
};

void printUsage(const char *pname) {
  std::cout << "Usage: " << pname
            << " --synthetic | --wav <file> | --mic | --help\n";
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
    if (argc < 3) {
      printUsage(argv[0]);
      return 1;
    }
    app.testWithWavFile(argv[2]);
  } else if (arg == "--mic") {
    std::vector<float> micData;
    std::cout << "Listening... say \""
              << "hey_sun"
              << "\" now." << std::endl;
    if (app.recordFromMic(micData)) {
      // Run single inference window
      app.testWithBuffer(micData, "microphone");
    }
  } else {
    printUsage(argv[0]);
    return 1;
  }

  return 0;
}
