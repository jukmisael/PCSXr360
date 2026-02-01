# Emulation Techniques for PCSXr360 Development

## Overview
This document covers advanced emulation techniques and optimization strategies for accurate and efficient PlayStation 1 emulation on Xbox 360 hardware, focusing on dynamic recompilation, graphics translation, and performance optimization.

## Dynamic Recompilation (Dynarec)

### MIPS to PowerPC Translation

#### Block-Based Dynamic Recompilation
```cpp
// Dynamic recompilation block structure
typedef struct {
    uint32_t mips_start_pc;        // Start PS1 PC address
    uint32_t mips_end_pc;          // End PS1 PC address  
    uint32_t ppc_code_size;         // Generated PowerPC code size
    uint8_t* ppc_code;              // PowerPC executable code
    uint32_t execution_count;          // Hot block tracking
    uint32_t last_access_time;        // LRU tracking
} dynarec_block_t;

#define MAX_BLOCKS 1024
#define BLOCK_SIZE_MIN 32
#define BLOCK_SIZE_MAX 1024
```

#### Translation Engine
```cpp
class CDynarecEngine {
private:
    std::map<uint32_t, dynarec_block_t*> m_blockCache;
    uint8_t* m_pCodeBuffer;              // PowerPC code generation buffer
    uint32_t m_bufferPos;
    
public:
    uint8_t* TranslateBlock(uint32_t mips_pc) {
        // Check block cache
        auto it = m_blockCache.find(mips_pc);
        if (it != m_blockCache.end()) {
            it->second->execution_count++;
            it->second->last_access_time = GetCurrentTime();
            return it->second->ppc_code;
        }
        
        // Generate new block
        dynarec_block_t* pBlock = GenerateBlock(mips_pc);
        CacheBlock(pBlock);
        
        return pBlock->ppc_code;
    }
    
private:
    dynarec_block_t* GenerateBlock(uint32_t start_pc) {
        dynarec_block_t* pBlock = new dynarec_block_t();
        pBlock->mips_start_pc = start_pc;
        
        // Reset code buffer
        m_bufferPos = 0;
        
        uint32_t current_pc = start_pc;
        uint32_t block_size = 0;
        
        // Translate until branch or block size limit
        while (block_size < BLOCK_SIZE_MAX) {
            uint32_t instruction = ReadMipsInstruction(current_pc);
            TranslateInstruction(instruction, current_pc);
            
            current_pc += 4;
            block_size += 4;
            
            if (IsBranchInstruction(instruction)) {
                break;
            }
        }
        
        pBlock->mips_end_pc = current_pc;
        pBlock->ppc_code_size = m_bufferPos;
        
        return pBlock;
    }
};
```

### MIPS Instruction Translation

#### Register Mapping
```cpp
// MIPS to PowerPC register mapping
static const int mips_to_ppc_reg[32] = {
    0,   // $zero  → r0 (always zero on PowerPC)
    1,   // $at     → r1
    2,   // $v0     → r2
    3,   // $v1     → r3
    4,   // $a0     → r4
    5,   // $a1     → r5
    6,   // $a2     → r6
    7,   // $a3     → r7
    8,   // $t0     → r8
    9,   // $t1     → r9
    10,  // $t2     → r10
    11,  // $t3     → r11
    12,  // $t4     → r12
    13,  // $t5     → r13
    14,  // $t6     → r14
    15,  // $t7     → r15
    16,  // $s0     → r16
    17,  // $s1     → r17
    18,  // $s2     → r18
    19,  // $s3     → r19
    20,  // $s4     → r20
    21,  // $s5     → r21
    22,  // $s6     → r22
    23,  // $s7     → r23
    24,  // $t8     → r24
    25,  // $t9     → r25
    26,  // $k0     → r26
    27,  // $k1     → r27
    28,  // $gp     → r28
    29,  // $sp     → r29 (stack pointer)
    30,  // $fp     → r30 (frame pointer)
    31,  // $ra     → r31 (return address)
};
```

#### Instruction Translation Examples
```cpp
// MIPS: add $t0, $t1, $t2
void Translate_ADD(uint32_t instruction, uint32_t pc) {
    uint8_t rs = (instruction >> 21) & 0x1F;
    uint8_t rt = (instruction >> 16) & 0x1F;
    uint8_t rd = (instruction >> 11) & 0x1F;
    uint8_t shamt = (instruction >> 6) & 0x1F;
    
    // Generate PowerPC equivalent
    EmitPowerPCInstruction(0x7C000214 |    // add (PowerPC)
                       (rd << 21) |
                       (ra << 16) |        // Use r1 as source for shifted value
                       (mips_to_ppc_reg[rt] << 11) |
                       (mips_to_ppc_reg[rs] << 6));
}

// MIPS: beq $t0, $t1, offset
void Translate_BEQ(uint32_t instruction, uint32_t pc) {
    uint8_t rs = (instruction >> 21) & 0x1F;
    uint8_t rt = (instruction >> 16) & 0x1F;
    int16_t offset = instruction & 0xFFFF;
    
    // PowerPC branch with prediction
    EmitPowerPCInstruction(0x41800000 |    // bc (PowerPC branch)
                       ((offset & 0xFFFC) << 16) |
                       (mips_to_ppc_reg[rs] << 21) |
                       (mips_to_ppc_reg[rt] << 16));
}
```

### Pipeline Optimization

#### Branch Delay Slot Handling
```cpp
// Handle MIPS branch delay slots efficiently
void TranslateBranchWithDelay(uint32_t branch_instr, uint32_t delay_instr, uint32_t pc) {
    // Translate branch instruction
    TranslateInstruction(branch_instr, pc);
    
    // Translate delay slot instruction (always executed)
    TranslateInstruction(delay_instr, pc + 4);
    
    // For conditional branches, emit PowerPC branch prediction
    if (IsConditionalBranch(branch_instr)) {
        EmitPowerPCPredictionCode(branch_instr);
    }
}

// Optimize NOP delay slots
void OptimizeDelaySlots() {
    // If delay slot contains NOP, schedule useful instructions
    for (int i = 0; i < block_size - 1; i++) {
        uint32_t delay_instr = ReadMipsInstruction(mips_pc + i * 4 + 4);
        
        if (IsNOP(delay_instr)) {
            // Try to move earlier instruction into delay slot
            TryToRescheduleInstructions(i, i + 1);
        }
    }
}
```

## Graphics Emulation Techniques

### PS1 GPU to Xbox 360 GPU Translation

#### Vertex Processing
```cpp
// Convert PS1 GPU commands to Xbox 360 GPU rendering
class CGPUTranslator {
private:
    D3DVERTEX* m_pVertexCache;
    uint32_t m_vertexCount;
    
public:
    void ProcessGPUCommand(uint32_t command, uint32_t* params) {
        switch (command & 0xFF000000) {
            case 0x20000000: // Draw triangle
                ProcessTriangle(params);
                break;
                
            case 0x24000000: // Draw rectangle  
                ProcessRectangle(params);
                break;
                
            case 0x28000000: // Draw line
                ProcessLine(params);
                break;
        }
    }
    
private:
    void ProcessTriangle(uint32_t* params) {
        // Convert PS1 coordinates to Xbox 360
        D3DVERTEX vertices[3];
        
        for (int i = 0; i < 3; i++) {
            // PS1 integer coordinates to Xbox 360 floating point
            vertices[i].x = (float)(params[i] & 0xFFFF);
            vertices[i].y = (float)((params[i] >> 16) & 0xFFFF);
            vertices[i].z = 0.0f; // PS1 has no Z-buffer
            
            // PS1 15-bit color to Xbox 360 32-bit
            uint16_t ps1_color = (params[i] >> 24) & 0xFFFF;
            vertices[i].color = ConvertPS1ColorToXbox360(ps1_color);
        }
        
        // Submit to Xbox 360 GPU
        m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 
                                D3DFVF_XYZ | D3DFVF_DIFFUSE,
                                vertices, 3);
    }
    
    DWORD ConvertPS1ColorToXbox360(uint16_t ps1_color) {
        // PS1 format: 0-0-0 (5 bits each for RGB)
        uint8_t r = ((ps1_color >> 10) & 0x1F) << 3;
        uint8_t g = ((ps1_color >> 5) & 0x1F) << 3;
        uint8_t b = (ps1_color & 0x1F) << 3;
        
        return D3DCOLOR_ARGB(0xFF, r, g, b);
    }
};
```

#### Texture Mapping
```cpp
// Handle PS1 affine texture mapping artifacts
class CTextureMapper {
public:
    void ApplyAffineTexture(const TEXTURE_PARAMS& ps1_params) {
        // Simulate PS1 texture warping by dividing coordinates
        // This creates the characteristic PS1 "warping" effect
        
        float u, v;
        float depth = ps1_params.depth;
        
        for (int y = 0; y < ps1_params.height; y++) {
            for (int x = 0; x < ps1_params.width; x++) {
                // Convert screen coordinates to texture coordinates
                u = (ps1_params.u_base + x * ps1_params.u_step) / depth;
                v = (ps1_params.v_base + y * ps1_params.v_step) / depth;
                
                // Nearest-neighbor sampling (PS1 characteristic)
                int texel_x = (int)u & (texture_width - 1);
                int texel_y = (int)v & (texture_height - 1);
                
                // Optional: Sub-pixel accuracy improvement
                if (m_enableSubpixel) {
                    ApplySubpixelCorrection(u, v, x, y);
                }
                
                WritePixel(x, y, SampleTexture(texel_x, texel_y));
            }
        }
    }
};
```

### Multi-threaded GPU Rendering
```cpp
// Xbox 360 GPU worker thread for PS1 emulation
class CThreadedRenderer {
private:
    HANDLE m_hRenderThread;
    HANDLE m_hCommandQueueMutex;
    std::queue<GPU_COMMAND> m_commandQueue;
    bool m_bThreadActive;
    
public:
    void StartRenderThread() {
        m_bThreadActive = true;
        m_hRenderThread = CreateThread(NULL, 0, RenderThreadProc, this, 0, NULL);
    }
    
private:
    static DWORD WINAPI RenderThreadProc(LPVOID lpParam) {
        CThreadedRenderer* pThis = (CThreadedRenderer*)lpParam;
        
        while (pThis->m_bThreadActive) {
            WaitForSingleObject(pThis->m_hCommandQueueMutex, INFINITE);
            
            if (!pThis->m_commandQueue.empty()) {
                GPU_COMMAND cmd = pThis->m_commandQueue.front();
                pThis->m_commandQueue.pop();
                
                pThis->ProcessGPUCommand(cmd);
            }
            
            ReleaseMutex(pThis->m_hCommandQueueMutex);
            Sleep(1); // Small yield
        }
        
        return 0;
    }
};
```

## Audio Emulation

### SPU to XAudio2 Translation

#### ADPCM Decompression
```cpp
class CADPCMDecoder {
public:
    void DecodeSample(const uint8_t* compressed, uint8_t* decompressed, 
                    uint32_t sample_count) {
        int16_t predict = 0;
        uint8_t step = 0;
        uint8_t shift = 0;
        
        for (uint32_t i = 0; i < sample_count; i++) {
            uint8_t nibble = compressed[i >> 1];
            
            if (i & 1) {
                nibble &= 0x0F; // Low nibble
            } else {
                nibble >>= 4;    // High nibble
            }
            
            // ADPCM decoding algorithm
            int16_t diff = adpcm_table[shift][nibble];
            int16_t sample = predict + diff;
            
            *decompressed++ = sample;
            
            // Update prediction parameters
            predict = sample;
            UpdateStep(nibble, &step, &shift);
        }
    }
};
```

#### Audio Buffer Management
```cpp
class CAudioProcessor {
private:
    IXAudio2* m_pXAudio2;
    IXAudio2MasteringVoice* m_pMasterVoice;
    std::vector<IXAudio2SourceVoice*> m_voices;
    
    // PS1 SPU RAM simulation
    uint8_t m_spuRam[512 * 1024]; // 512KB
    uint32_t m_channelRegisters[24];
    
public:
    void ProcessSPUCommand(uint32_t command, uint32_t data) {
        switch (command) {
            case SPU_WRITE_CHANNEL:
                WriteChannelData(data & 0x1F, data >> 5);
                break;
                
            case SPU_SET_VOLUME:
                SetChannelVolume(data & 0x1F, data >> 5);
                break;
                
            case SPU_KEY_ON:
                EnableChannel(data & 0x1F);
                break;
        }
    }
    
    void MixAudioOutput() {
        // Mix 24 PS1 channels to stereo output
        float left_mix = 0.0f;
        float right_mix = 0.0f;
        
        for (int i = 0; i < 24; i++) {
            if (IsChannelActive(i)) {
                left_mix += GetChannelSample(i, CHANNEL_LEFT) * m_volumes[i];
                right_mix += GetChannelSample(i, CHANNEL_RIGHT) * m_volumes[i];
            }
        }
        
        // Submit to XAudio2
        XAUDIO2_BUFFER buffer = {0};
        buffer.pAudioData = (BYTE*)&left_mix;
        buffer.AudioBytes = sizeof(float) * 2;
        buffer.PlayBegin = 0;
        buffer.PlayLength = 1; // One frame
        
        m_pMasterVoice->SubmitSourceBuffer(1, &buffer);
    }
};
```

## Performance Optimization Strategies

### Cache Management
```cpp
// Hot block caching for dynarec
class CHotBlockCache {
private:
    struct CacheEntry {
        uint32_t mips_pc;
        uint8_t* ppc_code;
        uint32_t size;
        uint64_t last_access;
        uint32_t execution_count;
    };
    
    static const uint32_t CACHE_SIZE = 256;
    CacheEntry m_cache[CACHE_SIZE];
    uint32_t m_lru_counter;
    
public:
    uint8_t* Lookup(uint32_t mips_pc) {
        // LRU replacement policy
        uint32_t oldest_index = 0;
        uint64_t oldest_time = m_cache[0].last_access;
        
        for (uint32_t i = 0; i < CACHE_SIZE; i++) {
            if (m_cache[i].mips_pc == mips_pc) {
                m_cache[i].last_access = m_lru_counter++;
                m_cache[i].execution_count++;
                return m_cache[i].ppc_code;
            }
            
            if (m_cache[i].last_access < oldest_time) {
                oldest_time = m_cache[i].last_access;
                oldest_index = i;
            }
        }
        
        return nullptr; // Cache miss
    }
    
    void Insert(uint32_t mips_pc, uint8_t* ppc_code, uint32_t size) {
        // Replace LRU entry
        m_cache[oldest_index].mips_pc = mips_pc;
        m_cache[oldest_index].ppc_code = ppc_code;
        m_cache[oldest_index].size = size;
        m_cache[oldest_index].last_access = m_lru_counter++;
        m_cache[oldest_index].execution_count = 1;
    }
};
```

### Profiling and Debugging
```cpp
// Performance profiler for dynarec
class CEmulatorProfiler {
private:
    struct ProfileEntry {
        uint32_t function_id;
        uint64_t cycles_spent;
        uint64_t call_count;
        const char* name;
    };
    
    std::vector<ProfileEntry> m_profiles;
    LARGE_INTEGER m_frequency;
    
public:
    CEmulatorProfiler() {
        QueryPerformanceFrequency(&m_frequency);
    }
    
    void BeginProfile(uint32_t function_id, const char* name) {
        LARGE_INTEGER start_time;
        QueryPerformanceCounter(&start_time);
        
        ProfileEntry entry;
        entry.function_id = function_id;
        entry.cycles_spent = 0;
        entry.call_count = 1;
        entry.name = name;
        entry.start_time = start_time.QuadPart;
        
        m_profiles.push_back(entry);
    }
    
    void EndProfile(uint32_t function_id) {
        LARGE_INTEGER end_time;
        QueryPerformanceCounter(&end_time);
        
        for (auto& entry : m_profiles) {
            if (entry.function_id == function_id) {
                entry.cycles_spent += (end_time.QuadPart - entry.start_time) * 1000000 / m_frequency.QuadPart;
                entry.call_count++;
                break;
            }
        }
    }
    
    void PrintResults() {
        printf("Emulator Performance Profile:\n");
        printf("Function\t\tCalls\t\tCycles\t\tCycles/Call\n");
        for (const auto& entry : m_profiles) {
            printf("%s\t\t%llu\t\t%llu\t\t%llu\n",
                   entry.name,
                   entry.call_count,
                   entry.cycles_spent,
                   entry.cycles_spent / entry.call_count);
        }
    }
};
```

## Accuracy vs Performance Trade-offs

### Cycle-Accurate Mode
```cpp
class CCycleAccurateEmulation {
private:
    uint64_t m_current_cycle;
    std::priority_queue<CPU_EVENT> m_event_queue;
    
public:
    void ExecuteInstruction(uint32_t instruction) {
        uint32_t cycles = GetInstructionCycles(instruction);
        
        // Execute instruction
        PerformInstruction(instruction);
        
        // Advance cycle counter
        m_current_cycle += cycles;
        
        // Process scheduled events
        while (!m_event_queue.empty() && 
               m_event_queue.top().cycle <= m_current_cycle) {
            CPU_EVENT event = m_event_queue.top();
            m_event_queue.pop();
            
            ProcessEvent(event);
        }
    }
    
    void ScheduleEvent(uint64_t delay_cycles, EVENT_TYPE type, void* data) {
        CPU_EVENT event;
        event.cycle = m_current_cycle + delay_cycles;
        event.type = type;
        event.data = data;
        
        m_event_queue.push(event);
    }
};
```

This comprehensive emulation techniques guide provides advanced strategies for high-performance, accurate PS1 emulation on Xbox 360 hardware.