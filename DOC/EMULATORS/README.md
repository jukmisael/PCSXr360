# Emuladores PS1 - Documentação Técnica de Áudio

Esta pasta contém documentação técnica detalhada sobre como diferentes emuladores de PlayStation 1 implementam o áudio (SPU - Sound Processing Unit).

## 📁 Arquivos Disponíveis

### 1. [DUCKSTATION.md](DUCKSTATION.md) - DuckStation
**Foco:** Emulador moderno, alta performance, C++17  
**Características:**
- ADPCM preciso com todas as peculiaridades
- Interpolação Gaussiana/Cúbica
- Reverb completo
- Otimizações SSE2/AVX
- Múltiplos backends de áudio

**Melhor para:** Referência de implementação precisa e moderna

### 2. [PCSX-REARMED.md](PCSX-REARMED.md) - PCSX-ReARMed
**Foco:** Otimizações ARM NEON, dispositivos móveis  
**Características:**
- SIMD ARM NEON para mixing
- Buffers configuráveis (256-2048 samples)
- Baixa latência para mobile
- Integração libretro

**Melhor para:** Técnicas de otimização SIMD (VMX-128 no Xbox 360)

### 3. [EPSXE.md](EPSXE.md) - ePSXe
**Foco:** Arquitetura plugin-based, PSEmu Pro  
**Características:**
- 6 níveis de latência configuráveis
- 3 níveis de qualidade (Fast/Simple/Full)
- Frame-based audio sync
- Múltiplos backends (DirectSound, XAudio2)

**Melhor para:** Configurações de buffer e estratégias de sync

### 4. [BEETLE_PSX.md](BEETLE_PSX.md) - Beetle PSX / Mednafen
**Foco:** Precisão acima de performance, cycle-exact  
**Características:**
- Emulação cycle-accurate da SPU
- Implementação exata de ADPCM
- Curvas ADSR exponenciais precisas
- Reverb completo com todos os efeitos

**Melhor for:** Referência de comportamento preciso do hardware

---

## 🔍 Quick Comparison Matrix

| Aspecto | DuckStation | PCSX-ReARMed | ePSXe | Beetle PSX |
|---------|-------------|--------------|-------|------------|
| **Foco** | Performance + Acc | Mobile/ARM | Plugins/Config | Precisão total |
| **ADPCM** | Preciso | Otimizado NEON | Padrão | Cycle-exact |
| **Interpolação** | Gaussian/Cubic | Linear/Simple | Configurável | Gaussian |
| **Buffer Size** | Dinâmico | 512-2048 | 256-8192 | 735/frame |
| **Latência** | 20-50ms | 11-46ms | 5-186ms | ~23ms |
| **SIMD** | SSE2/AVX | NEON | N/A | N/A |
| **Reverb** | Completo | Básico | Completo | Completo |
| **Configurável** | Alto | Médio | Muito Alto | Alto |

---

## 💡 Recomendações para PCSXr360

Baseado na análise dos emuladores, aqui estão as melhores práticas:

### 1. Tamanho do Buffer
**Consenso:** 2048-4096 samples é o "sweet spot"

```
- Too small (512): Pode causar underruns em jogos pesados
- Good (2048): 46ms, estável para maioria dos jogos
- Safe (4096): 93ms, elimina crackling mas adiciona delay
- Too large (22050): 500ms, muito delay (ATUAL PCSXr360!)
```

**Recomendação:** Mudar de 22050 para 4096 inicialmente

### 2. Interpolação
**Hierarquia de qualidade:**
1. **Gaussian** (DuckStation/Beetle) - Melhor qualidade
2. **Cubic** (DuckStation) - Boa qualidade
3. **Linear** (ePSXe/PCSX-ReARMed) - Balanceado
4. **None** (ePSXe Fast) - Mais rápido

**Recomendação:** Implementar Linear primeiro, Gaussian depois

### 3. ADPCM Decoder
**Todas as implementações usam as mesmas tabelas:**
- Step Table: 89 entradas
- Index Table: 8 entradas (-1, -1, -1, -1, 2, 4, 6, 8)

**Recomendação:** Usar lookup tables idênticas aos outros emuladores

### 4. Estratégia de Sync

**Abordagens encontradas:**
- **DuckStation:** Audio thread com ring buffer
- **PCSX-ReARMed:** Sync com emulação (lock-free)
- **ePSXe:** Frame-based (735 samples/frame)
- **Beetle:** libretro callback

**Recomendação para PCSXr360:**
- Usar abordagem frame-based similar ao ePSXe
- 735 samples por frame (60fps) ou 882 (50fps)
- Simplifica sincronização A/V

### 5. Otimizações SIMD

**PCSX-ReARMed (NEON):**
- Processa 8 samples simultaneamente
- Reduz uso de CPU de 10% para ~2%

**Aplicação no Xbox 360 (VMX-128):**
```cpp
// Processar 4-8 voices simultaneamente
// VMX-128 pode fazer 4 multiplicações paralelas
// Ideal para: Mixing de voices, ADPCM decode batch
```

**Recomendação:** VMX-128 para mixing de audio (fase 2 da otimização)

---

## 🎯 Prioridades de Implementação

### Fase 1: Quick Wins (1-2 semanas)
1. ✅ **Reduzir buffer** de 22050 para 4096 samples
2. ✅ **Adicionar audio sync** baseado em frame
3. ✅ **Corrigir ADSR** para usar curvas exponenciais

### Fase 2: Melhorias de Qualidade (2-4 semanas)
4. 🔄 **Interpolação linear** para todos os voices
5. 🔄 **ADPCM lookup tables** otimizadas
6. 🔄 **VMX-128 mixing** (4 voices simultâneos)

### Fase 3: Recursos Avançados (4+ semanas)
7. ⏳ **Reverb implementation** (opcional)
8. ⏳ **Gaussian interpolation** (qualidade máxima)
9. ⏳ **Pitch modulation** (efeitos especiais)
10. ⏳ **XA-ADPCM streaming** (CD audio)

---

## 📊 Arquivos de Código Relevantes no PCSXr360

Para implementar as melhorias, focar nestes arquivos:

```
plugins/dfsound/
├── xaudio_2.cpp          # Backend XAudio2 (buffer config)
├── spu.c                 # SPU principal (ADPCM, ADSR)
├── xa.c                  # XA-ADPCM (CD audio)
└── externals.h           # Configurações globais

libpcsxcore/
├── sio.c                 # Memory card (referência de I/O)
└── ...

360/common/
└── opti.h                # Otimizações Xbox 360 (VMX-128)
```

---

## 🔗 Referências Externas

- **PSX-SPX:** https://psx-spx.consoledev.net/soundprocessingunitspu/ (Documentação hardware)
- **jsgroth SPU Blog:** https://jsgroth.dev/blog/posts/ps1-spu-part-1/ (Implementação detalhada)
- **Nocash PSX:** https://problemkaputt.de/psx-spx.htm (Especificações técnicas)

---

**Pasta Criada:** January 31, 2026  
**Propósito:** Centralizar conhecimento técnico de implementações de áudio PS1
**Próximo Passo:** Usar estas referências para otimizar PCSXr360
