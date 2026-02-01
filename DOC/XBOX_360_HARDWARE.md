# Xbox 360 Hardware Technical Specifications

## Overview
The Xbox 360 is a seventh-generation video game console released by Microsoft in November 2005. It features a custom triple-core PowerPC-based CPU and advanced ATI graphics processor with embedded eDRAM.

## CPU Architecture

### Xenon Processor (XCPU)
- **Code Name**: Waternoose (later Loki)
- **Architecture**: Custom IBM PowerPC-based design
- **Cores**: 3 symmetrical cores
- **Clock Speed**: 3.2 GHz per core
- **Threading**: 2 hardware threads per core (6 total threads)
- **ISA**: 64-bit PowerPC with VMX128 vector extensions
- **Manufacturing Process**: 
  - Original: 90nm (2005)
  - Updated: 65nm SOI (2007)
  - Later: 45nm combined CPU/GPU chip (2010)
  - Latest: 32nm combined chip (2014)

### CPU Features
- **SIMD Processing**: VMX128 vector unit per core with dot-product instruction
- **Floating Point Performance**: 115.2 GFLOPS theoretical peak
- **Dot Product Operations**: 9.6 billion per second
- **Cache Architecture**:
  - **L1 Cache**: 32KB instruction + 32KB data per core (separate)
  - **L2 Cache**: 1MB shared, 8-way set associative, runs at half CPU speed
- **Execution Model**: In-order execution (simplified vs out-of-order)
- **Front Side Bus**: 21.6 GB/s aggregated bandwidth (10.8 GB/s upstream/downstream)

### Data Streaming Optimization
- **xDCBT Instruction**: Extended data cache block touch for direct L1 prefetch
- **Write Streaming**: Bypasses L1 cache, goes directly to L2 cache
- **XPS (Xbox Procedural Synthesis)**: CPU-to-GPU direct data transfer without main memory access

## Graphics Processing Unit

### Xenos GPU (ATI Design)
- **Code Name**: C1/R500 (later Gunga)
- **Clock Speed**: 500 MHz
- **Architecture**: Unified shader architecture (early implementation)
- **Manufacturing**: 90nm process on separate die from eDRAM

### eDRAM System
- **Capacity**: 10 MB embedded DRAM
- **Manufacturing**: NEC, separate die from GPU
- **Internal Bandwidth**: 256 GB/s (eDRAM to internal memory)
- **GPU to eDRAM Bandwidth**: 32 GB/s
- **Features**: 
  - 4x MSAA with no performance penalty
  - Z-buffering with no bandwidth cost
  - Alpha blending optimization

### GPU Capabilities
- **Theoretical Performance**: 240 GFLOPS
- **Shader Units**: 48-way parallel floating-point dynamically scheduled pipelines
- **Fill Rate**: 16 gigasamples per second (with 4x MSAA)
- **Polygon Performance**: 500 million triangles per second
- **Shader Operations**: 48 billion operations per second

## Memory System

### Main Memory
- **Type**: GDDR3 RAM
- **Capacity**: 512 MB
- **Clock Speed**: 700 MHz (effective 1.4 GHz)
- **Bus Width**: 128-bit
- **Architecture**: Unified memory system (CPU + GPU shared)
- **Interface Bandwidth**: 22.40 GB/s memory interface
- **Manufacturers**: Samsung or Qimonda

### Memory Bandwidth Breakdown
- **CPU to Memory**: Front side bus bandwidth
- **GPU to Memory**: Unified memory access
- **eDRAM Operations**: 256 GB/s internal
- **Southbridge**: 0.5 GB/s

## Storage Systems

### DVD Drive
- **Type**: 12x DVD-ROM
- **Data Transfer Rate**: 16.5 MB/s
- **Supported Formats**: 
  - DVD-ROM (dual-layer, 8.5 GB maximum)
  - DVD±R/RW, CD-ROM, CD-R/RW
  - CD-DA, XA Mode 2, WMA, MP3, JPEG Photo CD
- **Manufacturers**: LG/Samsung, Hitachi-LG, BenQ, Lite-On (various revisions)

### Hard Drive Storage
- **Interface**: SATA 2.5" in custom enclosure
- **Capacities**: 20GB, 60GB, 120GB, 250GB, 320GB, 500GB
- **System Reserved**: ~7GB (4GB for caching, 2GB for backward compatibility)
- **Compatibility**: Proprietary firmware, custom connector

## Input/Output Systems

### Controller Support
- **Simultaneous Controllers**: Up to 4 wireless controllers
- **Wireless Technology**: 2.4 GHz proprietary protocol
- **USB Ports**: 3 on original, 5 on Xbox 360 S
- **Memory Card Slots**: 2 on original, USB storage on S/E models

### Video Output
- **Standard Resolutions**: 480i, 480p, 720p, 1080i, 1080p
- **Connectors**: 
  - Component video (YPbPr)
  - VGA (via software update)
  - HDMI (added 2007)
  - Composite, S-Video (early models)

## Audio System

### Audio Processing
- **Channels**: 320 independent decompression channels
- **Processing**: 32-bit audio processing
- **Formats**: XMA (Microsoft proprietary), Dolby Digital 5.1 support
- **Output**: Multi-channel surround sound

## Motherboard Revisions

### Revision History
| Codename | CPU | GPU | eDRAM | Year | Key Features |
|----------|-----|-----|--------|------|---------------|
| Xenon | 90nm | 90nm | 90nm | 2005 | Original release, 203W PSU |
| Zephyr | 90nm | 90nm | 90nm | 2007 | HDMI, HANA chip, 203W PSU |
| Falcon | 65nm | 90nm | 90nm | 2007 | 65nm CPU, 175W PSU |
| Jasper | 65nm | 65nm | 90nm | 2008 | 65nm GPU, 150W PSU, 256/512MB NAND |
| Trinity | 45nm (combined) | 65nm | 65nm | 2010 | Xbox 360 S, CPU/GPU combined, 135W PSU |
| Corona | 45nm (combined) | 65nm | 65nm | 2011 | HANA integrated, 4GB eMMC option |
| Winchester | 32nm (combined) | 32nm | 32nm | 2014 | eDRAM integrated, patches reset glitch |

## Power Specifications

### Power Consumption
- **Original Models**: 203W (Xenon), 175W (Falcon), 150W (Jasper)
- **Xbox 360 S**: 135W (Trinity/Corona), 120W (later revisions)
- **Standby Power**: ~2W original, ~0.5W Xbox 360 S

### Thermal Management
- **Cooling**: Aluminum fins with copper base, heat pipe (original models)
- **Fans**: Dual 70mm rear fans (original), single side fan (Xbox 360 S)

## Network Connectivity

### Built-in Networking
- **Ethernet**: 10/100/1000 Mbps wired Ethernet
- **Wireless**: 
  - 802.11a/b/g via adapter (original)
  - Built-in 802.11b/g/n on Xbox 360 S/E
  - 5GHz band support via Wireless N adapter

## Development Features

### Debug and Development Support
- **Memory Access**: Full access to 512MB RAM for debugging
- **Remote Debugging**: Network-based debugging capabilities
- **Sidecar**: Development kit expansion for additional debugging features

### Security Features
- **Encrypted Keys**: ROM stores Microsoft private encrypted keys
- **Anti-Piracy**: Various drive firmware protections
- **Game Authentication**: Secure boot and verification processes

## Performance Metrics

### Computational Performance
- **Overall System**: ~1 TFLOP combined CPU+GPU performance
- **Memory Bandwidth**: 22.4 GB/s system + 256 GB/s eDRAM = ~278 GB/s effective
- **Graphics Fill Rate**: Up to 16 gigasamples/second with anti-aliasing
- **Polygon Throughput**: 500M triangles/second theoretical maximum

This technical specification provides the foundation for understanding Xbox 360 hardware capabilities essential for emulator development and optimization.