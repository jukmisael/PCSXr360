DFSound Audio Plugin Migration Summary
=====================================

## Overview
Migrated from peopsspu110 to dfsound audio system for PCSXr Xbox 360 emulator.

## Changes Made

### 1. Created dfsound Project
- Location: `360/Xdk/dfsound/dfsound.vcxproj`
- Type: Static Library for Xbox 360 platform
- Configurations: Debug, Release, Release_OP

### 2. Source Files Included
From `plugins/dfsound/`:
- **Core SPU Files:**
  - spu.c, spu.h - Main SPU implementation
  - registers.c, registers.h - SPU register handling
  - dma.c, dma.h - DMA transfer handling
  - xa.c, xa.h - XA audio decoding
  - adsr.c, adsr.h - ADSR envelope processing
  - reverb.c, reverb.h - Reverb effects
  
- **Configuration & State:**
  - cfg.c, cfg.h - Configuration management
  - freeze.c - Save state handling
  - externals.c, externals.h - External interface
  
- **Platform-Specific:**
  - xaudio_2.cpp - XAudio2 implementation for Xbox 360
  - nullsnd.c - Null sound driver fallback

### 3. Project Configuration
- **Preprocessor Definitions:** _XBOX, _LIB, _IN_SPU
- **Include Paths:**
  - ../../common/
  - ../../../libpcsxcore/
  - ../../../plugins/dfsound/
- **Linked Libraries:** xaudio2.lib

### 4. Updated pcsxr.vcxproj
- Removed: peopsspu110.lib references
- Removed: `../peopsspu110/Debug` library path
- Added: dfsound.lib
- Added: `../dfsound/$(Configuration)/` library paths for all configurations
- Updated project reference from audio.vcxproj to dfsound.vcxproj

### 5. Plugin System Integration
The SPU plugin functions are already mapped in `xbPlugins.h`:
- PLUGIN_SLOT_4 uses SPU_PEOPS_PLUGIN
- All PEOPS_SPU* functions map to dfsound implementations
- Function prefixes defined in `stdafx.h`:
  - SPUinit → PEOPS_SPUinit
  - SPUopen → PEOPS_SPUopen
  - SPUclose → PEOPS_SPUclose
  - etc.

## Initialization Flow
1. `DoPcsx()` calls `POKOPOM_Init()` - initializes input plugin (stays as-is)
2. `SysInit()` calls `LoadPlugins()` - loads all plugins from plugin slots
3. `LoadPlugins()` initializes SPU via SPU_PEOPS_PLUGIN (dfsound)
4. Core calls `SPU_init()` through function pointer

## Build Instructions
1. Build dfsound project first (generates dfsound.lib)
2. Build pcsxr project (links against dfsound.lib)
3. The xaudio2.lib is automatically linked from Xbox 360 SDK

## Notes
- dfsound uses XAudio2 for audio output on Xbox 360
- All SPU functions are properly exported with PEOPS_ prefix
- Save states (freeze) are supported
- CDDA audio playback is supported via PEOPS_SPUplayCDDAchannel
