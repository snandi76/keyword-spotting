#!/usr/bin/env python3
"""
Test data generator for keyword spotting application.
This script generates various test WAV files for testing the keyword spotting model.
"""

import numpy as np
import wave
import os
import argparse

def generate_tone(frequency, duration, sample_rate=16000, amplitude=0.5):
    """Generate a sine wave tone."""
    t = np.linspace(0, duration, int(sample_rate * duration), False)
    signal = amplitude * np.sin(2 * np.pi * frequency * t)
    return signal.astype(np.float32)

def generate_noise(duration, sample_rate=16000, amplitude=0.1):
    """Generate white noise."""
    signal = amplitude * np.random.randn(int(sample_rate * duration))
    return signal.astype(np.float32)

def generate_silence(duration, sample_rate=16000):
    """Generate silence (zeros)."""
    signal = np.zeros(int(sample_rate * duration))
    return signal.astype(np.float32)

def save_wav(filename, signal, sample_rate=16000):
    """Save signal as 16-bit PCM WAV file."""
    # Convert to 16-bit PCM
    signal_16bit = (signal * 32767).astype(np.int16)
    
    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(1)  # Mono
        wav_file.setsampwidth(2)  # 16-bit
        wav_file.setframerate(sample_rate)
        wav_file.writeframes(signal_16bit.tobytes())
    
    print(f"Generated: {filename}")

def main():
    parser = argparse.ArgumentParser(description='Generate test WAV files for keyword spotting')
    parser.add_argument('--output-dir', default='test_data', 
                       help='Output directory for test files (default: test_data)')
    parser.add_argument('--sample-rate', type=int, default=16000,
                       help='Sample rate in Hz (default: 16000)')
    parser.add_argument('--duration', type=float, default=1.0,
                       help='Duration in seconds (default: 1.0)')
    
    args = parser.parse_args()
    
    # Create output directory
    os.makedirs(args.output_dir, exist_ok=True)
    
    print(f"Generating test WAV files in '{args.output_dir}'")
    print(f"Sample rate: {args.sample_rate} Hz")
    print(f"Duration: {args.duration} seconds")
    print()
    
    # Generate test files
    test_files = [
        ("440hz_tone.wav", generate_tone(440, args.duration, args.sample_rate)),
        ("1000hz_tone.wav", generate_tone(1000, args.duration, args.sample_rate)),
        ("white_noise.wav", generate_noise(args.duration, args.sample_rate)),
        ("silence.wav", generate_silence(args.duration, args.sample_rate)),
        ("quiet_noise.wav", generate_noise(args.duration, args.sample_rate, 0.05)),
        ("loud_noise.wav", generate_noise(args.duration, args.sample_rate, 0.3)),
    ]
    
    for filename, signal in test_files:
        filepath = os.path.join(args.output_dir, filename)
        save_wav(filepath, signal, args.sample_rate)
    
    print()
    print("Test files generated successfully!")
    print("You can now test them with:")
    print(f"  ./keyword_spotting_test --wav {args.output_dir}/440hz_tone.wav")
    print(f"  ./keyword_spotting_test --wav {args.output_dir}/white_noise.wav")
    print(f"  ./keyword_spotting_test --wav {args.output_dir}/silence.wav")

if __name__ == "__main__":
    main()
