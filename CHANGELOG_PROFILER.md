# PCSXr360 - Resumo de Alterações e Roadmap

## Data: 2026-02-01
## Sessão: Profiling, Dynarec e Sincronia

---

## ✅ ALTERAÇÕES REALIZADAS

### 1. Profiler Simplificado (libpcsxcore/profiler.c/h)
**Status:** ✅ Implementado e Testado

**Alterações:**
- Removido sistema complexo de logging com buffering
- Mantido apenas: FPS atual e Latency (frame time)
- Adicionado detecção automática de região PAL/NTSC
- Log formatado: `[tempo] FPS: XX | Latency: YY ms`
- Arquivo: `game:\profiling\[GAMEID]_profile_[TICKS].log`
- Toggle: LB + RB + BACK (Select)

**Métricas no Log:**
- Região detectada (NTSC/PAL)
- Target FPS (60/50)
- Performance percentual vs target
- Status: GOOD / BELOW TARGET / POOR

### 2. Dynarec como Padrão
**Status:** ✅ Implementado

**Alterações:**
- `main.cpp:60`: `Config.Cpu = CPU_DYNAREC` (antes: CPU_INTERPRETER)
- `gui_main.cpp:265`: Valor padrão INI mudado de 0 para 1
- Menu alterado: "Use Dynarec" → "Use Interpreter (Legacy)"
- Lógica invertida: Checkbox desmarcado = Dynarec (padrão)

**Motivo:** Dynarec PPC nativo do Xbox 360 é 10-50x mais rápido que interpreter

### 3. Detecção de Região no Profiler
**Status:** ✅ Implementado

**Funcionalidade:**
- Detecta PAL vs NTSC via `Config.PsxType`
- Loga target FPS correto (50 ou 60)
- Calcula performance percentual
- Mostra status qualitativo

---

## 🔍 ANÁLISE DE PERFORMANCE REALIZADA

### Teste 1: SLUS01115 (Silent Hill, NTSC)
**Configuração:** Dynarec + GPU (padrão)
**Resultado:** 46 FPS (interpretador) → 80-130 FPS (dynarec)

**Problema Identificado:**
❌ FPS acima de 60 em jogo NTSC
❌ Áudio acelerado (chipmunk effect)
❌ Sincronia vblank quebrada

**Diagnóstico:**
- Com interpreter lento (46 FPS), o próprio slowness limitava naturalmente
- Com dynarec rápido, não há limitação automática
- Frame limiter existente não está sendo aplicado corretamente
- Threaded GPU não implementada (apenas stubs)

### Componentes Verificados:

**Dynarec PPC:** ✅ FUNCIONAL
- Local: `libpcsxcore/ppc/`
- Arquivos: pR3000A.c, ppc_dyn.c, ppc_mnemonics.h
- Status: Completo, recompila MIPS → PPC nativo
- Usa 28 registradores hardware
- Cache de instruções gerenciado

**Threaded GPU:** ❌ NÃO IMPLEMENTADA
- Local: `plugins/xbox_soft/draw_thread.cpp`
- Status: Apenas stubs vazios (29 linhas)
- Impacto: GPU bloqueia CPU durante renderização

**Frame Skip:** ⚠️ EXISTE MAS NÃO EXPOSTO
- Local: `plugins/xbox_soft/fps.c`
- Variável: `UseFrameSkip`
- Status: Implementado no plugin, sem checkbox no menu Xbox

**Frame Limiter:** ⚠️ CONFIGURÁVEL
- Local: `gui_main.cpp`, `plugins/xbox_soft/`
- Variáveis: `UseFrameLimiter`, `iFrameLimit`
- Status: Existe mas pode não estar funcionando com dynarec

---

## 🎯 ROADMAP - PRÓXIMAS TAREFAS

### Prioridade 1: Sincronia VBlank Inteligente
**Status:** 📋 PLANEJAMENTO

**Objetivo:** Implementar sincronia adaptativa ao invés de limitador rígido

**Arquitetura Proposta:**
```
VBlank Adaptativo:
├── Detectar região (PAL/NTSC) ✓ (já temos)
├── Timing target: 16.67ms (NTSC) ou 20ms (PAL)
├── Sincronizar com vblank real do PS1 (psxcounters.c)
├── Se FPS > target: delay mínimo para sincronia
├── Se FPS < target: frame skip inteligente
└── Manter áudio sincronizado com SPU
```

**Dados Necessários:**
- [ ] Tempo entre vblanks reais do PS1
- [ ] Tempo de renderização da GPU
- [ ] Estado do SPU (buffer acumulando ou em sync)
- [ ] Comportamento do frame limiter atual

### Prioridade 2: Adicionar Frame Skip ao Menu
**Status:** 📋 PENDENTE
**Esforço:** 5 minutos
**Impacto:** +10-20% performance quando FPS < target

### Prioridade 3: Implementar Threaded GPU
**Status:** 📋 PENDENTE (complexo)
**Esforço:** Alto
**Impacto:** Significativo em jogos com muita geometria

### Prioridade 4: Otimizações Rápidas de GPU
**Status:** 📋 IDEIAS GUARDADAS
- Redução de resolução interna
- Forçar 16-bit color ao invés de 32-bit
- Desativar filtros de textura
- Auto frame skip quando FPS < 90% do target

---

## 🐛 BUGS IDENTIFICADOS

### Bug #1: Sincronia Quebrada com Dynarec
**Severidade:** 🔴 ALTA
**Sintomas:**
- FPS 80-130 em jogo NTSC (deveria ser 60)
- Áudio acelerado
- Timing do jogo errado

**Causa Provável:**
Frame limiter não está sendo aplicado quando dynarec está ativo

**Possíveis Soluções:**
1. Verificar se `UseFrameLimiter` está sendo setado em `ApplySettings()`
2. Garantir que `iFrameLimit` está inicializado corretamente no GPU
3. Implementar sincronia vblank inteligente (solução definitiva)

---

## 📝 NOTAS TÉCNICAS

### Arquivos Modificados nesta Sessão:
1. `libpcsxcore/profiler.c` - Profiler simplificado
2. `libpcsxcore/profiler.h` - Header atualizado
3. `360/Xdk/pcsxr/main.cpp` - Dynarec como padrão
4. `360/Xdk/pcsxr/gui_main.cpp` - Valor padrão INI e aplicação
5. `360/Xdk/pcsxr/gui.cpp` - Lógica invertida do checkbox
6. `360/Xdk/pcsxr/media/Graphics/scene.xui` - Texto do checkbox

### Arquivos para Investigação Futura:
- `libpcsxcore/psxcounters.c` - VBlank timing
- `libpcsxcore/r3000a.c` - CPU loop
- `plugins/xbox_soft/fps.c` - Frame limiter/skip
- `plugins/xbox_soft/gpu.c` - Renderização
- `360/Xdk/pcsxr/gui_main.cpp:177` - Aplicação de configurações

### Constantes Importantes:
- `PSX_TYPE_NTSC = 0` (60 FPS target)
- `PSX_TYPE_PAL = 1` (50 FPS target)
- `CPU_DYNAREC = 0`
- `CPU_INTERPRETER = 1`
- `HSyncTotal[2] = {263, 314}` - linhas por frame

---

## 🎮 COMANDOS ÚTEIS

### Ativar Profiler:
LB + RB + BACK (Select)

### Arquivos de Log:
`game:\profiling\[GAMEID]_profile_*.log`

### Configurações INI:
`game:\gameprofile\[game].ini`
- `UseDynarec=1` (padrão)
- `UseFrameLimiter=0/1` (verificar se existe)

---

## 📊 PRÓXIMOS PASSOS IMEDIATOS

1. **Coletar mais dados:**
   - Testar com frame limiter ativado/desativado
   - Verificar se áudio muda comportamento
   - Logar tempo entre vblanks do PS1

2. **Investigar:**
   - Por que frame limiter não funciona com dynarec?
   - Onde exatamente ocorre o desalinhamento?

3. **Implementar:**
   - Profiling adicional para SPU/vblank timing
   - Frame skip no menu Xbox (rápido)
   - Sincronia vblank inteligente (complexo)

---

## 💡 DECISÕES TOMADAS

✅ **Dynarec como padrão:** Aprovado
✅ **Menu "Use Interpreter (Legacy)":** Implementado
✅ **Profiler simplificado:** Funcional
📋 **VBlank inteligente:** Aguardando dados
📋 **Threaded GPU:** Postergado (alta complexidade)
📋 **Frame Skip no menu:** Próxima tarefa rápida

---

Fim do documento.
