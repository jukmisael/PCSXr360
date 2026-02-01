# PCSXr360

[![Nightly](https://img.shields.io/badge/Version-Nightly-blue.svg)](https://github.com/mLoaDs/PCSXr360/releases)
[![Platform](https://img.shields.io/badge/Platform-Xbox%20360-green.svg)](https://www.xbox.com)
[![License](https://img.shields.io/badge/License-GPLv2-yellow.svg)](LICENSE)

**Emulador de PlayStation 1 para Xbox 360**

PCSXr360 é um emulador de PlayStation 1 (PS1/PSX) desenvolvido para o console Xbox 360, baseado no PCSX-Reloaded com melhorias e otimizações específicas para a plataforma.

## 📋 Recursos

- 🎮 **Compatibilidade** com grande biblioteca de jogos PS1
- 🎵 **Áudio melhorado** com sistema de reverb otimizado
- 📁 **Suporte a múltiplos formatos**: BIN/CUE, ISO, IMG
- 🎨 **Shaders e filtros** para melhoria visual
- 📊 **Profiler integrado** para análise de performance
- 🔄 **Game profiles** individuais por jogo
- 🌐 **Suporte Aurora/FSD** com carregamento de ROMs

## 🚀 Instalação

1. Copie a pasta do emulador para seu Xbox 360 (USB/HDD)
2. Coloque suas ROMs de PS1 na pasta `roms`
3. Execute o `default.xex`

## 📖 Documentação

- **Changelog completo**: Veja [psx_readme.txt](psx_readme.txt) para histórico de versões
- **Wiki**: [github.com/mLoaDs/PCSXr360/wiki](https://github.com/mLoaDs/PCSXr360/wiki)
- **Controles e configurações**: Disponíveis no menu do emulador

## 🛠️ Requisitos

- Xbox 360 com RGH/JTAG
- Freestyle Dash ou Aurora Dashboard
- Arquivos de BIOS do PS1 (colocar na pasta `bios`)

## 🎮 Controles In-Game

| Combinação | Função |
|------------|--------|
| LB + RB + A + B + X + Y | Menu OSD / Sair do jogo |
| LB + RB + BACK | Toggle profiler |
| Right Stick Click | Sair para dashboard |

> **Nota**: BACK + START é o atalho do Xbox 360 para screenshots (Aurora/FSD)

## 📁 Estrutura de Pastas

```
PCSXr360/
├── bios/           # Arquivos BIOS do PS1
├── roms/           # ROMs dos jogos
├── gameprofile/    # Perfis individuais por jogo
├── shaders/        # Shaders HLSL
├── covers/         # Imagens de capa (.png)
├── gameguides/     # Guias de jogos (.txt)
└── default.xex     # Executável principal
```

## 🔧 Perfil de Jogo

Cada jogo pode ter configurações individuais:
- CPU Bias (underclock para performance)
- Shaders personalizados
- Configurações de áudio
- Memory cards virtuais individuais

## 🐛 Debug

- Log de debug: `game:\debug_log.txt`
- Profiling: `game:\profiling\[GAMEID]_profile.log`

## 👥 Créditos

- **mLoaD** - Desenvolvimento e manutenção
- **Ced2911** - Base inicial do port
- **Swizzy** - Suporte a ROM loading do Aurora
- **Dreamboy, thomasmaruzs, cmkn1983** - Contribuições
- Comunidade PCSX-Reloaded

## 📄 Licença

Este projeto é licenciado sob GPLv2. Veja o arquivo [LICENSE](LICENSE) para detalhes.

---

**Nota**: Esta é uma versão nightly em desenvolvimento ativo. Para versões estáveis, verifique a página de releases.
