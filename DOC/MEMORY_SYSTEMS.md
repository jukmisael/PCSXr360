# Memory Systems: PS1 vs Xbox 360 Architecture

## Overview
This document details the fundamental differences between PlayStation 1 memory systems and Xbox 360 memory architecture, focusing on challenges and solutions for accurate PS1 emulation on Xbox 360 hardware.

## PlayStation 1 Memory Architecture

### Memory Map Layout
```
PS1 Memory Space (32-bit addressing)
┌─────────────────────────────────────────────────────────┐
│ 0x00000000 - 0x007FFFFF │ BIOS ROM (512KB)        │
│ 0x00800000 - 0x008FFFFF │ Memory Expansion         │
│ 0x00F00000 - 0x00F0FFFFF │ Parallel Port           │
│ 0x1F000000 - 0x1F7FFFFF │ Scratchpad (1KB)        │
│ 0x80000000 - 0x803FFFFF │ Main RAM (2MB)         │
│ 0x9F000000 - 0x9F7FFFFF │ DMA Registers            │
│ 0xA0000000 - 0xA000FFFFF │ Hardware Registers       │
│ 0xBFC00000 - 0xBFC7FFFFF │ Cache Control          │
│ 0xFFFE0130 - 0xFFFE013F │ I/O Ports               │
└─────────────────────────────────────────────────────────────────┘
```

### Memory Components

#### Main System RAM
- **Capacity**: 2MB EDO DRAM
- **Organization**: 4 × 512KB chips
- **Access Speed**: ~60ns typical latency
- **Bus Width**: 32-bit
- **Refresh Rate**: 15.6μs

#### Video RAM (VRAM)
- **Capacity**: 1MB (dual-ported VRAM or SGRAM)
- **Organization**: Frame buffers, textures, color tables
- **Access**: Concurrent CPU/GPU access
- **Bandwidth**: ~66 MB/s peak

#### Cache System
- **Instruction Cache**: 4KB (isolatable)
- **Data Cache**: 1KB (mapped as Scratchpad)
- **Cache Line**: 16 bytes
- **Write Policy**: Write-through

#### Sound Memory
- **SPU RAM**: 512KB dedicated audio memory
- **Sample Storage**: ADPCM compressed samples
- **Reverb Buffers**: Environmental audio effects
- **Independent Access**: Separate audio subsystem

## Xbox 360 Memory Architecture

### Memory Map Layout
```
Xbox 360 Memory Space (64-bit addressing)
┌─────────────────────────────────────────────────────────┐
│ 0x00000000 - 0x1FFFFFFF │ System RAM (512MB)     │
│ 0x80000000 - 0x800000FF │ GPU Registers            │
│ 0x90000000 - 0x900000FF │ eDRAM Registers          │
│ 0xA0000000 - 0xA00000FF │ Hardware Registers       │
│ 0xC0000000 - 0xFFFFFFFF │ System Reserved           │
└─────────────────────────────────────────────────────────────────┘
```

### Memory Components

#### Unified System Memory
- **Capacity**: 512MB GDDR3
- **Organization**: 128-bit bus, 700MHz effective (1.4GHz)
- **Bandwidth**: 22.4 GB/s memory interface
- **Architecture**: Unified (CPU + GPU shared)
- **Manufacturing**: Samsung or Qimonda

#### Embedded DRAM (eDRAM)
- **Capacity**: 10MB embedded in GPU
- **Bandwidth**: 256 GB/s internal to GPU
- **Features**: Z-buffering, 4×MSAA, alpha blending
- **Manufacturing**: NEC, separate die from GPU

#### Cache Hierarchy
- **L1 Cache**: 32KB I-cache + 32KB D-cache per core
- **L2 Cache**: 1MB shared, 8-way associative
- **Cache Line**: 64 bytes
- **Clock**: Half CPU speed (1.6GHz)

## Memory Emulation Challenges

### Address Translation
**PS1 32-bit to Xbox 360 64-bit**
- **Sign Extension**: Handle 32-bit PS1 addresses properly
- **Virtual Memory**: Map PS1 memory into Xbox 360 virtual space
- **Page Alignment**: Xbox 360 requires 64KB page alignment
- **Memory Protection**: Xbox 360 memory protection vs PS1 direct access

### Endianess Conversion
- **PS1**: Big-endian (MIPS standard)
- **Xbox 360**: Little-endian (PowerPC standard)
- **Conversion Overhead**: All multi-byte data requires byte-swapping
- **Performance Impact**: Significant for 16-bit/32-bit operations

### Bandwidth Optimization
**PS1 Memory Constraints**
- **2MB Limit**: Entire PS1 system fits in Xbox 360 cache
- **66 MB/s**: Original PS1 memory bandwidth
- **Emulation Overhead**: Multiple memory reads per emulated access

**Xbox 360 Advantages**
- **22.4 GB/s**: 339× PS1 memory bandwidth
- **Multiple Caches**: Reduce memory access latency
- **Unified Memory**: Flexible allocation strategies

## Emulation Strategies

### Memory Mapping Implementation

#### Linear Translation Strategy
```cpp
// PS1 memory base addresses
#define PS1_RAM_BASE    0x80000000
#define PS1_VRAM_BASE   0x00000000
#define PS1_SCRATCHPAD  0x1F000000

// Map to Xbox 360 virtual memory
void* ps1_ram = (void*)XENON_MEMORY_BASE + PS1_RAM_OFFSET;
void* ps1_vram = (void*)XENON_VRAM_BASE;
void* ps1_scratchpad = (void*)XENON_SCRATCHPAD_OFFSET;
```

#### Cache-Aware Memory Access
```cpp
// Optimize scratchpad access (1KB fast SRAM)
static inline uint8_t fast_scratchpad_read(uint32_t address) {
    // Map to Xbox 360 L1 cache line
    return *(volatile uint8_t*)(scratchpad_ptr + (address & 0x3FF));
}

// Optimize main RAM access with prefetch
static inline uint32_t optimized_ram_read(uint32_t address) {
    // Align to cache line boundaries
    address &= ~0x3F; // 64-byte alignment
    return *(volatile uint32_t*)(ram_ptr + (address & 0x1FFFFF));
}
```

### VRAM Emulation
#### Dual-Port Memory Simulation
- **Concurrent Access**: Simulate PS1 VRAM dual-porting
- **CPU Access**: Emulate CPU VRAM read/write latency
- **GPU Access**: Emulate GPU texture access patterns
- **Bandwidth Limiting**: Throttle to PS1 VRAM bandwidth limits

#### Texture Mapping Optimization
```cpp
// Simulate PS1 texture bandwidth limitations
void simulate_vram_texture_transfer(uint32_t address, uint8_t* data, uint32_t size) {
    // PS1 VRAM bandwidth: ~33 MB/s
    uint64_t transfer_time = size * 30; // 30ns per byte at 33MB/s
    
    // Throttle if exceeding PS1 limits
    if (last_vram_transfer + transfer_time < current_time) {
        memcpy(vram_ptr + address, data, size);
        last_vram_transfer = current_time + transfer_time;
    }
}
```

### SPU Memory Management
#### Audio Buffer Simulation
```cpp
// SPU memory with channel isolation
typedef struct {
    uint8_t adpcm_samples[512KB];  // Main sample storage
    uint8_t reverb_buffer[64KB];     // Reverb effects
    uint32_t channel_registers[24];   // 24 voice channels
    uint32_t current_voice;            // Active voice tracking
} spu_memory_t;
```

### Memory Card Emulation
#### PS1 Memory Card Format
```cpp
// Memory card block structure (8KB blocks)
typedef struct {
    uint8_t data[8192];                   // Block data
    uint32_t checksum;                   // XOR checksum
    uint32_t block_state;                // Allocation flags
    char filename[20];                    // ASCII filename
} memcard_block_t;

// Memory card directory (block 0, frames 1-15)
typedef struct {
    uint32_t allocation_state;              // 51h/52h/53h for allocated
    uint32_t file_size;                    // File size in bytes
    uint16_t next_block;                   // Linked list pointer
    char filename[20];                      // File name
    uint8_t garbage[0x5F];                 // Unused area
    uint8_t checksum;                       // Frame checksum
} memcard_frame_t;
```

#### Xbox 360 Storage Integration
```cpp
// Convert PS1 memory card to Xbox 360 file
HRESULT save_memory_card(const char* xbox_path, const memcard_block_t* card) {
    HANDLE hFile = CreateFile(xbox_path, GENERIC_WRITE, 
                          FILE_SHARE_READ, NULL, CREATE_ALWAYS, 
                          FILE_ATTRIBUTE_NORMAL);
    
    if (hFile == INVALID_HANDLE_VALUE) {
        return E_FAIL;
    }
    
    // Write PS1 memory card format
    DWORD bytesWritten;
    BOOL result = WriteFile(hFile, card->data, 8192, &bytesWritten);
    
    CloseHandle(hFile);
    return result ? S_OK : E_FAIL;
}
```

## Performance Optimization Techniques

### Cache Utilization
- **L1 Optimization**: Keep frequently accessed PS1 data in L1 cache
- **L2 Strategy**: Use 1MB L2 for PS1 main RAM working set
- **Prefetching**: Use Xbox 360 prefetch instructions
- **Avoid Cache Thrashing**: Optimize memory access patterns

### Memory Bandwidth Management
- **PS1 Limit Simulation**: Enforce original 66MB/s VRAM bandwidth
- **Burst Transfer**: Simulate PS1 DMA burst patterns
- **Concurrent Access**: Handle CPU/GPU VRAM contention
- **Latency Modeling**: Simulate original memory access delays

### Virtual Memory Optimization
- **Large Pages**: Use 64KB pages for PS1 memory regions
- **Address Translation**: Efficient PS1→Xbox 360 mapping
- **Memory Protection**: Proper Xbox 360 memory permissions
- **Fragmentation Management**: Optimize allocation patterns

## Debugging Memory Issues

### Common Memory Emulation Problems
1. **Endianess Errors**: Incorrect byte ordering in multi-byte values
2. **Cache Coherency**: CPU and GPU view of memory不一致
3. **Address Translation**: Incorrect PS1 address mapping
4. **Timing Issues**: Not respecting PS1 memory access latency
5. **Bandwidth Violation**: Exceeding original PS1 transfer rates

### Debug Tools
```cpp
// Memory access logging
void log_memory_access(uint32_t address, uint32_t size, bool is_write) {
    printf("[%08X] %s %d bytes\n", address, 
           is_write ? "WRITE" : "READ", size);
}

// Performance profiling
typedef struct {
    uint64_t total_accesses;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint32_t bandwidth_usage;
} memory_profile_t;
```

This memory systems documentation provides comprehensive understanding of memory architecture differences and optimization strategies for PCSXr360 emulator development.