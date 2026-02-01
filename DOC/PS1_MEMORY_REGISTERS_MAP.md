# PlayStation 1 Memory Map and I/O Registers Reference

## Table of Contents
1. [Memory Architecture Overview](#1-memory-architecture-overview)
2. [Virtual Memory Segments (KUSEG/KSEG)](#2-virtual-memory-segments)
3. [Physical Memory Layout](#3-physical-memory-layout)
4. [I/O Port Address Map](#4-io-port-address-map)
5. [GPU Registers](#5-gpu-registers)
6. [SPU Registers](#6-spu-registers)
7. [DMA Controller Registers](#7-dma-controller-registers)
8. [Timer Registers](#8-timer-registers)
9. [CD-ROM Registers](#9-cd-rom-registers)
10. [Controller/Memory Card Registers](#10-controllermemory-card-registers)
11. [Interrupt Controller Registers](#11-interrupt-controller-registers)
12. [Quick Reference Tables](#12-quick-reference-tables)

---

## 1. Memory Architecture Overview

The PlayStation 1 utilizes a 32-bit memory architecture based on the MIPS R3000A processor. Memory is accessed through virtual address segments that translate to physical addresses.

### Key Specifications
| Feature | Specification |
|---------|--------------|
| Address Space | 32-bit (4GB virtual) |
| Main RAM | 2MB EDO DRAM |
| VRAM | 1MB Dual-ported |
| SPU RAM | 512KB |
| BIOS ROM | 512KB |
| Scratchpad | 1KB (Data Cache) |
| Endianness | Little-endian |

---

## 2. Virtual Memory Segments

### MIPS Memory Segments
```
┌─────────────────────────────────────────────────────────┐
│ KUSEG (User)     │ 0x00000000 - 0x7FFFFFFF │ 2GB   │ Mapped, Cached    │
│ KSEG0 (Kernel)   │ 0x80000000 - 0x9FFFFFFF │ 512MB │ Unmapped, Cached  │
│ KSEG1 (Kernel)   │ 0xA0000000 - 0xBFFFFFFF │ 512MB │ Unmapped, Uncached│
│ KSEG2 (Kernel)   │ 0xC0000000 - 0xFFFFFFFF │ 1GB   │ Mapped, Cached    │
└─────────────────────────────────────────────────────────┘
```

### Segment Translation
```
KUSEG: Physical = Virtual (via TLB)
KSEG0: Physical = Virtual & 0x1FFFFFFF (Cached)
KSEG1: Physical = Virtual & 0x1FFFFFFF (Uncached)
KSEG2: Physical = Virtual (via TLB)
```

---

## 3. Physical Memory Layout

### Complete Physical Memory Map
```
┌─────────────────────────────────────────────────────────┐
│ Address Range          │ Size    │ Description          │
├─────────────────────────────────────────────────────────┤
│ 0x00000000-0x001FFFFF  │ 2MB     │ Main RAM             │
│ 0x00200000-0x1EFFFFFF  │ 477MB   │ Unmapped/Reserved    │
│ 0x1F000000-0x1F7FFFFF  │ 8MB     │ Expansion Region 1   │
│ 0x1F800000-0x1F8003FF  │ 1KB     │ Scratchpad (D-Cache) │
│ 0x1F801000-0x1F801FFF  │ 4KB     │ I/O Ports            │
│ 0x1F802000-0x1F802FFF  │ 4KB     │ Expansion Region 2   │
│ 0x1F803000-0x1FBFFFFF  │ 4MB-12KB│ Reserved             │
│ 0x1FC00000-0x1FC7FFFF  │ 512KB   │ BIOS ROM             │
│ 0x1FC80000-0xFFFFFFFF  │ ~1GB    │ Unmapped             │
└─────────────────────────────────────────────────────────┘
```

### Main RAM Mirror Mapping
```
Physical: 0x00000000-0x001FFFFF (2MB)
KUSEG:    0x00000000-0x001FFFFF
KSEG0:    0x80000000-0x801FFFFF
KSEG1:    0xA0000000-0xA01FFFFF
```

### Scratchpad RAM (Data Cache)
```
Physical: 0x1F800000-0x1F8003FF (1KB)
KSEG0:    0x9F800000-0x9F8003FF
KSEG1:    0xBF800000-0xBF8003FF

Usage: Fast temporary storage, stack operations
Note: Not a true cache - explicitly managed by software
```

### BIOS ROM
```
Physical: 0x1FC00000-0x1FC7FFFF (512KB)
KSEG1:    0xBFC00000-0xBFC7FFFF (Uncached access)

Contains:
- Boot code (Reset vector at 0xBFC00000)
- Kernel functions (A0h, B0h, C0h syscall tables)
- Interrupt handlers
```

---

## 4. I/O Port Address Map

### I/O Region Overview
```
Base Address: 0x1F801000-0x1F801FFF (4KB I/O space)
```

### Memory Control Registers
```
┌─────────────────────────────────────────────────────────┐
│ Address      │ Register    │ Description                │
├─────────────────────────────────────────────────────────┤
│ 0x1F801000   │ EXP1_BASE   │ Expansion 1 Base Address   │
│ 0x1F801004   │ EXP2_BASE   │ Expansion 2 Base Address   │
│ 0x1F801008   │ EXP1_DELAY  │ Expansion 1 Delay/Size     │
│ 0x1F80100C   │ EXP3_DELAY  │ Expansion 3 Delay/Size     │
│ 0x1F801010   │ BIOS_DELAY  │ BIOS ROM Delay/Size        │
│ 0x1F801014   │ SPU_DELAY   │ SPU Delay/Size             │
│ 0x1F801018   │ CDROM_DELAY │ CDROM Delay/Size           │
│ 0x1F80101C   │ EXP2_DELAY  │ Expansion 2 Delay/Size     │
│ 0x1F801020   │ COM_DELAY   │ Common Delay Control       │
│ 0x1F801060   │ RAM_SIZE    │ RAM Configuration          │
│ 0x1F801070   │ I_STAT      │ Interrupt Status           │
│ 0x1F801074   │ I_MASK      │ Interrupt Mask             │
│ 0x1F801080   │ I_STAT2     │ Interrupt Status (late)    │
│ 0x1F801084   │ I_MASK2     │ Interrupt Mask (late)      │
│ 0x1F801100   │ TMR0_CNT    │ Timer 0 Counter            │
│ 0x1F801104   │ TMR0_MOD    │ Timer 0 Mode               │
│ 0x1F801108   │ TMR0_TGT    │ Timer 0 Target             │
│ 0x1F801110   │ TMR1_CNT    │ Timer 1 Counter            │
│ 0x1F801114   │ TMR1_MOD    │ Timer 1 Mode               │
│ 0x1F801118   │ TMR1_TGT    │ Timer 1 Target             │
│ 0x1F801120   │ TMR2_CNT    │ Timer 2 Counter            │
│ 0x1F801124   │ TMR2_MOD    │ Timer 2 Mode               │
│ 0x1F801128   │ TMR2_TGT    │ Timer 2 Target             │
└─────────────────────────────────────────────────────────┘
```

---

## 5. GPU Registers

### GPU I/O Ports
```
┌─────────────────────────────────────────────────────────┐
│ Address      │ Name │ Description                       │
├─────────────────────────────────────────────────────────┤
│ 0x1F801810   │ GP0  │ GPU Command/Data (Write)          │
│              │      │ GPU Read Register (Read)          │
│ 0x1F801814   │ GP1  │ GPU Control Register (Write)      │
│              │      │ GPU Status Register (Read)        │
└─────────────────────────────────────────────────────────┘
```

### GP1 Status Register (Read - 0x1F801814)
```
Bit  0-3:   Texture page X base (0-15, in 64-pixel units)
Bit  4:     Texture page Y base (0-1, in 256-pixel units)
Bit  5-6:   Semi-transparency mode (0=B/2+F/2, 1=B+F, 2=B-F, 3=B+F/4)
Bit  7-8:   Texture page color mode (0=4-bit, 1=8-bit, 2=15-bit)
Bit  9:     Dither enable (0=off, 1=on)
Bit  10:    Drawing to display area (0=prohibited, 1=allowed)
Bit  11:    Set mask bit (force bit 15 of pixel to 1)
Bit  12:    Mask bit (don't draw to pixels with bit 15=1)
Bit  13:    Interlace field (0=even, 1=odd)
Bit  14:    Reverse flag (always 0)
Bit  15:    Texture page Y base (bit 2, always 0)
Bit  16:    Horizontal resolution (0=256, 1=320, 2=512, 3=640)
Bit  17:    Vertical resolution (0=240, 1=480)
Bit  18:    Video mode (0=NTSC, 1=PAL)
Bit  19:    Color depth (0=15-bit, 1=24-bit)
Bit  20:    Vertical interlace (0=off, 1=on)
Bit  21:    Display enable (0=off, 1=on)
Bit  22:    Interrupt request (0=off, 1=on)
Bit  23:    DMA request (always 0?)
Bit  24-28: Command ready + DMA ready (various bits)
Bit  29-30: DMA direction (0=off, 1=CPUtoGP0, 2=GP0toCPU, 3=reserved)
Bit  31:    Drawin