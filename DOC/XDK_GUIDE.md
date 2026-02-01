# Xbox Development Kit (XDK) Guide

## Overview
The Xbox Development Kit (XDK) is Microsoft's official software development environment for creating Xbox 360 games and applications. For PCSXr360 development, understanding XDK architecture is essential for hardware access, performance optimization, and system integration.

## XDK Versions and History

### XDK Development Phases
| Version | Year | Key Features | Target Platform |
|-----------|------|---------------|------------------|
| XDK 1.0 | 2001 | Original Xbox development tools | Xbox (original) |
| XDK 360 | 2005 | Xbox 360 development environment | Xbox 360 |
| XDK-GB | 2006 | Game development for Xbox Live Arcade | Xbox 360 |
| XDK S | 2008 | Enhanced debugging, multi-platform tools | Xbox 360 |
| XDK Updates | 2009-2012 | Performance profiling, enhanced libraries | Xbox 360 |

### Development Kit Variants
- **Standard XDK**: Full development environment
- **XDK-Lite**: Limited functionality for smaller developers
- **XDK-Beta**: Early access to new features
- **XDK Academic**: Educational institutions with restrictions

## XDK Architecture

### Core Components

#### Development Environment
- **Visual Studio Integration**: VS 2005, 2008, 2010 support
- **Compilers**: Xbox 360 specific Visual C++ compiler
- **Linkers**: Custom linkers for Xbox 360 PE format
- **Debuggers**: Hardware debugging via USB/Ethernet

#### Runtime Libraries
- **Xbox 360 Framework**: Core system APIs
- **Direct3D 360**: Custom graphics API
- **XAudio 2**: Xbox 360 audio system
- **XInput**: Controller input API
- **XUI**: Xbox 360 user interface system

#### System APIs
- **Memory Management**: Xbox 360 memory allocation APIs
- **Threading**: Xbox 360 thread management
- **File I/O**: Xbox storage system APIs
- **Network**: Xbox Live networking APIs

## XDK Memory Management

### Memory Architecture
- **512MB Shared**: Unified memory for all system components
- **Physical Addressing**: Direct memory access for optimization
- **Virtual Memory**: Xbox 360 virtual memory management
- **eDRAM Integration**: 10MB embedded GPU memory access

### Allocation APIs
```cpp
// Xbox 360 memory allocation examples
XPhysicalMemory* pPhysicalMem = XPhysicalAlloc(size, MAX_64KB_PAGES);
LPVOID pVirtualMem = MmAllocateContiguousMemory(size, PAGE_READWRITE);
```

### Memory Best Practices
- **Alignment**: 64KB page boundaries for optimal performance
- **Contiguous Allocation**: Minimize fragmentation for DMA operations
- **Pool Management**: Use memory pools for frequent allocations
- **Debug Allocation**: XDK provides debug memory tracking

## XDK Graphics Programming

### Direct3D 360 API
- **Custom Implementation**: DirectX 9.0c variant for Xbox 360
- **GPU Access**: Direct Xenos GPU programming
- **eDRAM Integration**: Built-in support for embedded DRAM

### Graphics Initialization
```cpp
D3DPRESENT_PARAMETERS d3dpp;
ZeroMemory(&d3dpp, sizeof(d3dpp));
d3dpp.BackBufferWidth = 800;
d3dpp.BackBufferHeight = 600;
d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
d3dpp.BackBufferCount = 2;
d3dpp.EnableAutoDepthStencil = TRUE;
d3dpp.MultisampleType = D3DMULTISAMPLE_4_SAMPLES;
```

### eDRAM Optimization
- **Render Targets**: Use eDRAM for frame buffers
- **MSAA**: 4× anti-aliasing with no performance cost
- **Bandwidth**: 256 GB/s internal to GPU memory

## XDK Audio System

### XAudio 2 API
- **Hardware Access**: Direct DSP and audio hardware control
- **Multi-channel**: 320 independent audio channels
- **XMA Decompression**: Microsoft proprietary audio format

### Audio Processing
```cpp
// XAudio 2 initialization
IXAudio2* pXAudio2;
HRESULT hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);

// Voice creation
WAVEFORMATEX wfx = {0};
wfx.wFormatTag = WAVE_FORMAT_PCM;
wfx.nChannels = 2;
wfx.nSamplesPerSec = 48000;
wfx.wBitsPerSample = 16;
wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
```

## XDK Input System

### XInput API
- **Controller Support**: Up to 4 wireless controllers
- **Vibration**: Rumble motors control
- **Battery Status**: Wireless controller battery monitoring
- **Guide Button**: Xbox Guide button access

### Input Processing
```cpp
XINPUT_STATE state;
DWORD dwResult = XInputGetState(dwUserIndex, &state);

// Vibration control
XINPUT_VIBRATION vibration;
vibration.wLeftMotorSpeed = 65535;
vibration.wRightMotorSpeed = 32768;
XInputSetState(dwUserIndex, &vibration);
```

## XDK User Interface (XUI)

### XUI Framework
- **Xbox UI**: Native interface components
- **Skin Support**: Custom UI theming
- **Localization**: Multi-language support
- **Accessibility**: Accessibility features integration

### Common XUI Components
```cpp
// XUI element creation
HRESULT hr;
CXuiElement* pRootElement = NULL;
hr = XuiCreateRootElement(&pRootElement, this);

// Text element creation
CXuiTextElement* pTextElement = NULL;
hr = XuiTextCreate(&pTextElement, L"Hello World");

// Button creation
CXuiButtonElement* pButtonElement = NULL;
hr = XuiButtonCreate(&pButtonElement, L"OK");
```

### Compilation Issues
The PCSXr360 project currently has unresolved external symbol errors with XUI functions:
- **XuiRenderUninit**: Missing XUI framework library
- **XuiDestroyObject**: XUI object cleanup functions
- **XuiAnimRun**: Animation and timing functions
- **XuiProcessInput**: Input handling functions

## XDK Network System

### Xbox Live Integration
- **Xbox Live API**: Online multiplayer and social features
- **Matchmaking**: Automated player matching systems
- **Achievements**: Achievement system integration
- **Marketplace**: Content download and purchase

### Network APIs
- **Live Sessions**: Multiplayer session management
- **Voice Chat**: Real-time voice communication
- **Leaderboards**: Score tracking and comparison
- **Friends System**: Social networking features

## XDK File System

### Storage Architecture
- **Hard Drive**: SATA 2.5" in custom enclosure
- **Memory Units**: USB storage for game saves
- **DLC Support**: Downloadable content management

### File I/O Operations
```cpp
// File operations
HANDLE hFile = CreateFile(L"game:\\save.dat", GENERIC_WRITE, 
    FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);

// Directory operations
WIN32_FIND_DATA findData;
HANDLE hFind = FindFirstFile(L"game:\\*.sav", &findData);
```

## XDK Debugging

### Debug Console
- **Debug Output**: Visual Studio integration
- **Memory Profiling**: Memory usage tracking
- **Performance Counters**: Hardware performance metrics
- **Crash Dumps**: Automatic crash reporting

### Breakpoints and Watchpoints
```cpp
// Debug assertion
DebugAssert(condition != FALSE, "Critical error occurred");

// Performance timing
LARGE_INTEGER startTime, endTime, frequency;
QueryPerformanceCounter(&startTime);
QueryPerformanceFrequency(&frequency);

// Code execution
QueryPerformanceCounter(&endTime);
DWORD elapsed = (DWORD)((endTime.QuadPart - startTime.QuadPart) * 1000 / frequency.QuadPart);
```

## XDK Performance Optimization

### Threading Model
- **Multiple Cores**: Utilize all 3 CPU cores
- **Hardware Threads**: 6 threads total (2 per core)
- **Synchronization**: Xbox 360 threading primitives
- **Lock-free Algorithms**: Optimize for concurrent access

### GPU Performance
- **Shader Optimization**: Optimize for unified shader architecture
- **Batch Rendering**: Minimize draw calls
- **eDMA**: Efficient GPU DMA operations
- **Texture Atlases**: Reduce texture switching

## XDK Limitations for PCSXr360

### Security Restrictions
- **Signed Code**: All binaries must be Microsoft signed
- **Hardware Access**: Limited to approved XDK APIs
- **Network Certificates**: Required for Xbox Live features
- **Memory Limitations**: 512MB physical memory constraints

### Development Challenges
- **Documentation**: Some XDK APIs limited documentation
- **Debugging**: Hardware debugger required for advanced debugging
- **Performance**: Multi-core optimization complexity
- **Compatibility**: XUI framework integration issues

## PCSXr360 Specific Considerations

### Emulator Architecture
- **PS1 CPU Emulation**: MIPS → PowerPC dynamic recompilation
- **Memory Mapping**: PS1 memory map → Xbox 360 virtual memory
- **Graphics Translation**: PS1 GPU → Xbox 360 GPU rendering
- **Audio Processing**: PS1 SPU → XAudio 2 conversion

### Integration Points
- **Xbox Controllers**: Map PS1 controller to Xbox 360 input
- **Memory Cards**: Emulate PS1 memory cards via Xbox 360 storage
- **CD-ROM**: PS1 CD reading → Xbox 360 DVD/ISO loading
- **Save States**: Xbox 360 storage for PS1 save state format

This XDK guide provides comprehensive foundation for understanding Xbox 360 development environment essential for PCSXr360 emulator development and optimization.