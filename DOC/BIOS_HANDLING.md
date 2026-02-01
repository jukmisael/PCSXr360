# BIOS Handling for PlayStation 1 Emulation

## Overview
This document covers PlayStation 1 BIOS functionality, region handling, and implementation strategies for BIOS emulation in PCSXr360 on Xbox 360.

## PlayStation BIOS Technical Specifications

### Physical BIOS Characteristics
- **Storage**: 512KB ROM chip
- **Location**: Memory mapped at 0xBFC00000 - 0xBFC7FFFF
- **Function**: Hardware initialization, memory card management, I/O control
- **Access**: Read-only during normal operation
- **Versions**: Different regional variants

### Regional BIOS Variants

#### Japan Region (SCPH-5500 v3.0J)
- **File**: `scph5500.bin`
- **SHA-256**: `9c0421858e217805f4abe18698afea8d5aa36ff0727eb8484944e00eb5e7eadb`
- **Region Code**: "BI" prefix in filenames
- **TV System**: NTSC-J
- **Languages**: Japanese

#### North America Region (SCPH-5501 v3.0A)
- **File**: `scph5501.bin`
- **SHA-256**: `11052b6499e466bbf0a709b1f9cb6834a9418e66680387912451e971cf8a1fef`
- **Region Code**: "BA" prefix in filenames
- **TV System**: NTSC-U/C
- **Languages**: English

#### Europe Region (SCPH-5502 v3.0E)
- **File**: `scph5502.bin`
- **SHA-256**: `1faaa18fa820a0225e488d9f086296b8e6c46df739666093987ff7d8fd352c09`
- **Region Code**: "BE" prefix in filenames
- **TV System**: PAL
- **Languages**: Multi-language support

### BIOS Memory Layout
```cpp
// BIOS ROM memory mapping
#define BIOS_BASE_ADDRESS    0xBFC00000
#define BIOS_SIZE           0x80000     // 512KB
#define BIOS_END_ADDRESS    0xBFC7FFFF

// Important BIOS function offsets
#define BIOS_RESET_VECTOR        0xBFC00000  // Reset vector
#define BIOS_EXCEPTION_HANDLERS 0xBFC00180  // Exception handler table
#define BIOS_INTERRUPT_TABLE   0xBFC00200  // Interrupt table
#define BIOS_INIT_ROUTINES     0xBFC01000  // Initialization routines
```

## BIOS Functionality

### Hardware Initialization
```cpp
// BIOS hardware initialization sequence
typedef struct {
    uint32_t cpu_frequency;      // CPU clock configuration
    uint32_t bus_configuration;  // Bus timing setup
    uint32_t memory_config;      // Memory controller setup
    uint32_t gpu_config;        // GPU initialization
    uint32_t spu_config;        // SPU initialization
    uint32_t cdrom_config;      // CD-ROM controller setup
    uint32_t interrupt_enable;     // Interrupt mask setup
    uint32_t region_detection;   // Console region detection
} bios_init_data_t;
```

### Memory Card Management
```cpp
// BIOS memory card functions
typedef struct {
    uint32_t mc_detect;          // Card detection routine
    uint32_t mc_init;            // Card initialization
    uint32_t mc_read_block;        // Block read routine
    uint32_t mc_write_block;       // Block write routine
    uint32_t mc_erase_block;       // Block erase routine
    uint32_t mc_format_card;       // Card format routine
    uint32_t mc_get_directory;      // Directory read routine
    uint32_t mc_write_directory;     // Directory write routine
} bios_memorycard_funcs_t;
```

### System Control Functions
```cpp
// BIOS system control interface
typedef struct {
    uint32_t set_interrupt_mask;   // Interrupt mask control
    uint32_t clear_interrupt_mask; // Interrupt clear
    uint32_t enable_dma;         // DMA controller enable
    uint32_t disable_dma;        // DMA controller disable
    uint32_t configure_cache;     // Cache control setup
    uint32_t set_bus_timing;     // Bus timing configuration
    uint32_t power_management;    // Power state control
} bios_system_control_t;
```

## BIOS Emulation Implementation

### BIOS Loading and Verification
```cpp
class CBSIEmulator {
private:
    uint8_t* m_pBiosData;
    bios_region_t m_region;
    bool m_bLoaded;
    
    // BIOS function pointers
    typedef void (BIOS_FUNCTION)(void);
    std::map<uint32_t, BIOS_FUNCTION*> m_functionMap;
    
public:
    HRESULT LoadBIOS(const char* bios_path) {
        FILE* hFile = fopen(bios_path, "rb");
        if (!hFile) {
            return E_FILE_NOT_FOUND;
        }
        
        // Allocate memory for BIOS
        m_pBiosData = new uint8_t[BIOS_SIZE];
        fread(m_pBiosData, 1, BIOS_SIZE, hFile);
        fclose(hFile);
        
        // Verify BIOS integrity
        if (!ValidateBIOS()) {
            delete[] m_pBiosData;
            return E_INVALID_DATA;
        }
        
        // Detect region
        m_region = DetectBIOSRegion();
        
        // Initialize function mapping
        InitializeFunctionMap();
        
        m_bLoaded = true;
        return S_OK;
    }
    
private:
    bool ValidateBIOS() {
        // Check known BIOS signatures
        if (memcmp(m_pBiosData, "PS-X", 4) != 0) {
            return false;
        }
        
        // Verify checksums for known regions
        uint32_t calculated_hash = CalculateSHA256(m_pBiosData, BIOS_SIZE);
        
        switch (m_region) {
            case BIOS_REGION_JAPAN:
                return (calculated_hash == JAPAN_BIOS_HASH);
            case BIOS_REGION_AMERICA:
                return (calculated_hash == AMERICA_BIOS_HASH);
            case BIOS_REGION_EUROPE:
                return (calculated_hash == EUROPE_BIOS_HASH);
            default:
                return false;
        }
    }
    
    bios_region_t DetectBIOSRegion() {
        // Check BIOS region based on build date and version string
        uint32_t version_offset = 0x100;
        uint32_t date_offset = 0x120;
        
        char version_string[16];
        memcpy(version_string, m_pBiosData + version_offset, 16);
        
        // Region detection heuristics
        if (strstr(version_string, "J")) {
            return BIOS_REGION_JAPAN;
        } else if (strstr(version_string, "A")) {
            return BIOS_REGION_AMERICA;
        } else if (strstr(version_string, "E")) {
            return BIOS_REGION_EUROPE;
        }
        
        return BIOS_REGION_UNKNOWN;
    }
};
```

### System Call Emulation (A0h, B0h, C0h)
```cpp
// BIOS system call handling
class CBSISystemCalls {
public:
    uint32_t HandleSystemCall(uint32_t opcode) {
        uint32_t function_number = opcode & 0x3F;
        uint32_t a0 = GetMIPSRegister(4);  // Argument register
        uint32_t a1 = GetMIPSRegister(5);
        uint32_t a2 = GetMIPSRegister(6);
        uint32_t a3 = GetMIPSRegister(7);
        
        switch (function_number) {
            case 0x01: // File open
                return Syscall_FileOpen(a0, a1, a2);
                
            case 0x02: // File seek
                return Syscall_FileSeek(a0, a1, a2);
                
            case 0x03: // File read
                return Syscall_FileRead(a0, a1, a2);
                
            case 0x04: // File write
                return Syscall_FileWrite(a0, a1, a2);
                
            case 0x05: // File close
                return Syscall_FileClose(a0);
                
            case 0x08: // Memory card info
                return Syscall_MemcardInfo(a0);
                
            case 0x09: // Memory card read
                return Syscall_MemcardRead(a0, a1, a2);
                
            case 0x0A: // Memory card write
                return Syscall_MemcardWrite(a0, a1, a2);
                
            case 0x0F: // File remove
                return Syscall_FileRemove(a0, a1);
                
            default:
                printf("Unknown BIOS system call: 0x%02X\n", function_number);
                return 0xFFFFFFFF;
        }
    }
    
private:
    uint32_t Syscall_FileOpen(const char* filename, uint32_t mode, uint32_t a2) {
        // Convert PS1 file path to Xbox 360 path
        char xbox_path[MAX_PATH];
        ConvertPS1PathToXbox360(filename, xbox_path);
        
        // Open file with appropriate mode
        DWORD access = 0;
        if (mode & 0x01) access |= GENERIC_READ;
        if (mode & 0x02) access |= GENERIC_WRITE;
        
        HANDLE hFile = CreateFile(xbox_path, access, FILE_SHARE_READ, 
                              OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL);
        
        if (hFile == INVALID_HANDLE_VALUE) {
            return 0xFFFFFFFF;
        }
        
        // Return file handle (or error code)
        return (uint32_t)hFile;
    }
};
```

## Memory Card Emulation via BIOS

### BIOS Memory Card Functions
```cpp
class BIOSMemoryCardHandler {
private:
    CMemoryCardManager* m_pCardManager;
    uint8_t* m_pCardRAM;  // 128KB virtual memory card space
    
public:
    uint32_t HandleMemoryCardCommand(uint32_t command, uint32_t port, uint32_t* params) {
        switch (command) {
            case MEMCARD_CMD_INIT:
                return InitializeCard(port);
                
            case MEMCARD_CMD_READ:
                return ReadFromCard(port, params[0], params[1]);
                
            case MEMCARD_CMD_WRITE:
                return WriteToCard(port, params[0], params[1], params[2]);
                
            case MEMCARD_CMD_ERASE:
                return EraseCard(port);
                
            case MEMCARD_CMD_FORMAT:
                return FormatCard(port);
                
            case MEMCARD_CMD_GET_INFO:
                return GetCardInfo(port);
                
            default:
                return MEMCARD_ERROR_INVALID_COMMAND;
        }
    }
    
private:
    uint32_t ReadFromCard(uint32_t port, uint32_t block, uint8_t* buffer) {
        // Validate card is present
        if (!IsCardPresent(port)) {
            return MEMCARD_ERROR_NO_CARD;
        }
        
        // Read 8KB block from virtual memory card
        uint32_t offset = block * 8192;
        if (offset + 8192 > 131072) {
            return MEMCARD_ERROR_INVALID_BLOCK;
        }
        
        memcpy(buffer, m_pCardRAM + offset, 8192);
        
        // Simulate PS1 memory card access time (~1KB/ms)
        Sleep(8);
        
        return MEMCARD_SUCCESS;
    }
    
    uint32_t WriteToCard(uint32_t port, uint32_t block, const uint8_t* buffer, uint32_t size) {
        // Validate write parameters
        if (!IsCardPresent(port)) {
            return MEMCARD_ERROR_NO_CARD;
        }
        
        if (size != 8192) {
            return MEMCARD_ERROR_INVALID_SIZE;
        }
        
        // Write protection check
        if (IsCardWriteProtected(port)) {
            return MEMCARD_ERROR_WRITE_PROTECTED;
        }
        
        // Write 8KB block to virtual memory card
        uint32_t offset = block * 8192;
        memcpy(m_pCardRAM + offset, buffer, size);
        
        // Update directory and checksums
        UpdateDirectoryEntry(block, buffer, size);
        UpdateCardChecksums();
        
        // Simulate PS1 memory card write time (~5KB/ms)
        Sleep(25);
        
        return MEMCARD_SUCCESS;
    }
};
```

## Hardware Registers Emulation

### I/O Port Register Mapping
```cpp
// PS1 I/O register addresses
#define IO_BASE_ADDRESS     0x1F000000

// Memory card I/O registers
#define MEMCARD_DATA_PORT    (IO_BASE_ADDRESS + 0x00)
#define MEMCARD_STATUS_PORT  (IO_BASE_ADDRESS + 0x01)
#define MEMCARD_COMMAND_PORT (IO_BASE_ADDRESS + 0x02)
#define MEMCARD_SELECT_PORT (IO_BASE_ADDRESS + 0x03)

// CD-ROM I/O registers
#define CDROM_STATUS_PORT   (IO_BASE_ADDRESS + 0x10)
#define CDROM_COMMAND_PORT  (IO_BASE_ADDRESS + 0x11)
#define CDROM_DATA_PORT    (IO_BASE_ADDRESS + 0x12)
#define CDROM_RESULT_PORT  (IO_BASE_ADDRESS + 0x13)

// Timer registers
#define TIMER0_VALUE       (IO_BASE_ADDRESS + 0x18)
#define TIMER1_VALUE       (IO_BASE_ADDRESS + 0x1C)
#define TIMER2_VALUE       (IO_BASE_ADDRESS + 0x20)
#define TIMER3_VALUE       (IO_BASE_ADDRESS + 0x24)

class CIORegisterHandler {
public:
    uint8_t ReadRegister(uint32_t address) {
        switch (address) {
            case MEMCARD_STATUS_PORT:
                return GetMemoryCardStatus();
                
            case CDROM_STATUS_PORT:
                return GetCDROMStatus();
                
            case CDROM_RESULT_PORT:
                return GetCDROMResult();
                
            case TIMER0_VALUE:
                return GetTimerValue(0);
                
            default:
                printf("Unknown I/O read: 0x%08X\n", address);
                return 0xFF;
        }
    }
    
    void WriteRegister(uint32_t address, uint8_t value) {
        switch (address) {
            case MEMCARD_COMMAND_PORT:
                ProcessMemoryCardCommand(value);
                break;
                
            case CDROM_COMMAND_PORT:
                ProcessCDROMCommand(value);
                break;
                
            case TIMER0_VALUE:
                SetTimerValue(0, value);
                break;
                
            default:
                printf("Unknown I/O write: 0x%08X = 0x%02X\n", address, value);
        }
    }
};
```

## Error Handling and Debugging

### BIOS Error Codes
```cpp
// Common BIOS error codes
#define BIOS_SUCCESS          0x00
#define BIOS_ERROR_INVALID    0xFFFFFFFF
#define BIOS_ERROR_NOT_FOUND  0xFFFFFFFE
#define BIOS_ERROR_IO_FAILURE  0xFFFFFFFD

class CBiosErrorHandler {
public:
    void LogError(uint32_t error_code, const char* context) {
        switch (error_code) {
            case BIOS_ERROR_INVALID:
                printf("BIOS Error: Invalid parameter in %s\n", context);
                break;
                
            case BIOS_ERROR_NOT_FOUND:
                printf("BIOS Error: File not found in %s\n", context);
                break;
                
            case BIOS_ERROR_IO_FAILURE:
                printf("BIOS Error: I/O failure in %s\n", context);
                break;
                
            default:
                printf("BIOS Error: Unknown error 0x%08X in %s\n", error_code, context);
        }
    }
};
```

## Performance Optimization

### BIOS Function Caching
```cpp
// Cache frequently used BIOS routines
class CBiosCache {
private:
    struct CacheEntry {
        uint32_t function_offset;
        uint8_t* cached_code;
        uint32_t instruction_count;
        uint64_t execution_count;
    };
    
    static const uint32_t CACHE_SIZE = 64;
    CacheEntry m_cache[CACHE_SIZE];
    uint32_t m_lru_index;
    
public:
    uint8_t* GetFunction(uint32_t function_offset) {
        for (uint32_t i = 0; i < CACHE_SIZE; i++) {
            if (m_cache[i].function_offset == function_offset) {
                m_cache[i].execution_count++;
                return m_cache[i].cached_code;
            }
        }
        
        return nullptr; // Cache miss
    }
    
    void CacheFunction(uint32_t function_offset, uint8_t* code, uint32_t size) {
        // Use LRU replacement
        uint32_t lru_entry = m_lru_index;
        m_lru_index = (m_lru_index + 1) % CACHE_SIZE;
        
        m_cache[lru_entry].function_offset = function_offset;
        m_cache[lru_entry].cached_code = code;
        m_cache[lru_entry].instruction_count = size;
        m_cache[lru_entry].execution_count = 1;
    }
};
```

This comprehensive BIOS handling documentation provides the technical foundation for implementing accurate PS1 BIOS emulation on Xbox 360 hardware, including region detection, system calls, and memory card management.