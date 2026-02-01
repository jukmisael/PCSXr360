# Análise de Instruções e Otimização - PS1 vs Xbox 360

## Visão Geral Comparativa

| Aspecto | PS1 (MIPS R3000A) | Xbox 360 (Xenon) |
|---------|-------------------|------------------|
| **Arquitetura** | RISC MIPS I | RISC PowerPC |
| **Clock** | 33.87 MHz | 3.2 GHz |
| **Cores** | 1 | 3 (6 threads) |
| **Pipeline** | 5 estágios | 23 estágios (in-order) |
| **Cache L1** | 4KB I + 1KB D | 32KB I + 32KB D por core |
| **Cache L2** | Não tem | 1MB shared |
| **SIMD** | Não tem (GTE separado) | VMX-128 por core |

---

## 1. Instruções MIPS R3000A (PlayStation 1)

### 1.1 Categorias de Instruções

#### ALU Básicas (1 ciclo cada)
```mips
ADD     rd, rs, rt      # Add com overflow trap (1 ciclo)
ADDU    rd, rs, rt      # Add sem overflow (1 ciclo) ← **Preferível**
SUB     rd, rs, rt      # Subtract com overflow (1 ciclo)
SUBU    rd, rs, rt      # Subtract sem overflow (1 ciclo) ← **Preferível**
AND     rd, rs, rt      # AND lógico (1 ciclo)
OR      rd, rs, rt      # OR lógico (1 ciclo)
XOR     rd, rs, rt      # XOR lógico (1 ciclo)
NOR     rd, rs, rt      # NOR lógico (1 ciclo)
SLT     rd, rs, rt      # Set less than (1 ciclo)
SLTU    rd, rs, rt      # Set less than unsigned (1 ciclo)
```

**⚡ Otimização**: Use `ADDU`/`SUBU` em vez de `ADD`/`SUB` - evita exceções de overflow

#### Operações Imediatas (1 ciclo)
```mips
ADDI    rt, rs, imm     # Add immediate com overflow
ADDIU   rt, rs, imm     # Add immediate sem overflow ← **Mais rápido**
ANDI    rt, rs, imm     # AND immediate
ORI     rt, rs, imm     # OR immediate ← **Mais comum**
XORI    rt, rs, imm     # XOR immediate
SLTI    rt, rs, imm     # Set less than immediate
SLTIU   rt, rs, imm     # Set less than immediate unsigned
LUI     rt, imm         # Load upper immediate (shift left 16)
```

#### Shifts (1 ciclo)
```mips
SLL     rd, rt, sa      # Shift left logical imediato
SRL     rd, rt, sa      # Shift right logical imediato
SRA     rd, rt, sa      # Shift right arithmetic imediato
SLLV    rd, rt, rs      # Shift left logical variável
SRLV    rd, rt, rs      # Shift right logical variável
SRAV    rd, rt, rs      # Shift right arithmetic variável
```

**⚡ Otimização**: Shifts imediatos são mais rápidos que variáveis (não precisam ler registrador)

#### Multiplicação/Divisão (MULTIPLICADORES SLOW!)
```mips
MULT    rs, rt          # Multiply signed - 12 ciclos
MULTU   rs, rt          # Multiply unsigned - 12 ciclos
DIV     rs, rt          # Divide signed - 36 ciclos ← **MUITO LENTO**
DIVU    rs, rt          # Divide unsigned - 36 ciclos ← **MUITO LENTO**
MFHI    rd              # Move from HI - 1 ciclo
MFLO    rd              # Move from LO - 1 ciclo
MTHI    rs              # Move to HI - 1 ciclo
MTLO    rs              # Move to LO - 1 ciclo
```

**⚡ Otimização CRÍTICA**:
- **Evite DIV/MULT quando possível**
- Use shifts para multiplicar/dividir por potências de 2:
  - `x * 8` → `SLL $t0, $t1, 3`
  - `x / 4` → `SRL $t0, $t1, 2`
- Use lookup tables para divisão por constantes
- GTE pode fazer divisões mais rápido para 3D

#### Load/Store (1 ciclo + delay)
```mips
LB      rt, offset(rs)  # Load byte (sign-extend) - 1 ciclo + 1 delay
LBU     rt, offset(rs)  # Load byte unsigned - 1 ciclo + 1 delay
LH      rt, offset(rs)  # Load halfword - 1 ciclo + 1 delay
LHU     rt, offset(rs)  # Load halfword unsigned - 1 ciclo + 1 delay
LW      rt, offset(rs)  # Load word - 1 ciclo + 1 delay
LWL     rt, offset(rs)  # Load word left (unaligned)
LWR     rt, offset(rs)  # Load word right (unaligned)
SB      rt, offset(rs)  # Store byte - 1 ciclo
SH      rt, offset(rs)  # Store halfword - 1 ciclo
SW      rt, offset(rs)  # Store word - 1 ciclo
SWL     rt, offset(rs)  # Store word left
SWR     rt, offset(rs)  # Store word right
```

**⚡ Otimização**:
- **Evite acessos desalinhados** - são mais lentos
- **Aproveite load delay slot**: Execute algo útil enquanto espera o load
- Use LBU/LHU em vez de LB/LH quando não precisa de sinal

#### Branches (1 ciclo + delay slot)
```mips
BEQ     rs, rt, offset  # Branch if equal
BNE     rs, rt, offset  # Branch if not equal ← **Mais comum**
BLEZ    rs, offset      # Branch if <= zero
BGTZ    rs, offset      # Branch if > zero
BLTZ    rs, offset      # Branch if < zero
BGEZ    rs, offset      # Branch if >= zero
BLTZAL  rs, offset      # Branch if < zero and link
BGEZAL  rs, offset      # Branch if >= zero and link
J       target          # Jump incondicional
JAL     target          # Jump and link
JR      rs              # Jump register
JALR    rd, rs          # Jump and link register
```

**⚡ Otimização**:
- **Sempre preencha delay slots** - nunca use NOP se puder evitar
- Branches baseados em zero (BLEZ/BGTZ) são mais rápidos que BEQ/BNE (não precisam comparar 2 regs)
- JR é mais rápido que JALR para retornos (não salva PC)

---

## 2. Instruções PowerPC Xenon (Xbox 360)

### 2.1 Características do Pipeline

**⚠️ IMPORTANTE**: Xenon é in-order com 23 estágios!

| Unidade | Latência | Throughput | Notas |
|---------|----------|------------|-------|
| Integer ALU 0 | 2 ciclos | 1/ciclo | Geral |
| Integer ALU 1 | 2 ciclos | 1/ciclo | Geral |
| Branch Unit | 1 ciclo | 1/ciclo | + misprediction penalty |
| Load/Store | 3-4 ciclos | 1/ciclo | Cache hit |
| FPU (FP1) | 6 ciclos | 1/2 ciclos | Double-precision |
| VMX-128 ALU | 4 ciclos | 1/ciclo | **Vector integer/float** |
| VMX-128 Permute | 4 ciclos | 1/ciclo | **Shuffle/rearrange** |
| VMX-128 Simple | 2 ciclos | 1/ciclo | **Operações simples** |

**⚡ Otimização**: Latência de load-to-use é ~4-6 ciclos - organize código para evitar stalls!

### 2.2 Integer Instructions

#### Aritméticas (2 ciclos de latência)
```asm
add     rD, rA, rB          # rD = rA + rB
add.    rD, rA, rB          # Add + update CR (cond register)
addo    rD, rA, rB          # Add + overflow check (mais lento!)
addi    rD, rA, SIMM        # Add immediate (SIMM = signed 16-bit)
addis   rD, rA, SIMM        # Add immediate shifted (<< 16)
subf    rD, rA, rB          # rD = rB - rA (subtract from)
subf.   rD, rA, rB          # Subtract from + update CR
neg     rD, rA              # rD = -rA
```

**⚡ Otimização**:
- `addi` com rA=0 é usado para carregar constantes pequenas
- `addis` + `ori` para carregar constantes 32-bit
- Evite `addo`/`subfo` se não precisa de overflow

#### Lógicas (1 ciclo de latência)
```asm
and     rD, rA, rB          # rD = rA & rB
and.    rD, rA, rB          # And + update CR
or      rD, rA, rB          # rD = rA | rB
xor     rD, rA, rB          # rD = rA ^ rB
nand    rD, rA, rB          # rD = ~(rA & rB)
nor     rD, rA, rB          # rD = ~(rA | rB)
andi.   rD, rA, UIMM        # And immediate + update CR (UIMM = unsigned 16-bit)
ori     rD, rA, UIMM        # Or immediate
xori    rD, rA, UIMM        # Xor immediate
```

**⚡ Otimização**:
- Instruções com `.` atualizam Condition Register (CR) - use só quando necessário
- `andi.` é útil para testar flags (atualiza CR automaticamente)

#### Shifts e Rotates (1-2 ciclos)
```asm
slw     rD, rA, rB          # Shift left word (rB & 0x1F bits)
srw     rD, rA, rB          # Shift right word
sraw    rD, rA, rB          # Shift right algebraic word (preserva sinal)
srawi   rD, rA, SH          # Shift right arithmetic immediate
rlwinm  rD, rA, SH, MB, ME  # Rotate left + mask (EXTREMAMENTE ÚTIL!)
rlwimi  rD, rA, SH, MB, ME  # Rotate left + mask insert
slwi    rD, rA, SH          # Shift left word immediate (macro: rlwinm)
srwi    rD, rA, SH          # Shift right word immediate (macro: rlwinm)
```

**⚡ Otimização CRÍTICA**:
- `rlwinm` é a instrução mais poderosa do PowerPC!
- Pode fazer: shift, mask, extract, insert em UMA instrução
- Exemplo: `rlwinm r3, r3, 24, 0, 31` = roda 24 bits + mask 0-31
- Substitui múltiplas operações MIPS em uma única instrução

#### Comparações (atualizam CR)
```asm
cmpw    crfD, rA, rB        # Compare word (32-bit)
cmpwi   crfD, rA, SIMM      # Compare word immediate
cmpd    crfD, rA, rB        # Compare doubleword (64-bit)
cmplw   crfD, rA, rB        # Compare logical word (unsigned)
cmplwi  crfD, rA, UIMM      # Compare logical word immediate
```

**⚡ Otimização**:
- `crfD` especifica qual campo do CR (0-7)
- Campo 0 é o padrão para comparações
- Use `cmpl*` para comparações unsigned

### 2.3 Branch Instructions

#### Incondicionais
```asm
b       target              # Branch (PC-relative)
ba      target              # Branch absolute
bl      target              # Branch and link (chamada de função)
bla     target              # Branch and link absolute
blr                         # Branch to link register (retorno)
bctr                        # Branch to count register (switch)
```

#### Condicionais
```asm
bc      BO, BI, target      # Branch conditional (base)
beq     target              # Branch if equal (macro: cr0[EQ])
bne     target              # Branch if not equal
blt     target              # Branch if less than
ble     target              # Branch if less or equal
bgt     target              # Branch if greater than
bge     target              # Branch if greater or equal
```

**⚡ Otimização**:
- Xenon tem branch prediction - evite branches imprevisíveis (data-dependent)
- `blr` para retornos é muito eficiente
- Unroll loops pequenos para reduzir overhead de branch

### 2.4 Load/Store

```asm
# Bytes
lbz     rD, disp(rA)        # Load byte and zero (unsigned)
lbzx    rD, rA, rB          # Load byte indexed
stb     rS, disp(rA)        # Store byte
stbx    rS, rA, rB          # Store byte indexed

# Halfwords
lhz     rD, disp(rA)        # Load halfword and zero
lhax    rD, rA, rB          # Load halfword algebraic (signed)
lhzx    rD, rA, rB          # Load halfword and zero indexed
sth     rS, disp(rA)        # Store halfword
sthx    rS, rA, rB          # Store halfword indexed

# Words (32-bit)
lwz     rD, disp(rA)        # Load word and zero
lwzx    rD, rA, rB          # Load word indexed
stw     rS, disp(rA)        # Store word
stwx    rS, rA, rB          # Store word indexed
stwu    rS, disp(rA)        # Store word with update (rA += disp)

# Doublewords (64-bit)
ld      rD, disp(rA)        # Load doubleword
ldx     rD, rA, rB          # Load doubleword indexed
std     rS, disp(rA)        # Store doubleword
stdx    rS, rA, rB          # Store doubleword indexed

# Múltiplos
lmw     rD, disp(rA)        # Load multiple words
stmw    rS, disp(rA)        # Store multiple words

# Sincronização
lwarx   rD, rA, rB          # Load word and reserve (atomic)
stwcx.  rS, rA, rB          # Store word conditional
sync                        # Memory barrier
eieio                       # Enforce in-order execution I/O
isync                       # Instruction synchronize
```

**⚡ Otimização**:
- Load-to-use latency: 3-4 ciclos (cache hit)
- **Evite dependent chains de loads** - intercale com outros cálculos
- `stwu` útil para stack manipulation
- `lwarx`/`stwcx.` para operações atômicas

---

## 3. VMX-128 SIMD Instructions (MUITO IMPORTANTE!)

### 3.1 Visão Geral VMX-128

VMX-128 (AltiVec) é o diferencial do Xbox 360!

- **128-bit vectors**: 4× int32, 4× float32, 8× int16, 16× int8
- **128 registros**: v0-v127
- **Throughput**: 1 operação por ciclo (na maioria)
- **Latência**: 2-4 ciclos

### 3.2 Operações Aritméticas

```asm
# Adição
vaddubm     vD, vA, vB      # Add unsigned byte modulo (16 bytes)
vadduhm     vD, vA, vB      # Add unsigned halfword modulo (8 halfwords)
vadduwm     vD, vA, vB      # Add unsigned word modulo (4 words) ← **COMUM**
vaddubs     vD, vA, vB      # Add unsigned byte saturate
vadduhs     vD, vA, vB      # Add unsigned halfword saturate
vadduws     vD, vA, vB      # Add unsigned word saturate
vaddsbs     vD, vA, vB      # Add signed byte saturate
vaddshs     vD, vA, vB      # Add signed halfword saturate
vaddsws     vD, vA, vB      # Add signed word saturate
vaddfp      vD, vA, vB      # Add floating-point (4 floats)

# Subtração
vsububm     vD, vA, vB      # Subtract unsigned byte modulo
vsubuhm     vD, vA, vB      # Subtract unsigned halfword modulo
vsubuwm     vD, vA, vB      # Subtract unsigned word modulo ← **COMUM**
vsububs     vD, vA, vB      # Subtract unsigned byte saturate
vsubuhs     vD, vA, vB      # Subtract unsigned halfword saturate
vsubuws     vD, vA, vB      # Subtract unsigned word saturate
vsubfp      vD, vA, vB      # Subtract floating-point

# Multiplicação
vmuloub     vD, vA, vB      # Multiply odd unsigned byte
vmulouh     vD, vA, vB      # Multiply odd unsigned halfword
vmulosb     vD, vA, vB      # Multiply odd signed byte
vmulosh     vD, vA, vB      # Multiply odd signed halfword
vmuleub     vD, vA, vB      # Multiply even unsigned byte
vmuleuh     vD, vA, vB      # Multiply even unsigned halfword
vmulesb     vD, vA, vB      # Multiply even signed byte
vmulesh     vD, vA, vB      # Multiply even signed halfword
vmulfp      vD, vA, vB      # Multiply floating-point ← **COMUM**
vmaddfp     vD, vA, vB, vC  # Multiply-add floating-point (vD = vA*vB + vC) ← **FMA!**

# Máximos/Mínimos
vmaxub      vD, vA, vB      # Maximum unsigned byte (16 bytes)
vmaxuh      vD, vA, vB      # Maximum unsigned halfword (8 halfwords)
vmaxuw      vD, vA, vB      # Maximum unsigned word (4 words)
vmaxsb      vD, vA, vB      # Maximum signed byte
vmaxsh      vD, vA, vB      # Maximum signed halfword
vmaxsw      vD, vA, vB      # Maximum signed word
vmaxfp      vD, vA, vB      # Maximum floating-point
vminub      vD, vA, vB      # Minimum unsigned byte
vminuh      vD, vA, vB      # Minimum unsigned halfword
vminuw      vD, vA, vB      # Minimum unsigned word
vminsb      vD, vA, vB      # Minimum signed byte
vminsh      vD, vA, vB      # Minimum signed halfword
vminsw      vD, vA, vB      # Minimum signed word
vminfp      vD, vA, vB      # Minimum floating-point

# Average
vavgub      vD, vA, vB      # Average unsigned byte (x + y + 1) >> 1
vavguh      vD, vA, vB      # Average unsigned halfword
vavgsb      vD, vA, vB      # Average signed byte
vavgsh      vD, vA, vB      # Average signed halfword
```

**⚡ Otimização CRÍTICA**:
- `vmaddfp` é FMA (Fused Multiply-Add) - faz 2 operações em 1!
- Use saturating arithmetic para evitar overflows em audio/gráficos
- Multiplicações odd/even permitem processar 8 elementos simultaneamente

### 3.3 Permutações e Shuffles

```asm
# Pack/Unpack
vpkubus     vD, vA, vB      # Pack unsigned byte unsigned saturate (16→8 bytes)
vpkuhus     vD, vA, vB      # Pack unsigned halfword unsigned saturate (8→4 halfwords)
vpkshss     vD, vA, vB      # Pack signed halfword signed saturate
vpkswss     vD, vA, vB      # Pack signed word signed saturate (4→2 words)
vpkuwus     vD, vA, vB      # Pack unsigned word unsigned saturate
vpkswus     vD, vA, vB      # Pack signed word unsigned saturate
vupkhsb     vD, vB          # Unpack high signed byte (8→16 bytes)
vupkhsh     vD, vB          # Unpack high signed halfword (4→8 halfwords)
vupklsb     vD, vB          # Unpack low signed byte
vupklsh     vD, vB          # Unpack low signed halfword
vupkhpx     vD, vB          # Unpack high pixel (formato especial PS1!)
vupklpx     vD, vB          # Unpack low pixel

# Merge/Interleave
vmrghb      vD, vA, vB      # Merge high bytes (interleave high 8 bytes)
vmrghh      vD, vA, vB      # Merge high halfwords
vmrghw      vD, vA, vB      # Merge high words
vmrglb      vD, vA, vB      # Merge low bytes
vmrglh      vD, vA, vB      # Merge low halfwords
vmrglw      vD, vA, vB      # Merge low words

# Shuffles específicos
vsldoi      vD, vA, vB, SH  # Shift left double by octet immediate (SHIFT EM BYTES!)
vspltisb    vD, SIMM        # Splat signed byte immediate (broadcast para 16 bytes)
vspltish    vD, SIMM        # Splat signed halfword immediate (broadcast para 8 halfwords)
vspltisw    vD, SIMM        # Splat signed word immediate (broadcast para 4 words)
vspltb      vD, vB, UIMM    # Splat byte (seleciona 1 byte de vB, replica)
vsplth      vD, vB, UIMM    # Splat halfword
vspltw      vD, vB, UIMM    # Splat word
```

**⚡ Otimização**:
- `vsldoi` é extremamente poderoso para alinhar dados - shift em bytes (não bits!)
- `vspltw` para replicar constantes (ex: alpha blending)
- `vpk*` útil para converter cores 32-bit → 16-bit (formato PS1!)

### 3.4 Lógicas

```asm
vand        vD, vA, vB      # AND
vor         vD, vA, vB      # OR
vxor        vD, vA, vB      # XOR
vnor        vD, vA, vB      # NOR
vandc       vD, vA, vB      # AND with complement
vorc        vD, vA, vB      # OR with complement
vnand       vD, vA, vB      # NAND
```

### 3.5 Comparações

```asm
vcmpequb    vD, vA, vB      # Compare equal unsigned byte
vcmpequh    vD, vA, vB      # Compare equal unsigned halfword
vcmpequw    vD, vA, vB      # Compare equal unsigned word ← **Útil para detecção**
vcmpgtsb    vD, vA, vB      # Compare greater than signed byte
vcmpgtsh    vD, vA, vB      # Compare greater than signed halfword
vcmpgtsw    vD, vA, vB      # Compare greater than signed word
vcmpgtub    vD, vA, vB      # Compare greater than unsigned byte
vcmpgtuh    vD, vA, vB      # Compare greater than unsigned halfword
vcmpgtuw    vD, vA, vB      # Compare greater than unsigned word
vcmpeqfp    vD, vA, vB      # Compare equal floating-point
vcmpgtfp    vD, vA, vB      # Compare greater than floating-point
vcmpgefp    vD, vA, vB      # Compare greater than or equal floating-point
vcmpbfp     vD, vA, vB      # Compare bounds floating-point
```

**⚡ Otimização**:
- Resultado é 0xFFFFFFFF (true) ou 0x00000000 (false) - pode usar como máscara!
- Combina com operações bitwise para seleção condicional (sem branches!)

### 3.6 Loads e Stores

```asm
# Aligned
lvx         vD, rA, rB      # Load vector indexed (rA + rB)
lvebx       vD, rA, rB      # Load vector element byte indexed
lvehx       vD, rA, rB      # Load vector element halfword indexed
lvewx       vD, rA, rB      # Load vector element word indexed
stvx        vS, rA, rB      # Store vector indexed
stvebx      vS, rA, rB      # Store vector element byte indexed
stvehx      vS, rA, rB      # Store vector element halfword indexed
stvewx      vS, rA, rB      # Store vector element word indexed

# Splats
lvsr        vD, rA, rB      # Load vector for shift right (calcula shift para alinhamento)
lvsl        vD, rA, rB      # Load vector for shift left
```

**⚡ Otimização**:
- `lvx`/`stvx` requerem endereços alinhados em 16 bytes!
- Use `lvsl`/`lvsr` + `vsldoi` para lidar com dados desalinhados
- 1 instrução carrega 16 bytes (4 words) - batch processing!

---

## 4. Comparação MIPS vs PowerPC - Otimizações

### 4.1 Tradução de Instruções Comuns

| Operação | MIPS (lento) | PowerPC (otimizado) | Benefício |
|----------|--------------|---------------------|-----------|
| **x * 2** | `MULT $t0, $t1` (12 ciclos) | `slwi r3, r4, 1` (1 ciclo) | **12x mais rápido** |
| **x / 4** | `DIV $t0, $t1` (36 ciclos) | `srwi r3, r4, 2` (1 ciclo) | **36x mais rápido** |
| **x % 8** | `DIV + MFHI` (36 ciclos) | `andi. r3, r4, 7` (1 ciclo) | **36x mais rápido** |
| **Abs(x)** | `BLTZ + SUB` (branch!) | `abs r3, r4` (2 ciclos) | Remove branch! |
| **Min(a,b)** | `SLT + BEQ` (branch!) | `vminfp` (4 ciclos, 4 elementos) | SIMD! |
| **Max(a,b)** | `SLT + BEQ` (branch!) | `vmaxfp` (4 ciclos, 4 elementos) | SIMD! |
| **Clamp 0-255** | Múltiplas instruções | `vpkshus` (saturate) | Hardware! |
| **RGBA pack** | 4 stores | `vpkuwus` (1 instrução) | SIMD! |

### 4.2 Exemplo: Processamento de Cores PS1

**MIPS (PS1) - Software**:
```mips
# Processar 1 pixel (RGBA 8888)
LW      $t0, 0($a0)         # Load cor (1 ciclo + delay)
NOP                         # Delay slot
ANDI    $t1, $t0, 0xFF      # Extrair R (1 ciclo)
SRL     $t2, $t0, 8         # Shift para G (1 ciclo)
ANDI    $t2, $t2, 0xFF      # Mascarar G (1 ciclo)
# ... continua para B e A
# Total: ~10 ciclos por pixel
```

**PowerPC VMX-128 (Xbox 360) - Otimizado**:
```asm
# Processar 4 pixels simultaneamente (RGBA 8888)
lvx     v0, r3, r4          # Load 4 pixels (16 bytes)
vspltisw v1, 0xFF           # Máscara 0xFFFFFFFF em todos
# Agora temos 4 pixels em v0, prontos para processar!
# Total: 2 ciclos para 4 pixels (8x mais rápido)
```

### 4.3 Exemplo: Transformação de Vértices (GTE)

**MIPS (GTE - coprocessador)**:
```mips
# RTPS - 1 vértice, ~10-20 ciclos
COP2    0x180001            # Perspective transform single
# Resultado em registradores GTE
```

**PowerPC VMX-128 - Otimizado**:
```asm
# Transformar 4 vértices simultaneamente
vmulfp  v10, v0, v4         # X * m00, m01, m02, m03 (4 vértices!)
vmulfp  v11, v1, v5         # Y * matriz
vmulfp  v12, v2, v6         # Z * matriz
vaddfp  v13, v10, v11       # X + Y
vaddfp  v13, v13, v12       # + Z (transformação completa!)
# Total: 5 ciclos para 4 vértices (vs 40-80 ciclos MIPS!)
```

---

## 5. Hierarquia de Cache e Memória

### 5.1 PlayStation 1 Cache

**MIPS R3000A Cache L1**:
- **I-Cache**: 4KB (instruções)
- **D-Cache**: 1KB (dados) - **SCRATCHPAD!**
- **Tipo**: Direct-mapped (não associativo)
- **Linha**: 4 bytes
- **Política**: Write-through

**⚡ Otimização**:
- D-Cache de 1KB é usado como scratchpad (não é cache tradicional)
- Mapeado em `0x1F800000 - 0x1F8003FF`
- Use para variáveis temporárias e stack
- Muito mais rápido que RAM principal (2MB)

```mips
# Usar scratchpad para variáveis frequentes
LW      $t0, 0x1F800000     # Acesso ao scratchpad (~1 ciclo)
LW      $t0, 0x80000000     # Acesso à RAM (~3-5 ciclos)
```

### 5.2 Xbox 360 Cache

**Xenon CPU Cache L1** (por core):
- **I-Cache**: 32KB, 2-way associativo, 64-byte lines
- **D-Cache**: 32KB, 8-way associativo, 64-byte lines
- **Latency**: 3-4 ciclos
- **Throughput**: 1 acesso por ciclo

**Cache L2** (shared):
- **Tamanho**: 1MB
- **Associatividade**: 8-way
- **Linha**: 128 bytes
- **Latency**: 20-30 ciclos
- **Throughput**: ~200 GB/s
- **Ratio**: 3:1 (3 cores compartilham)

**⚡ Otimização CRÍTICA**:
```
Cache Line Size: 64 bytes (L1), 128 bytes (L2)
→ Alinhe estruturas de dados em 64/128 bytes
→ Prefetch manual para dados sequenciais
→ Evite false sharing entre threads
```

**Dicas de Cache Xbox 360**:
1. **Alinhamento**: `__declspec(align(64))` para estruturas frequentes
2. **Cache Blocking**: Processar dados em blocos de 32KB (cabem no L1)
3. **Streaming Stores**: Use `__dcbst` para bypass L1 quando apropriado
4. **Prefetch**: `__dcbt` para trazer dados para L1 antes de usar

### 5.3 GPU eDRAM (Xenos)

**Especificações eDRAM**:
- **Capacidade**: 10MB
- **Bandwidth interno**: **256 GB/s**
- **Bandwidth para GPU**: 32 GB/s
- **Latência**: 10-20 ciclos
- **Função**: Framebuffer (color + Z/stencil)

**Layout eDRAM**:
```
10MB Total:
├── Color Buffer 0: 2-4MB
├── Color Buffer 1: 2-4MB (para blending/MSAA)
└── Z/Stencil Buffer: 4MB
```

**⚡ Otimização eDRAM**:
1. **4x MSAA é GRÁTIS** - feito durante resolve, sem overhead!
2. **Alpha blending sem custo** - processado no eDRAM
3. **Z-test sem custo** - comprimido no eDRAM
4. **Minimize resolves** - fique no eDRAM o máximo possível
5. **Tile-based rendering**: Renderize em tiles de 32KB para aproveitar cache

**Comparação de Bandwidth**:
```
RAM do Sistema:     22.4 GB/s
CPU L2 Cache:       ~200 GB/s
CPU L1 Cache:       ~400 GB/s
GPU eDRAM:          256 GB/s ← MAIS RÁPIDO!
```

---

## 6. Recomendações de Otimização por Subsistema

### 6.1 CPU Emulation (Dynarec)

**Prioridade 1: Substitua instruções lentas MIPS**
```python
# Tradução MIPS → PowerPC otimizada
traducoes = {
    'MULT':  'slwi/srwi/add (se for potência de 2)',
    'MULTU': 'mulhw/mullw (instrução nativa PowerPC!)',
    'DIV':   'divw (instrução nativa PowerPC - 12 ciclos vs 36)',
    'DIVU':  'divwu (instrução nativa PowerPC)',
    'SLL':   'slwi (1 ciclo)',
    'SRL':   'srwi (1 ciclo)',
}
```

**Prioridade 2: Batch processing com VMX-128**
- Processe 4 instruções MIPS simultaneamente quando possível
- Use para GTE (transformações 3D)
- Use para SPU (mixagem de áudio)

**Prioridade 3: Cache optimization**
- Mantenha blocos de código recompilado quentes no L2
- Alinhe entradas da lookup table em 64 bytes
- Use L1 para estado do emulador (regs MIPS, contadores)

### 6.2 GPU Emulation

**Prioridade 1: Use eDRAM!**
- Renderize para eDRAM, não para RAM do sistema
- PS1 VRAM (1MB) cabe perfeitamente no eDRAM (10MB)
- Use 4x MSAA para suavizar polígonos (grátis!)

**Prioridade 2: Vertex processing com VMX-128**
```asm
# Transformar 4 vértices PS1 de uma vez
lvx     v0, r3, r4          # Load 4 vértices
vmaddfp v1, v0, v5, v6      # Transformação matrix * vertex
stvx    v1, r7, r8          # Store resultado
```

**Prioridade 3: Pixel shaders para efeitos PS1**
- Affine texture mapping (simulado em shader)
- Dithering (padrão PS1)
- Color clamping (0-255)

### 6.3 SPU Emulation

**Prioridade 1: VMX-128 para ADPCM decode**
```asm
# Decodificar 16 amostras ADPCM simultaneamente
lvx     v0, r3, r4          # Load 16 nibbles ADPCM
vspltisb v1, 0x0F           # Máscara para low nibble
vand    v2, v0, v1          # Extrair low nibbles
vsrw    v3, v0, v1          # Shift para high nibbles
# ... decodificação paralela
```

**Prioridade 2: Audio mixing em batch**
- Mixe 4 canais PS1 por vez com `vaddfp`
- Use saturating arithmetic para evitar clipping

**Prioridade 3: Buffer management**
- Alinhe buffers de áudio em 64 bytes
- Use streaming stores para XAudio2

---

## 7. Resumo de Instruções Mais Eficientes

### 7.1 PowerPC - Quando usar cada instrução

| Cenário | Instrução | Por que? |
|---------|-----------|----------|
| Carregar constante 16-bit | `li rD, value` | 1 ciclo, nenhuma memória |
| Carregar constante 32-bit | `lis` + `ori` | 2 ciclos vs load da memória |
| Multiplicar/dividir por 2^n | `slwi`/`srwi` | 1 ciclo vs 12/36 |
| Extrair bits | `rlwinm` | 1 instrução faz tudo! |
| Máximo/mínimo | `vmax*`/`vmin*` | SIMD, 4 elementos de uma vez |
| Clamp/saturate | `vpkshs`/`vpkswus` | Hardware nativo |
| Blending alpha | `vmaddfp` | FMA em 1 instrução |
| Shuffle dados | `vsldoi`/`vmrg*` | Reorganiza 16 bytes em 1 ciclo |
| Comparação sem branch | `vcmp*` + `vand` | Remove branch misprediction |
| Load 4 words | `lvx` | 1 ciclo vs 4 loads |

### 7.2 MIPS - O que evitar no emulador

| Instrução MIPS | Ciclos | Alternativa PowerPC | Speedup |
|----------------|--------|---------------------|---------|
| `MULT` | 12 | `slwi` (shift) ou `mullw` | 6-12x |
| `MULTU` | 12 | `mullw` | 12x |
| `DIV` | 36 | `divw` (12 ciclos) ou `srwi` | 3-36x |
| `DIVU` | 36 | `divwu` (12 ciclos) | 3x |
| Loads sequenciais | N×3-5 | `lvx` (SIMD) | 4-16x |
| Branches data-dependent | 1 + misprediction | `vcmp*` + `vsel` | Remove misprediction! |

---

## 8. Métricas de Performance Esperadas

### 8.1 Projeções de Otimização

| Subsistema | Método Atual | Com VMX-128 | Speedup |
|------------|--------------|-------------|---------|
| CPU Emulation | Scalar PowerPC | VMX batch | 2-4x |
| GTE (3D math) | Interpreted | VMX matrix ops | 4-8x |
| GPU Software | CPU rasterizer | GPU shaders | 5-10x |
| SPU ADPCM | Scalar decode | VMX batch decode | 4-16x |
| Audio mixing | Canal por canal | VMX 4 canais | 4x |

### 8.2 Limitações do Xenon

**⚠️ Cuidado com**:
1. **In-order execution**: Instruções dependentes causam stalls
2. **Load latency**: 3-4 ciclos de load-to-use
3. **Branch misprediction**: Penalidade de ~15-20 ciclos
4. **SMT sharing**: 2 threads competem por recursos

**✅ Aproveite**:
1. **6 hardware threads**: Balance carga entre cores
2. **eDRAM 256 GB/s**: Zero-cost framebuffer
3. **VMX-128**: Processamento vetorial massivo
4. **1MB L2**: Cache grande para lookup tables

---

## Referências

1. MIPS R3000A Datasheet (LSI)
2. PowerPC Instruction Set Reference (IBM)
3. VMX128/AltiVec Programming Guide
4. Xbox 360 Hardware Overview (Microsoft)
5. "See MIPS Run" - Dominic Sweetman
6. Ars Technica: Inside the Xbox 360 CPU
7. PSXSPX Documentation (Nocash)

---

**Document Version**: 1.0  
**Created**: January 31, 2026  
**Purpose**: Otimização de PCSXr360 - PlayStation 1 Emulator para Xbox 360
