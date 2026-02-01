# Complete PCSXr360 Documentation Index

## Overview
This directory contains comprehensive technical documentation for PCSXr360 PlayStation 1 emulator development on Xbox 360. Each document focuses on specific technical aspects essential for accurate and efficient emulation.

## Document Structure

### Hardware Architecture Documentation (Kernel-Level)
| Document | Description | Focus Areas |
|----------|-------------|---------------|
| [XBOX_360_HARDWARE.md](XBOX_360_HARDWARE.md) | Complete Xbox 360 specifications | CPU, GPU, memory, eDRAM, I/O systems |
| [PS1_HARDWARE.md](PS1_HARDWARE.md) | PlayStation 1 specifications | MIPS CPU, memory systems, GPU, audio |
| **[PS1_ARCHITECTURE_KERNEL.md](PS1_ARCHITECTURE_KERNEL.md)** | **PS1 Kernel Architecture (NEW)** | MIPS R3000A, coprocessors, interrupts, memory map, GTE, SPU |
| **[XBOX360_ARCHITECTURE_KERNEL.md](XBOX360_ARCHITECTURE_KERNEL.md)** | **Xbox 360 Kernel Architecture (NEW)** | Xenon CPU, Xenos GPU, hypervisor, VMX-128, boot process |
| [CPU_COMPARISON.md](CPU_COMPARISON.md) | Architecture comparison | MIPS vs PowerPC translation challenges |

### Development Documentation  
| Document | Description | Technical Areas |
|----------|-------------|----------------|
| [XDK_GUIDE.md](XDK_GUIDE.md) | Xbox Development Kit | XDK APIs, memory management, graphics, audio |
| [EMULATION_TECHNIQUES.md](EMULATION_TECHNIQUES.md) | Emulation strategies | Dynarec, GPU translation, optimization |
| **[OPTIMIZATION_DEBUGGING_GUIDE.md](OPTIMIZATION_DEBUGGING_GUIDE.md)** | **Performance & Debugging (NEW)** | Xbox 360 optimizations, VMX-128, profiling, troubleshooting |
| **[BUILD_SETUP.md](../360/Xdk/pcsxr/BUILD_SETUP.md)** | **Build Configuration (NEW)** | Release build structure, XZP generation, data files |

### System Implementation Documentation
| Document | Description | Implementation Details |
|----------|-------------|---------------------|
| [MEMORY_SYSTEMS.md](MEMORY_SYSTEMS.md) | Memory architecture comparison | PS1 vs Xbox 360 memory mapping, optimization |
| [MEMORY_CARDS.md](MEMORY_CARDS.md) | Memory card emulation | PS1 card format, save states, Xbox 360 integration |
| [BIOS_HANDLING.md](BIOS_HANDLING.md) | BIOS functionality | Regional variants, system calls, I/O emulation |
| **[PS1_MEMORY_REGISTERS_MAP.md](PS1_MEMORY_REGISTERS_MAP.md)** | **PS1 Memory & Registers (NEW)** | Complete I/O map, register bit fields, addresses |
| **[XBOX360_MEMORY_REGISTERS_MAP.md](XBOX360_MEMORY_REGISTERS_MAP.md)** | **Xbox 360 Memory & Registers (NEW)** | Physical memory layout, MMIO, system registers |

### Reference & Opcode Documentation
| Document | Description | Technical Reference |
|----------|-------------|-------------------|
| **[MIPS_R3000A_OPCODES.md](MIPS_R3000A_OPCODES.md)** | **MIPS R3000A Opcode Reference (NEW)** | Complete instruction set, encoding, delay slots |
| **[HLSL_SHADERS_VIDEO_FILTERING.md](HLSL_SHADERS_VIDEO_FILTERING.md)** | **HLSL Shaders & Filters (NEW)** | Video filtering pipeline, shader algorithms |
| **[COMPATIBILITY_TROUBLESHOOTING.md](COMPATIBILITY_TROUBLESHOOTING.md)** | **Compatibility Guide (NEW)** | Game compatibility, known issues, debugging |

### Project & Build Documentation
| Document | Description | Purpose |
|----------|-------------|---------|
| [PROJECT_STATUS.md](PROJECT_STATUS.md) | Current development status | Roadmap, issues, priorities |
| [COMPILATION_ISSUES.md](COMPILATION_ISSUES.md) | Build troubleshooting | Common errors, solutions |

## Quick Reference

### Key Technical Challenges
1. **CPU Translation**: MIPS R3000A → PowerPC Xenon with 95× clock speed difference
2. **Memory Management**: 2MB PS1 → 512MB Xbox 360 unified memory with endianess conversion
3. **Graphics Translation**: PS1 integer GPU → Xbox 360 floating-point GPU with eDRAM optimization
4. **Audio Processing**: PS1 SPU → XAudio2 with ADPCM decompression
5. **Storage Integration**: PS1 memory cards → Xbox 360 file system with format conversion

### Performance Optimization Strategies
- **Dynamic Recompilation**: Block-based MIPS to PowerPC translation with caching
- **Multi-threading**: Utilize Xbox 360's 6 hardware threads (3 cores × 2 threads)
- **Cache Optimization**: Leverage 1MB L2 cache for PS1 memory working set
- **eDMA Utilization**: Efficient GPU memory transfers with 256 GB/s eDRAM bandwidth

### Development Best Practices
- **Region Handling**: Support all PS1 BIOS regions (Japan, America, Europe)
- **Accuracy vs Speed**: Balance cycle accuracy with performance optimization
- **Memory Protection**: Proper Xbox 360 memory permissions and virtualization
- **Error Handling**: Robust validation and recovery mechanisms

### Current Development Issues
The PCSXr360 project has compilation errors related to Xbox UI (XUI) framework:
- Missing XUI framework library integration
- Unresolved external symbols for rendering, input, and UI management
- Need to fix linkage or provide alternative implementations

## Usage Guidelines

### For Emulator Development
1. **Start with Hardware Documentation**: Understand PS1 and Xbox 360 architectures
2. **Review Implementation Guides**: Study emulation techniques and optimization strategies
3. **Apply System Integration**: Use memory, BIOS, and storage documentation
4. **Optimize for Xbox 360**: Leverage multi-core and GPU capabilities

### For Performance Tuning
1. **Profile Emulation Hotspots**: Use dynarec profiling to identify bottlenecks
2. **Optimize Memory Access**: Apply caching strategies from memory systems guide
3. **GPU Translation**: Use Xbox 360 eDMA and shader capabilities effectively
4. **Multi-threading**: Balance CPU, GPU, and audio processing threads

### For Compatibility Work
1. **Regional Testing**: Test with all PS1 BIOS variants
2. **Game-Specific Fixes**: Implement per-game compatibility patches
3. **Hardware Limitations**: Respect original PS1 timing and bandwidth constraints
4. **Validation**: Use memory card and BIOS validation routines

## Technical Specifications Summary

### PlayStation 1 System
- **CPU**: MIPS R3000A @ 33.87MHz, 5-stage pipeline with delay slots
- **Memory**: 2MB EDO RAM + 1MB VRAM + 512KB SPU RAM
- **Graphics**: Sony GPU with 15-bit color, integer coordinates, affine textures
- **Storage**: 2× CD-ROM, 128KB memory cards
- **Audio**: 16-bit SPU with 24 ADPCM channels, reverb effects

### Xbox 360 System  
- **CPU**: 3× PowerPC @ 3.2GHz, 6 hardware threads, 1MB L2 cache
- **Memory**: 512MB GDDR3 @ 700MHz, unified architecture, 10MB eDRAM
- **Graphics**: ATI Xenos @ 500MHz, unified shaders, 256 GB/s eDRAM bandwidth
- **Storage**: 12× DVD, 20-500GB HDD, USB storage
- **Audio**: XAudio2, 320 channels, XMA decompression

## Development Environment

### Required Tools
- **Visual Studio 2010**: Primary IDE and compiler
- **Xbox 360 XDK**: Development libraries and frameworks
- **Debugging**: Hardware debugger for low-level access
- **Performance Tools**: Profiling and optimization utilities

### Build Configuration
- **Multiple Configurations**: Debug, Release, Profile, LTCG optimization
- **Xbox 360 Targets**: Various memory and performance settings
- **Link Optimization**: Efficient symbol resolution and code generation

This comprehensive documentation structure provides complete technical reference for PCSXr360 emulator development, covering all major architectural components and implementation challenges.

## Additional Resources

### Development References
- **Microsoft XDK Documentation**: Official Xbox 360 development guides
- **Sony PS1 Technical References**: Original PlayStation specifications
- **Open Source Emulators**: PCSX-Reloaded, Mednafen, DuckStation
- **Homebrew Community**: Emulation research and optimization techniques

### Support Information
For questions about PCSXr360 development:
- **Technical Issues**: Refer to specific documentation sections
- **Compilation Problems**: Check XDK framework integration
- **Performance Optimization**: Apply techniques from EMULATION_TECHNIQUES.md
- **Compatibility Issues**: Use BIOS_HANDLING.md and MEMORY_CARDS.md guidance

---

**Document Version**: 2.0  
**Last Updated**: January 31, 2026  
**Target**: PCSXr360 PlayStation 1 Emulator on Xbox 360

---

## 📚 New Documentation Summary (January 2026)

### Kernel-Level Technical Documentation Created:
1. **PS1_ARCHITECTURE_KERNEL.md** - 600+ lines covering:
   - MIPS R3000A CPU architecture and opcodes
   - COP0 system control and exception handling
   - Memory segmentation (KUSEG/KSEG0/KSEG1/KSEG2)
   - GTE (Geometry Transformation Engine) commands
   - SPU ADPCM audio processing
   - DMA controller and interrupts
   - BIOS syscall interface

2. **XBOX360_ARCHITECTURE_KERNEL.md** - 1200+ lines covering:
   - Xenon CPU tri-core PowerPC architecture
   - Xenos GPU with unified shaders
   - VMX-128 vector unit (SIMD)
   - Memory architecture (512MB GDDR3 + 10MB eDRAM)
   - Hypervisor and kernel security layers
   - Boot process (1BL→2BL→4BL chain)
   - Exception handling and interrupts
   - Performance optimization strategies

3. **MIPS_R3000A_OPCODES.md** - Complete instruction reference:
   - All MIPS I instruction formats
   - Opcode tables with binary encoding
   - ALU, Load/Store, Branch, Coprocessor instructions
   - Delay slot rules and examples

4. **PS1_MEMORY_REGISTERS_MAP.md** - Complete hardware reference:
   - All I/O port addresses (GPU, SPU, DMA, Timers, CDROM)
   - Register bit fields and descriptions
   - Memory layout visualization

5. **XBOX360_MEMORY_REGISTERS_MAP.md** - Xbox 360 hardware reference:
   - Physical memory map
   - MMIO addresses
   - CPU/GPU shared memory architecture

6. **HLSL_SHADERS_VIDEO_FILTERING.md** - Video pipeline documentation:
   - HLSL shader architecture
   - Available filters and algorithms
   - Integration with Xenos GPU

7. **OPTIMIZATION_DEBUGGING_GUIDE.md** - Performance guide:
   - Recompiler optimization techniques
   - VMX-128 SIMD usage
   - Cache management
   - Multi-threading strategies (6 hardware threads)
   - Debugging and profiling tools

8. **COMPATIBILITY_TROUBLESHOOTING.md** - User/Dever guide:
   - Game compatibility list
   - Common problems and solutions
   - Debugging techniques
   - FAQ

### Total Documentation: 10+ comprehensive technical documents
### Focus Areas: Kernel architecture, opcodes, registers, memory maps, optimization, compatibility