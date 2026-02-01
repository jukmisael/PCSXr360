/***************************************************************************
                           StdAfx.h  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#if defined(_WINDOWS) || defined(_XBOX)

#ifdef _XBOX
#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <xtl.h>
#include <process.h>
#include <stdlib.h>

#ifndef INLINE
#define INLINE __inline
#endif

#pragma warning (disable:4996)
#else
#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#include <windowsx.h>
#include "mmsystem.h"
#include <process.h>
#include <stdlib.h>

#ifndef INLINE
#define INLINE __inline
#endif

#include "resource.h"

#pragma warning (disable:4996)
#endif
#else

#ifndef _MACOSX
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef USEOSS
#include <sys/soundcard.h>
#endif
#include <unistd.h>
#include <pthread.h>
#define RRand(range) (random()%range)  
#include <string.h> 
#include <sys/time.h>  
#include <math.h>  

#undef CALLBACK
#define CALLBACK
#define DWORD unsigned int
#define LOWORD(l)           ((unsigned short)(l)) 
#define HIWORD(l)           ((unsigned short)(((unsigned int)(l) >> 16) & 0xFFFF)) 

#ifndef INLINE
#define INLINE inline
#endif

#endif

#if defined (__GNUC__) || defined (__clang__)
#define UNUSED_VARIABLE __attribute__((unused))
#else
#define UNUSED_VARIABLE
#endif

#include "psemuxa.h"

// NOTE: These DFSOUND_* prefixes are for the DFSOUND audio plugin.
// The old PEOPS_* name was kept historically but this is DFSOUND, not PEOPS.
#ifdef _XBOX
#define SPUreadDMA				DFSOUND_SPUreadDMA
#define SPUreadDMAMem			DFSOUND_SPUreadDMAMem
#define SPUwriteDMA				DFSOUND_SPUwriteDMA
#define SPUwriteDMAMem			DFSOUND_SPUwriteDMAMem
#define SPUasync				DFSOUND_SPUasync
#define SPUupdate				DFSOUND_SPUupdate
#define SPUplayADPCMchannel		DFSOUND_SPUplayADPCMchannel
#define SPUinit					DFSOUND_SPUinit
#define SPUopen					DFSOUND_SPUopen
#define SPUsetConfigFile		DFSOUND_SPUsetConfigFile
#define SPUclose				DFSOUND_SPUclose
#define SPUshutdown				DFSOUND_SPUshutdown
#define SPUtest					DFSOUND_SPUtest
#define SPUconfigure			DFSOUND_SPUconfigure
#define SPUabout				DFSOUND_SPUabout
#define SPUregisterCallback		DFSOUND_SPUregisterCallback
#define SPUregisterCDDAVolume	DFSOUND_SPUregisterCDDAVolume
#define SPUplayCDDAchannel		DFSOUND_SPUplayCDDAchannel
#define SPUwriteRegister		DFSOUND_SPUwriteRegister
#define SPUreadRegister			DFSOUND_SPUreadRegister
#define SPUfreeze				DFSOUND_SPUfreeze

#define ReadConfig				SPUReadConfig
#define ReadConfigFile			SPUReadConfigFile
#define WriteConfig				SPUWriteConfig
#define iDebugMode				SPUiDebugMode

#define SWAP16(x) _byteswap_ushort(x)
#else
#endif