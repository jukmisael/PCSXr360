# Mapeamento Completo de Debug/Log/Console - PCSXR360

## Resumo Geral
Total de sistemas de debug encontrados: **6 categorias principais**

---

## 1. SYSPRINTF (Sistema Principal)

**Definido em:** `libpcsxcore/system.h` (linha 29)
```c
void SysPrintf(const char *fmt, ...);  // Printf usado por bios syscalls
```

**Usado em:**
- `libpcsxcore/r3000a.c:38` - Mensagem de inicialização
- `libpcsxcore/r3000a.c:115` - Debug de branch delay
- `libpcsxcore/ppc/pR3000A.c` - Múltiplos erros de registradores (231, 235, 278, 363, 402, 414, 553, 580, 611)
- `libpcsxcore/psxinterpreter.c:619` - psxRFE (comentado)
- `libpcsxcore/psxinterpreter.c:940` - MTC0 (comentado)
- `360/Xdk/pcsxr/gui_main.cpp:406, 409, 413, 418` - CheckCdrom, LoadCdrom
- `360/Xdk/pcsxr/main.cpp:109, 112, 115, 117` - Mesmas funções

---

## 2. OUTPUTDEBUGSTRINGA (Debug do Windows/Xbox)

**Definido em:** Windows API (xtl.h)

**Usado em:**
- `360/Xdk/pcsxr/gui_main.cpp:33` - "Menu combo pressed"
- `360/Xdk/pcsxr/gui_video.cpp:329, 347, 388, 406, 455, 473, 500, 518` - Erros de shader
- `plugins/xbox_soft/xb_video.cpp:117, 135` - Erros de vídeo
- `libpcsxcore/plugins_backup/xbox_soft/xb_video.cpp:117, 135` - Backup

---

## 3. PRINTF/FPRINTF/SPRINTF (Standard C)

### printf()
**Usado em:**
- `libpcsxcore/ppc/pR3000A.c:958, 961, 962` - Dump de registradores
- `360/Xdk/pcsxr/gui_main.cpp:128, 132, 380` - Erros de plugin
- `360/Xdk/pcsxr/main.cpp:86` - SysInit error
- `360/Xdk/pokopom_input/Pokopom/nullDC_Devices.cpp` - Vários (84, 220, 267, 274, 348)
- `360/Xdk/pokopom_input/Pokopom/nullDC.cpp` - Vários (89, 152, 160, 228, 243, 250, 269, 306, 314, 324)
- `360/Xdk/pokopom_input/Pokopom/PlayStation.cpp:257` - Wrong savestate
- `360/Xdk/pokopom_input/Pokopom/Controller.cpp:87` - Out of bound buffer

### sprintf/sprintf_s
**Usado em:** Muitos arquivos para formatação de strings
- `plugins/xbox_soft/gpu.c:357-402` - Informações do plugin
- `360/Xdk/pcsxr/gui_main.cpp:540` - Path extraction
- `360/Xdk/pcsxr/sys/Mount.cpp` - Paths de mount
- `360/Xdk/pokopom_input/Pokopom/FileIO.cpp` - Config files

### fprintf()
**Usado em:**
- `plugins/dfsound/sdl/SDL_audiotypecvt.c` - Conversões de áudio (múltiplos)
- `360/lib/zlib-1.2.5/zutil.h:250-254` - Trace macros

---

## 4. GPU_DISPLAYTEXT (Debug na tela)

**Definido em:** `plugins/xbox_soft/gpu.c:299`
```c
void CALLBACK GPUdisplayText(char * pText)  // some debug func
```

**Implementação Xbox:** `PEOPS_GPUdisplayText()` na linha 301

**Buffer:** `szDebugText[512]` em `plugins/xbox_soft/gpu.c:508`

**Mostrado em:** `plugins/xbox_soft/menu.c:82` - DisplayText() function
- Posição: Canto superior esquerdo (0,0)
- Cor: Verde (RGB(0,255,0))
- Timeout: 2 segundos

**Usado para:**
- FPS display (plugins/xbox_soft/gpu.c:699)
- Mensagens de debug do emulador

---

## 5. DEBUG.H (Sistema de Debug Condicional)

**Localização:** `libpcsxcore/debug.h`

**Configuração:**
```c
//#define LOG_STDOUT  // Comentado - desativado
```

**Incluído em:**
- `libpcsxcore/psxcommon.h:76`
- `libpcsxcore/core_backup/psxcommon.h:76`

**Macros em psxinterpreter.c:**
```c
#define debugI() PSXCPU_LOG("%s\n", disR3000AF(psxRegs.code, psxRegs.pc));
// ou vazio quando desativado
```

---

## 6. ARQUIVOS DE LOG TEMPORÁRIOS

### Arquivos que criamos e removemos:
- ❌ `360/Xdk/pcsxr/simple_logger.c/h` - REMOVIDO
- ❌ `360/Xdk/pcsxr/debug_log.cpp/h` - REMOVIDO
- ❌ `libpcsxcore/fps_counter.c/h/cpp` - REMOVIDO

---

## 7. OUTROS SISTEMAS DE OUTPUT

### puts() / fputs()
**Usado em:**
- `360/lib/zlib-1.2.5/inflate.c:310-329` - Geração de código (makefixed)
- `360/lib/zlib-1.2.5/contrib/infback9/infback9.c:80-110` - Geração deflate64
- `360/Xdk/bzip/src/dlltest.c:66` - Uso do minibz2
- `360/Xdk/pokopom_input/Pokopom/FileIO.cpp:61, 65` - Escrita em arquivos INI

### swprintf() / wprintf()
**Usado em:**
- `360/Xdk/pokopom_input/Pokopom/nullDC.cpp:269` - Player settings
- `360/Xdk/pokopom_input/Pokopom/ConfigDialog.cpp:45, 49` - Dialog text
- `360/Xdk/pcsxr/gui.cpp:213` - File browser info

### fwprintf() / fputws()
**Usado em:**
- `360/lib/zlib-1.2.5/contrib/iostream3/test.cc` - iostream test
- `360/lib/zlib-1.2.5/contrib/iostream2/zstream_test.cpp` - zstream test

### psxBios_puts
**Localização:** `libpcsxcore/psxbios.c:147`
- Função da BIOS do PS1 para output de texto
- Mapeada em: `biosA0[0x3e]` e `biosB0[0x3f]`

### stderr outputs
**Usado em:**
- `360/lib/zlib-1.2.5/examples/zpipe.c:153-202` - Mensagens de erro
- `360/lib/zlib-1.2.5/examples/gzjoin.c:436` - Uso do gzjoin
- `360/lib/zlib-1.2.5/examples/enough.c` - Várias mensagens de erro
- `360/lib/zlib-1.2.5/example.c:105` - gzputs error

---

## 8. OUTROS SISTEMAS

### DEBUG_print (Wii/GC specific)
**Usado em:** `plugins/xbox_soft/gpu.c`
- Linhas: 934-937, 1486-1489, 1597-1608, 1724-1727
- Apenas para SD Gecko debugging
- Provavelmente #ifdef protegido

### ConsoleOutput (Pokopom)
**Localização:** `360/Xdk/pokopom_input/Pokopom/ConsoleOutput.cpp`
```c
void GimmeConsole();  // Aloca console do Windows
```

**Macros em General.h:**
```c
#define Debug printf
#define DebugFunc() printf("Pokopom -> "__FUNCTION__"\n")
```

### BZip2 Debug
**Localização:** `360/Xdk/bzip/src/bzlib_private.h:65-84`
```c
#define fprintf(stderr, ...)  // Apenas em debug build
```

### zlib Trace
**Localização:** `360/lib/zlib-1.2.5/zutil.h:250-254`
```c
#define Trace(x) {if (z_verbose>=0) fprintf x ;}
#define Tracev(x) {if (z_verbose>0) fprintf x ;}
```

---

## 9. CATEGORIZAÇÃO POR MÓDULO

### Core Emulator (`libpcsxcore/`)
| Função | Arquivo | Propósito |
|--------|---------|-----------|
| SysPrintf | system.h | Mensagens gerais |
| printf | pR3000A.c | Dump de registradores |
| debugI | psxinterpreter.c | Disassembly (condicional) |

### GPU Plugin (`plugins/xbox_soft/`)
| Função | Arquivo | Propósito |
|--------|---------|-----------|
| GPUdisplayText | gpu.c | Texto na tela |
| sprintf | gpu.c | Formatação de info |
| DEBUG_print | gpu.c | Debug SD Gecko |

### Main Application (`360/Xdk/pcsxr/`)
| Função | Arquivo | Propósito |
|--------|---------|-----------|
| OutputDebugStringA | gui_main.cpp | Debug Xbox |
| SysPrintf | gui_main.cpp, main.cpp | Inicialização |
| printf | gui_main.cpp, main.cpp | Erros |

### Input Plugin (`360/Xdk/pokopom_input/`)
| Função | Arquivo | Propósito |
|--------|---------|-----------|
| printf | Vários | Debug de input |
| Debug | General.h | Macro de debug |
| GimmeConsole | ConsoleOutput.cpp | Console Windows |

---

## Recomendações de Uso

### Para Debug Xbox 360 (Neighborhood/DebugView):
```c
OutputDebugStringA("Mensagem aqui");
```

### Para Log em Arquivo (se implementado):
```c
// Arquivo: game:\pcsxr_log.txt
fprintf(log_file, "FPS: %d\n", fps);
```

### Para Mensagens na Tela (já existe):
```c
GPU_displayText("FPS: XX/60");
```

### Para Debug durante desenvolvimento:
```c
SysPrintf("Valor: %x\n", valor);  // Aparece no console do dev
```

---

## Sistema Atualmente Ativo

Após nossa limpeza, o único sistema de debug **ativo** é:
- **SysPrintf** - Para mensagens de inicialização
- **OutputDebugStringA** - Para debug Xbox (menu combo)
- **GPUdisplayText** - Para FPS e textos na tela (plugin GPU)

Todos os outros foram removidos ou estão desativados por #ifdef.
