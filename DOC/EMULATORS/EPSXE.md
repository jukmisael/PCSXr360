# ePSXe - Audio Implementation Analysis

## Overview
**ePSXe** (enhanced PSX emulator) is one of the most popular Windows PS1 emulators, known for its plugin-based architecture.

**Website:** https://www.epsxe.com/  
**Architecture:** Plugin-based (PSEmu Pro specification)  
**Platform:** Primarily Windows (also Android, Linux, macOS)

---

## Plugin Architecture

### 1. PSEmu Pro Plugin Specification

ePSXe uses the **PSEmu Pro** plugin system, where SPU (sound) is handled by separate plugins:

**Available SPU Plugins:**
- `ePSXe SPU Core 2.0.0` - Default, most compatible
- `P.E.Op.S. SPU` - Pete's plugin, older
- `Null2's SPU` - Alternative implementation
- `SPU Eternal` - Another alternative

### 2. ePSXe SPU Core 2.0.0 - Architecture

**Plugin Interface:**
```cpp
// PSEmu Pro SPU Plugin Interface
long SPUopen(void);                    // Initialize audio
long SPUclose(void);                   // Shutdown audio
long SPUshutdown(void);                // Cleanup

void SPUwriteRegister(unsigned long reg, unsigned short val);  // Write SPU reg
unsigned short SPUreadRegister(unsigned long reg);             // Read SPU reg

unsigned short SPUreadDMA(void);       // DMA read
void SPUwriteDMA(unsigned short val);  // DMA write
void SPUwriteDMAMem(unsigned short *pMem, int size); // DMA block write
void SPUreadDMAMem(unsigned short *pMem, int size);  // DMA block read

void SPUplayADPCMchannel(xa_decode_t *xap);  // XA-ADPCM streaming

void SPUasync(unsigned long cycle, unsigned long psxRegs);  // Async callback
void SPUupdate(void);                  // Update (called every frame)

// IRQ handling
void SPUregisterCallback(void (*callback)(void));
void SPUregisterCDDAVolume(void (*CDDAVcallback)(unsigned short, unsigned short));
```

### 3. Audio Buffer Management

**Configuration Options:**
```cpp
struct SPUConfig {
    // Sound Latency
    // 0: Low Latency (smallest buffer)
    // 1-4: Medium Latency
    // 5: High Latency (largest buffer, most stable)
    int sound_latency;
    
    // Sound Effects Quality
    // 0: None (fastest)
    // 1: Simple
    // 2: Full Sound Effects (default)
    int sound_effects;
    
    // Audio Device
    // DirectSound, XAudio2, WASAPI (Windows)
    int audio_backend;
    
    // Buffer Size (in samples)
    // Automatically calculated based on latency setting
    int buffer_size;
};

// Buffer size mapping:
// Latency 0: ~256-512 samples (5-11ms)
// Latency 3: ~1024-2048 samples (23-46ms)
// Latency 5: ~4096-8192 samples (93-186ms)
```

### 4. Sound Latency Settings

**ePSXe SPU Core 2.0.0 offers 6 latency levels:**

| Setting | Buffer Size | Latency | Use Case |
|---------|-------------|---------|----------|
| 0 | 256-512 | 5-11ms | Low latency, powerful PC |
| 1 | 512-1024 | 11-23ms | Balanced |
| 2 | 1024-2048 | 23-46ms | Default, most games |
| 3 | 2048-4096 | 46-93ms | Slower PCs |
| 4 | 4096 | 93ms | Very slow PCs |
| 5 | 8192 | 186ms | Maximum stability |

**Recommended for PCSXr360:**
- Start with **Latency 2-3** (2048-4096 samples)
- Can go lower if FPS is stable
- Higher if audio crackles

### 5. Sound Effects Quality

**Three levels of SPU accuracy:**

#### Level 0: None (Fastest)
- Basic ADPCM decode
- No interpolation
- No reverb
- Minimal envelope processing

#### Level 1: Simple
- ADPCM with basic interpolation
- Simple volume control
- No reverb

#### Level 2: Full (Default)
- Full ADPCM decode
- Gaussian interpolation
- Full ADSR envelopes
- Reverb processing
- Pitch modulation

**For PCSXr360:**
- Start with **Level 1** for performance
- Level 2 if VMX-128 optimizations are implemented

### 6. ADPCM Decoding

**ePSXe SPU Core implementation:**
```cpp
// Simplified ADPCM decoder
struct ADPCMVoice {
    int16_t sample[4];      // Last 4 samples for interpolation
    int16_t predict;        // Current predictor value
    int8_t shift;           // Current shift factor
    uint32_t start_addr;    // Sample start in SPU RAM
    uint32_t curr_addr;     // Current playback position
    uint32_t loop_addr;     // Loop point address
    bool loop_flag;         // Loop enabled
};

// Decode and interpolate
int16_t decode_sample(ADPCMVoice* voice, uint8_t nibble) {
    static const int16_t TABLE[16] = {
        0, 1, 2, 3, 4, 5, 6, 7,
        -8, -7, -6, -5, -4, -3, -2, -1
    };
    
    int16_t step = ADPCM_STEP_TABLE[voice->shift];
    int32_t diff = step * TABLE[nibble];
    
    voice->predict = (voice->predict + diff).clamp(-32768, 32767);
    voice->shift = (voice->shift + ADPCM_INDEX_TABLE[nibble]).clamp(0, 88);
    
    return voice->predict;
}
```

### 7. Audio Sync Strategy

**ePSXe handles sync differently than most emulators:**

```cpp
// Frame-based audio sync
void SPUupdate(void) {
    // Called once per frame (16.6ms for 60fps)
    // Generate exactly 735 samples (44100Hz / 60fps)
    
    static int samples_remaining = 0;
    samples_remaining += 735;  // Samples needed this frame
    
    while (samples_remaining > 0) {
        // Generate samples for all active voices
        int samples_to_generate = min(samples_remaining, BUFFER_SIZE);
        generate_samples(samples_to_generate);
        samples_remaining -= samples_to_generate;
    }
}

// Alternative: Async callback (more accurate)
void SPUasync(unsigned long cycle, unsigned long psxRegs) {
    // Called periodically during CPU execution
    // Generates samples based on cycles elapsed
    int samples_needed = (cycle * SAMPLE_RATE) / PSX_CLOCK;
    generate_samples(samples_needed);
}
```

### 8. XA-ADPCM Streaming (CD Audio)

**For CD-DA and XA-ADPCM audio tracks:**
```cpp
// XA-ADPCM decode for streaming audio
void SPUplayADPCMchannel(xa_decode_t *xap) {
    // xap->nsamples = number of samples
    // xap->stereo = 0/1
    // xap->pcm = decoded PCM buffer
    
    if (xap->stereo) {
        // Stereo XA audio
        mix_xa_samples(xap->pcm, xap->nsamples, 2);
    } else {
        // Mono XA audio
        mix_xa_samples(xap->pcm, xap->nsamples, 1);
    }
}
```

### 9. Configuration File Format

**ePSXe uses INI-style configuration:**
```ini
[Config]
Bios = bios\scph1001.bin
Mcd1 = memcards\Mcd001.mcr
Mcd2 = memcards\Mcd002.mcr

[Video]
GPU = gpu\gpuPeteOpenGL2.dll

[Sound]
SPU = spu\spuPeopsSound.dll
SoundEnabled = 1
SoundLatency = 3
SoundEffects = 2

[Input]
PAD1 = pad\padKaillera.dll
PAD2 = pad\padKaillera.dll
```

### 10. Performance Characteristics

**On modern PC (3GHz+):**
- SPU Core 2.0.0: ~1-3% CPU usage
- Full audio quality (Level 2)
- Latency 0-1 possible

**On slower hardware:**
- SPU Core 2.0.0: ~5-10% CPU usage
- May need Level 1 quality
- Latency 3-5 for stability

---

## Comparison with PCSXr360

| Feature | ePSXe | PCSXr360 |
|---------|-------|----------|
| Architecture | Plugin-based | Monolithic |
| Configurability | High (many options) | Limited |
| Latency Options | 6 levels | Fixed |
| Quality Levels | 3 levels | 1 level |
| Buffer Management | Dynamic | Static |
| XA-ADPCM | Full support | Partial |
| Audio Sync | Frame-based | ? |

---

## Key Recommendations for PCSXr360

### 1. Implement Configurable Buffer Sizes
```cpp
// Add to config:
Config.SpuBufferSize = 2048;  // Default
Config.SpuQuality = 1;        // 0=Fast, 1=Balanced, 2=Full
```

### 2. Add Audio Quality Settings
- **Fast**: Basic ADPCM, no interpolation (for low FPS)
- **Balanced**: ADPCM + linear interpolation (default)
- **Full**: ADPCM + Gaussian + reverb (for high FPS)

### 3. Frame-Based Audio Sync
Match ePSXe's approach:
- Generate exact samples per frame
- Sync with video refresh
- Prevents audio drift

### 4. Per-Game Configuration
Allow different settings per game:
```ini
[SLPM-86115] ; Harvest Moon
SpuBufferSize = 4096
SpuQuality = 1

[SCUS-12345] ; Game that needs low latency
SpuBufferSize = 1024
SpuQuality = 2
```

---

## References

1. ePSXe Official: https://www.epsxe.com/
2. PSEmu Pro Plugin Specification
3. Pete's SPU Documentation
4. PSX-SPX SPU Reference

---

**Document Created:** January 31, 2026  
**Purpose:** Technical reference for plugin-based PS1 audio emulation
