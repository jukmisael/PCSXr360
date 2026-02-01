# PS1 Memory Card Format and Save State Management

## Overview
This document details the PlayStation 1 memory card data format, save state management, and implementation strategies for PCSXr360 emulator on Xbox 360.

## PlayStation Memory Card Technical Specifications

### Physical Characteristics
- **Type**: EEPROM flash memory
- **Capacity**: 128KB per card (131,072 bytes)
- **Organization**: 16 blocks × 8KB each
- **Sectors per Block**: 64 sectors × 128 bytes
- **Interface**: Serial I/O at 125 KHz
- **Hot Swappable**: Can be changed during operation

### Memory Card Layout

#### Complete Structure
```
PS1 Memory Card (128KB = 20000h bytes)
┌─────────────────────────────────────────────────────┐
│ Block 0 │ Directory & Management │ 8KB │
├─────────────────────────────────────────────────────┤
│ Block 1  │ File Data Block 1      │ 8KB │
│ Block 2  │ File Data Block 2      │ 8KB │
│    ...    │                      │     │
│ Block 15 │ File Data Block 15     │ 8KB │
└─────────────────────────────────────────────────────┘
```

#### Block 0: Directory Structure
```
Block 0 (8KB = 2000h bytes)
┌─────────────────────────────────────────────────────┐
│ Frame 0    │ Header & Write Test     │ 128B │
├─────────────────────────────────────────────────────┤
│ Frame 1    │ Directory Entry 1       │ 128B │
│ Frame 2    │ Directory Entry 2       │ 128B │
│    ...     │                        │     │
│ Frame 15   │ Directory Entry 15      │ 128B │
├─────────────────────────────────────────────────────┤
│ Frame 16-35 │ Broken Sector List      │ 20 × 128B │
├─────────────────────────────────────────────────────┤
│ Frame 36-55 │ Broken Sector Replacement  │ 20 × 128B │
├─────────────────────────────────────────────────────┤
│ Frame 56-62 │ Unused                  │ 7 × 128B  │
├─────────────────────────────────────────────────────┤
│ Frame 63    │ Write Test              │ 128B │
└─────────────────────────────────────────────────────┘
```

## Memory Card Frame Formats

### Header Frame (Block 0, Frame 0)
```cpp
typedef struct {
    uint8_t  memory_card_id[2];    // ASCII "MC"
    uint8_t  unused[0x7C];         // Zero-filled
    uint8_t  checksum;             // XOR of bytes 00h-7Eh, usually 0Eh
} memcard_header_t;
```

### Directory Frame Structure (Block 0, Frames 1-15)
```cpp
typedef struct {
    uint32_t allocation_state;      // Block allocation status
    uint32_t file_size_bytes;     // File size in bytes (2000h-1E000h)
    uint16_t next_block_index;     // Next block number (0-14) or FFFFh
    uint8_t  filename[20];        // ASCII filename + null terminator
    uint8_t  reserved;            // Zero (unused)
    uint8_t  unused_data[0x5E];   // Garbage data (usually zeros)
    uint8_t  checksum;             // XOR checksum of all above bytes
} memcard_directory_entry_t;
```

### Allocation States
```cpp
#define BLOCK_STATE_IN_USE_FIRST       0x51    // First block of file
#define BLOCK_STATE_IN_USE_MIDDLE      0x52    // Middle block of file (3+ blocks)
#define BLOCK_STATE_IN_USE_LAST        0x53    // Last block of file (2+ blocks)
#define BLOCK_STATE_FREE_FRESH       0xA0    // Freshly formatted
#define BLOCK_STATE_FREE_DELETED      0xA1    // Deleted first block
#define BLOCK_STATE_FREE_DELETED_MID  0xA2    // Deleted middle block
#define BLOCK_STATE_FREE_DELETED_LAST  0xA3    // Deleted last block
```

### Filename Convention
```
Format: [Region][GameCode][CustomPart]

Region Codes:
  BI = Japan
  BE = Europe  
  BA = America

Game Codes (AAAA-NNNNN):
  SLPS/SCPS = Sony Japan
  SLUS/SCUS = Sony America
  SLES/SCES = Sony Europe

Example: "BISLES-12345MySave"
```

### Broken Sector Management
```cpp
typedef struct {
    uint32_t broken_sector_number;  // Block×64+Frame address or FFFFFFFFh
    uint8_t  unused_data[0x7A];   // Usually zeros
    uint8_t  checksum;             // XOR checksum
} memcard_broken_sector_t;
```

## Title and Icon Data

### Title Frame (First Block of File, Frame 0)
```cpp
typedef struct {
    uint8_t  id[2];               // ASCII "SC"
    uint8_t  icon_display_flag;     // Icon display mode
    uint8_t  block_count;          // Icon block count (usually 1-2)
    uint8_t  reserved[0x41];       // Reserved area
    uint16_t title_shiftjis[32];    // Shift-JIS encoded title
    uint8_t  reserved2[0x0C];      // More reserved space
    uint8_t  palette_data[0x20];    // 16-color palette data
} memcard_title_frame_t;
```

### Icon Display Modes
```cpp
#define ICON_MODE_STATIC     0x11    // Single frame, static display
#define ICON_MODE_ANIMATED_2 0x12    // 2 frames, changes every 16 PAL frames
#define ICON_MODE_ANIMATED_3 0x13    // 3 frames, changes every 11 PAL frames
```

### Icon Frame Data (Frames 1-3)
```cpp
typedef struct {
    uint8_t icon_bitmap[0x80];     // 16×16 pixels, 4-bit color depth
} memcard_icon_frame_t;
```

## Xbox 360 Storage Integration

### File System Mapping
```cpp
// Xbox 360 memory card file path
#define MEMORY_CARD_PATH "GAME:\\PS1_MC%d.mcd"

// Convert PS1 memory card to Xbox 360 file
HRESULT save_memory_card_to_xbox(const memcard_block_t* ps1_card, int card_slot) {
    char xbox_filename[MAX_PATH];
    sprintf_s(xbox_filename, MEMORY_CARD_PATH, card_slot);
    
    HANDLE hFile = CreateFile(xbox_filename, GENERIC_WRITE,
                          FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL);
    
    if (hFile == INVALID_HANDLE_VALUE) {
        return E_FAIL;
    }
    
    // Write PS1 memory card format
    DWORD bytesWritten;
    BOOL result = WriteFile(hFile, ps1_card, 131072, &bytesWritten);
    
    CloseHandle(hFile);
    return result ? S_OK : E_FAIL;
}
```

### Memory Card Manager
```cpp
class CMemoryCardManager {
private:
    std::string m_cardPath;
    memcard_block_t* m_pCardData;
    HANDLE m_hCardFile;
    
public:
    CMemoryCardManager(const char* path) : m_cardPath(path) {}
    
    HRESULT LoadCard(int slot) {
        char filename[MAX_PATH];
        sprintf_s(filename, "GAME:\\PS1_MC%d.mcd", slot);
        
        m_hCardFile = CreateFile(filename, GENERIC_READ,
                                  FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL);
        
        if (m_hCardFile == INVALID_HANDLE_VALUE) {
            return E_FAIL;
        }
        
        m_pCardData = new memcard_block_t();
        DWORD bytesRead;
        BOOL result = ReadFile(m_hCardFile, m_pCardData, 131072, &bytesRead);
        
        return result ? S_OK : E_FAIL;
    }
    
    HRESULT SaveCard(int slot) {
        // Implementation similar to LoadCard
        // Include checksum validation
        uint8_t checksum = calculate_checksum((uint8_t*)m_pCardData, 131071);
        return save_memory_card_to_xbox(m_pCardData, slot);
    }
    
    bool ValidateChecksum() {
        uint8_t calculated = calculate_checksum((uint8_t*)m_pCardData, 131071);
        return (calculated == m_pCardData[131071]); // Last byte
    }
};
```

## Save State Implementation

### PS1 Save State Format
```cpp
typedef struct {
    uint32_t magic_number;        // "PSXS" identifier
    uint32_t version_number;       // State format version
    uint32_t cpu_state_size;      // CPU context size
    uint32_t gpu_state_size;       // GPU registers size
    uint32_t spu_state_size;       // SPU registers size
    uint32_t memory_snapshot_size;   // RAM+VRAM size
    
    // Variable size sections follow
    // CPU context (registers, pipeline, cache)
    // GPU context (registers, VRAM state)
    // SPU context (channels, reverb)
    // Memory snapshot (RAM, VRAM, scratchpad)
    
    uint32_t checksum;             // State integrity check
} psx_save_state_t;
```

### Xbox 360 Save State Integration
```cpp
class CStateManager {
private:
    std::string m_savePath;
    std::map<std::string, psx_save_state_t*> m_saveStates;
    
public:
    HRESULT SaveState(const char* state_name) {
        psx_save_state_t* pState = new psx_save_state_t();
        
        // Capture current PS1 emulation state
        capture_cpu_state(pState);
        capture_gpu_state(pState);
        capture_spu_state(pState);
        capture_memory_snapshot(pState);
        
        // Calculate checksum
        pState->checksum = calculate_state_checksum(pState);
        
        // Save to Xbox 360 storage
        return save_state_to_xbox(state_name, pState);
    }
    
    HRESULT LoadState(const char* state_name) {
        // Load state from Xbox 360 storage
        psx_save_state_t* pState = load_state_from_xbox(state_name);
        
        if (!validate_state_checksum(pState)) {
            return E_INVALIDARG; // Corrupted state
        }
        
        // Restore PS1 emulation state
        restore_cpu_state(pState);
        restore_gpu_state(pState);
        restore_spu_state(pState);
        restore_memory_snapshot(pState);
        
        return S_OK;
    }
};
```

## Data Conversion Utilities

### Checksum Calculations
```cpp
// PS1 memory card checksum (XOR of all bytes)
uint8_t calculate_memory_card_checksum(const uint8_t* data, size_t size) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

// Save state checksum (more robust)
uint32_t calculate_save_state_checksum(const psx_save_state_t* state) {
    uint32_t checksum = 0;
    const uint8_t* data = (const uint8_t*)state;
    size_t size = sizeof(psx_save_state_t) - sizeof(uint32_t); // Exclude checksum
    
    for (size_t i = 0; i < size; i++) {
        checksum = ((checksum << 1) | (checksum >> 31)) ^ data[i];
    }
    
    return checksum;
}
```

### Endianess Conversion
```cpp
// Convert PS1 big-endian to Xbox 360 little-endian
uint16_t ps1_to_xbox16(uint16_t value) {
    return ((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8);
}

uint32_t ps1_to_xbox32(uint32_t value) {
    return ((value & 0xFF000000) >> 24) | 
           ((value & 0x00FF0000) >> 8)  |
           ((value & 0x0000FF00) << 8)  |
           ((value & 0x000000FF) << 24);
}
```

## Error Handling and Recovery

### Common Memory Card Issues
```cpp
typedef enum {
    MEMCARD_ERROR_NONE,
    MEMCARD_ERROR_NOT_FOUND,
    MEMCARD_ERROR_CORRUPTED,
    MEMCARD_ERROR_CHECKSUM_FAILED,
    MEMCARD_ERROR_NO_SPACE,
    MEMCARD_ERROR_WRITE_PROTECTED,
    MEMCARD_ERROR_IO_FAILURE
} memcard_error_t;

memcard_error_t validate_memory_card(const memcard_block_t* card) {
    // Check magic number
    if (memcmp(card->header.memory_card_id, "MC", 2) != 0) {
        return MEMCARD_ERROR_CORRUPTED;
    }
    
    // Validate directory entries
    for (int i = 1; i < 16; i++) {
        const memcard_directory_entry_t* entry = &card->directory_frames[i];
        
        // Check allocation state validity
        if (entry->allocation_state != 0x51 && 
            entry->allocation_state != 0x52 && 
            entry->allocation_state != 0x53 &&
            entry->allocation_state != 0xA0 &&
            entry->allocation_state != 0xA1 &&
            entry->allocation_state != 0xA2 &&
            entry->allocation_state != 0xA3) {
            return MEMCARD_ERROR_CORRUPTED;
        }
        
        // Validate checksum
        uint8_t calculated = calculate_directory_checksum(entry);
        if (calculated != entry->checksum) {
            return MEMCARD_ERROR_CHECKSUM_FAILED;
        }
    }
    
    return MEMCARD_ERROR_NONE;
}
```

## Performance Optimization

### Memory Access Patterns
```cpp
// Optimize sequential memory card access
class COptimizedMemoryCard {
private:
    uint8_t* m_pCachedData;
    uint32_t m_cacheLineSize;
    
public:
    uint8_t* ReadBlock(uint32_t block_number) {
        // Align to cache line boundaries
        uint32_t offset = block_number * 8192;
        uint32_t aligned_offset = offset & ~(m_cacheLineSize - 1);
        
        return m_pCachedData + aligned_offset;
    }
    
    void WriteBlock(uint32_t block_number, const uint8_t* data) {
        // Use Xbox 360 DMA for large transfers
        uint32_t offset = block_number * 8192;
        
        // Optimize for 64-byte alignment
        memcpy_aligned_64(m_pCachedData + offset, data, 8192);
    }
};
```

This comprehensive memory card and save state documentation provides the technical foundation for implementing robust PS1 memory card emulation on Xbox 360 hardware.