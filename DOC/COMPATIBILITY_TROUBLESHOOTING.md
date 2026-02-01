# Guia de Compatibilidade e Problemas Conhecidos - PCSXR360

## Status de Compatibilidade

### Jogos Testados

#### ✅ Funcionando Perfeitamente
- **Metal Gear Solid** - Funciona com FMVs e som
- **Final Fantasy VII** - Completo, sem problemas
- **Resident Evil 2** - Estável, save states funcionam
- **Castlevania: Symphony of the Night** - Roda a 60 FPS
- **Tekken 3** - Full speed, som sincronizado

#### ⚠️ Funcionando com Problemas
- **Gran Turismo 2** - Slowdown em cenas complexas (GPU)
- **Crash Bandicoot 3** - Rápido demais (timing issue)
- **Silent Hill** - Glitch gráfico em efeitos de névoa
- **Chrono Cross** - Lag em batalhas com múltiplos inimigos

#### ❌ Não Funcionando
- **Vagrant Story** - Crash no startup (GTE issue)
- **Parasite Eve II** - Freeze após menu principal
- **Final Fantasy IX** - Problemas de memória card

## Problemas Conhecidos e Soluções

### 1. Problemas de Timing

#### 1.1 Jogos Muito Rápidos
**Sintomas:**
- Jogo roda em velocidade 2x ou mais
- Cutscenes aceleradas
- Música acelerada

**Causas:**
- CPU clock não sincronizado com VBLANK
- Frame limiter desativado

**Solução:**
```c
// Habilitar frame limiter
Config.FrameLimit = 1;
Config.FrameRate = 60; // NTSC ou 50 para PAL
```

**Workaround:**
Ajustar "CPU Clock" nas configurações para ~70-80% se o jogo continuar rápido.

#### 1.2 Jogos Muito Lentos
**Sintomas:**
- FPS abaixo de 60
- Audio com delay
- Input lag

**Causas:**
- Emulação muito precisa (LLE)
- Shaders complexos
- Resolução muito alta

**Soluções:**
1. Usar HLE GPU quando disponível
2. Desativar filtros de vídeo pesados
3. Reduzir resolução interna (1x ou 2x)
4. Habilitar frame skip se necessário

### 2. Problemas Gráficos

#### 2.1 Texturas Tremendo (Wobble)
**Sintomas:**
- Texturas "dançando" ou tremendo
- Polígonos instáveis
- Efeito "PS1 clássico" excessivo

**Causa:**
- Comportamento correto do PS1 (coordenadas de textura são instáveis)
- Falta de subpixel precision

**Solução:**
Isso é comportamento autêntico do PS1. Para reduzir:
```c
// Habilitar "Wobble Reduction"
Config.WobbleReduction = 1;
// ou usar PGXP (se implementado)
Config.PGXP = 1;
```

#### 2.2 Falta de Texturas / Texturas Pretas
**Sintomas:**
- Modelos 3D aparecem pretos
- Texturas não carregam
- Efeitos de transparência ausentes

**Causas:**
1. VRAM não sincronizada
2. Problema de semi-transparência
3. VRAM address calculation error

**Solução:**
```c
// Forçar sync de VRAM a cada frame
Config.VRAMSyncMode = VRAM_SYNC_EVERY_FRAME;

// Ou ajustar blending mode
Config.BlendingMode = BLEND_ACCURATE;
```

#### 2.3 Linhas Pretas nas Texturas
**Sintomas:**
- Linhas pretas entre polígonos
- Gaps nas texturas

**Causa:**
- T-junctions no PS1
- Problema de precisão de coordenadas

**Workaround:**
Habilitar "Polygon Jitter Fix" nas configurações avançadas.

#### 2.4 Efeitos de Névoa / Fog Incorretos
**Sintomas:**
- Silent Hill: névoa aparece e desaparece
- Final Fantasy: efeitos de névoa preto/branco

**Causa:**
- GTE depth calculation inaccuracy
- Ordering table mal implementada

**Solução:**
Verificar implementação de `AVSZ3`/`AVSZ4` no GTE.

### 3. Problemas de Áudio

#### 3.1 Estalos e Chiados (Audio Crackling)
**Sintomas:**
- Som com estalos frequentes
- Ruído estático
- Audio dropouts

**Causas:**
1. Buffer size muito pequeno
2. Audio desync com vídeo
3. CPU overload

**Solução:**
```c
// Aumentar buffer de audio
Config.SpuBufferSize = 4096; // ou 8192

// Habilitar audio sync
Config.AudioSync = 1;

// Usar resample de alta qualidade
Config.SpuInterpolation = INTERPOLATION_CUBIC;
```

#### 3.2 Música Desaparecendo
**Sintomas:**
- Música para após alguns minutos
- Áudio volta ao reiniciar o jogo

**Causa:**
- ADPCM looping incorreto
- SPU voice end detection

**Solução:**
Verificar implementação de ENDX flags no SPU.

#### 3.3 Som Muito Baixo
**Sintomas:**
- Volume geral baixo
- Alguns efeitos não audíveis

**Causa:**
- SPU main volume não aplicada
- CD-DA volume separado

**Solução:**
```c
// Aumentar volume master
Config.SpuVolume = 100; // 0-100

// Ajustar CD volume separadamente
Config.CdAudioVolume = 80; // 0-100
```

### 4. Problemas de Memory Card

#### 4.1 Corrupção de Save
**Sintomas:**
- Save game corrompido
- "Memory card not formatted"
- Saves desaparecem

**Causas:**
1. Bug na emulação de memory card
2. Timing incorreto nas operações
3. Arquivo .mcd corrompido

**Solução:**
1. Criar novo memory card vazio
2. Verificar permissões do arquivo
3. Usar memory card do download de referência

#### 4.2 Memory Card Não Reconhecida
**Sintomas:**
- "No memory card inserted"
- Jogo não vê saves

**Causa:**
- Arquivo memory card não existe
- Path configurado incorretamente

**Solução:**
Verificar no `pcsx.ini`:
```ini
Mcd1 = game:\memcards\Memcard1.mcd
Mcd2 = game:\memcards\Memcard2.mcd
```

### 5. Problemas de CDROM

#### 5.1 FMVs Travadando
**Sintomas:**
- Vídeos travam ou pulam
- Audio continua mas vídeo para
- Crash durante FMV

**Causas:**
1. MDEC (Motion Decoder) não implementado corretamente
2. XA-ADCD audio stream problem
3. Timing de setloc

**Solução:**
- Verificar implementação de XA-ADPCM decoding
- Ajustar CD-ROM speed: `Config.CdromSpeed = 2x;`
- Usar buffer maior para FMVs

#### 5.2 Leitura Lenta
**Sintomas:**
- Loading screens muito longas
- Transições lentas
- Pause antes de músicas

**Causa:**
- Emulação de CDROM muito precisa
- Latência de leitura excessiva

**Solução:**
```c
// Habilitar CDROM cache
Config.CdromCache = 1;
Config.CdromCacheSize = 16 * 1024 * 1024; // 16MB

// Ou usar async reads
Config.CdromAsync = 1;
```

### 6. Problemas de Input

#### 6.1 Input Lag
**Sintomas:**
- Delay entre apertar botão e ação
- Jogos de luta impossíveis de jogar
- Resposta lenta

**Causas:**
1. Frame buffering excessivo
2. VSync causando delay
3. Poll de input no momento errado

**Soluções:**
1. Desabilitar VSync: `Config.VSync = 0;`
2. Reduzir frame buffer: `Config.FrameBufferCount = 1;`
3. Poll input imediatamente antes de emular frame

#### 6.2 Controle Não Responde
**Sintomas:**
- Nenhum input detectado
- Só funciona no menu

**Causa:**
- Configuração de input incorreta
- Driver/controller plugin problem

**Solução:**
Reconfigurar input no menu ou usar Pokopom input plugin.

### 7. Problemas de Crash

#### 7.1 Crash no Startup
**Sintomas:**
- Tela preta ao iniciar jogo
- Freeze no logo da Sony
- Retorno ao menu

**Possíveis Causas:**
1. BIOS incorreta ou corrompida
2. Imagem ISO danificada
3. Plugin de GPU incompatível

**Solução:**
1. Usar BIOS SCPH1001.BIN (verificar checksum)
2. Verificar ISO com redump.org
3. Trocar plugin de GPU

#### 7.2 Crash Aleatório
**Sintomas:**
- Crash em momentos aleatórios
- Não reproduzível consistentemente

**Causas:**
1. Memory corruption
2. Race condition
3. JIT compiler bug

**Debugging:**
```c
// Habilitar debug mode
Config.DebugMode = 1;
Config.LogLevel = LOG_VERBOSE;

// Verificar últimas operações antes do crash
// Analisar log em C:\Users\<user>\AppData\Roaming\pcsxr\pcsxr.log
```

### 8. Problemas Específicos de Jogos

#### 8.1 Final Fantasy Tactics
**Problema:** Glitch gráfico em menus
**Solução:** Habilitar "Accurate Framebuffer"

#### 8.2 Metal Gear Solid
**Problema:** Lag na água
**Solução:** Usar HLE GPU, desativar filtros

#### 8.3 Gran Turismo 2
**Problema:** Menu lento
**Solução:** Overclock de CPU (150%)

#### 8.4 Vagrant Story
**Problema:** Não inicia
**Status:** Bug conhecido, requer fix no GTE

## Debugging Avançado

### 1. Enable Full Logging

```c
Config.LogLevel = LOG_DEBUG;
Config.LogToFile = 1;
Config.LogFilePath = "game:\logs\pcsxr_debug.log";

// Logs específicos
Config.LogCpu = 1;
Config.LogGpu = 1;
Config.LogSpu = 1;
Config.LogCdrom = 1;
Config.LogGte = 0; // Muito verbose
```

### 2. CPU Breakpoints

```c
// Breakpoint em endereço específico
cpu_set_breakpoint(0x80012340);

// Breakpoint em syscall
cpu_set_syscall_breakpoint(0xA0, 0x3D); // get_system_info
```

### 3. GPU Debugging

```c
// Dump de VRAM a cada frame
Config.GpuVramDump = 1;
Config.GpuVramDumpPath = "game:\vram_dumps\";

// Log de comandos GPU
Config.GpuLogCommands = 1;
```

### 4. SPU Debugging

```c
// Dump de samples ADPCM
Config.SpuDumpSamples = 1;

// Visualização de ADSR
Config.SpuVisualizeAdsr = 1;
```

## FAQ

### Q: Por que alguns jogos rodam melhor no modo HLE?
**A:** HLE (High Level Emulation) emula comportamento em vez de hardware, permitindo otimizações. Melhor para jogos que não usam features obscuras.

### Q: Posso usar save states?
**A:** Sim, mas podem ser incompatíveis entre versões diferentes do emulador.

### Q: Como melhorar performance em jogos lentos?
**A:**
1. Desabilitar filtros de vídeo
2. Usar resolução nativa (1x)
3. Habilitar frame skip
4. Overclock de CPU
5. Fechar outros aplicativos no Xbox 360

### Q: O emulador suporta cheats?
**A:** Sim, formato .cht ou memory patching via debugger.

### Q: Posso rodar jogos PAL em NTSC?
**A:** Sim, mas pode haver problemas de velocidade. Use "Force NTSC" nas opções.

## Reportando Problemas

Para reportar um problema, inclua:
1. **Versão do PCSXR360** (ex: 2.1.1a)
2. **Jogo** (nome e código/region)
3. **Problema** (descrição detalhada)
4. **Configurações** (plugins usados, opções habilitadas)
5. **Log file** (se disponível)
6. **Reprodução** (passos para reproduzir)

## Links Úteis

- [PSXDATACENTER](http://psxdatacenter.com) - Database de jogos PS1
- [Redump](http://redump.org) - Verificação de ISOs
- [PSXDEV](https://www.psxdev.net) - Fórum de desenvolvimento
- [GitHub Issues](https://github.com/<repo>/issues) - Reportar bugs

---
*Guia de compatibilidade PCSXR360 v2.1.1*
