# Problem
Creating a software to detect keyword spotting. At present I am trying to build in PC later I want to transfer in MCU(STM32F7) board.
Create a stepby step plan  to develop this project.

# Plan
I'll open the problem statement in `docs/technical_study.md` to understand the requirements, then propose a concise, actionable plan and a short implementation todo list for a quick demo.

[1 tool called]

I read the problem statement. I’ll draft a concise, step-by-step plan for a fast PC demo first, then outline follow-ups for MCU (STM32F7), and provide an actionable TODO list you can execute.

[1 tool called]

I defined the quick demo scope and target keywords; next I’ll outline the implementation plan and provide ready-to-run setup commands.

[1 tool called]

### Quick demo plan (PC first, STM32F7 later)

- Scope
  - Target keywords: yes, no, stop, go (+ background noise/silence).
  - 1-second audio clips, 16 kHz mono, MFCC features, sliding window inference.

- Environment
  - Python 3.10+
  - Key libs: numpy, scipy, librosa, sounddevice/soundfile, torch+torchaudio or tensorflow, onnxruntime, tflite-runtime (optional), webrtcvad (optional).

- Data
  - Use Google Speech Commands v2 subset: yes/no/stop/go and `_background_noise_`.
  - Split train/val/test with fixed seed.

- Features
  - MFCC: 40 coefficients, 25 ms window, 10 ms hop; per-utterance mean-variance norm.
  - Augmentations: time shift, background noise mix, small pitch/tempo (optional).

- Model
  - Tiny KWS: DS-CNN (depthwise-separable conv) or MobileNetV1 tiny.
  - Size target: <100k params to ease MCU porting.
  - Output classes: 5 (yes/no/stop/go/unknown-or-silence).

- Training
  - Cross-entropy, Adam, lr schedule; 20–30 epochs.
  - Early stop on val accuracy; save best.

- Evaluation
  - Report accuracy, confusion matrix; ROC per class; pick threshold(s).

- Export
  - Convert to TFLite (float and int8 post-training quant) and ONNX.
  - Verify parity with test set and a few mic samples.

- Live PC demo
  - Mic stream at 16 kHz; 20–30 ms frames.
  - Optional VAD to skip silence.
  - Sliding window: 1 s context, update every 100 ms.
  - Debounce: require N consecutive hits before printing keyword.
  - CLI prints detections with timestamp and score.

- STM32F7 plan (next)
  - Use TFLite Micro; compute MFCC via CMSIS-DSP; 16 kHz PDM->PCM.
  - Memory budget: ~256–512 KB RAM; store model consts in Flash.
  - Replace floating ops with int8 path; test on recorded waveforms.

### Actionable TODOs and commands

- Set up environment
  - Create venv and install deps:
    ```bash
    cd /Users/sunandan/worksplace/AI_ML/keyword-spotting
    python3 -m venv .venv && source .venv/bin/activate
    python -m pip install --upgrade pip
    pip install numpy scipy librosa soundfile sounddevice onnxruntime webrtcvad tqdm matplotlib scikit-learn
    # Choose one of:
    pip install torch torchaudio --index-url https://download.pytorch.org/whl/cpu
    # or
    pip install tensorflow-cpu
    ```

- Data scripts
  - Download and split Speech Commands subset.
  - Create `data/`, `scripts/`, `kws/` (features, model, train, infer).

- Implementation sequence
  - Build MFCC pipeline (batch + streaming).
  - Implement tiny DS-CNN model.
  - Train and save best checkpoint.
  - Export to ONNX + TFLite (float + int8).
  - Build CLI live-mic demo with sliding window and debounce.

- Shortcuts (if you want results fast)
  - Start with a pre-trained tiny KWS model (e.g., torchaudio/tutorial, TF KWS example), then wire up inference and live demo first; backfill training later.

# Ref
- https://www.mlsysbook.ai/contents/labs/arduino/nicla_vision/kws/kws.html#:~:text=Stage%201%3A%20A%20small%20microprocessor,processed%20on%20a%20larger%20model.



