# PlayStation 1 (PSX) - Arquitetura Técnica de Nível Kernel

## Visão Geral do Sistema

O PlayStation 1 utiliza uma arquitetura baseada no processador **MIPS R3000A** (LSI CoreWare CW33300), operando a **33.8688 MHz**. É um sistema little-endian com 2MB de RAM principal e arquitetura de coprocessadores altamente integrada.

## 1. CPU - MIPS R3000A (CW33300)

### 1.1 Especificações do Processador
- **Arquitetura**: MIPS R3000A (MIPS I ISA)
- **Clock**: 33.8688 MHz
- **Cache L1**: 4KB instrução + 1KB dados (SRAM não-associativo)
- **Pipeline**: 5 estágios (IF, ID, EX, MEM, WB)
- **Modos de Operação**: Kernel (KSEG0/KSEG1) e Usuário (KUSEG)
- **Endianness**: Little-endian

### 1.2 Registradores de Proposito Geral (GPR)

```
R0   ($zero)  - Sempre zero (hardwired)
R1   ($at)    - Assembler temporário
R2-3 ($v0-1)  - Valores de retorno
R4-7 ($a0-3)  - Argumentos de função
R8-15($t0-7)  - Temporários (caller-saved)
R16-23($s0-7) - Saved (callee-saved)
R24-25($t8-9) - Temporários adicionais
R26-27($k0-1) - Kernel reserved (não usar)
R28  ($gp)    - Global pointer
R29  ($sp)    - Stack pointer
R30  ($fp/s8) - Frame pointer / Saved
R31  ($ra)    - Return address
```

### 1.3 Registradores Especiais

**HI/LO**: Registradores de 32-bit para multiplicação/divisão
- `HI`: Resultado alto / Resto
- `LO`: Resultado baixo / Quociente

**PC**: Program Counter (alinhado em 4 bytes)

### 1.4 Formatos de Instrução MIPS I

```
Tipo R (Register):
| opcode(6) | rs(5) | rt(5) | rd(5) | shamt(5) | funct(6) |

Tipo I (Immediate):
| opcode(6) | rs(5) | rt(5) | immediate(16) |

Tipo J (Jump):
| opcode(6) | target address(26) |
```

### 1.5 Instruções Principais (Opcode Reference)

#### ALU Instruções
```mips
ADD     rd, rs, rt      # Add com overflow trap
ADDU    rd, rs, rt      # Add sem overflow
SUB     rd, rs, rt      # Subtract com overflow
SUBU    rd, rs, rt      # Subtract sem overflow
AND     rd, rs, rt      # AND lógico
OR      rd, rs, rt      # OR lógico
XOR     rd, rs, rt      # XOR lógico
NOR     rd, rs, rt      # NOR lógico
SLT     rd, rs, rt      # Set less than (signed)
SLTU    rd, rs, rt      # Set less than unsigned
SLL     rd, rt, sa      # Shift left logical
SRL     rd, rt, sa      # Shift right logical
SRA     rd, rt, sa      # Shift right arithmetic
SLLV    rd, rt, rs      # Shift left logical variable
SRLV    rd, rt, rs      # Shift right logical variable
SRAV    rd, rt, rs      # Shift right arithmetic variable
```

#### Instruções Imediatas
```mips
ADDI    rt, rs, imm     # Add immediate com overflow
ADDIU   rt, rs, imm     # Add immediate sem overflow
ANDI    rt, rs, imm     # AND immediate
ORI     rt, rs, imm     # OR immediate
XORI    rt, rs, imm     # XOR immediate
SLTI    rt, rs, imm     # Set less than immediate
SLTIU   rt, rs, imm     # Set less than immediate unsigned
LUI     rt, imm         # Load upper immediate
```

#### Load/Store
```mips
LB      rt, offset(rs)  # Load byte (signed)
LBU     rt, offset(rs)  # Load byte unsigned
LH      rt, offset(rs)  # Load halfword (signed)
LHU     rt, offset(rs)  # Load halfword unsigned
LW      rt, offset(rs)  # Load word
SB      rt, offset(rs)  # Store byte
SH      rt, offset(rs)  # Store halfword
SW      rt, offset(rs)  # Store word
LWL     rt, offset(rs)  # Load word left (unaligned)
LWR     rt, offset(rs)  # Load word right (unaligned)
SWL     rt, offset(rs)  # Store word left (unaligned)
SWR     rt, offset(rs)  # Store word right (unaligned)
```
⚠️ **Load Delay Slot**: Instruções load têm 1 ciclo de delay antes do dado estar disponível!

#### Multiplicação e Divisão
```mips
MULT    rs, rt          # Multiply (signed) - resultado em HI/LO
MULTU   rs, rt          # Multiply unsigned
DIV     rs, rt          # Divide (signed)
DIVU    rs, rt          # Divide unsigned
MFHI    rd              # Move from HI
MFLO    rd              # Move from LO
MTHI    rs              # Move to HI
MTLO    rs              # Move to LO
```
⏱️ Latência: MULT/MULTU = 12 ciclos, DIV/DIVU = 36 ciclos

#### Branches e Jumps
```mips
BEQ     rs, rt, offset  # Branch if equal
BNE     rs, rt, offset  # Branch if not equal
BLEZ    rs, offset      # Branch if <= zero
BGTZ    rs, offset      # Branch if > zero
BLTZ    rs, offset      # Branch if < zero
BGEZ    rs, offset      # Branch if >= zero
BLTZAL  rs, offset      # Branch if < zero and link
BGEZAL  rs, offset      # Branch if >= zero and link
J       target          # Jump
JAL     target          # Jump and link
JR      rs              # Jump register
JALR    rs, rd          # Jump and link register
```
⚠️ **Branch Delay Slot**: A instrução após um branch SEMPRE executa!

#### Coprocessador 0 (Sistema)
```mips
MFC0    rt, rd          # Move from cop0
MTC0    rt, rd          # Move to cop0
RFE                     # Return from exception
TLBR                    # Read TLB entry
TLBWI                   # Write TLB entry (indexed)
TLBWR                   # Write TLB entry (random)
TLBP                    # Probe TLB
```

## 2. System Control Coprocessor (COP0)

O COP0 gerencia memória virtual, exceções, interrupções e breakpoints.

### 2.1 Registradores COP0

| Reg | Nome | Descrição |
|-----|------|-----------|
| 0   | Index | Indexador para TLB operations |
| 1   | Random | Indexador randômico para TLBWR |
| 2   | EntryLo | Entrada TLB low (bits físicos) |
| 4   | Context | Ponteiro para tabela de páginas |
| 8   | BadVAddr | Endereço virtual inválido |
| 10  | EntryHi | Entrada TLB high (bits virtuais) |
| 11  | Compare | Timer compare para interrupção |
| 12  | SR (Status) | Registrador de status |
| 13  | Cause | Causa da última exceção |
| 14  | EPC | Endereço de retorno da exceção |
| 15  | PRId | Processor ID |

### 2.2 Status Register (SR - cop0r12)

```
Bit 0-1:  CU0-1  - Coprocessor enable (CU0=kernel, CU1+2=GTE)
Bit 2:    -      - Reservado
Bit 3:    BEV    - Boot exception vectors (1=ROM, 0=RAM)
Bit 4-5:  -      - Reservado
Bit 6-7:  IM0-1  - Interrupt mask (software)
Bit 8-15: IM2-9  - Interrupt mask (hardware)
Bit 16-27: -     - Reservado
Bit 28:   RE     - Reverse endian (user mode)
Bit 29-30: -     - Reservado  
Bit 31:   -      - Reservado
```

### 2.3 Cause Register (cop0r13)

```
Bit 0-1:  -      - Reservado
Bit 2-6:  ExcCode - Código de exceção
Bit 7:    -      - Reservado
Bit 8-15: IP0-7  - Interrupt pending bits
Bit 16-27: -     - Reservado
Bit 28:   CE     - Coprocessor error
Bit 29-30: -     - Reservado
Bit 31:   BD     - Branch delay slot
```

**Exception Codes:**
- 0: Interrupt (I)
- 1: TLB modification
- 2: TLB load miss
- 3: TLB store miss
- 4: Address error load
- 5: Address error store
- 6: Bus error instruction
- 7: Bus error data
- 8: Syscall
- 9: Breakpoint
- 10: Reserved instruction
- 11: Coprocessor unusable
- 12: Arithmetic overflow
- 13: Trap

## 3. Mapa de Memória

### 3.1 Regiões de Memória Virtual

```
KUSEG (User):     0x00000000 - 0x7FFFFFFF (2GB) - Mapped, Cached
KSEG0 (Kernel):   0x80000000 - 0x9FFFFFFF (512MB) - Unmapped, Cached
KSEG1 (Kernel):   0xA0000000 - 0xBFFFFFFF (512MB) - Unmapped, Uncached
KSEG2 (Kernel):   0xC0000000 - 0xFFFFFFFF (1GB) - Mapped, Cached
```

### 3.2 Layout Físico de Memória

```
0x00000000 - 0x001FFFFF: Main RAM (2MB) [KUSEG: 0x00000000, KSEG0: 0x80000000]
0x1F000000 - 0x1F7FFFFF: Expansion Region (ROM/RAM)
0x1F800000 - 0x1F8003FF: Scratchpad RAM (1KB D-Cache)
0x1F801000 - 0x1F801FFF: I/O Ports (Hardware Registers)
0x1F802000 - 0x1F802FFF: Expansion 2 (Reserved)
0x1FC00000 - 0x1FC7FFFF: BIOS ROM (512KB) [KSEG1: 0xBFC00000]
```

## 4. Geometry Transformation Engine (GTE)

O GTE é um coprocessador matemático integrado para operações 3D.

### 4.1 Registradores de Dados (cop2r0-31)

```
VXY0 (r0-1):   Vetor 0 (X,Y,Z) - 16-bit each
VXY1 (r2-3):   Vetor 1 (X,Y,Z) - 16-bit each
VXY2 (r4-5):   Vetor 2 (X,Y,Z) - 16-bit each
RGB  (r6):     Cor/Fog (R,G,B,CD) - 8-bit each
OTZ  (r7):     Z-value para ordering table
IR0  (r8):     Accumulator 0 (16-bit)
IR1-3 (r9-11): Accumulators 1-3 (32-bit)
SXY0 (r12-13): Screen XY 0
SXY1 (r14-15): Screen XY 1
SXY2 (r16-17): Screen XY 2
SXYP (r18):    Screen XY previous
SZ0-3 (r19-22): Z-depth values
RGB0-2 (r23-25): Color FIFO
MAC0-3 (r26-29): Math accumulators
IRGB (r30):    RGB convert input
ORGB (r31):    RGB convert output
```

### 4.2 Registradores de Controle (cop2r32-63)

```
RT11-33 (r32-36): Rotation matrix (3x3) - 16-bit each element
TRX-Z   (r37-39): Translation vector - 32-bit each
L11-33  (r40-44): Light matrix (3x3) - 16-bit each
RBK-GBK-BBK (r45-47): Background color - 32-bit each
LR1-3,LG1-3,LB1-3 (r48-52): Light color matrix
RFC-GFC-BFC (r53-55): Far color - 32-bit each
OFX-Y   (r56-57): Screen offset - 32-bit each
H       (r58):    Projection plane distance - 16-bit
DQA     (r59):    Depth cueing parameter A - 16-bit
DQB     (r60):    Depth cueing parameter B - 32-bit
ZSF3-4  (r61-62): Z scale factors - 16-bit
FLAG    (r63):    Error flags
```

### 4.3 Comandos GTE Principais

```
COP2 0x180001: RTPS - Perspective transformation (single)
COP2 0x280030: RTPT - Perspective transformation (triple)
COP2 0x040010: DPCS - Depth cue color (single)
COP2 0x0C0010: DPCT - Depth cue color (triple)
COP2 0x108041: MVMVA - Multiply vector by matrix
COP2 0x140000: CC   - Color color
COP2 0x15802D: CDP  - Color depth cue
COP2 0x168002: NCLIP - Normal clipping
COP2 0x178003: AVSZ3  - Average Z (3 vertices)
COP2 0x178004: AVSZ4  - Average Z (4 vertices)
COP2 0x198003: NCS   - Normal color (single)
COP2 0x1A8004: NCT   - Normal color (triple)
COP2 0x1E8043: NCDS  - Normal color depth cue (single)
COP2 0x1F8044: NCDT  - Normal color depth cue (triple)
```

## 5. Graphics Processing Unit (GPU)

### 5.1 Portas de I/O

```
1F801810h (GP0):  Comandos de renderização e dados
1F801814h (GP1):  Comandos de controle e status
```

### 5.2 Comandos GP0 (Renderização)

```
00h: NOP
01h: Clear Cache
02h: Fill Rectangle in VRAM
03h-1Ch: Reserved
1Fh: Interrupt Request (IRQ1)
20h: Monochrome Triangle (flat)
22h: Monochrome Triangle (flat, semi-transparent)
24h: Textured Triangle (flat)
25h: Textured Triangle (flat, blended)
26h: Textured Triangle (flat, raw)
27h: Textured Triangle (flat, blended+raw)
28h: Monochrome Quad (flat)
2Ah: Monochrome Quad (flat, semi-transparent)
2Ch: Textured Quad (flat)
2Dh: Textured Quad (flat, blended)
30h: Shaded Triangle (gouraud)
32h: Shaded Triangle (gouraud, semi-transparent)
34h: Textured Triangle (gouraud)
38h: Shaded Quad (gouraud)
3Ch: Textured Quad (gouraud)
40h-5Fh: Line primitives
60h-7Fh: Sprite/Rectangle primitives
80h-9Fh: Transfer data (VRAM write)
A0h-BFh: Transfer data (VRAM read)
C0h-DFh: Transfer data (CPU write)
E0h-FFh: Environment commands
```

### 5.3 Comandos GP1 (Controle)

```
00h: Reset GPU
01h: Reset Command Buffer
02h: Acknowledge IRQ1
03h: Display Enable
04h: DMA Direction
05h: Display Area Start
06h: Display Horizontal Range
07h: Display Vertical Range
08h: Display Mode
09h-1Fh: Reserved
```

### 5.4 Modos de Display (GP1 08h)

```
Bit 0-1: Horizontal resolution (0=256, 1=320, 2=512, 3=640)
Bit 2:   Vertical resolution (0=240, 1=480, interlace)
Bit 3:   Video mode (0=NTSC, 1=PAL)
Bit 4:   Color depth (0=15bit, 1=24bit)
Bit 5:   Vertical interlace (0=off, 1=on)
Bit 6:   Display area (0=off, 1=on)
Bit 7:   Interrupt (0=off, 1=on)
```

## 6. Sound Processing Unit (SPU)

### 6.1 Visão Geral

- **Canais**: 24 canais ADPCM
- **Sample Rate**: Até 44.1 kHz
- **Memória**: 512KB RAM
- **Formato**: ADPCM (4-bit, compressed)
- **Efeitos**: Pitch modulation, ADSR, looping, reverb

### 6.2 Registradores de Voz (N = 0-23)

```
1F801C00h+N*10h: Volume Left (16-bit)
1F801C02h+N*10h: Volume Right (16-bit)
1F801C04h+N*10h: Sample Rate (16-bit)
1F801C06h+N*10h: Start Address (16-bit, em 8-byte blocks)
1F801C08h+N*10h: ADSR (32-bit)
1F801C0Ch+N*10h: ADSR Volume (16-bit)
1F801C0Eh+N*10h: Repeat Address (16-bit)
```

### 6.3 ADSR Envelope

```
Bit 0-4:  Attack rate (0-31)
Bit 5-8:  Attack shift (0-15)
Bit 9:    Attack direction (0=exponential, 1=linear)
Bit 10-13: Decay rate (0-15)
Bit 14-18: Sustain rate (0-31)
Bit 19-22: Sustain shift (0-15)
Bit 23:   Sustain direction (0=decrease, 1=increase)
Bit 24:   Sustain mode (0=linear, 1=exponential)
Bit 25-28: Release rate (0-15)
Bit 29-30: Release shift (0-3)
Bit 31:   Release direction (0=exponential, 1=linear)
```

### 6.4 Registradores de Controle

```
1F801D80h: Main Volume Left
1F801D82h: Main Volume Right
1F801D84h: Reverb Volume Left
1F801D86h: Reverb Volume Right
1F801D88h: Voice ON (1 bit per voice, write)
1F801D8Ah: Voice OFF (1 bit per voice, write)
1F801D8Ch: Voice ON/OFF status (read)
1F801D8Eh: Voice 0..15 ENDX status (read)
1F801D90h: Unknown
1F801D92h: Voice 16..23 ENDX status (read)
```

## 7. DMA Controller

### 7.1 Canais DMA

```
Canal 0: MDECin (Input para Motion Decoder)
Canal 1: MDECout (Output do Motion Decoder)
Canal 2: GPU (comandos e VRAM)
Canal 3: CDROM (leitura)
Canal 4: SPU (som)
Canal 5: PIO (expansão paralela)
Canal 6: OTC (Ordering Table clear)
Canal 7: Reservado
```

### 7.2 Registradores DMA (Base = 1F8010X0h)

```
+0h (MADR): Memory Address
+4h (BCR):  Block Control
            Bits 0-15: Blocksize
            Bits 16-31: Number of blocks
+8h (CHCR): Channel Control
            Bit 0: Start/busy
            Bit 8-9: Direction (0=to memory, 1=from memory, 2=none)
            Bit 10: Memory step (0=increment, 1=decrement)
            Bit 16-17: Sync mode (0=burst, 1=request, 2=linked list)
            Bit 24: Enable IRQ
            Bit 28-30: Unknown
            Bit 31: Force start
```

### 7.3 Registradores de Controle DMA

```
1F8010F0h (DPCR): DMA Priority/Enable
1F8010F4h (DICR): DMA Interrupt Control
```

## 8. Timer/Root Counters

### 8.1 Registradores de Timer

```
Timer 0 (1F801100h): Root Counter 0 (Sysclk ou Dotclk)
Timer 1 (1F801110h): Root Counter 1 (Sysclk ou H-blank)
Timer 2 (1F801120h): Root Counter 2 (Sysclk ou Sysclk/8)
```

### 8.2 Estrutura dos Registradores

```
+0h (CURRENT): Valor atual do counter (halfword)
+4h (MODE):    Modo de operação
               Bit 0-1: Clock source
               Bit 2:   IRQ on overflow
               Bit 3:   IRQ on target
               Bit 4:   IRQ enable
               Bit 5:   Counter reset on target
               Bit 6:   Counter IRQ (read)
               Bit 7:   Counter reach target (read)
               Bit 8:   Counter overflow (read)
               Bit 9:   Timer enable
+8h (TARGET):  Valor alvo (halfword)
```

## 9. Sistema de Interrupções

### 9.1 Registradores de Interrupção

```
1F801070h (I_STAT): Status de interrupção (R=status, W=acknowledge)
1F801074h (I_MASK): Máscara de interrupção (R/W)
```

### 9.2 Fontes de Interrupção

```
Bit 0:  VBLANK (PAL=50Hz, NTSC=60Hz)
Bit 1:  GPU
Bit 2:  CDROM
Bit 3:  DMA
Bit 4:  TMR0 (Timer 0)
Bit 5:  TMR1 (Timer 1)
Bit 6:  TMR2 (Timer 2)
Bit 7:  Controller/Memory Card
Bit 8:  SIO
Bit 9:  SPU
Bit 10: Controller (lightpen) / PIO / DTL
Bit 11-15: Não usado (sempre 0)
```

### 9.3 Vetores de Exceção

```
BFC00180h: Vetor de exceção geral (modo kernel)
80000080h: Vetor de exceção geral (BEV=0)
```

## 10. CD-ROM Interface

### 10.1 Registradores

```
1F801800h (CD_INDEX): Index/Status
1F801801h (CD_DATA):  Data FIFO (quando index=0)
1F801802h (CD_REG):   Register write
1F801803h (CD_CMD):   Command FIFO / Status
```

### 10.2 Comandos CD-ROM

```
01h: Getstat
02h: Setloc
03h: Play
04h: Forward
05h: Backward
06h: ReadN
07h: Standby
08h: Stop
09h: Pause
0Ah: Reset
0Bh: Mute
0Ch: Demute
0Dh: Setfilter
0Eh: Setmode
0Fh: Getparam
10h: GetlocL
11h: GetlocP
12h: SetSession
13h: GetTN
14h: GetTD
15h: SeekL
16h: SeekP
17h: Setclock
18h: Getclock
19h: Test
1Ah: GetID
1Bh: ReadS
1Ch: ReadToc
```

## 11. BIOS e Kernel

### 11.1 Mapa de Memória do BIOS

```
0x00000000-0x0000FFFF: BIOS reserved (exception vectors)
0x00010000-0x001FFFFF: User memory
0xBFC00000-0xBFC7FFFF: BIOS ROM (512KB)
```

### 11.2 Syscalls BIOS (A0h-A2h)

O BIOS implementa 3 tabelas de funções:
- **A0h**: Funções principais do sistema
- **B0h**: Funções de biblioteca
- **C0h**: Funções de kernel

**Chamada via syscall:**
```mips
li   $t1, 0xA0       # ou 0xB0, 0xC0
li   $t2, function_number
jr   $t1
```

### 11.3 Funções Principais (A0h)

```
00h: no_care
01h: set_card_auto_format
02h: card_info_internal
03h: set_card_manual_format
...
3Ch: init_timer
3Dh: get_timer
3Eh: enable_timer_IRQ
3Fh: disable_timer_IRQ
...
```

### 11.4 Funções de Biblioteca (B0h)

```
00h: alloc_card_buffer
01h: free_card_buffer
02h: get_card_status
03h: wait_card_status
...
17h: DeliverEvent
18h: TestEvent
19h: EnableEvent
1Ah: DisableEvent
1Bh: OpenEvent
1Ch: CloseEvent
...
3Dh: get_system_info
...
```

### 11.5 Funções de Kernel (C0h)

```
00h: EnqueueTimerAndVblankIrqs
01h: EnqueueSyscallHandler
02h: SysEnqIntRP
03h: SysDeqIntRP
...
07h: InstallExceptionHandlers
08h: SysInitMemory
09h: SysInitKernelVariables
0Ah: ChangeThreadSubFunction
...
0Ch: InitDefInt
0Dh: SetIrqAutoAck
...
```

## 12. Controladores e Memory Cards

### 12.1 Portas de I/O

```
1F801040h (JOY_TXDATA):  Dados TX
1F801044h (JOY_RXDATA):  Dados RX
1F801048h (JOY_STAT):    Status
1F80104Ah (JOY_MODE):    Modo
1F80104Ch (JOY_CTRL):    Controle
1F80104Eh (JOY_BAUD):    Baud rate
```

### 12.2 Protocolo de Comunicação

```
Frame:  | Start | ID | Command | Data... | Stop |

Start:  0x81 (para controller 1) ou 0x88 (para controller 2)
ID:     0x00 = Digital, 0x01 = Analog, 0x03 = Namco
Command: 0x42 = Read pad
```

## 13. Dicas de Emulação

### 13.1 Precisão de Timing

- **CPU**: Execute instruções em blocos, mas verifique interrupções periodicamente
- **GPU**: Renderize por scanline ou frame completo
- **SPU**: Processe amostras a 44.1kHz (1 sample por ~22.7 µs)
- **DMA**: Respeite timings de transferência real
- **GTE**: Algumas operações têm latência específica

### 13.2 Problemas Comuns

1. **Load Delay Slots**: Não esqueça que loads têm 1 ciclo de delay!
2. **Branch Delay Slots**: A instrução após branch sempre executa
3. **DMA Linked List**: Usado pelo GPU para ordering tables
4. **GTE Flags**: Verifique FLAG register após operações
5. **SPU ADPCM**: Decode com atenção aos nibbles e repeat flags

### 13.3 Otimizações

- Use JIT compilation para CPU
- Cache GTE matrix operations
- Batch GPU primitives
- Stream SPU samples
- Lazy DMA evaluation

## Referências

1. PSXSPX Documentation (Nocash) - https://psx-spx.consoledev.net
2. MIPS R3000A Manual (LSI)
3. PlayStation Hardware Manual (Sony)
4. "Everything You Have Always Wanted to Know about the Playstation"
5. jsgroth's PS1 Emulation Blog

---
*Documentação técnica para PCSXR360 - Emulador PS1 para Xbox 360*
