# MIPS R3000A - Opcode Reference Guide

## Instruções por Categoria

### 1. ALU Instruções (Tipo R e I)

#### Operações Aritméticas (Signed)
```
Opcode    | Mnemonic    | Operation
----------|-------------|----------------------------------------
000000    | ADD rd,rs,rt| rd = rs + rt (overflow trap)
000000    | SUB rd,rs,rt| rd = rs - rt (overflow trap)
001000    | ADDI rt,rs,i| rt = rs + imm16 (sign-extended, overflow)
001001    | ADDIU rt,rs,i| rt = rs + imm16 (sign-extended)
```

#### Operações Aritméticas (Unsigned)
```
Opcode    | Mnemonic     | Operation
----------|--------------|----------------------------------------
000000    | ADDU rd,rs,rt| rd = rs + rt
000000    | SUBU rd,rs,rt| rd = rs - rt
001001    | ADDIU rt,rs,i| rt = rs + imm16 (sign-extended)
```

#### Operações Lógicas
```
Opcode    | Mnemonic     | Operation
----------|--------------|----------------------------------------
000000    | AND rd,rs,rt | rd = rs & rt
000000    | OR rd,rs,rt  | rd = rs | rt
000000    | XOR rd,rs,rt | rd = rs ^ rt
000000    | NOR rd,rs,rt | rd = ~(rs | rt)
001100    | ANDI rt,rs,i | rt = rs & imm16 (zero-extended)
001101    | ORI rt,rs,i  | rt = rs | imm16 (zero-extended)
001110    | XORI rt,rs,i | rt = rs ^ imm16 (zero-extended)
```

#### Comparações (Set)
```
Opcode    | Mnemonic     | Operation
----------|--------------|----------------------------------------
000000    | SLT rd,rs,rt | rd = (rs < rt) ? 1 : 0 (signed)
000000    | SLTU rd,rs,rt| rd = (rs < rt) ? 1 : 0 (unsigned)
001010    | SLTI rt,rs,i | rt = (rs < imm16) ? 1 : 0 (signed)
001011    | SLTIU rt,rs,i| rt = (rs < imm16) ? 1 : 0 (unsigned)
```

### 2. Instruções de Shift

```
Opcode    | Mnemonic     | Operation
----------|--------------|----------------------------------------
000000    | SLL rd,rt,sa | rd = rt << sa (logical)
000000    | SRL rd,rt,sa | rd = rt >> sa (logical)
000000    | SRA rd,rt,sa | rd = rt >> sa (arithmetic)
000000    | SLLV rd,rt,rs| rd = rt << (rs & 0x1F)
000000    | SRLV rd,rt,rs| rd = rt >> (rs & 0x1F) (logical)
000000    | SRAV rd,rt,rs| rd = rt >> (rs & 0x1F) (arithmetic)
```

### 3. Multiplicação e Divisão

```
Opcode    | Mnemonic     | Operation                    | Cycles
----------|--------------|------------------------------|-------
000000    | MULT rs,rt   | HI:LO = rs * rt (signed)     | 12
000000    | MULTU rs,rt  | HI:LO = rs * rt (unsigned)   | 12
000000    | DIV rs,rt    | LO = rs / rt, HI = rs % rt   | 36
000000    | DIVU rs,rt   | LO = rs / rt, HI = rs % rt   | 36
000000    | MFHI rd      | rd = HI                      | 1
000000    | MFLO rd      | rd = LO                      | 1
000000    | MTHI rs      | HI = rs                      | 1
000000    | MTLO rs      | LO = rs                      | 1
```

### 4. Instruções de Load/Store

```
Opcode    | Mnemonic     | Operation
----------|--------------|----------------------------------------
100000    | LB rt,off(rs)| rt = mem8[rs+off] (sign-extended)
100100    | LBU rt,off(rs)| rt = mem8[rs+off] (zero-extended)
100001    | LH rt,off(rs)| rt = mem16[rs+off] (sign-extended)
100101    | LHU rt,off(rs)| rt = mem16[rs+off] (zero-extended)
100011    | LW rt,off(rs)| rt = mem32[rs+off]
100010    | LWL rt,off(rs)| Load word left (unaligned)
100110    | LWR rt,off(rs)| Load word right (unaligned)
101000    | SB rt,off(rs)| mem8[rs+off] = rt
101001    | SH rt,off(rs)| mem16[rs+off] = rt
101011    | SW rt,off(rs)| mem32[rs+off] = rt
101010    | SWL rt,off(rs)| Store word left (unaligned)
101110    | SWR rt,off(rs)| Store word right (unaligned)
```
⚠️ **Load Delay**: 1 ciclo de delay antes do dado estar disponível!

### 5. Branches e Jumps

#### Branches Condicionais
```
Opcode    | Mnemonic     | Condition              | Delay Slot
----------|--------------|------------------------|------------
000100    | BEQ rs,rt,o  | rs == rt               | Yes
000101    | BNE rs,rt,o  | rs != rt               | Yes
000001    | BGEZ rs,o    | rs >= 0                | Yes
000001    | BLTZ rs,o    | rs < 0                 | Yes
000001    | BGTZ rs,o    | rs > 0                 | Yes
000001    | BLEZ rs,o    | rs <= 0                | Yes
000001    | BGEZAL rs,o  | rs >= 0 (and link)     | Yes
000001    | BLTZAL rs,o  | rs < 0 (and link)      | Yes
```

#### Jumps Incondicionais
```
Opcode    | Mnemonic     | Operation              | Delay Slot
----------|--------------|------------------------|------------
000010    | J target     | PC = (PC & 0xF0000000) | Yes
          |              |   | (target << 2)      |
000011    | JAL target   | R31 = PC + 8; J        | Yes
000000    | JR rs        | PC = rs                | Yes
000000    | JALR rd,rs   | rd = PC + 8; PC = rs   | Yes
```

### 6. Instruções de Coprocessador

#### COP0 (Sistema)
```
Opcode    | Mnemonic     | Operation
----------|--------------|----------------------------------------
010000    | MFC0 rt,rd   | rt = cop0_reg[rd]
010000    | MTC0 rt,rd   | cop0_reg[rd] = rt
010000    | RFE          | Return from exception
010000    | TLBR         | Read TLB entry
010000    | TLBWI        | Write TLB entry (indexed)
010000    | TLBWR        | Write TLB entry (random)
010000    | TLBP         | Probe TLB
```

#### COP2 (GTE)
```
Opcode    | Mnemonic     | Operation
----------|--------------|----------------------------------------
010010    | MFC2 rt,rd   | rt = cop2_data[rd]
010010    | MTC2 rt,rd   | cop2_data[rd] = rt
010010    | CFC2 rt,rd   | rt = cop2_ctrl[rd]
010010    | CTC2 rt,rd   | cop2_ctrl[rd] = rt
010010    | COP2 opcode  | Execute GTE command
```

## Formato de Instruções

### Tipo R (Register)
```
31 26 25 21 20 16 15 11 10 6   5    0
+--------+--------+--------+--------+------+--------+
| opcode |   rs   |   rt   |   rd   | shamt|  funct |
+--------+--------+--------+--------+------+--------+
   6        5        5        5       5       6

Examples:
ADD $t0, $t1, $t2  =>  0x012A4020 (opcode=0, rs=$t1(9), rt=$t2(10), rd=$t0(8), shamt=0, funct=0x20)
SLL $t0, $t1, 4    =>  0x00094100 (opcode=0, rs=0, rt=$t1(9), rd=$t0(8), shamt=4, funct=0x00)
```

### Tipo I (Immediate)
```
31 26 25 21 20 16 15                    0
+--------+--------+--------+------------------------+
| opcode |   rs   |   rt   |       immediate        |
+--------+--------+--------+------------------------+
   6        5        5               16

Examples:
ADDI $t0, $t1, 100    =>  0x21280064 (opcode=0x08, rs=$t1(9), rt=$t0(8), imm=100)
LW $t0, 0($t1)        =>  0x8D280000 (opcode=0x23, rs=$t1(9), rt=$t0(8), offset=0)
BEQ $t0, $t1, label   =>  0x110900XX (opcode=0x04, rs=$t0(8), rt=$t1(9), offset=calculated)
```

### Tipo J (Jump)
```
31 26 25                                   0
+--------+------------------------------------------+
| opcode |              target address               |
+--------+------------------------------------------+
   6                       26

Calculation: target26 = (target_address & 0x0FFFFFFC) >> 2

Example:
J 0x00400000  =>  0x08100000 (opcode=0x02, target=0x00400000>>2=0x100000)
```

## Opcode Reference Table (Funct field for SPECIAL)

```
Funct   | Mnemonic | Description
--------|----------|------------------------------------------------
00h     | SLL      | Shift left logical
02h     | SRL      | Shift right logical
03h     | SRA      | Shift right arithmetic
04h     | SLLV     | Shift left logical variable
06h     | SRLV     | Shift right logical variable
07h     | SRAV     | Shift right arithmetic variable
08h     | JR       | Jump register
09h     | JALR     | Jump and link register
0Ch     | SYSCALL  | System call
0Dh     | BREAK    | Breakpoint
10h     | MFHI     | Move from HI
11h     | MTHI     | Move to HI
12h     | MFLO     | Move from LO
13h     | MTLO     | Move to LO
18h     | MULT     | Multiply (signed)
19h     | MULTU    | Multiply unsigned
1Ah     | DIV      | Divide (signed)
1Bh     | DIVU     | Divide unsigned
20h     | ADD      | Add (with overflow)
21h     | ADDU     | Add unsigned
22h     | SUB      | Subtract (with overflow)
23h     | SUBU     | Subtract unsigned
24h     | AND      | AND
25h     | OR       | OR
26h     | XOR      | XOR
27h     | NOR      | NOR
2Ah     | SLT      | Set less than (signed)
2Bh     | SLTU     | Set less than unsigned
```

## Branch Delay Slot

**IMPORTANTE**: A instrução imediatamente após um branch ou jump **SEMPRE** é executada, independente do resultado do branch!

```mips
# Exemplo 1: Branch taken
    BEQ $t0, $t1, target
    NOP           <- Esta instrução executa mesmo se branch for taken!
target:
    ...

# Exemplo 2: Pipeline hazard com load
    LW $t0, 0($a0)
    BEQ $t0, $zero, skip  <- PERIGO! $t0 ainda não foi carregado!
    NOP
skip:
    ...
    
# Correção:
    LW $t0, 0($a0)
    NOP                   <- Delay slot do load
    BEQ $t0, $zero, skip
    NOP
skip:
    ...
```

## Load Delay Slot

Instruções load (LB, LBU, LH, LHU, LW, LWL, LWR) têm **1 ciclo de delay** antes do dado estar disponível:

```mips
# Errado:
    LW $t0, 0($a0)
    ADD $t1, $t0, $t2    <- $t0 ainda tem valor antigo!

# Correto:
    LW $t0, 0($a0)
    NOP                  <- Delay slot
    ADD $t1, $t0, $t2    <- $t0 agora tem valor correto
```

**Exceção**: Se a instrução no delay slot for outro load/store, o resultado está disponível imediatamente para a 3ª instrução.

## Endianness

O PlayStation 1 usa **Little Endian**:

```
Endereço 0x1000: 0x12345678

Memória:
0x1000: 0x78 (LSB)
0x1001: 0x56
0x1002: 0x34
0x1003: 0x12 (MSB)
```

## Address Alignment

- **Word (32-bit)**: Alinhado em 4 bytes (endereço & 3 == 0)
- **Halfword (16-bit)**: Alinhado em 2 bytes (endereço & 1 == 0)
- **Byte**: Sem restrição

Acessos desalinhados geram **Address Error Exception**.

## Emulação Tips

1. **Pipeline Simulation**: Modelar 5 estágios (IF, ID, EX, MEM, WB)
2. **Delay Slots**: Implementar branch e load delay slots corretamente
3. **Coprocessor Timing**: GTE operations têm latência específica
4. **Exception Priority**: Checkar na ordem correta
5. **Self-Modifying Code**: Flush cache quando necessário

## Referências

1. MIPS R3000A Datasheet
2. "See MIPS Run" by Dominic Sweetman
3. PSXSPX Documentation

---
*Reference guide for PCSXR360 emulator development*
