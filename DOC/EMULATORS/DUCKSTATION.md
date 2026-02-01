# DuckStation - Audio Implementation Analysis

## Overview
**DuckStation** is a modern, highly accurate PlayStation 1 emulator written in C++ with focus on accuracy and performance.

**Source:** https://github.com/stenzek/duckstation  
**Architecture:** Modern C++17, highly threaded, accurate SPU emulation

---

## SPU (Sound Processing Unit) Architecture

### 1. Core Design Philosophy

DuckStation uses a **highly accurate SPU implementation** that prioritizes:
- **Cycle-accurate emulation** where possible
- **Proper ADPCM decoding** with all quirks
- **Correct interpolation** (Gaussian/Cubic)
- **Accurate reverb processing**
- **Pitch modulation** support

### 2. Voice Processing Pipeline

Each of the 24 SPU voices follows this pipeline:

```
ADPCM Decode → Interpolation → Volume ADSR → Pitch Modulation → Mix
     ↓              ↓              ↓              ↓            ↓
  Sample      Gaussian      Envelope      Pitch Mod      Master
  Decode      Interp        Control       (Optional)     Mix
```

### 3. ADPCM Implementation

**Key Features:**
- Full 4-bit ADPCM decoding with proper index tables
- **KON (Key On)** and **KOFF (Key Off)** handling
- **Looping support** with proper end-of-sample detection
- **Noise generation** mode per voice

**Code Pattern (Rust-inspired pseudo-code):**
```cpp
// ADPCM Index Tables (standard PS1 tables)
const INDEX_TABLE: [i32; 8] = [-1, -1, -1, -1, 2, 4, 6, 8];
const STEP_TABLE: [i32; 16] = [
    7, 8, 9, 10, 11, 12, 13, 14,
    16, 17, 19, 21, 23, 25, 28, 31
];

// ADPCM Decode Step
fn decode_adpcm(nibble: u8, step_index: &mut i32, predictor: &mut i32) -> i16 {
    let step = STEP_TABLE[*step_index as usize];
    let mut diff = step >> 3;
    if nibble & 4 != 0 { diff += step; }
    if nibble & 2 != 0 { diff += step >> 1; }
    if nibble & 1 != 0 { diff += step >> 2; }
    if nibble & 8 != 0 { diff = -diff; }
    
    *predictor = (*predictor + diff).clamp(-32768, 32767);
    *step_index = (*step_index + INDEX_TABLE[nibble as usize]).clamp(0, 88);
    
    *predictor as i16
}
```

### 4. Interpolation Methods

DuckStation implements multiple interpolation modes:

#### Gaussian Interpolation (Default)
- 512-entry lookup table
- 4-sample window
- Highest quality, most CPU intensive

```cpp
// 512-entry Gaussian table (simplified)
const GAUSSIAN_TABLE: [i32; 512] = [...]; // Pre-calculated

fn gaussian_interpolate(samples: &[i16; 4], pitch_counter: u32) -> i32 {
    let idx = ((pitch_counter >> 4) & 0xFF) as usize;
    let s = samples.map(|x| i32::from(x));
    
    let mut result = (GAUSSIAN_TABLE[0x0FF - idx] * s[0]) >> 15;
    result += (GAUSSIAN_TABLE[0x1FF - idx] * s[1]) >> 15;
    result += (GAUSSIAN_TABLE[0x100 + idx] * s[2]) >> 15;
    result += (GAUSSIAN_TABLE[idx] * s[3]) >> 15;
    
    result
}
```

#### Cubic Interpolation (Alternative)
- Smoother than Gaussian
- Less accurate to real hardware
- Useful for high-quality audio output

### 5. ADSR Envelope Generator

**Attack Phase:**
- Exponential curve
- Step size based on attack rate
- From 0 to full volume

**Decay Phase:**
- Exponential fall
- Target: sustain level
- Duration based on decay rate

**Sustain Phase:**
- Maintains level
- Until KOFF or end

**Release Phase:**
- Exponential fall to 0
- Triggered by KOFF

### 6. Audio Output Architecture

#### Threading Model
```
Main Thread          Audio Thread           Output Thread
     ↓                      ↓                      ↓
  Emulation           SPU Processing        Audio Backend
  (CPU/GPU)           (Generate Samples)    (SDL/PortAudio/XAudio)
     ↓                      ↓                      ↓
  SPU Registers    →   Sample Buffers    →   Device Callback
  Update               (44100Hz)              (Real-time)
```

#### Buffer Management
- **Ring buffer** with lock-free design
- **Dynamic resampling** for audio sync
- **Underrun/overrun detection**

### 7. Key Implementation Details

#### Volume Control
```cpp
// Master volumes
struct VolumeControl {
    i32 main_left;      // 0x1F801D80
    i32 main_right;     // 0x1F801D82
    i32 cd_left;        // 0x1F801DB0
    i32 cd_right;       // 0x1F801DB2
    i32 ext_left;       // 0x1F801DB4
    i32 ext_right;      // 0x1F801DB6
};
```

#### Pitch Calculation
```cpp
// Pitch counter handling
// Pitch = 0x0000 to 0xFFFF
// Sample rate = (Pitch * 44100) / 4096

fn update_pitch_counter(counter: &mut u32, pitch: u16) {
    *counter += pitch as u32;
    if *counter >= (28 << 12) { // Wrap at sample end
        *counter -= 28 << 12;
    }
}
```

### 8. Performance Optimizations

#### SIMD Usage
- **SSE2/AVX** for sample mixing
- **Vectorized interpolation**
- **Batch processing** of voices

#### Cache Optimization
- Voice data packed tightly
- Sequential memory access patterns
- Minimized cache misses in hot loops

### 9. Audio Backend Support

| Backend | Platform | Features |
|---------|----------|----------|
| SDL2 | Cross-platform | Default, good compatibility |
| XAudio2 | Windows | Low latency, Windows native |
| Cubeb | Cross-platform | Firefox audio library |
| OpenSL ES | Android | Mobile native |
| CoreAudio | macOS | Native macOS/iOS |

### 10. Latency Control

**Target Latency:** 20-50ms (configurable)

**Buffer Strategy:**
1. Small buffer for low latency
2. Larger buffer for stability
3. Dynamic adjustment based on performance

```cpp
struct AudioLatency {
    u32 target_ms;          // Target latency in ms
    u32 buffer_size;        // Samples in buffer
    u32 sample_rate;        // Usually 44100Hz
    u32 safety_margin;      // Extra samples for stability
};
```

### 11. Reverb Processing

DuckStation implements full PS1 reverb:
- **Echo/Reflection simulation**
- **Comb filters** (4 taps)
- **All-pass filters** (2 taps)
- **Configurable reverb work area** in SPU RAM

**Reverb Formula:**
```cpp
// Simplified reverb processing
output = (input * reverb_volume) + 
         (comb1 + comb2 + comb3 + comb4) * 0.25 +
         (allpass1 + allpass2) * 0.5;
```

---

## Comparison with PCSXr360

| Feature | DuckStation | PCSXr360 |
|---------|-------------|----------|
| ADPCM | Full accuracy | Basic implementation |
| Interpolation | Gaussian/Cubic | Simple/None |
| Reverb | Full implementation | Partial/None |
| Pitch Mod | Supported | May not be supported |
| Threading | Multi-threaded | Basic threading |
| SIMD | SSE2/AVX | Limited VMX-128 |
| Latency Control | Configurable | Fixed buffer size |

---

## Recommendations for PCSXr360

### 1. Quick Wins
- **Increase audio buffer** from current size to 4096-8192 samples
- **Add audio sync** to prevent drift
- **Implement basic interpolation** (simple linear or Gaussian)

### 2. Medium Term
- **VMX-128 optimization** for sample mixing
- **ADPCM decode optimization** using lookup tables
- **Proper envelope handling** for all ADSR phases

### 3. Long Term
- **Full SPU accuracy** implementation
- **Proper reverb support**
- **Pitch modulation** implementation

---

## References

1. DuckStation Source: https://github.com/stenzek/duckstation
2. PSX-SPX SPU Documentation: https://psx-spx.consoledev.net/soundprocessingunitspu/
3. jsgroth SPU Blog Series: https://jsgroth.dev/blog/posts/ps1-spu-part-1/

---

**Document Created:** January 31, 2026  
**Purpose:** Technical reference for PS1 SPU audio emulation optimization
