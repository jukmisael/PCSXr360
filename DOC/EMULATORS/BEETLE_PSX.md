# Beetle PSX / Mednafen - Audio Implementation Analysis

## Overview
**Beetle PSX** is the libretro port of **Mednafen PSX**, one of the most accurate PS1 emulators.

**Source:** https://github.com/libretro/beetle-psx-libretro  
**Core:** Mednafen PSX (originally based on PCX-Rewind)  
**Architecture:** Highly accurate, cycle-exact SPU emulation  
**Platform:** libretro (RetroArch, Lakka, etc.)

---

## Philosophy: Accuracy First

Beetle PSX prioritizes **hardware accuracy** over performance:
- Cycle-exact SPU emulation
- Full ADPCM implementation with all quirks
- Accurate reverb processing
- Proper interpolation
- Verified against real hardware

---

## SPU Architecture

### 1. Core Implementation

**File Structure:**
```
libretro/
├── mednafen/
│   ├── psx/
│   │   ├── spu.cpp          # Main SPU implementation
│   │   ├── spu.h            # SPU header
│   │   ├── spu_reverb.cpp   # Reverb processing
│   │   └── spu_adpcm.cpp    # ADPCM decoder
```

### 2. Voice Processing (24 Voices)

Each voice follows this exact pipeline:

```cpp
struct Voice {
    // ADPCM State
    int16_t decode_hist[4];     // Last 4 decoded samples
    uint8_t decode_shift;       // Current step size index
    uint8_t decode_weight;      // Current predictor
    
    // Playback State
    uint32_t addr;              // Current address in SPU RAM
    uint32_t loop_addr;         // Loop point address
    uint32_t start_addr;        // Sample start address
    
    // Pitch
    uint16_t pitch;             // 0x0000-0xFFFF
    uint32_t pitch_counter;     // Accumulator
    
    // ADSR
    uint16_t adsr;              // Attack/Decay/Sustain/Release
    uint16_t adsr_vol;          // Current envelope volume
    uint8_t adsr_phase;         // Current phase (A/D/S/R)
    
    // Volume
    int16_t vol_l;              // Left volume (-0x8000 to +0x7FFF)
    int16_t vol_r;              // Right volume
    
    // Flags
    bool loop_flag;
    bool noise_flag;
    bool reverb_flag;
};
```

### 3. ADPCM Decoder (Exact Implementation)

**Beetle uses the exact PS1 ADPCM tables:**

```cpp
// ADPCM Step Table (89 entries)
static const int16_t ADPCM_STEP_TABLE[89] = {
    7, 8, 9, 10, 11, 12, 13, 14,
    16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
    7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
};

// ADPCM Index Table
static const int8_t ADPCM_INDEX_TABLE[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

// Decode single nibble
inline int16_t DecodeADPCM(uint8_t nibble, int16_t* predictor, int8_t* step_index) {
    int16_t step = ADPCM_STEP_TABLE[*step_index];
    
    // Calculate difference
    int32_t diff = step >> 3;
    if (nibble & 4) diff += step;
    if (nibble & 2) diff += step >> 1;
    if (nibble & 1) diff += step >> 2;
    if (nibble & 8) diff = -diff;
    
    // Update predictor
    *predictor = std::clamp<int32_t>(*predictor + diff, -32768, 32767);
    
    // Update step index
    *step_index = std::clamp<int8_t>(*step_index + ADPCM_INDEX_TABLE[nibble], 0, 88);
    
    return *predictor;
}
```

### 4. Interpolation Modes

Beetle supports multiple interpolation modes:

#### 4.1 Nearest (None)
- No interpolation
- Fastest, lowest quality
- Used for performance mode

#### 4.2 Linear Interpolation
```cpp
// Simple linear interpolation
int16_t LinearInterpolate(int16_t s1, int16_t s2, uint32_t frac) {
    // frac is 0-4095 (12-bit fraction)
    return ((s1 * (4096 - frac)) + (s2 * frac)) >> 12;
}
```

#### 4.3 Gaussian Interpolation (Default, Most Accurate)
```cpp
// Gaussian interpolation using 512-entry table
// Matches real PS1 hardware

static const int32_t GAUSS_TABLE[512] = {
    // Pre-calculated Gaussian coefficients
    // See jsgroth's blog for exact values
};

int32_t GaussianInterpolate(const int16_t samples[4], uint32_t pitch_counter) {
    uint32_t idx = (pitch_counter >> 4) & 0xFF;
    
    int32_t result = 0;
    result += (GAUSS_TABLE[0x0FF - idx] * samples[0]) >> 15;
    result += (GAUSS_TABLE[0x1FF - idx] * samples[1]) >> 15;
    result += (GAUSS_TABLE[0x100 + idx] * samples[2]) >> 15;
    result += (GAUSS_TABLE[idx] * samples[3]) >> 15;
    
    return std::clamp(result, -32768, 32767);
}
```

### 5. ADSR Envelope Generator

**Exact PS1 ADSR implementation:**

```cpp
// ADSR rates and levels
struct ADSRConfig {
    uint8_t attack_rate;     // 0-127
    uint8_t decay_rate;      // 0-127
    uint8_t sustain_rate;    // 0-127
    uint8_t release_rate;    // 0-127
    uint8_t sustain_level;   // 0-15
};

enum ADSRPhase {
    ADSR_ATTACK,
    ADSR_DECAY,
    ADSR_SUSTAIN,
    ADSR_RELEASE,
    ADSR_OFF
};

void UpdateADSR(Voice* voice) {
    switch (voice->adsr_phase) {
        case ADSR_ATTACK:
            // Exponential attack
            voice->adsr_vol += ADSRAttackStep(voice->attack_rate);
            if (voice->adsr_vol >= 0x7FFF) {
                voice->adsr_vol = 0x7FFF;
                voice->adsr_phase = ADSR_DECAY;
            }
            break;
            
        case ADSR_DECAY:
            // Exponential decay to sustain level
            voice->adsr_vol -= ADSRDecayStep(voice->decay_rate);
            if (voice->adsr_vol <= SustainLevel(voice->sustain_level)) {
                voice->adsr_vol = SustainLevel(voice->sustain_level);
                voice->adsr_phase = ADSR_SUSTAIN;
            }
            break;
            
        case ADSR_SUSTAIN:
            // Sustain (increase or decrease)
            voice->adsr_vol += ADSRSustainStep(voice->sustain_rate);
            voice->adsr_vol = std::clamp(voice->adsr_vol, 0, 0x7FFF);
            break;
            
        case ADSR_RELEASE:
            // Release to 0
            voice->adsr_vol -= ADSRReleaseStep(voice->release_rate);
            if (voice->adsr_vol <= 0) {
                voice->adsr_vol = 0;
                voice->adsr_phase = ADSR_OFF;
            }
            break;
            
        case ADSR_OFF:
            voice->adsr_vol = 0;
            break;
    }
}
```

### 6. Reverb Implementation

**Full PS1 reverb with all effects:**

```cpp
struct ReverbState {
    // Reverb work area in SPU RAM
    uint32_t start_addr;
    uint32_t curr_addr;
    
    // Reverb coefficients
    int16_t vLOUT;      // Left output volume
    int16_t vROUT;      // Right output volume
    int16_t mBASE;      // Reverb base address
    
    // All-pass filter delays
    uint16_t dAPF1;     // APF offset 1
    uint16_t dAPF2;     // APF offset 2
    
    // Comb filter volumes
    int16_t vIIR;       // Reflection volume 1
    int16_t vCOMB1;     // Comb volume 1
    int16_t vCOMB2;     // Comb volume 2
    int16_t vCOMB3;     // Comb volume 3
    int16_t vCOMB4;     // Comb volume 4
    int16_t vWALL;      // Reflection volume 2
    int16_t vAPF1;      // APF volume 1
    int16_t vAPF2;      // APF volume 2
    
    // Input volumes
    int16_t vLIN;       // Left input
    int16_t vRIN;       // Right input
};

void ProcessReverb(int16_t input_l, int16_t input_r, 
                   int16_t* output_l, int16_t* output_r) {
    // Read from reverb work area
    int16_t comb1_l = ReadReverbRAM(reverb.curr_addr + dCOMB1_L);
    int16_t comb1_r = ReadReverbRAM(reverb.curr_addr + dCOMB1_R);
    // ... similar for comb2, 3, 4
    
    // Comb filters
    int32_t comb_l = (comb1_l * vCOMB1 + comb2_l * vCOMB2 + 
                      comb3_l * vCOMB3 + comb4_l * vCOMB4) >> 15;
    int32_t comb_r = (comb1_r * vCOMB1 + comb2_r * vCOMB2 + 
                      comb3_r * vCOMB3 + comb4_r * vCOMB4) >> 15;
    
    // All-pass filters
    // APF1
    int16_t apf1_in_l = ReadReverbRAM(reverb.curr_addr + dAPF1_L);
    int16_t apf1_in_r = ReadReverbRAM(reverb.curr_addr + dAPF1_R);
    int32_t apf1_out_l = comb_l - ((apf1_in_l * vAPF1) >> 15);
    int32_t apf1_out_r = comb_r - ((apf1_in_r * vAPF1) >> 15);
    
    // APF2
    int16_t apf2_in_l = ReadReverbRAM(reverb.curr_addr + dAPF2_L);
    int16_t apf2_in_r = ReadReverbRAM(reverb.curr_addr + dAPF2_R);
    *output_l = apf1_out_l - ((apf2_in_l * vAPF2) >> 15);
    *output_r = apf1_out_r - ((apf2_in_r * vAPF2) >> 15);
    
    // Write back to reverb work area
    WriteReverbRAM(reverb.curr_addr + dAPF1_L, apf1_out_l + ((comb_l * vAPF1) >> 15));
    WriteReverbRAM(reverb.curr_addr + dAPF1_R, apf1_out_r + ((comb_r * vAPF1) >> 15));
    
    // Advance reverb pointer
    reverb.curr_addr = (reverb.curr_addr + 2) & 0x3FFFF;
}
```

### 7. Audio Output & libretro Integration

**libretro audio callback:**

```cpp
// Audio buffer
static int16_t audio_buffer[2 * 4096];  // Stereo, 4096 samples
static uint32_t audio_buffer_ptr = 0;

// Generate audio samples
void GenerateSamples(uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        int32_t mix_l = 0, mix_r = 0;
        
        // Process all 24 voices
        for (int v = 0; v < 24; v++) {
            if (voices[v].adsr_phase == ADSR_OFF) continue;
            
            // Get interpolated sample
            int16_t sample = GetInterpolatedSample(&voices[v]);
            
            // Apply volume and ADSR
            sample = (sample * voices[v].adsr_vol) >> 15;
            
            mix_l += (sample * voices[v].vol_l) >> 15;
            mix_r += (sample * voices[v].vol_r) >> 15;
        }
        
        // Add CD audio (XA-ADPCM)
        mix_l += cd_audio_l;
        mix_r += cd_audio_r;
        
        // Add reverb
        int16_t reverb_l, reverb_r;
        ProcessReverb(mix_l >> 1, mix_r >> 1, &reverb_l, &reverb_r);
        mix_l += reverb_l;
        mix_r += reverb_r;
        
        // Clamp and store
        audio_buffer[audio_buffer_ptr++] = std::clamp(mix_l, -32768, 32767);
        audio_buffer[audio_buffer_ptr++] = std::clamp(mix_r, -32768, 32767);
        
        // Send to libretro when buffer is full
        if (audio_buffer_ptr >= 2048) {
            audio_batch_cb(audio_buffer, audio_buffer_ptr / 2);
            audio_buffer_ptr = 0;
        }
    }
}
```

### 8. Core Options (libretro)

**Beetle PSX exposes many audio options:**

```cpp
// Core options structure
struct CoreOption {
    { "beetle_psx_spu_interpolation", "SPU Interpolation; gaussian|simple|cubic|off" },
    { "beetle_psx_cd_audio", "CD Audio; enabled|disabled" },
    { "beetle_psx_spu_reverb", "Reverb; enabled|disabled" },
    { "beetle_psx_audio_resampler_quality", "Audio Resampler Quality; medium|low|high" },
    { "beetle_psx_audio_out_rate", "Audio Output Rate; 44100|48000|88200|96000" }
};
```

### 9. Performance vs Accuracy Modes

**Beetle PSX has two variants:**

#### Beetle PSX (Default)
- Full accuracy
- All features enabled
- Higher CPU usage
- Best compatibility

#### Beetle PSX HW (Hardware Mode)
- Uses GPU for rendering
- Same SPU accuracy
- Better for 3D games
- More demanding on GPU

---

## Comparison with PCSXr360

| Feature | Beetle PSX | PCSXr360 |
|---------|------------|----------|
| Accuracy | Cycle-exact | Approximate |
| ADPCM | Full implementation | Basic |
| Interpolation | Gaussian (default) | Simple/None |
| Reverb | Full | Partial/None |
| ADSR | Exact PS1 curves | Simplified |
| Pitch Mod | Supported | ? |
| Performance | Medium | Low (needs optimization) |
| Configurability | High | Low |

---

## Recommendations for PCSXr360

### Immediate Improvements

1. **Replace ADPCM decoder** with accurate implementation
2. **Add Gaussian interpolation** option
3. **Implement proper ADSR curves** (exponential, not linear)

### Advanced Features

4. **Add reverb support** (can be disabled for performance)
5. **Implement pitch modulation** (for special effects)
6. **Add XA-ADPCM streaming** for CD audio

### Configuration Options

```cpp
// Suggested config options for PCSXr360
Config.SpuAccuracy = { "Low", "Medium", "High" };
Config.SpuInterpolation = { "None", "Linear", "Gaussian" };
Config.SpuReverb = { "Disabled", "Enabled" };
Config.SpuBufferSize = { 1024, 2048, 4096, 8192 };
```

---

## References

1. Beetle PSX Source: https://github.com/libretro/beetle-psx-libretro
2. Mednafen Documentation: https://mednafen.github.io/
3. jsgroth SPU Blog Series: https://jsgroth.dev/blog/posts/ps1-spu-part-1/
4. PSX-SPX SPU Documentation: https://psx-spx.consoledev.net/soundprocessingunitspu/

---

**Document Created:** January 31, 2026  
**Purpose:** Technical reference for accurate PS1 SPU emulation
