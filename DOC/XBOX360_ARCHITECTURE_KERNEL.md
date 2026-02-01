# Xbox 360 Architecture - Kernel-Level Technical Documentation

## Overview

This document provides comprehensive technical details about the Xbox 360 hardware architecture for emulator development purposes.

**Processor:** IBM Xenon (PowerPC-based Tri-Core)  
**GPU:** ATI/AMD Xenos  
**RAM:** 512MB GDDR3 unified memory  
**CPU Clock:** 3.2 GHz (per core)  
**Manufacturing:** 90nm → 65nm → 45nm process nodes

---

## 1. CPU Architecture (Xenon - PowerPC-based)

### 1.1 Core Architecture

The Xenon processor is a custom IBM PowerPC implementation featuring:

| Feature | Specification |
|---------|--------------|
| Architecture | PowerPC 2.02 (PPE-based) |
| Cores | 3 symmetric cores |
| Threads per core | 2 SMT threads |
| Total threads | 6 hardware threads |
| Pipeline stages | 23-stage in-order |
| Clock speed | 3.2 GHz |
| L1 Instruction Cache | 32KB per core |
| L1 Data Cache | 32KB per core |
| L2 Cache | 1MB shared (3:1 ratio) |
| Fabrication | Initially 90nm SOI |
| Die size | ~168 mm² |
| Transistors | ~165 million |

### 1.2 Core Block Diagram

```
                    ┌─────────────────────────────────────┐
                    │           Xenon CPU (3 Cores)       │
                    ├─────────────────────────────────────┤
    ┌───────────┐   │  ┌─────────┐ ┌─────────┐ ┌────────┐ │
    │  1MB L2   │◄──┼──┤ Core 0  │ │ Core 1  │ │ Core 2 │ │
    │   Cache   │   │  │(2 threads)│(2 threads)│(2 threads)│
    │ (3:1 ratio)│   │  └────┬────┘ └────┬────┘ └───┬────┘ │
    └─────┬─────┘   │       │           │          │      │
          │         │  ┌────┴────┐ ┌────┴────┐ ┌───┴────┐ │
          ▼         │  │32KB I$  │ │32KB I$  │ │32KB I$ │ │
    ┌───────────┐   │  │32KB D$  │ │32KB D$  │ │32KB D$ │ │
    │  Memory   │◄──┼──┴─────────┴─┴─────────┴─┴────────┘ │
    │ Controller│   │                                      │
    └───────────┘   └─────────────────────────────────────┘
```

### 1.3 Pipeline Architecture

The Xenon uses an **in-order execution** pipeline with 23 stages:

| Stage Range | Function | Description |
|-------------|----------|-------------|
| 1-4 | Fetch | Instruction fetch from I-Cache |
| 5-8 | Decode | Instruction decode and grouping |
| 9-12 | Dispatch | Issue to execution units |
| 13-18 | Execute | ALU/FPU/VMX execution |
| 19-21 | Complete | Write results |
| 22-23 | Commit | Update architectural state |

**Key Characteristics:**
- In-order execution (no out-of-order completion)
- 2-way SMT (Simultaneous Multi-Threading)
- 2 instructions can issue per cycle (1 per thread max)
- Pipeline flushes on branch mispredictions
- Load-to-use latency: ~4-6 cycles

### 1.4 SMT (Simultaneous Multi-Threading)

Each core supports 2 threads sharing resources:

```
Thread 0 ──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──
           │  │  │  │  │  │  │  │  │  │  │  │  │  │  │
Thread 1 ──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──
           
           Time ──>

Resource sharing model:
- Private per thread: GPRs, FPRs, VMX registers, CR, LR, CTR, XER
- Shared: Execution units, L1 caches, L2 cache, pipeline slots
- Round-robin arbitration for pipeline slots
```

**SMT Benefits:**
- Hides memory latency
- Improves utilization when one thread stalls
- Hardware thread ID in CTRL register bits [54:56]

### 1.5 Execution Units

| Unit | Latency | Throughput | Description |
|------|---------|------------|-------------|
| Integer ALU 0 | 1 cycle | 1/cycle | General integer ops |
| Integer ALU 1 | 1 cycle | 1/cycle | General integer ops |
| Branch Unit | 1 cycle | 1/cycle | Branches, CR ops |
| Load/Store | 3-4 cycles | 1/cycle | Memory operations |
| FPU (FP1) | 6 cycles | 1/2 cycles | Double-precision |
| VMX-128 ALU | 4 cycles | 1/cycle | Vector integer/float |
| VMX-128 Permute | 4 cycles | 1/cycle | Vector shuffle |
| VMX-128 Simple | 2 cycles | 1/cycle | Simple vector ops |

---

## 2. GPU Architecture (Xenos)

### 2.1 Xenos Overview

| Feature | Specification |
|---------|--------------|
| Architecture | Custom ATI/AMD R500-based |
| Shader model | Unified shader architecture |
| GPU clock | 500 MHz |
| Shader units | 48 unified shaders |
| Memory | 10MB eDRAM framebuffer |
| Bus interface | 128-bit unified memory bus |
| Tesselation | Hardware tessellation unit |
| Anti-aliasing | 4x MSAA free (eDRAM) |
| Z/Stencil compression | Yes |

### 2.2 Unified Shader Architecture

The Xenos pioneered unified shaders before PC GPUs:

```
                    ┌─────────────────────────────────┐
                    │        Xenos GPU (500MHz)       │
                    ├─────────────────────────────────┤
    ┌───────────┐   │  ┌─────────────────────────────┐ │
    │  Command  │──►│  │     Graphics Command        │ │
    │  Processor│   │  │       Processor             │ │
    └───────────┘   │  └─────────────┬───────────────┘ │
                    │                │                 │
    ┌───────────┐   │  ┌─────────────┴───────────────┐ │
    │   10MB    │◄──┼──┤      48 Unified Shaders     │ │
    │   eDRAM   │   │  │  (Can be vertex or pixel)   │ │
    │Framebuffer│   │  │  - 16 Vec4 ALUs per shader  │ │
    └───────────┘   │  │  - Dynamic allocation       │ │
                    │  └─────────────┬───────────────┘ │
                    │                ▼                 │
                    │  ┌─────────────────────────────┐ │
                    │  │    Texture Units (16)       │ │
                    │  │    ROP Units (8)            │ │
                    │  │    Tessellation Unit        │ │
                    │  └─────────────┬───────────────┘ │
                    │                ▼                 │
                    │  ┌─────────────────────────────┐ │
                    │  │     Memory Interface        │ │
                    │  │    (128-bit GDDR3 Bus)      │ │
                    │  └─────────────────────────────┘ │
                    └─────────────────────────────────┘
```

### 2.3 Shader Allocation

| Mode | Vertex Shaders | Pixel Shaders | Total Used |
|------|----------------|---------------|------------|
| Balanced | 16 | 32 | 48 |
| Vertex Heavy | 32 | 16 | 48 |
| Pixel Heavy | 8 | 40 | 48 |
| Custom | Variable | Variable | ≤48 |

### 2.4 eDRAM Framebuffer

The 10MB eDRAM provides zero-bandwidth framebuffer operations:

```
eDRAM Layout:
┌─────────────────────────────────────────────────────────┐
│                    10MB eDRAM Array                      │
├─────────────────────────────────────────────────────────┤
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐│
│  │  Color 0    │ │  Color 1    │ │      Z/Stencil      ││
│  │  (2-4MB)    │ │  (2-4MB)    │ │      (4MB)          ││
│  └─────────────┘ └─────────────┘ └─────────────────────┘│
│                                                          │
│  Features:                                               │
│  - 4x MSAA effectively free (done during resolve)        │
│  - Alpha blending at full speed                          │
│  - Z-test/stencil with compression                       │
│  - No memory bandwidth cost for framebuffer ops          │
└─────────────────────────────────────────────────────────┘
```

**eDRAM Resolution Limits:**

| Format | Max Resolution | MSAA |
|--------|---------------|------|
| 32-bit color | 1280×720 | 4x |
| 64-bit HDR | 1280×720 | 2x |
| 32-bit color | 1920×1080 | None |

---

## 3. Memory Subsystem and Architecture

### 3.1 Memory Architecture Overview

| Component | Size | Bandwidth | Latency (approx) |
|-----------|------|-----------|------------------|
| Main RAM (GDDR3) | 512MB | 22.4 GB/s | ~200-300 cycles |
| GPU eDRAM | 10MB | 256 GB/s internal | ~10-20 cycles |
| CPU L2 Cache | 1MB | ~200 GB/s | ~20-30 cycles |
| CPU L1 Cache | 32KB I + 32KB D | ~400 GB/s | ~3-4 cycles |

### 3.2 Unified Memory Architecture

The Xbox 360 uses a **unified memory architecture (UMA)**:

```
┌─────────────────────────────────────────────────────────────┐
│              Unified Memory Controller                       │
│                    (22.4 GB/s)                              │
└─────────────┬───────────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────────┐
│                    512MB GDDR3                               │
│            (700 MHz, 128-bit bus)                           │
└─────────────┬───────────────────────────────────────────────┘
              │
    ┌─────────┴─────────┐
    │                   │
    ▼                   ▼
┌───────────┐     ┌───────────┐
│   CPU     │     │    GPU    │
│ (via FSB) │     │ (Direct)  │
└───────────┘     └───────────┘
```

### 3.3 Memory Map

```
┌────────────────────────────────────────────────────────────┐
│                      Physical Address Space                 │
├────────────────────────────────────────────────────────────┤
│ 0x0000_0000_0000 ─┐                                        │
│                   │  Reserved / Kernel Space                │
│ 0x0000_0001_0000 ─┤  (16KB - exception vectors)             │
│                   │                                        │
│ 0x0000_8000_0000 ─┤  Kernel text/data (512MB)               │
│                   │                                        │
│ 0x0001_0000_0000 ─┤  User space (512MB per process)         │
│                   │                                        │
│ 0x0001_8000_0000 ─┤  Physical RAM mapping                   │
│                   │  (512MB mirrored/real)                  │
│ 0x0002_0000_0000 ─┤                                        │
│                   │  GPU Registers / MMIO                   │
│ 0x0002_1000_0000 ─┤  (16MB aperture)                        │
│                   │                                        │
│ 0x0003_0000_0000 ─┤  PCI/PCIe config space                  │
│                   │                                        │
│ 0xFFFF_FFFF_FFFF ─┘  End of 48-bit address space            │
└────────────────────────────────────────────────────────────┘
```

### 3.4 Page Table and Translation

**Page Sizes:**
- 4KB (standard)
- 64KB (large pages)
- 16MB (huge pages - kernel only)

**Translation Flow:**
```
Virtual Address (64-bit)
         │
         ▼
┌─────────────────┐
│   Segment Lookaside Buffer (SLB)  │
│   - Maps EA to virtual segment    │
│   - 64 entries                    │
└────────┬────────┘
         │ Virtual Page Number
         ▼
┌─────────────────┐
│  Page Table Walk (if TLB miss)    │
│  - Radix tree page table          │
│  - 3 levels for 4KB pages         │
└────────┬────────┘
         │ Real Page Number
         ▼
┌─────────────────┐
│    Physical Address               │
└─────────────────┘
```

### 3.5 Memory Bandwidth Allocation

```
Total Bandwidth: 22.4 GB/s

Typical Allocation:
┌────────────────────────────────────────┐
│ GPU Rendering      │  8-12 GB/s (70%) │
│ CPU Access         │  4-6 GB/s (20%)  │
│ Audio/DMA/Other    │  2-3 GB/s (10%)  │
└────────────────────────────────────────┘

Note: GPU has priority access. CPU may stall on GPU-heavy workloads.
```

---

## 4. Kernel/Hypervisor Details

### 4.1 Privilege Levels

| Level | Name | Description |
|-------|------|-------------|
| 0 | Hypervisor (Ring -1) | Lowest level, controls hardware |
| 1 | Kernel (Ring 0) | OS kernel, privileged operations |
| 2 | Unused | Reserved |
| 3 | User (Ring 3) | Applications, unprivileged |

### 4.2 Hypervisor Architecture

The Xbox 360 uses a **hypervisor** for security and virtualization:

```
┌─────────────────────────────────────────────────────────┐
│  Privilege Level 0 - Hypervisor ("Security Kernel")     │
│  - Runs in hypervisor mode (MSR[HV]=1)                  │
│  - Manages shadow page tables                            │
│  - Enforces code signing                                 │
│  - Handles secure boot                                   │
│  - Encrypts/decrypts memory when needed                  │
│  - Traps sensitive instructions                          │
└─────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────┐
│  Privilege Level 1 - Xbox Kernel                         │
│  - Standard PowerPC privileged mode                      │
│  - Thread/process management                             │
│  - Memory management (via hypervisor)                    │
│  - Device drivers                                        │
│  - System calls                                          │
└─────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────┐
│  Privilege Level 3 - User Applications                   │
│  - Games, Dashboard, XNA apps                            │
│  - Unprivileged execution                                │
│  - Syscall interface to kernel                           │
└─────────────────────────────────────────────────────────┘
```

### 4.3 Hypervisor Operations

**Key Hypervisor Functions:**

| Function | Description |
|----------|-------------|
| `sc` (syscall) | Trapped by HV, dispatched to kernel |
| `tlbie` | TLB invalidate - trapped, validated by HV |
| `mtspr` (key SPRs) | Privileged SPR writes trapped |
| Memory access | Shadow page tables maintain security |
| I/O access | All MMIO trapped and filtered |
| Interrupts | HV handles, delegates to kernel |

### 4.4 Kernel Components

```
Xbox 360 Kernel (xboxkrnl.exe):
├─ Executive
│  ├─ Process/Thread Manager
│  ├─ Memory Manager (works with HV)
│  ├─ I/O Manager
│  └─ Object Manager
│
├─ Kernel
│  ├─ Scheduler (SMT-aware)
│  ├─ Dispatcher
│  ├─ Interrupt Handler
│  └─ Synchronization
│
├─ Hardware Abstraction Layer (HAL)
│  ├─ CPU abstraction
│  ├─ Bus/interrupt handling
│  └─ Timer services
│
└─ Drivers
   ├─ GPU (xgkrnl)
   ├─ Audio (xma)
   ├─ Network (xnet)
   ├─ Storage (xhdd)
   └─ Input (xinput)
```

### 4.5 Shadow Page Tables

The hypervisor maintains **shadow page tables** for security:

```
Guest Page Tables (Kernel)
         │
         │ Shadowing
         ▼
Shadow Page Tables (Hypervisor)
         │
         │ Hardware uses these
         ▼
Physical Memory

Security checks on shadowing:
- Writable pages cannot be executable (W^X)
- Code pages require valid signatures
- Kernel pages protected from user access
- Hardware-enforced via page table permissions
```

---

## 5. Interrupts and Exceptions

### 5.1 Exception Types

| Vector Offset | Exception Type | Priority | Description |
|--------------|----------------|----------|-------------|
| 0x0100 | System Reset | 0 | Hard reset |
| 0x0200 | Machine Check | 1 | Hardware error |
| 0x0300 | DSI (Data Storage) | 2 | Data access violation |
| 0x0400 | ISI (Instr Storage) | 3 | Instruction access violation |
| 0x0500 | External Interrupt | 4 | External IRQ |
| 0x0600 | Alignment | 5 | Unaligned access |
| 0x0700 | Program | 6 | Illegal instruction |
| 0x0800 | FPU Unavailable | 7 | FPU disabled |
| 0x0900 | Decrementer | 8 | Timer interrupt |
| 0x0A00 | Reserved | - | Unused |
| 0x0B00 | Reserved | - | Unused |
| 0x0C00 | System Call | 9 | `sc` instruction |
| 0x0D00 | Trace | 10 | Single-step |
| 0x0E00 | FPU Assist | 11 | FPU denormal |
| 0x0F00 | Performance Monitor | 12 | PMU interrupt |
| 0x0F20 | VMX Unavailable | 13 | VMX disabled |
| 0x0F40 | VMX Assist | 14 | VMX denormal |
| 0x1600 | IMISS | - | Instruction TLB miss |
| 0x1700 | DLMISS | - | Data load TLB miss |
| 0x1800 | DSMISS | - | Data store TLB miss |

### 5.2 Exception Processing

```
When exception occurs:

1. Save state:
   SRR0 ← PC (next instruction address)
   SRR1 ← MSR (current machine state)

2. Modify MSR:
   MSR[WE] ← 0  (disable wait state)
   MSR[CE] ← 0  (disable critical interrupts)
   MSR[EE] ← 0  (disable external interrupts)
   MSR[PR] ← 0  (switch to supervisor mode)
   MSR[FP] ← 0  (disable FPU)
   MSR[FE0,FE1] ← 0 (disable FP exceptions)
   MSR[VMX] ← 0 (disable VMX)
   MSR[HV] ← 0  (clear hypervisor mode - kernel handles)

3. Branch to vector:
   PC ← IVPR || IVOR[n]

4. Execute handler

5. Return with rfi:
   MSR ← SRR1
   PC ← SRR0
```

### 5.3 Interrupt Sources

| Source | Type | Vector | Handler |
|--------|------|--------|---------|
| GPU | External | 0x500 | GPU ISR |
| Audio | External | 0x500 | Audio ISR |
| Network | External | 0x500 | Network ISR |
| Storage | External | 0x500 | Storage ISR |
| Timer | Decrementer | 0x900 | Scheduler |
| IPI | External | 0x500 | Cross-core interrupt |
| Page Fault | DSI/ISI | 0x300/0x400 | VMM |
| Syscall | System Call | 0xC00 | System call handler |

### 5.4 SPU Interrupts (Inter-Processor)

```
SPU (Signal Processing Unit) is actually:
- Not the Cell SPU!
- Inter-processor interrupt mechanism
- Used for cross-core communication

Register: SPU (SPR 304)
- Write to trigger interrupt on target core
- Used by kernel for IPIs (Inter-Processor Interrupts)
- Used for thread migration notifications
```

---

## 6. PowerPC Instruction Set

### 6.1 Instruction Formats

PowerPC uses fixed 32-bit instructions with several formats:

| Format | Layout | Usage |
|--------|--------|-------|
| I-form | 6|24 |2| Unconditional branch |
| B-form | 6|5|5|14|1|1| Conditional branch |
| D-form | 6|5|5|16| Load/store, immediate |
| DS-form | 6|5|5|14|2| Load/store double |
| X-form | 6|5|5|5|10|1| Register-register |
| XL-form | 6|5|5|5|10|1| Branch register |
| XFX-form | 6|5|5|5|10|1| SPR access |
| XS-form | 6|5|5|5|9|1| Shift double |
| XO-form | 6|5|5|5|9|1| Integer arithmetic |
| A-form | 6|5|5|5|5|6|1| Floating-point |
| M-form | 6|5|5|5|6|5|1| Rotate/shift |
| MD-form | 6|5|5|5|6|3|2|1| Rotate double |
| MDS-form | 6|5|5|5|5|6|4|1| Rotate double reg |
| VX-form | 6|5|5|5|11| VMX operations |
| VXA-form | 6|5|5|5|5|6| VMX A-class |
| VXR-form | 6|5|5|5|5|6|1| VMX compare |

### 6.2 Instruction Categories

#### Integer Instructions

```asm
; Arithmetic
add     rD, rA, rB      ; rD = rA + rB
add.    rD, rA, rB      ; Add + update CR
addo    rD, rA, rB      ; Add + overflow check
addi    rD, rA, SIMM    ; Add immediate
addis   rD, rA, SIMM    ; Add immediate shifted
subf    rD, rA, rB      ; rD = rB - rA (subtract from)
neg     rD, rA          ; rD = -rA

; Logical
and     rD, rA, rB      ; rD = rA & rB
or      rD, rA, rB      ; rD = rA | rB
xor     rD, rA, rB      ; rD = rA ^ rB
nand    rD, rA, rB      ; rD = ~(rA & rB)
nor     rD, rA, rB      ; rD = ~(rA | rB)
andi.   rD, rA, UIMM    ; And immediate + update CR
ori     rD, rA, UIMM    ; Or immediate
xori    rD, rA, UIMM    ; Xor immediate

; Shift/Rotate
slw     rD, rA, rB      ; Shift left word
srw     rD, rA, rB      ; Shift right word
sraw    rD, rA, rB      ; Shift right algebraic word
rlwinm  rD, rA, SH, MB, ME ; Rotate left + mask
rlwimi  rD, rA, SH, MB, ME ; Rotate left + mask insert

; Compare
cmpw    crfD, rA, rB    ; Compare word
cmpwi   crfD, rA, SIMM  ; Compare word immediate
cmpd    crfD, rA, rB    ; Compare doubleword (64-bit)
```

#### Branch Instructions

```asm
; Unconditional
b       target          ; Branch
ba      target          ; Branch absolute
bl      target          ; Branch and link (subroutine call)
bla     target          ; Branch and link absolute
blr                     ; Branch to link register (return)
bctr                    ; Branch to count register (switch)

; Conditional
bc      BO, BI, target  ; Branch conditional
bca     BO, BI, target  ; Branch conditional absolute
bcl     BO, BI, target  ; Branch conditional and link

; Common conditional patterns
beq     target          ; Branch if equal (cr0[EQ])
bne     target          ; Branch if not equal
blt     target          ; Branch if less than
ble     target          ; Branch if less or equal
bgt     target          ; Branch if greater than
bge     target          ; Branch if greater or equal

; BO field (5 bits): Branch Options
; Bit 4: Decrement CTR (if 1)
; Bit 3: Test CTR (0= CTR!=0, 1= CTR==0)
; Bit 2: Test condition (0= test, 1= don't test)
; Bit 1: Condition polarity
; Bit 0: Always (unconditional)

; BI field (5 bits): Condition register bit to test
; 0-3: cr0[LT,GT,EQ,SO]
; 4-7: cr1[LT,GT,EQ,SO], etc.
```

#### Load/Store Instructions

```asm
; Byte operations
lbz     rD, disp(rA)    ; Load byte and zero
lbzx    rD, rA, rB      ; Load byte indexed
stb     rS, disp(rA)    ; Store byte
stbx    rS, rA, rB      ; Store byte indexed

; Halfword operations
lhz     rD, disp(rA)    ; Load halfword and zero
lhax    rD, rA, rB      ; Load halfword algebraic (signed)
sth     rS, disp(rA)    ; Store halfword

; Word operations (32-bit)
lwz     rD, disp(rA)    ; Load word and zero
lwzx    rD, rA, rB      ; Load word indexed
stw     rS, disp(rA)    ; Store word
stwx    rS, rA, rB      ; Store word indexed
stwu    rS, disp(rA)    ; Store word with update (rA += disp)

; Doubleword operations (64-bit)
ld      rD, disp(rA)    ; Load doubleword
ldx     rD, rA, rB      ; Load doubleword indexed
std     rS, disp(rA)    ; Store doubleword
stdx    rS, rA, rB      ; Store doubleword indexed

; Multiple/string
lmw     rD, disp(rA)    ; Load multiple words
stmw    rS, disp(rA)    ; Store multiple words

; Synchronization
lwarx   rD, rA, rB      ; Load word and reserve (atomic)
stwcx.  rS, rA, rB      ; Store word conditional
sync                    ; Synchronize (memory barrier)
eieio                   ; Enforce in-order execution I/O
isync                   ; Instruction synchronize
```

#### System Instructions

```asm
; SPR access
mfspr   rD, sprN        ; Move from SPR
mtspr   sprN, rS        ; Move to SPR

; MSR access
mfmsr   rD              ; Move from MSR
mtmsr   rS              ; Move to MSR
mtmsrd  rS              ; Move to MSR doubleword

; System call/trap
sc                      ; System call
tw      TO, rA, rB      ; Trap word
twi     TO, rA, SIMM    ; Trap word immediate

; Return from interrupt
rfi                     ; Return from interrupt
rfid                    ; Return from interrupt doubleword

; Cache/TLB management
dcbf    rA, rB          ; Data cache block flush
dcbst   rA, rB          ; Data cache block store
dcbz    rA, rB          ; Data cache block zero
dcbi    rA, rB          ; Data cache block invalidate
icbi    rA, rB          ; Instruction cache block invalidate
tlbie   rB              ; TLB invalidate entry
tlbiel  rB              ; TLB invalidate entry local
tlbsync                 ; TLB synchronize
```

### 6.3 Common Instruction Patterns

```asm
; Function prologue
stwu    r1, -32(r1)     ; Allocate stack frame
mflr    r0              ; Save LR
stw     r0, 36(r1)      ; Store LR to stack
stmw    r13, 8(r1)      ; Save non-volatile regs

; Function epilogue
lmw     r13, 8(r1)      ; Restore non-volatile regs
lwz     r0, 36(r1)      ; Load saved LR
mtlr    r0              ; Restore LR
addi    r1, r1, 32      ; Deallocate stack
blr                     ; Return

; 64-bit constant load
lis     r3, 0x1234      ; Load upper 16 bits
ori     r3, r3, 0x5678  ; OR in next 16 bits
sldi    r3, r3, 32      ; Shift left 32
oris    r3, r3, 0x9ABC  ; OR in next 16 bits
ori     r3, r3, 0xDEF0  ; OR in lower 16 bits

; Atomic compare-and-swap
@retry:
lwarx   r5, 0, r4       ; Load with reservation
cmpw    r5, r6          ; Compare
bne     @exit           ; If not equal, exit
stwcx.  r7, 0, r4       ; Store conditional
bne     @retry          ; If failed, retry
@exit:
```

---

## 7. Registers and System Calls

### 7.1 Register Overview

#### General Purpose Registers (GPRs)

| Register | Name | Usage |
|----------|------|-------|
| r0 | - | Volatile, used in prologue/epilogue |
| r1 | SP | Stack pointer |
| r2 | TOC | Table of contents (global data pointer) |
| r3-r4 | - | First two argument registers / return values |
| r5-r10 | - | Argument registers 3-8 |
| r11 | - | Volatile, environment pointer |
| r12 | - | Volatile, function entry address |
| r13 | - | Small data area pointer (read-only) |
| r14-r31 | - | Non-volatile (callee-saved) |

#### Special Purpose Registers (SPRs)

| SPR # | Name | Description |
|-------|------|-------------|
| 1 | XER | Fixed-point exception register |
| 8 | LR | Link register (return address) |
| 9 | CTR | Count register (loops/switch) |
| 18 | DSISR | Data storage interrupt status |
| 19 | DAR | Data address register |
| 25 | SDR1 | Page table base |
| 26 | SRR0 | Save/restore register 0 (PC) |
| 27 | SRR1 | Save/restore register 1 (MSR) |
| 32 | SR0-15 | Segment registers (legacy) |
| 136 | CTRL | Processor control (thread ID) |
| 144-159 | SPRG0-15 | SPR general registers |
| 152 | TBL | Time base lower (read) |
| 153 | TBU | Time base upper (read) |
| 268 | TBL | Time base lower (write) |
| 269 | TBU | Time base upper (write) |
| 284 | DEC | Decrementer |
| 285 | TB | Time base (64-bit) |
| 304 | SPU | Signal processing unit (IPI) |
| 318 | DABR | Data address breakpoint |
| 319 | DABRX | Data address breakpoint extended |
| 401 | PPR | Program priority register |
| 464/465 | DSCR | Data stream control |
| 568/569 | CTRL | Control register (read/write) |
| 944 | MVR | VMX save/restore |
| 1023 | PIR | Processor ID |

#### Machine State Register (MSR)

| Bit | Name | Description |
|-----|------|-------------|
| 0 | SF | Sixty-four bit mode |
| 1-3 | - | Reserved |
| 4 | HV | Hypervisor mode |
| 5 | - | Reserved |
| 6 | VE | VMX enable |
| 7 | - | Reserved |
| 8 | CE | Critical interrupt enable |
| 9-12 | - | Reserved |
| 13 | EE | External interrupt enable |
| 14 | PR | Problem state (user mode) |
| 15 | FP | Floating-point available |
| 16 | ME | Machine check enable |
| 17 | FE0 | Floating-point exception mode 0 |
| 18 | SE | Single-step trace enable |
| 19 | BE | Branch trace enable |
| 20 | DE | Debug interrupt enable |
| 21 | FE1 | Floating-point exception mode 1 |
| 22 | - | Reserved |
| 23 | - | Reserved |
| 24 | - | Reserved |
| 25 | - | Reserved |
| 26 | - | Reserved |
| 27 | - | Reserved |
| 28 | - | Reserved |
| 29 | - | Reserved |
| 30 | - | Reserved |
| 31 | - | Reserved |
| 32-33 | - | Reserved |
| 34 | - | Reserved |
| 35 | - | Reserved |
| 36 | - | Reserved |
| 37 | - | Reserved |
| 38 | - | Reserved |
| 39 | - | Reserved |
| 40 | - | Reserved |
| 41 | - | Reserved |
| 42 | - | Reserved |
| 43 | - | Reserved |
| 44 | - | Reserved |
| 45 | - | Reserved |
| 46 | - | Reserved |
| 47 | - | Reserved |
| 48 | - | Reserved |
| 49 | - | Reserved |
| 50 | - | Reserved |
| 51 | - | Reserved |
| 52 | - | Reserved |
| 53 | - | Reserved |
| 54 | - | Reserved |
| 55 | - | Reserved |
| 56 | - | Reserved |
| 57 | - | Reserved |
| 58 | - | Reserved |
| 59 | - | Reserved |
| 60 | - | Reserved |
| 61 | - | Reserved |
| 62 | - | Reserved |
| 63 | - | Reserved |

### 7.2 System Call Interface

System calls are invoked with the `sc` instruction:

```asm
; System call format
li      r0, syscall_number    ; System call number in r0
li      r3, arg1              ; First argument
li      r4, arg2              ; Second argument
...                           ; Additional arguments in r5-r10
sc                            ; Execute system call
; Result in r3 (negative = error)
```

### 7.3 Common System Calls

| Number | Name | Description | Args | Return |
|--------|------|-------------|------|--------|
 | 0 | NtCreateFile | Create/open file | r3-r9 | Handle |
| 1 | NtOpenFile | Open existing file | r3-r6 | Handle |
| 2 | NtClose | Close handle | r3 | Status |
| 3 | NtReadFile | Read from file | r3-r9 | Status |
| 4 | NtWriteFile | Write to file | r3-r9 | Status |
| 5 | NtQueryInformationFile | Query file info | r3-r6 | Status |
| 6 | NtSetInformationFile | Set file info | r3-r6 | Status |
| 7 | NtCreateDirectoryObject | Create directory | r3-r5 | Handle |
| 8 | NtCreateSymbolicLinkObject | Create symlink | r3-r6 | Handle |
| 9 | NtCreateDevice | Create device | - | Handle |
| 10 | NtCreateEvent | Create event | r3-r7 | Handle |
| 11 | NtCreateEventPair | Create event pair | r3-r5 | Handle |
| 12 | NtCreateMutant | Create mutex | r3-r6 | Handle |
| 13 | NtCreateSemaphore | Create semaphore | r3-r7 | Handle |
| 14 | NtCreateTimer | Create timer | r3-r6 | Handle |
| 15 | NtCreateIoCompletion | Create I/O completion | r3-r6 | Handle |
| 16 | NtCreateThread | Create thread | r3-r9 | Handle |
| 17 | NtOpenThread | Open thread | r3-r6 | Handle |
| 18 | NtExitThread | Exit thread | r3 | - |
| 19 | NtSuspendThread | Suspend thread | r3-r4 | Status |
| 20 | NtResumeThread | Resume thread | r3-r4 | Status |
| 21 | NtWaitForMultipleObjects | Wait for objects | r3-r7 | Status |
| 22 | NtWaitForSingleObject | Wait for single | r3-r5 | Status |
| 23 | NtSetEvent | Set event | r3-r4 | Status |
| 24 | NtSetEventBoostPriority | Set event + boost | r3 | Status |
| 25 | NtClearEvent | Clear event | r3 | Status |
| 26 | NtPulseEvent | Pulse event | r3-r4 | Status |
| 27 | NtQueryEvent | Query event | r3-r5 | Status |
| 28 | NtCreateProcess | Create process | r3-r6 | Handle |
| 29 | NtOpenProcess | Open process | r3-r6 | Handle |
| 30 | NtTerminateProcess | Terminate process | r3-r4 | Status |
| 31 | NtQueryVirtualMemory | Query memory | r3-r7 | Status |
| 32 | NtAllocateVirtualMemory | Allocate memory | r3-r7 | Status |
| 33 | NtFreeVirtualMemory | Free memory | r3-r5 | Status |
| 34 | NtProtectVirtualMemory | Protect memory | r3-r6 | Status |
| 35 | NtReadVirtualMemory | Read process memory | r3-r7 | Status |
| 36 | NtWriteVirtualMemory | Write process memory | r3-r7 | Status |
| 37 | NtLockVirtualMemory | Lock memory | r3-r6 | Status |
| 38 | NtUnlockVirtualMemory | Unlock memory | r3-r6 | Status |
| 39 | NtFlushInstructionCache | Flush I-cache | r3-r5 | Status |
| 40 | NtFlushWriteBuffer | Flush write buffer | - | Status |
| 41 | NtQueryDirectoryObject | Query directory | r3-r7 | Status |
| 42 | NtQueryDirectoryFile | Query directory file | r3-r9 | Status |
| 43 | NtCreatePort | Create port | r3-r6 | Handle |
| 44 | NtConnectPort | Connect to port | r3-r9 | Status |
| 45 | NtListenPort | Listen on port | r3-r4 | Status |
| 46 | NtAcceptConnectPort | Accept connection | r3-r7 | Status |
| 47 | NtCompleteConnectPort | Complete connection | r3 | Status |
| 48 | NtRequestWaitReplyPort | Request/reply | r3-r5 | Status |
| 49 | NtReplyWaitReceivePort | Reply/wait/receive | r3-r6 | Status |
| 50 | NtSetInformationThread | Set thread info | r3-r6 | Status |
| 51 | NtQueryInformationThread | Query thread info | r3-r6 | Status |
| 52 | NtQueryInformationProcess | Query process info | r3-r6 | Status |
| 53 | NtSetInformationProcess | Set process info | r3-r6 | Status |
| 54 | NtQuerySystemTime | Query system time | r3 | Status |
| 55 | NtSetSystemTime | Set system time | r3 | Status |
| 56 | NtQueryTimerResolution | Query timer res | r3-r5 | Status |
| 57 | NtSetTimerResolution | Set timer res | r3-r5 | Status |
| 58 | NtDelayExecution | Delay execution | r3-r4 | Status |
| 59 | NtYieldExecution | Yield execution | - | Status |
| 60 | NtQueryPerformanceCounter | Query perf counter | r3-r4 | Status |
| 61 | NtCreateKeyedEvent | Create keyed event | r3-r5 | Handle |
| 62 | NtOpenKeyedEvent | Open keyed event | r3-r5 | Handle |
| 63 | NtReleaseKeyedEvent | Release keyed event | r3-r5 | Status |
| 64 | NtWaitForKeyedEvent | Wait for keyed event | r3-r5 | Status |

**Xbox 360 Specific Syscalls (257+):**

| Number | Name | Description |
|--------|------|-------------|
| 257 | NtCreateDebugObject | Create debug object |
| 258 | NtDebugActiveProcess | Debug process |
| 259 | NtReadDebuggeeMemory | Read debuggee memory |
| 260 | NtWriteDebuggeeMemory | Write debuggee memory |
| 261 | NtDebugBreak | Debug break |
| 262 | NtContinue | Continue execution |
| 263 | NtExecuteThread | Execute thread |
| 264 | NtRenameTransaction | Rename transaction |
| 265 | NtSaveDebugRegisters | Save debug registers |
| 266 | NtRestoreDebugRegisters | Restore debug registers |
| 267 | NtCloseAllHandles | Close all handles |
| 268 | NtUserIoApc | User I/O APC |
| 269 | NtQueryPerformanceFrequency | Query perf frequency |
| 270 | NtAllocateUserPhysicalPages | Allocate physical pages |
| 271 | NtFreeUserPhysicalPages | Free physical pages |
| 272 | NtMapUserPhysicalPages | Map physical pages |
| 273 | NtMapUserPhysicalPagesScatter | Scatter mapping |
| 274 | NtLoadDll | Load DLL |
| 275 | NtUnloadDll | Unload DLL |
| 276 | NtQueueApcThread | Queue APC |
| 277 | NtFlushUtilityRam | Flush utility RAM |
| 278 | NtGetLogicalProcessorInformation | Get CPU info |
| 279 | NtGetTickCount | Get tick count |
| 280 | NtGetModuleHandle | Get module handle |
| 281 | NtGetProcAddress | Get procedure address |
| 282 | NtGetModuleHandleEx | Get module handle (extended) |
| 283 | NtSetSystemTimeRefresh | Set time refresh |
| 284 | NtQueryModuleInformation | Query module info |
| 285 | NtCreateFileEx | Create file (extended) |
| 286 | NtOpenFileEx | Open file (extended) |
| 287 | NtReadFileEx | Read file (extended) |
| 288 | NtWriteFileEx | Write file (extended) |
| 289 | NtQueryInformationFileEx | Query file info (extended) |
| 290 | NtSetInformationFileEx | Set file info (extended) |
| 291 | NtQueryDirectoryFileEx | Query directory (extended) |
| 292 | NtQueryVolumeInformationFileEx | Query volume info |
| 293 | NtSetVolumeInformationFileEx | Set volume info |
| 294 | NtFsControlFileEx | FS control (extended) |
| 295 | NtDeviceIoControlFileEx | Device I/O control |
| 296 | NtCancelIoFileEx | Cancel I/O (extended) |
| 297 | NtLockFileEx | Lock file (extended) |
| 298 | NtUnlockFileEx | Unlock file (extended) |
| 299 | NtQueryQuotaInformationFileEx | Query quota info |
| 300 | NtSetQuotaInformationFileEx | Set quota info |

---

## 8. Hardware Specifications

### 8.1 System Specifications

| Component | Specification |
|-----------|--------------|
| **CPU** | IBM Xenon, 3.2 GHz, 3 cores, 6 threads |
| **CPU Cache** | 32KB L1 I/D per core, 1MB shared L2 |
| **GPU** | ATI Xenos, 500 MHz, 48 unified shaders |
| **GPU eDRAM** | 10MB frame buffer |
| **System RAM** | 512MB GDDR3 @ 700 MHz |
| **Memory Bus** | 128-bit unified, 22.4 GB/s |
| **Storage** | 12x DVD drive (7-8 MB/s), 20-250GB HDD |
| **Audio** | XMA decoder, 5.1 surround |
| **Network** | 10/100 Ethernet, 802.11b/g (later models) |
| **USB** | 3 ports (front/rear) |
| **Video Output** | Component, VGA, HDMI (later models) |
| **Power** | 170W (early), 175W (later), 150W (slim) |

### 8.2 Xenon CPU Specifications

| Feature | Details |
|---------|---------|
| Process | 90nm → 65nm → 45nm SOI |
| Die size | ~168 mm² (90nm), ~121 mm² (65nm), ~77 mm² (45nm) |
| Transistors | ~165 million |
| Power | ~70-80W (CPU portion) |
| Package | 65nm: Flip-chip BGA |
| Core voltage | 1.0-1.15V (dynamic) |
| Thermal design | 3 heat pipes + fan |

### 8.3 Xenos GPU Specifications

| Feature | Details |
|---------|---------|
| Process | 90nm → 65nm → 45nm |
| Die size | ~182 mm² (90nm) |
| Transistors | ~232 million |
| GPU clock | 500 MHz |
| Shader clock | 500 MHz |
| Memory clock | 500 MHz (eDRAM), 700 MHz (GDDR3) |
| Shader performance | 240 GFLOPS |
| Fill rate | 16 GP/s (pixel), 4 GT/s (texture) |
| Polygon rate | 500M triangles/s |
| Tessellation | Hardware tessellation unit |

### 8.4 Memory Timings

| Operation | Latency | Throughput |
|-----------|---------|------------|
| L1 Hit | 3-4 cycles | 1/cycle |
| L2 Hit | 20-30 cycles | ~200 GB/s |
| L2 Miss | 200-300 cycles | 22.4 GB/s |
| Memory Read | 200-400 cycles | 22.4 GB/s |
| Memory Write | 200-400 cycles | 22.4 GB/s |
| eDRAM Access | 10-20 cycles | 256 GB/s |

### 8.5 Bus Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Xbox 360 Bus Architecture                │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌───────────┐         ┌──────────────┐                     │
│  │   CPU     │◄───────►│   Memory     │                     │
│  │ (Xenon)   │  FSB    │ Controller   │                     │
│  │           │         │              │                     │
│  └───────────┘         └──────┬───────┘                     │
│       ▲                       │                             │
│       │                       │                             │
│       │                ┌──────┴───────┐                     │
│       │                │   512MB      │                     │
│       │                │   GDDR3      │                     │
│       │                └──────┬───────┘                     │
│       │                       │                             │
│       │              ┌────────┴────────┐                    │
│       │              │                 │                    │
│  ┌────┴────┐   ┌────┴────┐      ┌────┴────┐               │
│  │  GPU    │◄──┤ Front   │      │ Southbridge│             │
│  │ (Xenos) │   │ Side Bus│      │ (PCIe/USB)│              │
│  └─────────┘   └─────────┘      └─────────┘               │
│                                                              │
│  Bus Speeds:                                                 │
│  - CPU FSB: ~21.6 GB/s (to memory controller)               │
│  - GPU Memory: 22.4 GB/s shared                              │
│  - GPU eDRAM: 256 GB/s internal                              │
│  - Southbridge: 1 GB/s (PCIe x4)                             │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## 9. Boot Process and Security

### 9.1 Boot ROM and 1BL

The Xbox 360 boot process begins in secure ROM:

```
Boot Flow:
┌─────────────────────────────────────────────────────────┐
│  1BL - First Boot Loader (ROM)                          │
│  - Located at physical address 0xFFFF_FF00_0000         │
│  - Size: 32KB (encoded, effectively ~16KB)              │
│  - Immutable, stored in CPU mask ROM                    │
│  - Validates and decrypts 2BL                           │
│  - Initializes minimal hardware                         │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│  2BL - Second Boot Loader (NAND Flash)                  │
│  - Located in NAND flash (CB/CD)                        │
│  - Decrypted and verified by 1BL                        │
│  - Initializes more hardware                            │
│  - Loads and verifies 4BL/5BL                           │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│  4BL - Fourth Boot Loader                               │
│  - Kernel loader                                        │
│  - Sets up memory                                       │
│  - Loads hypervisor and kernel                          │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│  Hypervisor + Kernel (6BL/7BL)                          │
│  - Initializes full system                              │
│  - Sets up security                                     │
│  - Loads dashboard                                      │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│  Dashboard / Game                                       │
│  - User-mode execution                                  │
│  - Signed and verified                                  │
└─────────────────────────────────────────────────────────┘
```

### 9.2 Security Architecture

```
Xbox 360 Security Layers:

Layer 0: Hardware Root of Trust
├── CPU ROM (1BL) - Immutable
├── CPU JTAG - Disabled in production
├── eFUSEs - Store security settings
│   - Lock bits prevent downgrade
│   - CPU key unique per console
│   - Boot loader revocation
│
Layer 1: Secure Boot
├── Chain of trust from ROM
├── Each stage validates next
├── Cryptographic signatures
│   - RSA-2048 for verification
│   - AES-128 for decryption
│
Layer 2: Hypervisor Security
├── Shadow page tables
├── W^X enforcement
├── Code signing validation
│   - XEX files must be signed
│   - Title keys encrypted
│
Layer 3: Runtime Security
├── Memory protection
├── Handle restrictions
├── Syscall filtering
└── Debugger detection
```

### 9.3 eFUSE Configuration

| eFUSE Bank | Purpose |
|------------|---------|
| CPU Key | Unique console encryption key (256-bit) |
| Lock Bits | Prevent security downgrade |
| Boot Loader | Revoke compromised boot loaders |
| Debug | Enable debug features (devkits) |
| Region | Console region encoding |
| Odd Features | Odd hardware features |

### 9.4 XEX File Format

```
XEX (Xbox Executable) Structure:
┌────────────────────────────────────────┐
│ XEX Header                             │
│ - Magic: 0x58455832 ("XEX2")           │
│ - Module flags                         │
│ - PE header offset                     │
│ - Security header offset               │
│ - Import table offset                  │
├────────────────────────────────────────┤
│ Security Header                        │
│ - Image size                           │
│ - RSA signature                        │
│ - Image digest                         │
│ - Import table digest                  │
│ - Module ID                            │
│ - Title ID                             │
│ - Region mask                          │
│ - Media types                          │
│ - Page descriptor flags                │
├────────────────────────────────────────┤
│ Import Table                           │
│ - List of import libraries             │
│ - Function ordinals/names              │
├────────────────────────────────────────┤
│ Optional Headers                       │
│ - Execution ID                         │
│ - Game ratings                         │n│ - Resource info                        │
│ - Base address                         │
│ - Entry point                          │
├────────────────────────────────────────┤
│ PE Image (Raw)                         │
│ - Compressed or uncompressed           │
│ - Contains code and data               │
└────────────────────────────────────────┘
```

### 9.5 Memory Encryption

```
Memory Encryption Architecture:

Physical Memory (GDDR3)
         ▲
         │ Decrypt
         │
┌────────┴────────────────┐
│   Memory Controller      │
│   - Encryption engine    │
│   - Uses CPU key         │
└────────┬────────────────┘
         │
         │ Encrypted bus
         ▼
┌─────────────────────────┐
│   Hypervisor Access      │
│   - Controls encryption  │
│   - Manages keys         │
└─────────────────────────┘

Encryption Modes:
- None: Plain text (boot, some kernel)
- XOR: Simple XOR with key
- ROT: Rotate bits
- Full: AES-128 encryption

Note: Not all memory is encrypted for performance.
Typically: User code/data encrypted, kernel structures not.
```

---

## 10. VMX-128 Vector Unit

### 10.1 VMX-128 Overview

The Xenon extends standard PowerPC VMX (AltiVec) with 128-bit registers:

| Feature | Specification |
|---------|--------------|
| Registers | 128 × 128-bit VRs (vs 32 on standard VMX) |
| Organization | 3 × 128-bit banks per SMT thread |
| Data types | 8/16/32-bit integers, 32-bit floats |
| Operations | Vector arithmetic, logical, permute |
| Denormal handling | Flush-to-zero |
| Latency | 2-4 cycles for most ops |
| Throughput | 1 vector op per cycle |

### 10.2 Register Banks

```
VMX-128 Register Banks (per hardware thread):

Bank 0 (vr0-vr31):    Bank 1 (vr32-vr63):    Bank 2 (vr64-vr95):
┌─────────────┐       ┌─────────────┐        ┌─────────────┐
│   vr0-vr31  │       │  vr32-vr63  │        │  vr64-vr95  │
│  (Primary)  │       │  (Alias 0)  │        │  (Alias 1)  │
└─────────────┘       └─────────────┘        └─────────────┘
      │                     │                      │
      └─────────────────────┼──────────────────────┘
                            │
                    ┌─────────┴─────────┐
                    │  Physical Storage  │
                    │  (96 registers)    │
                    └───────────────────┘

Note: vr96-vr127 are transients (not saved/restored)
Special registers:
- vscr: Vector status/control register
- vrsave: VR save/restore mask
```

### 10.3 Data Types and Layout

```
128-bit Vector Register Layouts:

Byte (16 × 8-bit):
┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐
│ b0 │ b1 │ b2 │ b3 │ b4 │ b5 │ b6 │ b7 │ b8 │ b9 │ b10│ b11│ b12│ b13│ b14│ b15│
│127:120│119:112│111:104│103:96│95:88│87:80│79:72│71:64│63:56│55:48│47:40│39:32│31:24│23:16│15:8 │7:0 │
└────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┴────┘

Halfword (8 × 16-bit):
┌──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│    h0    │    h1    │    h2    │    h3    │    h4    │    h5    │    h6    │    h7    │
│ 127:112  │ 111:96   │  95:80   │  79:64   │  63:48   │  47:32   │  31:16   │   15:0   │
└──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┴──────────┘

Word (4 × 32-bit):
┌────────────────┬────────────────┬────────────────┬────────────────┐
│      w0        │      w1        │      w2        │      w3        │
│    127:96      │     95:64      │     63:32      │      31:0      │
└────────────────┴────────────────┴────────────────┴────────────────┘

Single-precision float (4 × 32-bit):
┌────────────────┬────────────────┬────────────────┬────────────────┐
│      f0        │      f1        │      f2        │      f3        │
│    127:96      │     95:64      │     63:32      │      31:0      │
└────────────────┴────────────────┴────────────────┴────────────────┘

Doubleword (2 × 64-bit) - limited support:
┌────────────────────────────────┬────────────────────────────────┐
│              d0                │              d1                │
│            127:64              │             63:0               │
└────────────────────────────────┴────────────────────────────────┘
```

### 10.4 VMX-128 Instruction Set

#### Vector Integer Arithmetic

```asm
; Add
vaddubm  vrD, vrA, vrB    ; Vector add unsigned byte modulo
vadduhm  vrD, vrA, vrB    ; Vector add unsigned halfword modulo
vadduwm  vrD, vrA, vrB    ; Vector add unsigned word modulo
vaddubs  vrD, vrA, vrB    ; Vector add unsigned byte saturate
vadduhs  vrD, vrA, vrB    ; Vector add unsigned halfword saturate
vadduws  vrD, vrA, vrB    ; Vector add unsigned word saturate
vaddsbs  vrD, vrA, vrB    ; Vector add signed byte saturate
vaddshs  vrD, vrA, vrB    ; Vector add signed halfword saturate
vaddsws  vrD, vrA, vrB    ; Vector add signed word saturate

; Subtract
vsububm  vrD, vrA, vrB    ; Vector subtract unsigned byte modulo
vsubuhm  vrD, vrA, vrB    ; Vector subtract unsigned halfword modulo
vsubuwm  vrD, vrA, vrB    ; Vector subtract unsigned word modulo
vsububs  vrD, vrA, vrB    ; Vector subtract unsigned byte saturate
vsubuhs  vrD, vrA, vrB    ; Vector subtract unsigned halfword saturate
vsubuws  vrD, vrA, vrB    ; Vector subtract unsigned word saturate
vsubsbs  vrD, vrA, vrB    ; Vector subtract signed byte saturate
vsubshs  vrD, vrA, vrB    ; Vector subtract signed halfword saturate
vsubsws  vrD, vrA, vrB    ; Vector subtract signed word saturate

; Multiply
vmuloub  vrD, vrA, vrB    ; Vector multiply odd unsigned byte
vmulouh  vrD, vrA, vrB    ; Vector multiply odd unsigned halfword
vmulouwm vrD, vrA, vrB    ; Vector multiply odd unsigned word (Xenon)
vmuleub  vrD, vrA, vrB    ; Vector multiply even unsigned byte
vmuleuh  vrD, vrA, vrB    ; Vector multiply even unsigned halfword
vmuleuwm vrD, vrA, vrB    ; Vector multiply even unsigned word (Xenon)
vmulosb  vrD, vrA, vrB    ; Vector multiply odd signed byte
vmulosh  vrD, vrA, vrB    ; Vector multiply odd signed halfword
vmulesb  vrD, vrA, vrB    ; Vector multiply even signed byte
vmulesh  vrD, vrA, vrB    ; Vector multiply even signed halfword

; Multiply-add
vmaddfp  vrD, vrA, vrB, vrC  ; Vector multiply-add float
vnmsubfp vrD, vrA, vrB, vrC  ; Vector negative multiply-sub float
```

#### Vector Floating-Point

```asm
; Basic arithmetic
vaddfp   vrD, vrA, vrB    ; Vector add float
vsubfp   vrD, vrA, vrB    ; Vector subtract float
vmulfp   vrD, vrA, vrB    ; Vector multiply float (Xenon specific)
vdivfp   vrD, vrA, vrB    ; Vector divide float (Xenon specific)
vsqrtfp  vrD, vrA         ; Vector square root float
vres     vrD, vrA         ; Vector reciprocal estimate
vrsqrtefp vrD, vrA        ; Vector reciprocal square root estimate

; Comparison
vcmpgtfp vrD, vrA, vrB    ; Vector compare greater-than float
vcmpgefp vrD, vrA, vrB    ; Vector compare greater-or-equal float
vcmpbfp  vrD, vrA, vrB    ; Vector compare bounds float
vcmpgtuw vrD, vrA, vrB    ; Vector compare greater-than unsigned word

; Conversion
vcsxwux  vrD, vrA         ; Convert signed word to unsigned word
vcuxwcs  vrD, vrA         ; Convert unsigned word to signed word
vcfpuxws vrD, vrA         ; Convert float to unsigned word (round toward 0)
vcsxwfp  vrD, vrA         ; Convert signed word to float
vcuxwfp  vrD, vrA         ; Convert unsigned word to float
```

#### Vector Logical and Permute

```asm
; Logical
vand     vrD, vrA, vrB    ; Vector AND
vor      vrD, vrA, vrB    ; Vector OR
vxor     vrD, vrA, vrB    ; Vector XOR
vnor     vrD, vrA, vrB    ; Vector NOR
vandc    vrD, vrA, vrB    ; Vector AND with complement
vorc     vrD, vrA, vrB    ; Vector OR with complement (Xenon)
nand     vrD, vrA, vrB    ; Vector NAND (Xenon)

; Permute/Shuffle
vperm    vrD, vrA, vrB, vrC  ; Vector permute (vrC = pattern)
vsldoi   vrD, vrA, vrB, SH   ; Vector shift left double by octet immediate
vrlb     vrD, vrA, vrB    ; Vector rotate left byte
vrlh     vrD, vrA, vrB    ; Vector rotate left halfword
vrlw     vrD, vrA, vrB    ; Vector rotate left word

; Pack/Unpack
vpkuhum  vrD, vrA, vrB    ; Vector pack unsigned halfword to unsigned byte
vpkuwum  vrD, vrA, vrB    ; Vector pack unsigned word to unsigned halfword
vpkshss  vrD, vrA, vrB    ; Vector pack signed halfword to signed byte saturate
vpkswss  vrD, vrA, vrB    ; Vector pack signed word to signed halfword saturate
vupkhsb  vrD, vrB         ; Vector unpack high signed byte
vupkhsh  vrD, vrB         ; Vector unpack high signed halfword
vupklsb  vrD, vrB         ; Vector unpack low signed byte
vupklsh  vrD, vrB         ; Vector unpack low signed halfword

; Merge
vmrghb   vrD, vrA, vrB    ; Vector merge high byte
vmrghh   vrD, vrA, vrB    ; Vector merge high halfword
vmrghw   vrD, vrA, vrB    ; Vector merge high word
vmrglb   vrD, vrA, vrB    ; Vector merge low byte
vmrglh   vrD, vrA, vrB    ; Vector merge low halfword
vmrglw   vrD, vrA, vrB    ; Vector merge low word

; Splat
vspltb   vrD, vrB, UIMM   ; Vector splat byte
vsplth   vrD, vrB, UIMM   ; Vector splat halfword
vspltw   vrD, vrB, UIMM   ; Vector splat word
vspltisb vrD, SIMM        ; Vector splat immediate signed byte
vspltish vrD, SIMM        ; Vector splat immediate signed halfword
vspltisw vrD, SIMM        ; Vector splat immediate signed word
```

#### Vector Load/Store

```asm
; Load
lvlx     vrD, rA, rB      ; Load vector left indexed
lvrx     vrD, rA, rB      ; Load vector right indexed
lvsl     vrD, rA, rB      ; Load vector for shift left
lvsr     vrD, rA, rB      ; Load vector for shift right

; Store
stvlx    vrS, rA, rB      ; Store vector left indexed
stvrx    vrS, rA, rB      ; Store vector right indexed

; Simplified (aligned only)
lvx      vrD, rA, rB      ; Load vector indexed (16-byte aligned)
stvx     vrS, rA, rB      ; Store vector indexed (16-byte aligned)
```

### 10.5 VSCR (Vector Status and Control Register)

| Bit | Name | Description |
|-----|------|-------------|
| 0 | SAT | Saturation flag (sticky) |
| 1-14 | - | Reserved |
| 15 | NJ | Non-Java mode (always 1 on Xenon) |
| 16-31 | - | Reserved |
| 32-63 | - | Reserved |
| 64-127 | - | Reserved |

```asm
; VSCR operations
mfvscr   vrD               ; Move from VSCR to vector register
mtvscr   vrB               ; Move to VSCR from vector register
```

### 10.6 VMX-128 Code Examples

```asm
; Example 1: Vector dot product (4D)
; Input: vr1 = vector A, vr2 = vector B
; Output: vr3 = dot product in all elements

vmulfp   vr3, vr1, vr2     ; vr3 = A * B (component-wise)
vspltw   vr4, vr3, 0       ; vr4 = [x, x, x, x]
vspltw   vr5, vr3, 1       ; vr5 = [y, y, y, y]
vspltw   vr6, vr3, 2       ; vr6 = [z, z, z, z]
vspltw   vr7, vr3, 3       ; vr7 = [w, w, w, w]
vaddfp   vr4, vr4, vr5     ; vr4 = [x+y, ...]
vaddfp   vr4, vr4, vr6     ; vr4 = [x+y+z, ...]
vaddfp   vr3, vr4, vr7     ; vr3 = [x+y+z+w, ...] = dot

; Example 2: Matrix-vector multiply (4x4)
; Matrix rows in vr10-vr13, vector in vr20
; Result in vr30

vmulfp   vr30, vr10, vr20  ; Row 0 * vector
vmulfp   vr31, vr11, vr20  ; Row 1 * vector
vmulfp   vr32, vr12, vr20  ; Row 2 * vector
vmulfp   vr33, vr13, vr20  ; Row 3 * vector

; Sum each row (horizontal sum)
; Using permute to shift and add
vspltw   vr4, vr30, 0
vspltw   vr5, vr30, 1
vspltw   vr6, vr30, 2
vspltw   vr7, vr30, 3
vaddfp   vr30, vr4, vr5
vaddfp   vr30, vr30, vr6
vaddfp   vr30, vr30, vr7   ; vr30[0] = sum of row 0
; (repeat for other rows)

; Example 3: Unaligned vector load
; Load 16 bytes from potentially unaligned address in r3

lvsl     vr10, 0, r3       ; Get shift pattern for alignment
lvx      vr11, 0, r3       ; Load left part (aligned down)
lvx      vr12, r3, r4      ; Load right part (aligned up)
vperm    vr13, vr11, vr12, vr10  ; Permute to align

; Example 4: 4x4 matrix transpose
; Input: vr0-vr3 contain rows
; Output: vr0-vr3 contain columns

vmrghw   vr4, vr0, vr1     ; Interleave high words
vmrglw   vr5, vr0, vr1     ; Interleave low words
vmrghw   vr6, vr2, vr3     ; Interleave high words
vmrglw   vr7, vr2, vr3     ; Interleave low words

vmrghw   vr0, vr4, vr6     ; First two columns
vmrglw   vr1, vr4, vr6     ; Last two columns
vmrghw   vr2, vr5, vr7     ; First two columns (lower)
vmrglw   vr3, vr5, vr7     ; Last two columns (lower)
```

---

## 11. Cache Architecture

### 11.1 Cache Hierarchy

```
Xenon Cache Hierarchy (per core):

┌─────────────────────────────────────────┐
│              L2 Cache (1MB)             │
│         8-way set associative           │
│         64-byte lines                   │
│         Unified (I+D)                   │
│         Inclusive of L1                 │
└─────────────┬───────────────────────────┘
              │
      ┌───────┴───────┐
      │               │
      ▼               ▼
┌─────────────┐ ┌─────────────┐
│  L1 I-Cache │ │  L1 D-Cache │
│   (32KB)    │ │   (32KB)    │
│  4-way      │ │  8-way      │
│  64B lines  │ │  64B lines  │
│  Write-through│ │ Write-back  │
└─────────────┘ └─────────────┘
```

### 11.2 Cache Specifications

| Property | L1 Instruction | L1 Data | L2 |
|----------|---------------|---------|-----|
| Size | 32KB | 32KB | 1MB |
| Associativity | 4-way | 8-way | 8-way |
| Line size | 64 bytes | 64 bytes | 128 bytes |
| Sets | 128 | 64 | 1024 |
| Tag bits | 44 | 44 | 35 |
| Write policy | N/A (read-only) | Write-through | Write-back |
| Allocation | Read | Read/Write | Read/Write |

### 11.3 Cache Coherency

The Xenon uses **snooping** for cache coherency:

```
Coherency Protocol (MOESI-like):

M - Modified (dirty, exclusive)
O - Owned (dirty, shared)
E - Exclusive (clean, exclusive)
S - Shared (clean, shared)
I - Invalid

Transitions:
- Read miss: Request from other caches or L2
- Write miss: Invalidate other copies
- Snoop hit: Update state based on transaction type

Snooping:
- All L1 caches snoop L2 requests
- L2 acts as coherency point
- Directory-based tracking in L2
- No broadcast snooping (scalable)
```

### 11.4 Cache Operations

```asm
; Data cache operations
dcbz     rA, rB          ; Data cache block zero (set to 0)
dcbf     rA, rB          ; Data cache block flush (writeback + invalidate)
dcbst    rA, rB          ; Data cache block store (writeback)
dcbi     rA, rB          ; Data cache block invalidate (no writeback)
dcbt     rA, rB          ; Data cache block touch (prefetch)
dcbtst   rA, rB          ; Data cache block touch for store

; Instruction cache operations
icbi     rA, rB          ; Instruction cache block invalidate
isync                    ; Instruction synchronize (context sync)
sync                     ; Synchronize (memory barrier)
eieio                    ; Enforce in-order execution I/O

; Usage examples:

; Clear cache line (fast zero)
dcbz     r0, r3          ; Zero 64 bytes at [r3]

; Flush modified data before DMA
dcbf     r0, r3          ; Flush cache line at [r3]
sync                     ; Ensure completion

; Invalidate instruction cache after code modification
icbi     r0, r3          ; Invalidate line at [r3]
isync                    ; Context synchronization

; Prefetch data
dcbt     r0, r3          ; Prefetch line at [r3] into cache
```

### 11.5 L2 Cache Organization

```
L2 Cache Line (128 bytes) Organization:

Line Address: Index [6:15], Tag [16:50]

Within a line (4 × 32-byte sectors):
┌─────────────────────────────────────────────────────────────┐
│ Sector 0 (32 bytes) │ Sector 1 (32 bytes) │ ECC (7 bits)   │
│  [0-31]             │  [32-63]            │ per sector     │
├─────────────────────────────────────────────────────────────┤
│ Sector 2 (32 bytes) │ Sector 3 (32 bytes) │ ECC (7 bits)   │
│  [64-95]            │  [96-127]           │ per sector     │
└─────────────────────────────────────────────────────────────┘

L2 can do sector-level allocation:
- Partial line fills possible
- Reduces bandwidth for small allocations
```

### 11.6 Cache Aliasing

```
Virtual Index, Physical Tag (VIPT) Caches:

L1 D-Cache: 8-way, 32KB, 64B lines
- Index: VA[20:26] (7 bits = 128 sets)
- Tag: PA[0:43] (44 bits)
- Alias issue: VA[20:26] = PA[20:26] required

L1 I-Cache: 4-way, 32KB, 64B lines
- Index: VA[21:26] (6 bits = 64 sets)
- Tag: PA[0:43]
- Smaller index = fewer alias issues

L2 Cache: 8-way, 1MB, 128B lines
- Index: PA[15:24] (10 bits = 1024 sets)
- Tag: PA[0:14] + PA[25:50]
- Physically indexed = no aliasing

Handling Aliases:
- Kernel uses page coloring
- VA bits [12:20] must match PA bits [12:20] for 4KB pages
- Ensures no cache aliases in L1
```

---

## 12. Performance Considerations for Emulation

### 12.1 Emulation Challenges

| Challenge | Impact | Mitigation Strategy |
|-----------|--------|-------------------|
| In-order pipeline | High penalty for dependencies | Accurate timing model or JIT optimization |
| SMT complexity | 6 threads, resource sharing | Core-per-thread or cooperative scheduling |
| VMX-128 | 128 vector registers | JIT compilation, vector code translation |
| Unified memory | CPU/GPU contention | Unified memory model, synchronization |
| Cache coherency | Snoop traffic | Coherency protocol simulation |
| Security hypervisor | Page table shadowing | Emulate HV behavior or HLE |
| GPU unified shaders | Dynamic allocation | Shader translation/recompilation |
| eDRAM | High bandwidth framebuffer | Framebuffer emulation or GPU acceleration |

### 12.2 CPU Emulation Strategies

#### 1. Interpreter (Accuracy-focused)

```cpp
// Example interpreter loop structure
void RunInterpreter() {
    while (running) {
        uint32_t pc = GetPC();
        uint32_t instr = ReadMemory32(pc);
        
        // Decode
        uint8_t opcode = (instr >> 26) & 0x3F;
        uint8_t ext_opcode = (instr >> 1) & 0x3FF; // For X-form
        
        // Execute
        switch (opcode) {
            case 0x1F: // X-form (integer)
                ExecuteXForm(instr, ext_opcode);
                break;
            case 0x04: // VMX
                ExecuteVMX(instr);
                break;
            // ... other opcodes
        }
        
        // Update timing
        TickTiming(GetInstructionLatency(instr));
        
        // Check interrupts
        if (CheckInterrupts()) {
            HandleInterrupt();
        }
    }
}
```

**Pros:** 100% accurate, easy debugging  
**Cons:** 100-1000x slower than native

#### 2. JIT Compilation (Performance-focused)

```cpp
// Example JIT structure
class JITCompiler {
public:
    void CompileBlock(uint32_t pc) {
        // Check if already compiled
        if (block_cache_[pc]) {
            ExecuteCompiledBlock(block_cache_[pc]);
            return;
        }
        
        // Compile new block
        CompiledBlock* block = new CompiledBlock();
        
        uint32_t current_pc = pc;
        while (!IsBlockEnd(current_pc)) {
            uint32_t instr = ReadMemory32(current_pc);
            EmitInstruction(instr, block);
            current_pc += 4;
        }
        
        EmitBlockEnd(block);
        
        // Cache and execute
        block_cache_[pc] = block;
        ExecuteCompiledBlock(block);
    }
    
private:
    void EmitInstruction(uint32_t instr, CompiledBlock* block) {
        uint8_t opcode = (instr >> 26) & 0x3F;
        
        switch (opcode) {
            case 0x0C: // addi
                // Emit x86_64 equivalent
                // mov eax, [gpr_base + ra*8]
                // add eax, imm
                // mov [gpr_base + rd*8], eax
                break;
            // ... other instructions
        }
    }
};
```

**Pros:** 10-50x faster than interpreter  
**Cons:** Complex, self-modifying code issues

#### 3. Cached Interpreter (Middle ground)

Decode once, cache decoded form:

```cpp
struct DecodedInstruction {
    void (*handler)(CPUState*, uint32_t);
    uint32_t opcode;
    uint8_t latency;
    bool branch;
};

DecodedInstruction* DecodeBlock(uint32_t pc) {
    if (decoded_cache_[pc]) return decoded_cache_[pc];
    
    std::vector<DecodedInstruction> block;
    uint32_t current_pc = pc;
    
    while (!IsBlockEnd(current_pc)) {
        uint32_t instr = ReadMemory32(current_pc);
        DecodedInstruction dec;
        dec.opcode = instr;
        dec.handler = GetHandler(instr);
        dec.latency = GetLatency(instr);
        dec.branch = IsBranch(instr);
        block.push_back(dec);
        current_pc += 4;
    }
    
    decoded_cache_[pc] = StoreBlock(block);
    return decoded_cache_[pc];
}
```

**Pros:** 5-10x faster than interpreter, simpler than JIT  
**Cons:** Still slower than JIT

### 12.3 SMT Emulation

```cpp
// SMT-aware core emulation
class SMTEngine {
    static constexpr int THREADS_PER_CORE = 2;
    
    struct ThreadState {
        uint64_t gpr[32];
        uint64_t lr, ctr;
        uint32_t cr, xer;
        uint128_t vr[128];  // VMX-128
        uint32_t pc;
        bool running;
    };
    
    ThreadState threads[THREADS_PER_CORE];
    SharedResources shared;  // L1, L2, execution units
    
public:
    void RunCycle() {
        // Round-robin or priority scheduling
        for (int i = 0; i < THREADS_PER_CORE; i++) {
            if (threads[i].running && CanExecute(i)) {
                ExecuteInstruction(&threads[i]);
            }
        }
        
        // Advance shared resources
        TickSharedResources();
    }
    
    bool CanExecute(int thread_id) {
        // Check resource availability
        // L1 port available?
        // Execution unit available?
        // No structural hazards?
        return shared.CanAllocate(thread_id);
    }
};
```

### 12.4 Memory Emulation Optimization

```cpp
// Fast memory access with TLB caching
class MemoryManager {
    // TLB cache - direct mapped or set associative
    struct TLBEntry {
        uint64_t vpn;        // Virtual page number
        uint64_t ppn;        // Physical page number
        uint32_t flags;      // R/W/X, cacheability
        bool valid;
    };
    
    static constexpr int TLB_SIZE = 128;
    TLBEntry itlb[TLB_SIZE];  // Instruction TLB
    TLBEntry dtlb[TLB_SIZE];  // Data TLB
    
public:
    template<bool IsWrite>
    uint8_t* TranslateFast(uint64_t va) {
        uint64_t vpn = va >> 12;
        int index = vpn % TLB_SIZE;
        
        TLBEntry& entry = IsWrite ? dtlb[index] : itlb[index];
        
        if (entry.valid && entry.vpn == vpn) {
            // TLB hit
            if (IsWrite && !(entry.flags & PAGE_WRITE)) {
                TrapDSI(va);  // Page fault
            }
            return physical_memory + (entry.ppn << 12) + (va & 0xFFF);
        }
        
        // TLB miss - slow path
        return TranslateSlow(va, IsWrite);
    }
};
```

### 12.5 GPU Emulation Strategies

#### 1. High-Level Emulation (HLE)

Intercept GPU commands, translate to host GPU:

```cpp
class GPUHLE {
    void ProcessCommandBuffer(uint32_t addr, uint32_t size) {
        Command* cmds = (Command*)(memory + addr);
        
        for (uint32_t i = 0; i < size / sizeof(Command); i++) {
            switch (cmds[i].type) {
                case CMD_DRAW_PRIMITIVE:
                    TranslateDraw(cmds[i]);
                    break;
                case CMD_SET_TEXTURE:
                    TranslateTexture(cmds[i]);
                    break;
                case CMD_SHADER_LOAD:
                    TranslateShader(cmds[i]);
                    break;
                // ...
            }
        }
    }
    
    void TranslateDraw(const Command& cmd) {
        // Convert Xenos primitive to host format
        // Handle eDRAM via host framebuffer
        // Translate shader microcode to GLSL/SPIR-V
    }
};
```

**Pros:** Fast, runs on host GPU  
**Cons:** Inaccurate, compatibility issues

#### 2. Low-Level Emulation (LLE)

Simulate Xenos GPU at register/command level:

```cpp
class GPULLE {
    struct XenosGPU {
        uint32_t registers[0x10000];  // MMIO registers
        uint8_t edram[10 * 1024 * 1024];  // 10MB eDRAM
        
        // Shader cores
        UnifiedShader shaders[48];
        
        // Texture units
        TextureUnit tex_units[16];
        
        // ROPs
        ROPUnit rops[8];
    } gpu;
    
public:
    void WriteRegister(uint32_t reg, uint32_t value) {
        gpu.registers[reg] = value;
        
        switch (reg) {
            case REG_RB_SURFACE_INFO:
                UpdateFramebufferConfig(value);
                break;
            case REG_PA_SU_SC_MODE_CNTL:
                UpdateRasterConfig(value);
                break;
            // ...
        }
    }
    
    void ProcessPrimitive(uint32_t type, uint32_t count) {
        // Run vertex shaders
        for (uint32_t i = 0; i < count; i++) {
            RunVertexShader(i);
        }
        
        // Setup and rasterize
        SetupPrimitive(type);
        Rasterize();
        
        // Run pixel shaders
        RunPixelShader();
        
        // ROP operations
        ROPBlend();
    }
};
```

**Pros:** Accurate  
**Cons:** Extremely slow without GPU acceleration

### 12.6 Shader Translation

```cpp
// Xenos shader microcode to host shader translation
class ShaderTranslator {
public:
    std::string TranslateToGLSL(const uint32_t* microcode, uint32_t size) {
        std::stringstream glsl;
        
        glsl << "#version 450 core\n";
        
        // Analyze shader type (vertex/pixel)
        bool is_vertex = IsVertexShader(microcode);
        
        // Generate inputs/outputs
        if (is_vertex) {
            glsl << GenerateVertexInputs();
            glsl << GenerateVertexOutputs();
        } else {
            glsl << GeneratePixelInputs();
            glsl << GeneratePixelOutputs();
        }
        
        // Generate uniforms (constants, samplers)
        glsl << GenerateUniforms();
        
        // Translate instructions
        glsl << "void main() {\n";
        
        for (uint32_t i = 0; i < size; i++) {
            auto instr = DecodeInstruction(microcode[i]);
            glsl << TranslateInstruction(instr);
        }
        
        glsl << "}\n";
        
        return glsl.str();
    }
    
private:
    std::string TranslateInstruction(const Instruction& instr) {
        switch (instr.opcode) {
            case OP_ADD:
                return fmt::format("  v{} = v{} + v{};\n",
                    instr.dst, instr.src0, instr.src1);
            case OP_MUL:
                return fmt::format("  v{} = v{} * v{};\n",
                    instr.dst, instr.src0, instr.src1);
            case OP_MAD:
                return fmt::format("  v{} = v{} * v{} + v{};\n",
                    instr.dst, instr.src0, instr.src1, instr.src2);
            case OP_TEX:
                return fmt::format("  v{} = texture(sampler{}, v{});\n",
                    instr.dst, instr.tex_unit, instr.tex_coord);
            // ... hundreds more
        }
    }
};
```

### 12.7 eDRAM Emulation

```cpp
// eDRAM framebuffer emulation
class EDRAMEmulator {
    // eDRAM layout simulation
    uint32_t color_base;
    uint32_t depth_base;
    uint32_t color_format;
    uint32_t depth_format;
    uint32_t resolution[2];
    
    // Host framebuffer for final output
    HostFramebuffer host_fb;
    
public:
    void Configure(uint32_t base, uint32_t format, uint32_t width, uint32_t height) {
        color_base = base;
        color_format = format;
        resolution[0] = width;
        resolution[1] = height;
        
        // Allocate host resources
        host_fb.Resize(width, height, TranslateFormat(format));
    }
    
    void WritePixel(uint32_t x, uint32_t y, uint32_t color) {
        // Simulate eDRAM write
        // Handle tiling format
        // Handle compression
        // Update host framebuffer
        
        uint32_t tiled_addr = TileAddress(x, y);
        edram[tiled_addr] = color;
        
        // Immediate host update or batch
        host_fb.Write(x, y, ConvertColor(color));
    }
    
    void ResolveToMemory(uint32_t dest_addr) {
        // eDRAM resolve operation (copy to main memory)
        // Handle format conversion
        // Handle MSAA resolve
        // Handle tiling/untile
        
        for (uint32_t y = 0; y < resolution[1]; y++) {
            for (uint32_t x = 0; x < resolution[0]; x++) {
                uint32_t edram_addr = TileAddress(x, y);
                uint32_t mem_addr = LinearAddress(x, y, dest_addr);
                
                memory[mem_addr] = Untile(edram[edram_addr]);
            }
        }
    }
    
private:
    uint32_t TileAddress(uint32_t x, uint32_t y) {
        // Xenos uses 32×32 pixel tiles for color, 64×64 for depth
        // Implement tiling algorithm
        uint32_t tile_x = x / 32;
        uint32_t tile_y = y / 32;
        uint32_t local_x = x % 32;
        uint32_t local_y = y % 32;
        
        return color_base + (tile_y * tiles_per_row + tile_x) * 32*32*4 +
               (local_y * 32 + local_x) * 4;
    }
};
```

### 12.8 Recommended Emulation Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Xbox 360 Emulator                         │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌────────────────────────────────────────────────────────┐│
│  │                    CPU Emulation                        ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐     ││
│  │  │   Core 0    │  │   Core 1    │  │   Core 2    │     ││
│  │  │ (2 threads) │  │ (2 threads) │  │ (2 threads) │     ││
│  │  │  ┌───────┐  │  │  ┌───────┐  │  │  ┌───────┐  │     ││
│  │  │  │  JIT  │  │  │  │  JIT  │  │  │  │  JIT  │  │     ││
│  │  │  │Backend│  │  │  │Backend│  │  │  │Backend│  │     ││
│  │  │  └───┬───┘  │  │  └───┬───┘  │  │  └───┬───┘  │     ││
│  │  └──────┼──────┘  └──────┼──────┘  └──────┼──────┘     ││
│  │         └─────────────────┼─────────────────┘           ││
│  │                          │                             ││
│  │                   ┌──────┴──────┐                       ││
│  │                   │ Shared L2 & │                       ││
│  │                   │  Coherency  │                       ││
│  │                   └─────────────┘                       ││
│  └──────────────────────────┬─────────────────────────────┘│
│                             │                              │
│  ┌──────────────────────────┼─────────────────────────────┐│
│  │                    GPU Emulation                        ││
│  │         ┌────────────────┴────────────────┐             ││
│  │         ▼                                 ▼             ││
│  │  ┌──────────────┐                ┌──────────────┐       ││
│  │  │   Command    │                │   Shader     │       ││
│  │  │  Processor   │◄──────────────►│  Translator  │       ││
│  │  └──────┬───────┘                └──────┬───────┘       ││
│  │         │                               │               ││
│  │         ▼                               ▼               ││
│  │  ┌──────────────┐                ┌──────────────┐       ││
│  │  │    eDRAM     │                │   Texture    │       ││
│  │  │  Emulation   │                │   Cache      │       ││
│  │  └──────┬───────┘                └──────┬───────┘       ││
│  │         │                               │               ││
│  │         └───────────────┬───────────────┘               ││
│  │                         ▼                               ││
│  │  ┌───────────────────────────────────────────────┐      ││
│  │  │         Host Graphics API (Vulkan/D3D12)      │      ││
│  │  │              or Software Renderer             │      ││
│  │  └───────────────────────────────────────────────┘      ││
│  └─────────────────────────────────────────────────────────┘│
│                                                              │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                    Memory Subsystem                      ││
│  │  - Unified 512MB address space                          ││
│  │  - Physical RAM + MMIO regions                          ││
│  │  - Shadow page table emulation                          ││
│  │  - TLB caching                                          ││
│  └─────────────────────────────────────────────────────────┘│
│                                                              │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                    I/O Devices                           ││
│  │  - Storage (HDD/DVD)                                    ││
│  │  - Audio (XMA decoder)                                  ││
│  │  - Network                                              ││
│  │  - Input                                                ││
│  └─────────────────────────────────────────────────────────┘│
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 12.9 Performance Targets

| Component | Target | Notes |
|-----------|--------|-------|
| CPU JIT | 50-100 MIPS | Per emulated core |
| SMT efficiency | 70%+ | Thread switching overhead |
| GPU HLE | 30-60 FPS | Depends on title complexity |
| GPU LLE | 5-15 FPS | Software renderer |
| Memory latency | <50 host ns | TLB + fast path critical |
| Shader compilation | <10ms | Pipeline stalls if too slow |

### 12.10 Debugging and Profiling

```cpp
// Performance counters and debugging
class Profiler {
    struct Counter {
        uint64_t count;
        uint64_t cycles;
        const char* name;
    };
    
    Counter counters[256];
    
public:
    void RecordEvent(uint32_t id, uint64_t cycles) {
        counters[id].count++;
        counters[id].cycles += cycles;
    }
    
    void DumpStats() {
        for (int i = 0; i < 256; i++) {
            if (counters[i].count > 0) {
                printf("%s: %lu calls, %lu cycles avg\n",
                    counters[i].name,
                    counters[i].count,
                    counters[i].cycles / counters[i].count);
            }
        }
    }
};

// Useful metrics to track:
// - JIT code cache hit rate
// - TLB hit rate
// - SMT thread utilization
// - GPU command buffer processing time
// - Shader translation cache hit rate
// - eDRAM resolve frequency
// - Memory bandwidth utilization
```

---

## Appendix A: Instruction Latency Reference

| Category | Instruction | Latency | Throughput |
|----------|-------------|---------|------------|
| Integer | add/sub | 1 | 2/cycle |
| Integer | mul (32-bit) | 3-4 | 1/cycle |
| Integer | mul (64-bit) | 4-5 | 1/2 cycle |
| Integer | div | 20-40 | 1/40 cycle |
| Load | L1 hit | 3-4 | 1/cycle |
| Load | L2 hit | 20-30 | 1/cycle |
| Store | L1 | 3-4 | 1/cycle |
| Branch | taken | 4-6 | 1/cycle |
| Branch | not taken | 1 | 2/cycle |
| FPU | add/sub | 6 | 1/2 cycle |
| FPU | mul | 6 | 1/2 cycle |
| FPU | div | 20-40 | 1/40 cycle |
| VMX | add/sub | 4 | 1/cycle |
| VMX | mul | 4 | 1/cycle |
| VMX | permute | 4 | 1/cycle |
| VMX | load | 4-6 | 1/cycle |
| VMX | store | 4-6 | 1/cycle |

## Appendix B: Memory Map Details

| Address Range | Size | Purpose |
|---------------|------|---------|
| 0x0000_0000_0000-0x0000_0000_FFFF | 64KB | Exception vectors |
| 0x0000_0001_0000-0x0000_7FFF_FFFF | ~2GB | Kernel space |
| 0x0000_8000_0000-0x0000_FFFF_FFFF | 2GB | User space (per process) |
| 0x0001_0000_0000-0x0001_FFFF_FFFF | 4GB | Physical RAM mirror |
| 0x0002_0000_0000-0x0002_0FFF_FFFF | 256MB | GPU MMIO |
| 0x0002_1000_0000-0x0002_1FFF_FFFF | 256MB | PCI config |
| 0x0003_0000_0000-0x0003_FFFF_FFFF | 4GB | I/O space |
| 0x8000_0000_0000+ | - | Kernel virtual (extended) |

## Appendix C: GPU Register Reference

Key GPU register ranges:

| Range | Purpose |
|-------|---------|
| 0x0000-0x0FFF | RB - Render Backend (eDRAM, blending) |
| 0x1000-0x1FFF | PA - Primitive Assembly |
| 0x2000-0x2FFF | PC - Primitive Cache |
| 0x3000-0x3FFF | VGT - Vertex Group Transfer |
| 0x4000-0x4FFF | SQ - Sequencer (shader control) |
| 0x5000-0x5FFF | SPI - Shader Pipe Interpolator |
| 0x6000-0x6FFF | SX - Surface Export |
| 0x8000-0x8FFF | CB - Color Buffer |
| 0x9000-0x9FFF | DB - Depth Buffer |
| 0xA000-0xAFFF | CR - Command Router |
| 0xB000-0xBFFF | CP - Command Processor |
| 0xC000-0xCFFF | TC - Texture Cache |
| 0xD000-0xDFFF | TF - Texture Fetch |
| 0xE000-0xEFFF | VR - Vertex Request |

---

## Document Information

**Version:** 1.0  
**Last Updated:** 2026-01-31  
**Author:** PCSXr360 Documentation Team  
**Purpose:** Kernel-level technical reference for Xbox 360 emulation development

**References:**
- IBM PowerPC 2.02 Architecture Book
- Xenon Software Development Kit (XDK) documentation
- Free60 Project Wiki
- Xenia Emulator source code and documentation
- Various reverse engineering documentation from the Xbox 360 homebrew community

**Disclaimer:** This document is for educational and emulator development purposes only. The Xbox 360 and its components are proprietary products of Microsoft Corporation.
