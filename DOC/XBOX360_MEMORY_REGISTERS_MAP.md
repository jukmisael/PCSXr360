# Xbox 360 Memory Map and Hardware Registers Reference

## Table of Contents
1. [Architecture Overview](#1-architecture-overview)
2. [Physical Memory Map](#2-physical-memory-map)
3. [Virtual Memory Architecture](#3-virtual-memory-architecture)
4. [CPU Registers and SPRs](#4-cpu-registers-and-sprs)
5. [GPU/Xenos Registers](#5-gpuxenos-registers)
6. [Memory Controller Registers](#6-memory-controller-registers)
7. [PCI Express Configuration](#7-pci-express-configuration)
8. [SMC and Hardware Monitor](#8-smc-and-hardware-monitor)
9. [Hypervisor Memory Regions](#9-hypervisor-memory-regions)
10. [Kernel Memory Layout](#10-kernel-memory-layout)
11. [eDRAM Registers](#11-edram-registers)
12. [Quick Reference Tables](#12-quick-reference-tables)

---

## 1. Architecture Overview

The Xbox 360 features a unified memory architecture with a custom IBM PowerPC tri-core CPU (XCPU) and ATI Xenos GPU sharing 512MB of GDDR3 memory.

### System Specifications
| Component | Specification |
|-----------|--------------|
| CPU | IBM Xenon (3x PowerPC cores @ 3.2GHz) |
| GPU | ATI Xenos @ 500MHz |
| System RAM | 512MB GDDR3 @ 700MHz |
| eDRAM | 10MB embedded GPU memory |
| CPU L1 Cache | 32KB I-cache + 32KB D-cache per core |
| CPU L2 Cache | 1MB shared across 3 cores |
| Memory Bus | 128-bit @ 1.4GHz effective |
| Memory Bandwidth | 22.4 GB/s (system), 256 GB/s (eDRAM) |
| Address Space | 64-bit physical, 44-bit virtual |

---

## 2. Physical Memory Map

### Complete 512MB Memory Layout
```
┌─────────────────────────────────────────────────────────┐
│ Address Range            │ Size    │ Purpose            │
├─────────────────────────────────────────────────────────┤
│ 0x00000000-0x0FFFFFFF    │ 256MB   │ Kernel/User Space  │
│ 0x10000000-0x1FFFFFFF    │ 256MB   │ System Reserved    │
│ 0x20000000-0x6FFFFFFF    │ 1.25GB  │ Unmapped/Reserved  │
│ 0x70000000-0x7FFFFFFF    │ 256MB   │ I/O Space (MMIO)   │
│ 0x80000000-0x8FFFFFFF    │ 256MB   │ GPU Registers      │
│ 0x90000000-0x9FFFFFFF    │ 256MB   │ eDRAM Space        │
│ 0xC0000000-0xCFFFFFFF    │ 256MB   │ PCI Configuration  │
│ 0xE0000000-0xEFFFFFFF    │ 256MB   │ Southbridge MMIO   │
│ 0xF0000000-0xFFFFFFFF    │ 256MB   │ Boot ROM/Security  │
└─────────────────────────────────────────────────────────┘
```

### Main Memory Regions (512MB GDDR3)
```
┌─────────────────────────────────────────────────────────┐
│ 0x00000000-0x03FFFFFF    │ 64MB    │ Kernel Code/Data   │
│ 0x04000000-0x07FFFFFF    │ 64MB    │ Kernel Heap/Stacks │
│ 0x08000000-0x0FFFFFFF    │ 128MB   │ User Space         │
│ 0x10000000-0x13FFFFFF    │ 64MB    │ GPU Command Buffer │
│ 0x14000000-0x17FFFFFF    │ 64MB    │ GPU Framebuffer    │
│ 0x18000000-0x1BFFFFFF    │ 64MB    │ GPU Textures       │
│ 0x1C000000-0x1FFFFFFF    │ 64MB    │ System Reserved    │
└─────────────────────────────────────────────────────────┘
```

### Memory Layout for Homebrew (XDK/XNA)
```
┌─────────────────────────────────────────────────────────┐
│ 0x00000000-0x01FFFFFF    │ 32MB    │ Hypervisor (HV)    │
│ 0x02000000-0x03FFFFFF    │ 32MB    │ Kernel (XOS)       │
│ 0x04000000-0x07FFFFFF    │ 64MB    │ System Reserved    │
│ 0x08000000-0x1FFFFFFF    │ 384MB   │ Title/User Memory  │
│ 0x20000000-0x3FFFFFFF    │ 512MB   │ (Reserved/Mirrors) │
│ 0x40000000-0x7FFFFFFF    │ 1GB     │ (Reserved)         │
│ 0x80000000-0xFFFFFFFF    │ 2GB     │ MMIO/Registers     │
└─────────────────────────────────────────────────────────┘
```

---

## 3. Virtual Memory Architecture

### Page Table Configuration
```
Page Size: 4KB, 64KB (default), or 16MB (large pages)
Address Space: 44-bit virtual (16TB theoretical)
              40-bit physical (1TB physical max)

Page Table Entry (PTE) Format:
┌─────────────────────────────────────────────────────────┐
│ Bit 0     │ Present (1=valid)                          │
│ Bit 1     │ Page Size (0=4KB, 1=64KB/16MB)             │
│ Bit 2-3   │ Protection Key                             │
│ Bit 4-7   │ Reserved                                   │
│ Bit 8-11  │ Protection Bits (read/write/execute)       │
│ Bit 12-39 │ Physical Page Number (PPN)                 │
│ Bit 40-51 │ Reserved                                   │
│ Bit 52-62 │ Software-defined                           │
│ Bit 63    │ No-Execute (NX) bit                        │
└─────────────────────────────────────────────────────────┘
```

### Memory Protection Keys
```
Key 0: User Read/Write (RW)
Key 1: User Read-Only (RO)
Key 2: User No-Access (NA)
Key 3: Supervisor Read/Write
Key 4-7: Reserved/Hypervisor
```

---

## 4. CPU Registers and SPRs

### General Purpose Registers (GPRs)
```
┌─────────────────────────────────────────────────────────┐
│ Register │ Name      │ Usage                             │
├─────────────────────────────────────────────────────────┤
│ r0       │ zero      │ Hardwired to 0                    │
│ r1       │ sp        │ Stack pointer                     │
│ r2       │ toc       │ Table of contents (Xenon)         │
│ r3       │ r3        │ Return value / Parameter 1        │
│ r4-r10   │ r4-r10    │ Parameters 2-8                    │
│ r11      │ r11       │ Environment pointer               │
│ r12      │ r12       │ Function entry address            │
│ r13      │ r13       │ Thread-local storage              │
│ r14-r31  │ r14-r31   │ Non-volatile (callee-saved)       │
└─────────────────────────────────────────────────────────┘
```

### Special Purpose Registers (SPRs)
```
┌─────────────────────────────────────────────────────────┐
│ SPR #   │ Name      │ Description                       │
├─────────────────────────────────────────────────────────┤
│ 1       │ XER       │ Fixed-Point Exception Register    │
│ 8       │ LR        │ Link Register                     │
│ 9       │ CTR       │ Count Register                    │
│ 18      │ DSISR     │ Data Storage Interrupt Status     │
│ 19      │ DAR       │ Data Address Register             │
│ 25      │ SDR1      │ Page Table Base                   │
│ 26      │ SRR0      │ Save/Restore Register 0           │
│ 27      │ SRR1      │ Save/Restore Register 1           │
│ 28      │ CSRR0     │ Critical Save/Restore Register 0  │
│ 29      │ CSRR1     │ Critical Save/Restore Register 1  │
│ 32-47   │ EVPR      │ Exception Vector Prefix           │
│ 48      │ PIR       │ Processor ID Register             │
│ 53      │ DEC       │ Decrementer                       │
│ 54      │ DECAR     │ Decrementer Auto-Reload           │
│ 61      │ EPR       │ External Proxy Register           │
│ 129     │ CTRL      │ Control Register                  │
│ 136     │ CTRLW     │ Control Register Write            │
│ 152     │ CTRLD     │ Control Register Data             │
│ 153     │ CTRLW     │ Control Register Write            │
│ 287     │ PPE_TLB0  │ PPE TLB Index 0                   │
│ 288     │ PPE_TLB1  │ PPE TLB Index 1                   │
│ 289     │ PPE_TLB2  │ PPE TLB Index 2                   │
│ 304     │ TLB0      │ TLB Entry 0                       │
│ 305     │ TLB1      │ TLB Entry 1                       │
│ 306     │ TLB2      │ TLB Entry 2                       │
│ 307     │ TLB3      │ TLB Entry 3                       │
│ 318     │ TLB_INDEX │ TLB Index                         │
│ 319     │ TLB_VPN   │ TLB Virtual Page Number           │
│ 336     │ EAR       │ External Access Register          │
│ 338     │ HASH1     │ Primary Hash Entry                │
│ 339     │ HASH2     │ Secondary Hash Entry              │
│ 340     │ IMC       │ Instruction Match Control         │
│ 341     │ IMA0      │ Instruction Match Address 0       │
│ 342     │ IMA1      │ Instruction Match Address 1       │
│ 343     │ IMA2      │ Instruction Match Address 2       │
│ 344     │ IMA3      │ Instruction Match Address 3       │
│ 345     │ DMC       │ Data Match Control                │
│ 346     │ DMA0      │ Data Match Address 0              │
│ 347     │ DMA1      │ Data Match Address 1              │
│ 348     │ DMA2      │ Data Match Address 2              │
│ 349     │ DMA3      │ Data Match Address 3              │
│ 350     │ MMCR0     │ Monitor Mode Control Register 0   │
│ 351     │ MMCR1     │ Monitor Mode Control Register 1   │
│ 352     │ MMCRC     │ Monitor Mode Control Register C   │
│ 353     │ MMCR2     │ Monitor Mode Control Register 2   │
│ 372-375 │ PMC1-4    │ Performance Monitor Counters      │
│ 376-379 │ PMC5-8    │ Performance Monitor Counters      │
│ 400     │ HID0      │ Hardware Implementation Dependent 0│
│ 401     │ HID1      │ Hardware Implementation Dependent 1│
│ 402     │ HID2      │ Hardware Implementation Dependent 2│
│ 403     │ HID3      │ Hardware Implementation Dependent 3│
│ 404     │ HID4      │ Hardware Implementation Dependent 4│
│ 405     │ HID5      │ Hardware Implementation Dependent 5│
│ 406     │ HID6      │ Hardware Implementation Dependent 6│
│ 464     │ DABR      │ Data Address Breakpoint Register  │
│ 465     │ DABRX     │ Data Address Breakpoint Register X│
│ 477     │ CIABR     │ Code Instruction Address Breakpoint│
│ 478     │ ICDBDR    │ Instruction Cache Debug Data Reg  │
│ 487     │ PVR       │ Processor Version Register        │
│ 496     │ LPIDR     │ Logical Partition ID              │
│ 497     │ PPR       │ Program Priority Register         │
│ 498     │ PPR32     │ Program Priority Register (32-bit)│
│ 503     │ DSCR      │ Data Stream Control Register      │
│ 509     │ TENSR     │ Thread Enable Status              │
│ 510     │ TENS      │ Thread Enable Set                 │
│ 511     │ TENC      │ Thread Enable Clear               │
└─────────────────────────────────────────────────────────┘
```

### Machine State Register (MSR) - SPR 287
```
Bit 0:     SF       - Sixty-Four bit mode (always 1 on Xenon)
Bit 1:     TA       - Trace Alarm (reserved)
Bit 2:     CM       - Computation Mode (0=32-bit, 1=64-bit)
Bit 3:     ICM      - Interrupt Computation Mode
Bit 4-6:   Reserved
Bit 7:     IS       │ Instruction Address Space (HV=0, other=1)
Bit 8:     DS       │ Data Address Space (HV=0, other=1)
Bit 9:     Reserved
Bit 10:    RI       │ Recoverable Interrupt
Bit 11:    Reserved
Bit 12:    DR       │ Data Relocate (enable MMU)
Bit 13:    IR       │ Instruction Relocate (enable MMU)
Bit 14:    IP       │ Interrupt Prefix
Bit 15:    Reserved
Bit 16:    EE       │ External Interrupt Enable
Bit 17:    PR       │ Problem State (0=privileged, 1=user)
Bit 18:    FP       │ Floating Point Available
Bit 19:    ME       │ Machine Check Enable
Bit 20:    FE0      │ Floating Point Exception 0
Bit 21:    SE       │ Single Step Trace Enable
Bit 22:    BE       │ Branch Trace Enable
Bit 23:    FE1      │ Floating Point Exception 1
Bit 24:    Reserved
Bit 25:    IT       │ Instruction Trace
Bit 26:    ID       │ Instruction Debug
Bit 27:    Reserved
Bit 28:    DWE      │ Debug Wait Enable
Bit 29:    DE       │ Debug Interrupt Enable
Bit 30:    CE       │ Critical Interrupt Enable
Bit 31:    Reserved
```

---

## 5. GPU/Xenos Registers

### GPU Register Base
```
GPU Register Space: 0x80000000-0x8FFFFFFF (256MB)
Actual Used: 0x80000000-0x8000FFFF (64KB primary registers)
```

### Primary GPU Registers
```
┌─────────────────────────────────────────────────────────┐
│ Offset     │ Name      │ Description                     │
├─────────────────────────────────────────────────────────┤
│ 0x0000     │ PA_CL_CLIP_CNTL      │ Clip Control           │
│ 0x0004     │ PA_SU_SC_MODE_CNTL   │ Setup Mode Control     │
│ 0x0008     │ PA_CL_VPORT_XSCALE   │ Viewport X Scale       │
│ 0x000C     │ PA_CL_VPORT_XOFFSET  │ Viewport X Offset      │
│ 0x0010     │ PA_CL_VPORT_YSCALE   │ Viewport Y Scale       │
│ 0x0014     │ PA_CL_VPORT_YOFFSET  │ Viewport Y Offset      │
│ 0x0018     │ PA_CL_VPORT_ZSCALE   │ Viewport Z Scale       │
│ 0x001C     │ PA_CL_VPORT_ZOFFSET  │ Viewport Z Offset      │
│ 0x0020     │ PA_CL_VTE_CNTL       │ Viewport Transform     │
│ 0x0024     │ PA_SU_VTX_CNTL       │ Vertex Control         │
│ 0x0028     │ PA_SU_POINT_SIZE     │ Point Size             │
│ 0x002C     │ PA_SU_POINT_MINMAX   │ Point Min/Max Size     │
│ 0x0030     │ PA_SU_LINE_CNTL      │ Line Control           │
│ 0x0034     │ PA_SC_LINE_CNTL      │ Scanline Line Control  │
│ 0x0040     │ PA_SU_POLY_OFFSET_   │ Polygon Offset         │
│ 0x0044     │ PA_SU_POLY_OFFSET_   │ Polygon Offset Scale   │
│ 0x0050     │ PA_SC_WINDOW_SCISSOR │ Window Scissor         │
│ 0x0054     │ PA_SC_WINDOW_OFFSET  │ Window Offset          │
│ 0x0058     │ PA_SC_WINDOW_EXTENT  │ Window Extent          │
│ 0x0060     │ PA_SC_SCREEN_SCISSOR │ Screen Scissor         │
│ 0x0080     │ VGT_MAX_VTX_INDX     │ Max Vertex Index       │
│ 0x0084     │ VGT_MIN_VTX_INDX     │ Min Vertex Index       │
│ 0x0088     │ VGT_INDX_OFFSET      │ Index Offset           │
│ 0x008C     │ VGT_MULTI_PRIM_IB_   │ Multi-Prim Reset       │
│ 0x0090     │ VGT_OUT_DEALLOC_     │ Output Dealloc         │
│ 0x0094     │ VGT_VTX_INDX         │ Vertex Index           │
│ 0x0100     │ VGT_DRAW_INITIATOR   │ Draw Initiator         │
│ 0x0104     │ VGT_IMMED_DATA       │ Immediate Data         │
│ 0x0108     │ VGT_EVENT_INITIATOR  │ Event Initiator        │
│ 0x010C     │ VGT_DMA_SIZE         │ DMA Size               │
│ 0x0110     │ VGT_DMA_BASE         │ DMA Base Address       │
│ 0x0114     │ VGT_DMA_BASE_HI      │ DMA Base (High)        │
│ 0x0118     │ VGT_BINNING_CONFIG   │ Binning Configuration  │
│ 0x0200     │ DB_RENDER_OVERRIDE   │ Render Override        │
│ 0x0204     │ DB_RENDER_OVERRIDE2  │ Render Override 2      │
│ 0x0208     │ DB_DEPTH_CLEAR       │ Depth Clear Value      │
│ 0x020C     │ DB_STENCIL_CLEAR     │ Stencil Clear Value    │
│ 0x0210     │ DB_DEPTH_CONTROL     │ Depth Control          │
│ 0x0214     │ DB_STENCIL_CONTROL   │ Stencil Control        │
│ 0x0218     │ DB_STENCILREFMASK    │ Stencil Ref/Mask       │
│ 0x021C     │ DB_STENCILREFMASK_BF │ Stencil Ref/Mask BF    │
│ 0x0220     │ DB_DEPTH_BOUNDS_     │ Depth Bounds           │
│ 0x0224     │ DB_DEPTH_VIEW        │ Depth View             │
│ 0x0228     │ DB_Z_INFO            │ Z Buffer Info          │
│ 0x022C     │ DB_STENCIL_INFO      │ Stencil Buffer Info    │
│ 0x0230     │ DB_Z_READ_BASE       │ Z Read Base            │
│ 0x0234     │ DB_STENCIL_READ_     │ Stencil Read Base      │
│ 0x0238     │ DB_Z_WRITE_BASE      │ Z Write Base           │
│ 0x023C     │ DB_STENCIL_WRITE_    │ Stencil Write Base     │
│ 0x0240     │ DB_DEPTH_SIZE        │ Depth Buffer Size      │
│ 0x0244     │ DB_DEPTH_SLICE       │ Depth Slice            │
│ 0x0280     │ CB_TARGET_MASK       │ Color Buffer Target    │
│ 0x0284     │ CB_SHADER_MASK       │ Color Buffer Shader    │
│ 0x0288     │ CB_COLOR_CONTROL     │ Color Control          │
│ 0x028C     │ CB_COLOR0_BASE       │ Color Buffer 0 Base    │
│ 0x0290     │ CB_COLOR0_PITCH      │ Color Buffer 0 Pitch   │
│ 0x0294     │ CB_COLOR0_SLICE      │ Color Buffer 0 Slice   │
│ 0x0298     │ CB_COLOR0_VIEW       │ Color Buffer 0 View    │
│ 0x029C     │ CB_COLOR0_INFO       │ Color Buffer 0 Info    │
│ 0x02A0     │ CB_COLOR0_ATTRIB     │ Color Buffer 0 Attr    │
│ 0x02A4     │ CB_COLOR0_CMASK      │ Color Buffer 0 Cmask   │
│ 0x02A8     │ CB_COLOR0_CMASK_     │ Color Buffer 0 Slice   │
│ 0x02AC     │ CB_COLOR0_FMASK      │ Color Buffer 0 Fmask   │
│ 0x02B0     │ CB_COLOR0_FMASK_     │ Color Buffer 0 Slice   │
│ 0x02C0-02F0│ CB_COLOR1-3_*        │ Color Buffers 1-3      │
│ 0x0300     │ SQ_CONFIG            │ Sequencer Config       │
│ 0x0304     │ SQ_GPR_RESOURCE_     │ GPR Resources          │
│ 0x0308     │ SQ_THREAD_           │ Thread Resources       │
│ 0x030C     │ SQ_STACK_            │ Stack Resources        │
│ 0x0310     │ SQ_ESGS_RING_        │ ESGS Ring Size         │
│ 0x0314     │ SQ_GSVS_RING_        │ GSVS Ring Size         │
│ 0x0320     │ SQ_INST_PREFETCH_    │ Instruction Prefetch   │
│ 0x0324     │ SQ_ALU_CONST_        │ ALU Constants          │
│ 0x0328     │ SQ_ALU_CONST_        │ ALU Constants Buffer   │
│ 0x0330     │ SQ_VTX_START_        │ Vertex Start Loc       │
│ 0x0334     │ SQ_VTX_CONSTANT      │ Vertex Constant        │
│ 0x0340     │ SQ_PS_PUSH_          │ PS Push Constants      │
│ 0x0344     │ SQ_VS_PUSH_          │ VS Push Constants      │
│ 0x0380     │ SX_ALPHA_TEST_       │ Alpha Test Control     │
│ 0x0384     │ SX_ALPHA_REF         │ Alpha Reference        │
│ 0x0388     │ SX_ALPHA_TEST_       │ Alpha Test Enable      │
│ 0x0400     │ SPI_CONFIG_          │ Shader Pipe Config     │
│ 0x0404     │ SPI_GPR_             │ GPR Management         │
│ 0x0408     │ SPI_THREAD_          │ Thread Management      │
│ 0x040C     │ SPI_INPUT_           │ Input Configuration    │
│ 0x0410     │ SPI_PS_IN_           │ PS Input Config        │
│ 0x0414     │ SPI_VS_OUT_          │ VS Output Config       │
│ 0x0418     │ SPI_RESOURCE_        │ Resource Management    │
│ 0x0500     │ TA_CNTL              │ Texture Array Control  │
│ 0x0504     │ TA_CNTL_AUX          │ TA Control Aux         │
│ 0x0508     │ TA_SAMPLER_          │ Sampler Config         │
│ 0x050C     │ TA_SAMPLER_          │ Sampler Border Color   │
│ 0x0600     │ TC_CNTL              │ Texture Cache Control  │
│ 0x0604     │ TC_CONFIG            │ Texture Cache Config   │
│ 0x0610     │ TF_CNTL              │ Texture Filter Control │
│ 0x0614     │ TF_ADDR_MODE         │ Texture Address Mode   │
│ 0x0700     │ RB_TILEMODE          │ Render Tile Mode       │
│ 0x0704     │ RB_MODECONTROL       │ Render Mode Control    │
│ 0x0708     │ RB_COLORCONTROL      │ Color Control          │
│ 0x070C     │ RB_BLENDCONTROL      │ Blend Control          │
│ 0x0710     │ RB_BLENDCOLOR        │ Blend Constant Color   │
│ 0x0800     │ CP_RB_BASE           │ Ring Buffer Base       │
│ 0x0804     │ CP_RB_CNTL           │ Ring Buffer Control    │
│ 0x0808     │ CP_RB_RPTR           │ Ring Buffer Read Ptr   │
│ 0x080C     │ CP_RB_RPTR           │ Ring Buffer Rptr Wr    │
│ 0x0810     │ CP_RB_WPTR           │ Ring Buffer Write Ptr  │
│ 0x0814     │ CP_RB_WPTR_          │ RB WPTR Delay          │
│ 0x0818     │ CP_RB_RPTR_          │ RB RPTR Delay          │
│ 0x081C     │ CP_RB_OFFSET         │ RB Offset              │
│ 0x0820     │ CP_IB_BASE           │ Indirect Buffer Base   │
│ 0x0824     │ CP_IB_BUFSZ          │ Indirect Buffer Size   │
│ 0x0828     │ CP_CNTL              │ Command Processor Cntl │
│ 0x082C     │ CP_STATUS            │ Command Processor Stat │
│ 0x0830     │ CP_ME_CNTL           │ Micro-Engine Control   │
│ 0x0834     │ CP_ME_RAM_           │ ME RAM Address         │
│ 0x0838     │ CP_ME_RAM_           │ ME RAM Data            │
│ 0x083C     │ CP_DEBUG             │ CP Debug               │
│ 0x0840     │ CP_CSQ_CNTL          │ Command Sequencer Cntl │
│ 0x0844     │ CP_CSQ_STAT          │ Command Sequencer Stat │
│ 0x0848     │ CP_CSQ_MODE          │ Command Sequencer Mode │
│ 0x084C     │ CP_DRAW_             │ Draw Control           │
│ 0x0850     │ CP_DRAW_             │ Draw Start             │
│ 0x0900     │ GRBM_CNTL            │ Graphics Master Cntl   │
│ 0x0904     │ GRBM_STATUS          │ Graphics Master Status │
│ 0x0908     │ GRBM_STATUS2         │ Graphics Status 2      │
│ 0x090C     │ GRBM_SOFT_RESET      │ Graphics Soft Reset    │
│ 0x0A00     │ SRBM_CNTL            │ System Master Cntl     │
│ 0x0A04     │ SRBM_STATUS          │ System Master Status   │
│ 0x0A08     │ SRBM_STATUS2         │ System Status 2        │
│ 0x0A0C     │ SRBM_SOFT_RESET      │ System Soft Reset      │
│ 0x0A10     │ SRBM_GFX_CNTL        │ System Graphics Cntl   │
│ 0x1000     │ GRPH_PRIMARY_        │ Primary Surface        │
│ 0x1004     │ GRPH_SECONDARY_      │ Secondary Surface      │
│ 0x1008     │ GRPH_PITCH           │ Surface Pitch          │
│ 0x100C     │ GRPH_X               │ X Position             │
│ 0x1010     │ GRPH_Y               │ Y Position             │
│ 0x1014     │ GRPH_H               │ Height                 │
│ 0x1018     │ GRPH_W               │ Width                  │
│ 0x1020     │ GRPH_MODE            │ Graphics Mode          │
│ 0x1024     │ GRPH_COLOR_          │ Color Space            │
│ 0x1100     │ OVRSYNC              │ Overlay Sync           │
│ 0x1104     │ OVRSURFACE_          │ Overlay Surface        │
│ 0x1108     │ OVRPITCH             │ Overlay Pitch          │
│ 0x110C     │ OVRX                 │ Overlay X              │
│ 0x1110     │ OVRY                 │ Overlay Y              │
│ 0x1114     │ OVRH                 │ Overlay Height         │
│ 0x1118     │ OVRW                 │ Overlay Width          │
│ 0x1120     │ OVR_COLOR_           │ Overlay Color Space    │
│ 0x2000     │ D1CRTC_H_            │ Display 1 CRTC H       │
│ 0x2004     │ D1CRTC_H_            │ Display 1 CRTC H Sync  │
│ 0x2008     │ D1CRTC_V_            │ Display 1 CRTC V       │
│ 0x200C     │ D1CRTC_V_            │ Display 1 CRTC V Sync  │
│ 0x2010     │ D1CRTC_V_            │ Display 1 CRTC V Total │
│ 0x2014     │ D1CRTC_CNTL          │ Display 1 CRTC Control │
│ 0x2018     │ D1CRTC_STATUS        │ Display 1 CRTC Status  │
│ 0x3000     │ D2CRTC_H_            │ Display 2 CRTC H       │
│ 0x3004     │ D2CRTC_H_            │ Display 2 CRTC H Sync  │
│ 0x3008     │ D2CRTC_V_            │ Display 2 CRTC V       │
│ 0x300C     │ D2CRTC_V_            │ Display 2 CRTC V Sync  │
│ 0x3010     │ D2CRTC_V_            │ Display 2 CRTC V Total │
│ 0x3014     │ D2CRTC_CNTL          │ Display 2 CRTC Control │
│ 0x3018     │ D2CRTC_STATUS        │ Display 2 CRTC Status  │
│ 0x4000     │ DC_I2C_              │ I2C Control            │
│ 0x4004     │ DC_I2C_              │ I2C Status             │
│ 0x4008     │ DC_I2C_              │ I2C Speed              │
│ 0x4010     │ DC_GPIO_             │ GPIO Control           │
│ 0x4014     │ DC_GPIO_             │ GPIO Mask              │
│ 0x4018     │ DC_GPIO_             │ GPIO Enable            │
│ 0x4020     │ DC_GENERIC_          │ Generic Control        │
│ 0x4030     │ DC_LUT_RW_           │ LUT Read/Write Index   │
│ 0x4034     │ DC_LUT_              │ LUT Data               │
│ 0x4038     │ DC_LUT_              │ LUT Control            │
│ 0x4040     │ DC_DISP_DEBUG        │ Display Debug          │
└─────────────────────────────────────────────────────────┘
```

### GPU Register Bit Fields

#### PA_SU_SC_MODE_CNTL (0x0004)
```
Bit 0:     CULL_FRONT           │ Front-face culling enable
Bit 1:     CULL_BACK            │ Back-face culling enable
Bit 2-3:   FACE                 │ Face type (0=cw, 1=ccw)
Bit 4:     POLY_MODE            │ Polygon mode enable
Bit 5-6:   POLYMODE_FRONT       │ Front polygon mode
Bit 7-8:   POLYMODE_BACK        │ Back polygon mode
Bit 9:     PROVOKING_VTX_LAST   │ Provoking vertex select
Bit 10-15: Reserved
Bit 16-19: MULTI_PRIM_IB        │ Multi-primitive reset
Bit 20-31: Reserved
```

#### DB_DEPTH_CONTROL (0x0210)
```
Bit 0:     DEPTH_ENABLE         │ Depth test enable
Bit 1:     DEPTH_WRITE_ENABLE   │ Depth write enable
Bit 2-4:   DEPTH_FUNC           │ Depth comparison func
           │   0=NEVER, 1=LESS, 2=EQUAL, 3=LEQUAL
           │   4=GREATER, 5=NOTEQUAL, 6=GEQUAL, 7=ALWAYS
Bit 5:     STENCIL_ENABLE       │ Stencil test enable
Bit 6-7:   Reserved
Bit 8-10:  STENCIL_FUNC         │ Stencil comparison
Bit 11-13: STENCIL_FAIL         │ Stencil fail op
Bit 14-16: STENCIL_ZPASS        │ Stencil z-pass op
Bit 17-19: STENCIL_ZFAIL        │ Stencil z-fail op
Bit 20-31: Reserved
```

#### CB_COLOR_CONTROL (0x0288)
```
Bit 0-5:   ROGUE_ENABLE         │ Rogue enable bits
Bit 6:     DEGAMMA_ENABLE       │ De-gamma enable
Bit 7:     SPECIAL_OP           │ Special operation
Bit 8-15:  Reserved
Bit 16-23: FOG_ENABLE           │ Fog enable bits
Bit 24:    FOG_MODE             │ Fog mode (0=exp, 1=linear)
Bit 25-31: Reserved
```

---

## 6. Memory Controller Registers

### Memory Controller Base
```
Base Address: 0xE0000000-0xE0FFFFFF (Southbridge Memory Controller)
```

### Memory Configuration Registers
```
┌─────────────────────────────────────────────────────────┐
│ Offset     │ Name      │ Description                     │
├─────────────────────────────────────────────────────────┤
│ 0x0000     │ MC_CONFIG │ Memory Controller Config        │
│ 0x0004     │ MC_STATUS │ Memory Controller Status        │
│ 0x0008     │ MC_TIMING │ Memory Timing Parameters        │
│ 0x000C     │ MC_REFRESH│ Refresh Control                 │
│ 0x0010     │ MC_ARB    │ Arbitration Control             │
│ 0x0014     │ MC_EMEM_SIZE│ Memory Size                   │
│ 0x0018     │ MC_EMEM_CFG│ Memory Configuration           │
│ 0x0020     │ MC_REQUEST│ Memory Request Control          │
│ 0x0024     │ MC_LATENCY│ Latency Control                 │
│ 0x0030     │ MC_RAS_CAS│ RAS/CAS Timing                  │
│ 0x0034     │ MC_PRECHRG│ Precharge Timing                │
│ 0x0038     │ MC_ACTIVE │ Active Timing                   │
│ 0x0040     │ MC_PWR_CTRL│ Power Control                  │
│ 0x0044     │ MC_PWR_STATUS│ Power Status                 │
│ 0x0050     │ MC_TEMP   │ Temperature Status              │
│ 0x0100     │ MC_ERROR  │ Error Status                    │
│ 0x0104     │ MC_ERR_MASK│ Error Mask                     │
│ 0x0200     │ MC_FB_BASE│ Framebuffer Base                │
│ 0x0204     │ MC_FB_SIZE│ Framebuffer Size                │
│ 0x0208     │ MC_Z_BASE │ Z Buffer Base                   │
│ 0x020C     │ MC_Z_SIZE │ Z Buffer Size                   │
│ 0x0300     │ MC_PERF_CNT0│ Performance Counter 0         │
│ 0x0304     │ MC_PERF_CNT1│ Performance Counter 1         │
│ 0x0308     │ MC_PERF_CTRL│ Performance Control           │
└─────────────────────────────────────────────────────────┘
```

### MC_CONFIG (0x0000)
```
Bit 0:     MEM_INIT_DONE        │ Memory initialization complete
Bit 1:     MEM_TYPE             │ Memory type (0=DDR2, 1=GDDR3)
Bit 2-3:   BUS_WIDTH            │ Bus width (0=32, 1=64, 2=128)
Bit 4-7:   NUM_CHIPS            │ Number of memory chips
Bit 8-15:  CHIP_DENSITY         │ Chip density config
Bit 16-23: ROW_BITS             │ Row address bits
Bit 24-27: COL_BITS             │ Column address bits
Bit 28-31: BANK_BITS            │ Bank address bits
```

---

## 7. PCI Express Configuration

### PCI Configuration Space
```
Base Address: 0xC0000000-0xCFFFFFFF (256MB PCI config space)
Configuration Type 1: Direct access to PCI config space
```

### PCI Configuration Registers
```
┌─────────────────────────────────────────────────────────┐
│ Offset     │ Name      │ Description                     │
├─────────────────────────────────────────────────────────┤
│ 0x0000     │ PCI_VENDOR│ Vendor ID (0x10DE for nVidia)   │
│ 0x0002     │ PCI_DEVICE│ Device ID (0x0201 for GPU)      │
│ 0x0004     │ PCI_CMD   │ Command Register                │
│ 0x0006     │ PCI_STATUS│ Status Register                 │
│ 0x0008     │ PCI_REV   │ Revision ID                     │
│ 0x0009     │ PCI_CLASS │ Class Code (0x03=Display)       │
│ 0x000C     │ PCI_CACHE │ Cache Line Size                 │
│ 0x000D     │ PCI_LATENCY│ Latency Timer                  │
│ 0x000E     │ PCI_HDR   │ Header Type (0x00=Standard)     │
│ 0x000F     │ PCI_BIST  │ BIST                            │
│ 0x0010     │ PCI_BAR0  │ Base Address Register 0         │
│ 0x0014     │ PCI_BAR1  │ Base Address Register 1         │
│ 0x0018     │ PCI_BAR2  │ Base Address Register 2         │
│ 0x001C     │ PCI_BAR3  │ Base Address Register 3         │
│ 0x0020     │ PCI_BAR4  │ Base Address Register 4         │
│ 0x0024     │ PCI_BAR5  │ Base Address Register 5         │
│ 0x002C     │ PCI_SUBVID│ Subsystem Vendor ID             │
│ 0x002E     │ PCI_SUBSYS│ Subsystem ID                    │
│ 0x0030     │ PCI_ROM   │ Expansion ROM Base              │
│ 0x0034     │ PCI_CAP   │ Capabilities Pointer            │
│ 0x003C     │ PCI_IRQ   │ Interrupt Line                  │
│ 0x003D     │ PCI_PIN   │ Interrupt Pin                   │
│ 0x003E     │ PCI_MIN_GNT│ Minimum Grant                  │
│ 0x003F     │ PCI_MAX_LAT│ Maximum Latency                │
└─────────────────────────────────────────────────────────┘
```

### PCI Command Register (0x0004)
```
Bit 0:     IO_SPACE_ENABLE      │ I/O space access enable
Bit 1:     MEM_SPACE_ENABLE     │ Memory space access enable
Bit 2:     BUS_MASTER_ENABLE    │ Bus mastering enable
Bit 3:     SPECIAL_CYCLES       │ Special cycles enable
Bit 4:     MEM_WRITE_INVALIDATE │ Memory write & invalidate
Bit 5:     VGA_PALETTE_SNOOP    │ VGA palette snoop
Bit 6:     PARITY_ERROR_RESP    │ Parity error response
Bit 7:     Reserved
Bit 8:     SERR_ENABLE          │ SERR# driver enable
Bit 9:     FAST_BACK_ENABLE     │ Fast back-to-back enable
Bit 10-15: Reserved
```

---

## 8. SMC and Hardware Monitor

### System Management Controller (SMC)
```
Base Address: 0xE1000000 (Southbridge SMC registers)
SMC is responsible for power management, temperature monitoring, and fan control.
```

### SMC Registers
```
┌─────────────────────────────────────────────────────────┐
│ Offset     │ Name      │ Description                     │
├─────────────────────────────────────────────────────────┤
│ 0x0000     │ SMC_VERSION│ SMC Firmware Version           │
│ 0x0004     │ SMC_STATUS│ SMC Status                      │
│ 0x0008     │ SMC_CTRL  │ SMC Control                     │
│ 0x0010     │ TEMP_CPU  │ CPU Temperature (0.001°C units) │
│ 0x0014     │ TEMP_GPU  │ GPU Temperature                 │
│ 0x0018     │ TEMP_MEM  │ Memory Temperature              │
│ 0x001C     │ TEMP_CASE │ Case/Ambient Temperature        │
│ 0x0020     │ FAN_CPU   │ CPU Fan Speed (RPM)             │
│ 0x0024     │ FAN_GPU   │ GPU Fan Speed (RPM)             │
│ 0x0028     │ FAN_CASE  │ Case Fan Speed (RPM)            │
│ 0x0030     │ VOLT_VCORE│ CPU Core Voltage                │
│ 0x0034     │ VOLT_3.3  │ 3.3V Rail                       │
│ 0x0038     │ VOLT_5.0  │ 5.0V Rail                       │
│ 0x003C     │ VOLT_12.0 │ 12.0V Rail                      │
│ 0x0040     │ PWR_TOTAL │ Total Power Consumption         │
│ 0x0050     │ SMC_MSG   │ SMC Message Register            │
│ 0x0054     │ SMC_RESP  │ SMC Response Register           │
│ 0x0060     │ ANA_XTAL  │ Crystal Oscillator Control      │
│ 0x0064     │ ANA_PLL   │ PLL Configuration               │
│ 0x0070     │ GPIO_DIR  │ GPIO Direction                  │
│ 0x0074     │ GPIO_DATA │ GPIO Data                       │
│ 0x0078     │ GPIO_INT  │ GPIO Interrupt                  │
│ 0x0100     │ DVD_TRAY  │ DVD Tray Status                 │
│ 0x0104     │ DVD_STATUS│ DVD Drive Status                │
│ 0x0110     │ HDD_STATUS│ Hard Drive Status               │
│ 0x0120     │ USB_STATUS│ USB Status                      │
│ 0x0130     │ ETH_STATUS│ Ethernet Status                 │
│ 0x0200     │ POWER_BTN │ Power Button Status             │
│ 0x0204     │ EJECT_BTN │ Eject Button Status             │
│ 0x0208     │ RING_BTN  │ Ring of Light Button            │
│ 0x0300     │ LED_PWR   │ Power LED Control               │
│ 0x0304     │ LED_RING  │ Ring of Light Control           │
│ 0x0308     │ LED_FLASH │ LED Flash Pattern               │
│ 0x0400     │ SMC_RESET │ SMC Reset Control               │
│ 0x0404     │ SMC_CLOCK │ SMC Clock Control               │
│ 0x0410     │ SMC_INT_MASK│ Interrupt Mask                │
│ 0x0414     │ SMC_INT_STAT│ Interrupt Status              │
└─────────────────────────────────────────────────────────┘
```

### SMC_LED_RING (0x0304) - Ring of Light
```
Bit 0-1:   QUADRANT1            │ Quadrant 1 color
Bit 2-3:   QUADRANT2            │ Quadrant 2 color
Bit 4-5:   QUADRANT3            │ Quadrant 3 color
Bit 6-7:   QUADRANT4            │ Quadrant 4 color
Bit 8-11:  FLASH_SPEED          │ Flashing speed
Bit 12-15: Reserved

Colors: 0=Off, 1=Green, 2=Red, 3=Orange
```

---

## 9. Hypervisor Memory Regions

### Hypervisor (HV) Memory Layout
```
The Xbox 360 Hypervisor occupies the lowest 32MB of physical memory (0x00000000-0x02000000).
This region is protected and inaccessible to the kernel and user applications.
```

### HV Memory Map
```
┌─────────────────────────────────────────────────────────┐
│ 0x00000000-0x000FFFFF    │ 1MB     │ HV Code (locked)   │
│ 0x00100000-0x001FFFFF    │ 1MB     │ HV Data (locked)   │
│ 0x00200000-0x003FFFFF    │ 2MB     │ Security Engine    │
│ 0x00400000-0x007FFFFF    │ 4MB     │ Security Policies  │
│ 0x00800000-0x00FFFFFF    │ 8MB     │ Encryption Keys    │
│ 0x01000000-0x01FFFFFF    │ 16MB    │ Trusted Storage    │
│ 0x02000000-0x03FFFFFF    │ 32MB    │ Kernel Space       │
└─────────────────────────────────────────────────────────┘
```

### HV SPRs and Privileged Registers
```
┌─────────────────────────────────────────────────────────┐
│ SPR/Addr   │ Name      │ Description                   │
├─────────────────────────────────────────────────────────┤
│ SPR 508    │ LPCR      │ Logical Partition Control     │
│ SPR 509    │ LPID      │ Logical Partition ID          │
│ SPR 512    │ PSSCR     │ Processor Stop Status         │
│ SPR 513    │ LPCR      │ LPAR Configuration            │
│ SPR 514    │ PPR       │ Program Priority Register     │
│ SPR 515    │ PPR32     │ PPR (32-bit version)          │
│ HV 0x8000  │ HV_MAGIC  │ HV Magic Number (0x4856)      │
│ HV 0x8004  │ HV_VERSION│ HV Version                    │
│ HV 0x8008  │ HV_FLAGS  │ HV Feature Flags              │
│ HV 0x8010  │ HV_KEYS   │ Encryption Key Status         │
│ HV 0x8020  │ HV_POLICIES│ Security Policies            │
│ HV 0x8100  │ HV_SYSCALL│ Syscall Handler Table         │
│ HV 0x8200  │ HV_EXCEPT │ Exception Handler Table       │
│ HV 0x8300  │ HV_INT    │ Interrupt Handler Table       │
│ HV 0x9000  │ HV_ENCRYPTION│ Encryption Engine Control │
│ HV 0x9004  │ HV_DECRYPTION│ Decryption Engine Control │
│ HV 0x9100  │ HV_HASH   │ SHA-1/MD5 Hash Engine         │
│ HV 0x9200  │ HV_RSA    │ RSA Crypto Engine             │
│ HV 0x9300  │ HV_AES    │ AES Encryption Engine         │
└─────────────────────────────────────────────────────────┘
```

### HV Encryption Engine (0x9000)
```
Bit 0:     ENGINE_ENABLE        │ Enable encryption engine
Bit 1:     DECRYPT_MODE         │ 0=encrypt, 1=decrypt
Bit 2-3:   ALGORITHM            │ 0=AES, 1=RSA, 2=SHA
Bit 4:     KEY_SELECT           │ Select key slot (0-1)
Bit 5-7:   KEY_SIZE             │ 0=128bit, 1=192bit, 2=256bit
Bit 8-15:  Reserved
Bit 16:    DMA_ENABLE           │ Enable DMA transfer
Bit 17:    IRQ_ENABLE           │ Enable completion IRQ
Bit 18-31: Reserved
```

---

## 10. Kernel Memory Layout

### XOS (Xbox Operating System) Memory
```
The Xbox 360 Kernel (XOS) occupies 32MB starting at 0x02000000.
```

### Kernel Memory Map
```
┌─────────────────────────────────────────────────────────┐
│ 0x02000000-0x023FFFFF    │ 4MB     │ Kernel Code        │
│ 0x02400000-0x027FFFFF    │ 4MB     │ Kernel Data        │
│ 0x02800000-0x02BFFFFF    │ 4MB     │ Kernel Heap        │
│ 0x02C00000-0x02FFFFFF    │ 4MB     │ Kernel Stacks      │
│ 0x03000000-0x033FFFFF    │ 4MB     │ Object Manager     │
│ 0x03400000-0x037FFFFF    │ 4MB     │ I/O Manager        │
│ 0x03800000-0x03BFFFFF    │ 4MB     │ Memory Manager     │
│ 0x03C00000-0x03FFFFFF    │ 4MB     │ Reserved           │
└─────────────────────────────────────────────────────────┘
```

### Kernel Data Structures
```
┌─────────────────────────────────────────────────────────┐
│ Structure        │ Address    │ Description             │
├─────────────────────────────────────────────────────────┤
│ KPCR             │ 0x02000000 │ Kernel Processor Control│
│ KPRCB            │ 0x02000400 │ Kernel Processor Block  │
│ KTHREAD          │ 0x02000800 │ Current Thread          │
│ KPROCESS         │ 0x02000C00 │ Current Process         │
│ KeServiceTable   │ 0x02001000 │ System Call Table       │
│ KiExceptionTable │ 0x02002000 │ Exception Dispatch      │
│ ObpRootDirectory │ 0x02010000 │ Object Root Directory   │
│ MmPhysicalMap    │ 0x02020000 │ Physical Memory Map     │
│ MmVirtualMap     │ 0x02030000 │ Virtual Memory Map      │
│ IoDeviceTable    │ 0x02040000 │ Device Object Table     │
│ KeTimerTable     │ 0x02050000 │ Timer Object Table      │
│ PsThreadTable    │ 0x02060000 │ Thread Table            │
│ PsProcessTable   │ 0x02070000 │ Process Table           │
│ ExPoolDescriptor │ 0x02080000 │ Pool Memory Descriptor  │
│ HalInterruptTable│ 0x02090000 │ HAL Interrupt Table     │
│ NlsTableBase     │ 0x020A0000 │ National Language Table │
│ SystemTime       │ 0x020B0000 │ System Time             │
│ TimeZoneBias     │ 0x020B0010 │ Time Zone Bias          │
└─────────────────────────────────────────────────────────┘
```

---

## 11. eDRAM Registers

### eDRAM (Embedded DRAM) Overview
```
The Xenos GPU contains 10MB of embedded DRAM for render targets, Z-buffer, and AA resolve.
This provides 256GB/s internal bandwidth independent of system memory.
```

### eDRAM Base Addresses
```
eDRAM MMIO Base: 0x90000000-0x9000FFFF (64KB register space)
eDRAM Physical:  10MB embedded in GPU die
```

### eDRAM Registers
```
┌─────────────────────────────────────────────────────────┐
│ Offset     │ Name      │ Description                     │
├─────────────────────────────────────────────────────────┤
│ 0x0000     │ EDRAM_CONFIG│ eDRAM Configuration           │
│ 0x0004     │ EDRAM_STATUS│ eDRAM Status                  │
│ 0x0008     │ EDRAM_SIZE  │ eDRAM Size (10MB)             │
│ 0x0010     │ EDRAM_TILE  │ Tile Configuration            │
│ 0x0014     │ EDRAM_PITCH │ Pitch Configuration           │
│ 0x0020     │ EDRAM_COLOR │ Color Base Address            │
│ 0x0024     │ EDRAM_DEPTH │ Depth Base Address            │
│ 0x0030     │ EDRAM_LOAD  │ Load Control                  │
│ 0x0034     │ EDRAM_STORE │ Store Control                 │
│ 0x0040     │ EDRAM_CLEAR │ Clear Value                   │
│ 0x0050     │ EDRAM_LOAD_ADDR│ Load Source Address        │
│ 0x0054     │ EDRAM_STORE_ADDR│ Store Destination Address │
│ 0x0100     │ EDRAM_PERF0 │ Performance Counter 0         │
│ 0x0104     │ EDRAM_PERF1 │ Performance Counter 1         │
│ 0x0108     │ EDRAM_PERF_CTRL│ Performance Control        │
└─────────────────────────────────────────────────────────┘
```

### EDRAM_CONFIG (0x0000)
```
Bit 0:     ENABLE               │ eDRAM enable
Bit 1-3:   TILE_SIZE            │ Tile size (0=32x32, 1=64x64)
Bit 4-7:   COMPRESSION          │ Compression mode
Bit 8:     AA_ENABLE            │ Anti-aliasing enable
Bit 9-11:  AA_SAMPLES           │ AA sample count (0=2x, 1=4x)
Bit 12:    DOUBLE_BUFFER        │ Double buffering
Bit 13-15: Reserved
Bit 16-19: COLOR_FORMAT         │ Color buffer format
Bit 20-23: DEPTH_FORMAT         │ Depth buffer format
Bit 24:    LOAD_ON_DRAW         │ Auto-load on draw
Bit 25:    STORE_ON_END         │ Auto-store on end
Bit 26-31: Reserved
```

### EDRAM_TILE (0x0010)
```
Bit 0-3:   TILE_X               │ Tiles in X dimension
Bit 4-7:   TILE_Y               │ Tiles in Y dimension
Bit 8-15:  TILE_SIZE_X          │ Tile width in pixels
Bit 16-23: TILE_SIZE_Y          │ Tile height in pixels
Bit 24-31: TILE_STRIDE          │ Tile stride in bytes
```

### eDRAM Memory Organization
```
Total: 10MB organized as tiles

┌─────────────────────────────────────────────────────────┐
│ 0x00000-0x7FFFF   │ 512KB │ Tile 0-15 (Color Buffers)  │
│ 0x80000-0x9FFFF   │ 128KB │ Tile 16-19 (Z-Buffer)      │
│ 0xA0000-0xBFFFF   │ 128KB │ Tile 20-23 (Stencil)       │
│ 0xC0000-0xFFFFF   │ 256KB │ Tile 24-31 (AA Buffers)    │
│ 0x100000-0x17FFFF │ 512KB │ Tile 32-47 (Overflow)      │
│ 0x180000-0x1FFFFF │ 512KB │ Tile 48-63 (Reserved)      │
│ ...               │ ...   │ Additional tiles           │
│ 0x980000-0x9FFFFF │ 512KB │ Tile 304-319 (Reserved)    │
└─────────────────────────────────────────────────────────┘
```

---

## 12. Quick Reference Tables

### Address Space Summary
```
┌─────────────────────────────────────────────────────────┐
│ Region          │ Start      │ End        │ Size        │
├─────────────────────────────────────────────────────────┤
│ User Space      │ 0x00000000 │ 0x1FFFFFFF │ 512MB       │
│ System RAM      │ 0x00000000 │ 0x1FFFFFFF │ 512MB       │
│ Kernel Space    │ 0x00000000 │ 0x03FFFFFF │ 64MB        │
│ Title Memory    │ 0x08000000 │ 0x1FFFFFFF │ 384MB       │
│ GPU Registers   │ 0x80000000 │ 0x8000FFFF │ 64KB        │
│ eDRAM Space     │ 0x90000000 │ 0x9000FFFF │ 64KB        │
│ PCI Config      │ 0xC0000000 │ 0xCFFFFFFF │ 256MB       │
│ Southbridge MMIO│ 0xE0000000 │ 0xEFFFFFFF │ 256MB       │
│ Boot ROM        │ 0xF0000000 │ 0xFFFFFFFF │ 256MB       │
└─────────────────────────────────────────────────────────┘
```

### GPU Register Categories
```
┌─────────────────────────────────────────────────────────┐
│ Category    │ Base       │ Size   │ Purpose             │
├─────────────────────────────────────────────────────────┤
│ PA_*        │ 0x80000000 │ 128B   │ Primitive Assembly  │
│ VGT_*       │ 0x80000080 │ 128B   │ Vertex & Index Gen  │
│ DB_*        │ 0x80000200 │ 128B   │ Depth Buffer        │
│ CB_*        │ 0x80000280 │ 256B   │ Color Buffer        │
│ SQ_*        │ 0x80000300 │ 256B   │ Shader Sequencer    │
│ SX_*        │ 0x80000380 │ 128B   │ Scissor & Export    │
│ SPI_*       │ 0x80000400 │ 256B   │ Shader Pipe Input   │
│ TA_*        │ 0x80000500 │ 128B   │ Texture Array       │
│ TC_*        │ 0x80000600 │ 128B   │ Texture Cache       │
│ TF_*        │ 0x80000610 │ 64B    │ Texture Filter      │
│ RB_*        │ 0x80000700 │ 256B   │ Render Backend      │
│ CP_*        │ 0x80000800 │ 256B   │ Command Processor   │
│ GRBM_*      │ 0x80000900 │ 64B    │ Graphics Master     │
│ SRBM_*      │ 0x80000A00 │ 64B    │ System Master       │
│ GRPH_*      │ 0x80001000 │ 128B   │ Graphics Surfaces   │
│ OVR_*       │ 0x80001100 │ 64B    │ Overlay             │
│ D1CRTC_*    │ 0x80002000 │ 64B    │ Display 1 CRTC      │
│ D2CRTC_*    │ 0x80003000 │ 64B    │ Display 2 CRTC      │
│ DC_*        │ 0x80004000 │ 256B   │ Display Controller  │
└─────────────────────────────────────────────────────────┘
```

### CPU SPR Quick Reference
```
┌─────────────────────────────────────────────────────────┐
│ SPR  │ Name      │ Access │ Common Usage               │
├─────────────────────────────────────────────────────────┤
│ 1    │ XER       │ R/W    │ Integer exceptions         │
│ 8    │ LR        │ R/W    │ Function return addresses  │
│ 9    │ CTR       │ R/W    │ Branch loop counts         │
│ 18   │ DSISR     │ R      │ Data storage exceptions    │
│ 19   │ DAR       │ R      │ Data address (exceptions)  │
│ 25   │ SDR1      │ R/W    │ Page table base address    │
│ 26   │ SRR0      │ R/W    │ Exception save/restore 0   │
│ 27   │ SRR1      │ R/W    │ Exception save/restore 1   │
│ 48   │ PIR       │ R      │ Processor ID               │
│ 53   │ DEC       │ R/W    │ Decrementer (timer)        │
│ 287  │ PVR       │ R      │ Processor version          │
│ 304  │ TLB0      │ R/W    │ TLB entry 0                │
│ 305  │ TLB1      │ R/W    │ TLB entry 1                │
│ 306  │ TLB2      │ R/W    │ TLB entry 2                │
│ 400  │ HID0      │ R/W    │ Hardware config 0          │
│ 401  │ HID1      │ R/W    │ Hardware config 1          │
│ 464  │ DABR      │ R/W    │ Data breakpoint            │
│ 487  │ PVR       │ R      │ Processor version          │
│ 509  │ TENSR     │ R      │ Thread enable status       │
│ 510  │ TENS      │ W      │ Thread enable set          │
│ 511  │ TENC      │ W      │ Thread enable clear        │
└─────────────────────────────────────────────────────────┘
```

### Memory Protection Summary
```
┌─────────────────────────────────────────────────────────┐
│ Key │ Name       │ Kernel │ Title │ System │ Description│
├─────────────────────────────────────────────────────────┤
│ 0   │ USER_RW    │ RW     │ RW    │ RW     │ Full access│
│ 1   │ USER_RO    │ RW     │ RO    │ RO     │ Read-only  │
│ 2   │ USER_NA    │ RW     │ NA    │ NA     │ No access  │
│ 3   │ SUPERVISOR │ RW     │ NA    │ NA     │ Kernel only│
│ 4   │ HYPERVISOR │ RW     │ NA    │ NA     │ HV only    │
│ 5   │ TITLE_EXEC │ RWX    │ RX    │ RX     │ Code pages │
│ 6   │ GPU_RW     │ RW     │ RW    │ RW     │ GPU shared │
│ 7   │ ENCRYPTED  │ RW     │ NA    │ NA     │ Encrypted  │
└─────────────────────────────────────────────────────────┘
```

### SMC Status Codes
```
┌─────────────────────────────────────────────────────────┐
│ Code │ Meaning                                         │
├─────────────────────────────────────────────────────────┤
│ 0x00 │ SMC_OK - Operation successful                   │
│ 0x01 │ SMC_ERROR_INVALID_PARAM - Invalid parameter     │
│ 0x02 │ SMC_ERROR_BUSY - SMC busy, try again            │
│ 0x03 │ SMC_ERROR_TIMEOUT - Operation timed out         │
│ 0x10 │ SMC_ERROR_TEMP_HIGH - Temperature too high      │
│ 0x11 │ SMC_ERROR_FAN_FAIL - Fan failure detected       │
│ 0x20 │ SMC_ERROR_POWER - Power supply error            │
│ 0x30 │ SMC_ERROR_DVD - DVD drive error                 │
│ 0x40 │ SMC_ERROR_HDD - Hard drive error                │
│ 0x80 │ SMC_ERROR_SECURITY - Security violation         │
│ 0xFF │ SMC_FATAL - Fatal error, system halt            │
└─────────────────────────────────────────────────────────┘
```

### CPU MSR Bits Quick Reference
```
┌─────────────────────────────────────────────────────────┐
│ Bit │ Name     │ Description                           │
├─────────────────────────────────────────────────────────┤
│ 4   │ CM       │ 64-bit computation mode               │
│ 7   │ IS       │ Instruction address space (HV=0)      │
│ 8   │ DS       │ Data address space (HV=0)             │
│ 10  │ RI       │ Recoverable interrupt                 │
│ 12  │ DR       │ Data relocation (MMU enable)          │
│ 13  │ IR       │ Instruction relocation (MMU enable)   │
│ 16  │ EE       │ External interrupt enable             │
│ 17  │ PR       │ Problem state (0=privileged)          │
│ 18  │ FP       │ Floating point available              │
│ 19  │ ME       │ Machine check enable                  │
│ 20  │ FE0      │ FP exception mode 0                   │
│ 23  │ FE1      │ FP exception mode 1                   │
│ 25  │ IT       │ Instruction trace                     │
│ 26  │ ID       │ Instruction debug                     │
│ 29  │ DE       │ Debug interrupt enable                │
│ 30  │ CE       │ Critical interrupt enable             │
│ 48  │ SF       │ 64-bit mode (always 1 on Xenon)       │
└─────────────────────────────────────────────────────────┘
```

---

## Appendix A: Memory Access Performance

### Bandwidth Specifications
```
┌─────────────────────────────────────────────────────────┐
│ Interface     │ Bandwidth  │ Latency │ Usage           │
├─────────────────────────────────────────────────────────┤
│ CPU L1 Cache  │ ~100 GB/s  │ 4 cycles│ Per core        │
│ CPU L2 Cache  │ ~25 GB/s   │ 12 cycles│ Shared 1MB     │
│ System RAM    │ 22.4 GB/s  │ ~60 ns  │ Unified         │
│ GPU eDRAM     │ 256 GB/s   │ ~20 ns  │ Internal        │
│ GPU→System    │ 22.4 GB/s  │ ~60 ns  │ Shared bus      │
│ GPU→eDRAM     │ 256 GB/s   │ ~10 ns  │ Internal GPU    │
│ PCIe x16      │ 8 GB/s     │ ~500 ns │ External devices│
└─────────────────────────────────────────────────────────┘
```

### Cache Line Sizes
```
┌─────────────────────────────────────────────────────────┐
│ Cache         │ Line Size  │ Associativity │ Total Size│
├─────────────────────────────────────────────────────────┤
│ L1 Instruction│ 64 bytes   │ 2-way         │ 32KB/core │
│ L1 Data       │ 64 bytes   │ 4-way         │ 32KB/core │
│ L2 Unified    │ 128 bytes  │ 8-way         │ 1MB (shared)│
│ GPU Texture   │ 64 bytes   │ 4-way         │ 32KB      │
│ GPU Vertex    │ 64 bytes   │ 2-way         │ 16KB      │
│ eDRAM         │ 256 bytes  │ N/A (tile)    │ 10MB      │
└─────────────────────────────────────────────────────────┘
```

---

*Document Version: 1.0*
*Last Updated: 2026-01-31*
*For: PCSXr360 - PlayStation 1 Emulator for Xbox 360*