/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2002  Pcsx Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef GAMECUBE_PLUGINS_H
#define GAMECUBE_PLUGINS_H

#ifndef NULL
#define NULL ((void*)0)
#endif
#include "../../libpcsxcore/PSEmu_Plugin_Defs.h"
#include "../../libpcsxcore/plugins.h"

#define SYMS_PER_LIB 32
typedef struct {
	char* lib;
	int   numSyms;
	struct {
		char* sym;
		void* pntr;
	} syms[SYMS_PER_LIB];
} PluginTable;
#define NUM_PLUGINS 8

long CDRCIMGplay(unsigned char *time);
long CDRCIMGgetTN(unsigned char *buffer);
long CDRCIMGgetTD(unsigned char track, unsigned char *buffer);
long CDRCIMGreadTrack(unsigned char *time);
unsigned char *CDRCIMGgetBuffer(void);
long CDRCIMGplay(unsigned char *time);
long CDRCIMGstop(void);
unsigned char* CDRCIMGgetBufferSub(void);
long CDRCIMGgetStatus(struct CdrStat *stat);
long CDRCIMGclose(void);
long CDRCIMGshutdown(void);
long CDRCIMGinit(void);
long CDRCIMGopen(void);

void cdrcimg_set_fname(const char *fname);

/* PAD */
//typedef long (* PADopen)(unsigned long *);
extern long PAD__init(long);
extern long PAD__shutdown(void);	
extern long PAD__open(void);
extern long PAD__close(void);
extern long PAD__readPort1(PadDataS*);
extern long PAD__readPort2(PadDataS*);

/* SPU NULL */
//typedef long (* SPUopen)(void);
void NULL_SPUwriteRegister(unsigned long reg, unsigned short val);
unsigned short NULL_SPUreadRegister(unsigned long reg);
unsigned short NULL_SPUreadDMA(void);
void NULL_SPUwriteDMA(unsigned short val);
void NULL_SPUwriteDMAMem(unsigned short * pusPSXMem,int iSize);
void NULL_SPUreadDMAMem(unsigned short * pusPSXMem,int iSize);
void NULL_SPUplayADPCMchannel(xa_decode_t *xap);
long NULL_SPUinit(void);
long NULL_SPUopen(void);
void NULL_SPUsetConfigFile(char * pCfg);
long NULL_SPUclose(void);
long NULL_SPUshutdown(void);
long NULL_SPUtest(void);
void NULL_SPUregisterCallback(void (*callback)(void));
void NULL_SPUregisterCDDAVolume(void (*CDDAVcallback)(unsigned short,unsigned short));
char * NULL_SPUgetLibInfos(void);
void NULL_SPUabout(void);
long NULL_SPUfreeze(unsigned long ulFreezeMode,SPUFreeze_t *);

/* SPU DFSOUND 1.7 */
//dma.c
unsigned short CALLBACK DFSOUND_SPUreadDMA(void);
void CALLBACK DFSOUND_SPUreadDMAMem(unsigned short * pusPSXMem,int iSize);
void CALLBACK DFSOUND_SPUwriteDMA(unsigned short val);
void CALLBACK DFSOUND_SPUwriteDMAMem(unsigned short * pusPSXMem,int iSize);
//spu.c
void CALLBACK DFSOUND_SPUasync(unsigned long cycle);
void CALLBACK DFSOUND_SPUupdate(void);
void CALLBACK DFSOUND_SPUplayADPCMchannel(xa_decode_t *xap);
long CALLBACK DFSOUND_SPUinit(void);
long DFSOUND_SPUopen(void);
void DFSOUND_SPUsetConfigFile(char * pCfg);
long CALLBACK DFSOUND_SPUclose(void);
long CALLBACK DFSOUND_SPUshutdown(void);
long CALLBACK DFSOUND_SPUtest(void);
long CALLBACK DFSOUND_SPUconfigure(void);
void CALLBACK DFSOUND_SPUabout(void);
void CALLBACK DFSOUND_SPUregisterCallback(void (CALLBACK *callback)(void));
void CALLBACK DFSOUND_SPUregisterCDDAVolume(void (CALLBACK *CDDAVcallback)(unsigned short,unsigned short));
void CALLBACK DFSOUND_SPUplayCDDAchannel(short *pcm, int nbytes);
//registers.c
void CALLBACK DFSOUND_SPUwriteRegister(unsigned long reg, unsigned short val);
unsigned short CALLBACK DFSOUND_SPUreadRegister(unsigned long reg);
//freeze.c
long CALLBACK DFSOUND_SPUfreeze(unsigned long ulFreezeMode,SPUFreeze_t * pF);


/* CDR */
long ISOopen(void);
long ISOinit(void);
long ISOshutdown(void);
long ISOopen(void);
long ISOclose(void);
long ISOgetTN(unsigned char *);
long ISOgetTD(unsigned char , unsigned char *);
long ISOreadTrack(unsigned char *);
unsigned char *ISOgetBuffer(void);
unsigned char *ISOgetBufferSub(void);

/* NULL GPU */
//typedef long (* GPUopen)(unsigned long *, char *, char *);
long GPU__open(void);  
long GPU__init(void);
long GPU__shutdown(void);
long GPU__close(void);
void GPU__writeStatus(unsigned long);
void GPU__writeData(unsigned long);
unsigned long GPU__readStatus(void);
unsigned long GPU__readData(void);
long GPU__dmaChain(unsigned long *,unsigned long);
void GPU__updateLace(void);

/* PEOPS GPU */
long PEOPS_GPUopen(unsigned long *, char *, char *); 
long PEOPS_GPUinit(void);
long PEOPS_GPUshutdown(void);
long PEOPS_GPUclose(void);
void PEOPS_GPUwriteStatus(unsigned long);
void PEOPS_GPUwriteData(unsigned long);
void PEOPS_GPUwriteDataMem(unsigned long *, int);
unsigned long PEOPS_GPUreadStatus(void);
unsigned long PEOPS_GPUreadData(void);
void PEOPS_GPUreadDataMem(unsigned long *, int);
long PEOPS_GPUdmaChain(unsigned long *,unsigned long);
void PEOPS_GPUupdateLace(void);
void PEOPS_GPUdisplayText(char *);
long PEOPS_GPUfreeze(unsigned long,GPUFreeze_t *);
void PEOPS_GPUvBlank( int val );

/* hw gpu plugins */
long HW_GPUopen(unsigned long *, char *, char *);
long HW_GPUinit(void);
long HW_GPUshutdown(void);
long HW_GPUclose(void);
void HW_GPUwriteStatus(unsigned long);
void HW_GPUwriteData(unsigned long);
void HW_GPUwriteDataMem(unsigned long *, int);
unsigned long HW_GPUreadStatus(void);
unsigned long HW_GPUreadData(void);
void HW_GPUreadDataMem(unsigned long *, int);
long HW_GPUdmaChain(unsigned long *, unsigned long);
void HW_GPUupdateLace(void);
void HW_GPUdisplayText(char *);
long HW_GPUfreeze(unsigned long, GPUFreeze_t *);
void HW_GPUvBlank(int val);
void HW_GPUvisualVibration(uint32_t iSmall, uint32_t iBig);
void HW_GPUmakeSnapshot(void);
void HW_GPUcursor(int iPlayer, int x, int y);
void HW_GPUaddVertex(short sx, short sy, s64 fx, s64 fy, s64 fz);

#define EMPTY_PLUGIN \
	{ NULL,      \
	  0,         \
	  { { NULL,  \
	      NULL }, } }

// HW GPU
#define GPU_HW_PEOPS_PLUGIN \
{ "GPU",      \
16,         \
{ { "GPUinit",  \
HW_GPUinit }, \
{ "GPUshutdown",	\
HW_GPUshutdown}, \
{ "GPUopen", \
HW_GPUopen}, \
{ "GPUclose", \
HW_GPUclose}, \
{ "GPUwriteStatus", \
HW_GPUwriteStatus}, \
{ "GPUwriteData", \
HW_GPUwriteData}, \
{ "GPUwriteDataMem", \
HW_GPUwriteDataMem}, \
{ "GPUreadStatus", \
HW_GPUreadStatus}, \
{ "GPUreadData", \
HW_GPUreadData}, \
{ "GPUreadDataMem", \
HW_GPUreadDataMem}, \
{ "GPUdmaChain", \
HW_GPUdmaChain}, \
{ "GPUfreeze", \
HW_GPUfreeze}, \
{ "GPUvisualVibration", \
HW_GPUvisualVibration}, \
{ "GPUcursor", \
HW_GPUcursor}, \
{ "GPUupdateLace", \
HW_GPUupdateLace}, \
{ "GPUvBlank", \
HW_GPUvBlank}, \
} }
	      
#define PAD1_PLUGIN \
	{ "PAD1",      \
	  5,         \
	  { { "PADinit",  \
	      PAD__init }, \
	    { "PADshutdown",	\
	      PAD__shutdown}, \
	    { "PADopen", \
	      PAD__open}, \
	    { "PADclose", \
	      PAD__close}, \
	    { "PADreadPort1", \
	      PAD__readPort1} \
	       } } 
	    
#define PAD2_PLUGIN \
	{ "PAD2",      \
	  5,         \
	  { { "PADinit",  \
	      PAD__init }, \
	    { "PADshutdown",	\
	      PAD__shutdown}, \
	    { "PADopen", \
	      PAD__open}, \
	    { "PADclose", \
	      PAD__close}, \
	    { "PADreadPort2", \
	      PAD__readPort2} \
	       } }

#define SPU_NULL_PLUGIN \
	{ "SPU",      \
	  17,         \
	  { { "SPUinit",  \
	      NULL_SPUinit }, \
	    { "SPUshutdown",	\
	      NULL_SPUshutdown}, \
	    { "SPUopen", \
	      NULL_SPUopen}, \
	    { "SPUclose", \
	      NULL_SPUclose}, \
	    { "SPUconfigure", \
	      NULL_SPUsetConfigFile}, \
	    { "SPUabout", \
	      NULL_SPUabout}, \
	    { "SPUtest", \
	      NULL_SPUtest}, \
	    { "SPUwriteRegister", \
	      NULL_SPUwriteRegister}, \
	    { "SPUreadRegister", \
	      NULL_SPUreadRegister}, \
	    { "SPUwriteDMA", \
	      NULL_SPUwriteDMA}, \
	    { "SPUreadDMA", \
	      NULL_SPUreadDMA}, \
	    { "SPUwriteDMAMem", \
	      NULL_SPUwriteDMAMem}, \
	    { "SPUreadDMAMem", \
	      NULL_SPUreadDMAMem}, \
	    { "SPUplayADPCMchannel", \
	      NULL_SPUplayADPCMchannel}, \
	    { "SPUfreeze", \
	      NULL_SPUfreeze}, \
	    { "SPUregisterCallback", \
	      NULL_SPUregisterCallback}, \
	    { "SPUregisterCDDAVolume", \
	      NULL_SPUregisterCDDAVolume} \
	       } }

#define SPU_DFSOUND_PLUGIN \
	{ "SPU",      \
	  19,         \
	  { { "SPUinit",  \
	      DFSOUND_SPUinit }, \
	    { "SPUshutdown",	\
	      DFSOUND_SPUshutdown}, \
	    { "SPUopen", \
	      DFSOUND_SPUopen}, \
	    { "SPUclose", \
	      DFSOUND_SPUclose}, \
	    { "SPUconfigure", \
	      DFSOUND_SPUconfigure}, \
	    { "SPUabout", \
	      DFSOUND_SPUabout}, \
	    { "SPUtest", \
	      DFSOUND_SPUtest}, \
	    { "SPUwriteRegister", \
	      DFSOUND_SPUwriteRegister}, \
	    { "SPUreadRegister", \
	      DFSOUND_SPUreadRegister}, \
	    { "SPUwriteDMA", \
	      DFSOUND_SPUwriteDMA}, \
	    { "SPUreadDMA", \
	      DFSOUND_SPUreadDMA}, \
	    { "SPUwriteDMAMem", \
	      DFSOUND_SPUwriteDMAMem}, \
	    { "SPUreadDMAMem", \
	      DFSOUND_SPUreadDMAMem}, \
	    { "SPUplayADPCMchannel", \
	      DFSOUND_SPUplayADPCMchannel}, \
	    { "SPUfreeze", \
	      DFSOUND_SPUfreeze}, \
	    { "SPUregisterCallback", \
	      DFSOUND_SPUregisterCallback}, \
	    { "SPUregisterCDDAVolume", \
	      DFSOUND_SPUregisterCDDAVolume}, \
		{ "SPUplayCDDAchannel", \
	      DFSOUND_SPUplayCDDAchannel}, \
	    { "SPUasync", \
	      DFSOUND_SPUasync} \
	       } }
      
// Note: PEOPS110_SPU* functions removed - using DFSOUND instead

#define GPU_NULL_PLUGIN \
	{ "GPU",      \
	  10,         \
	  { { "GPUinit",  \
	      GPU__init }, \
	    { "GPUshutdown",	\
	      GPU__shutdown}, \
	    { "GPUopen", \
	      GPU__open}, \
	    { "GPUclose", \
	      GPU__close}, \
	    { "GPUwriteStatus", \
	      GPU__writeStatus}, \
	    { "GPUwriteData", \
	      GPU__writeData}, \
	    { "GPUreadStatus", \
	      GPU__readStatus}, \
	    { "GPUreadData", \
	      GPU__readData}, \
	    { "GPUdmaChain", \
	      GPU__dmaChain}, \
	    { "GPUupdateLace", \
	      GPU__updateLace} \
	       } }

#define GPU_PEOPS_PLUGIN \
	{ "GPU",      \
	  15,         \
	  { { "GPUinit",  \
	      PEOPS_GPUinit }, \
	    { "GPUshutdown",	\
	      PEOPS_GPUshutdown}, \
	    { "GPUopen", \
	      PEOPS_GPUopen}, \
	    { "GPUclose", \
	      PEOPS_GPUclose}, \
	    { "GPUwriteStatus", \
	      PEOPS_GPUwriteStatus}, \
	    { "GPUwriteData", \
	      PEOPS_GPUwriteData}, \
	    { "GPUwriteDataMem", \
	      PEOPS_GPUwriteDataMem}, \
	    { "GPUreadStatus", \
	      PEOPS_GPUreadStatus}, \
	    { "GPUreadData", \
	      PEOPS_GPUreadData}, \
	    { "GPUreadDataMem", \
	      PEOPS_GPUreadDataMem}, \
	    { "GPUdmaChain", \
	      PEOPS_GPUdmaChain}, \
	    { "GPUdisplayText", \
	      PEOPS_GPUdisplayText}, \
	    { "GPUfreeze", \
	      PEOPS_GPUfreeze}, \
		{ "GPUvBlank", \
			PEOPS_GPUvBlank}, \
	    { "GPUupdateLace", \
	      PEOPS_GPUupdateLace} \
	       } }


/*
GPUopen 
GPUinit
GPUshutdown
GPUclose
GPUwriteStatus
GPUwriteData
GPUwriteDataMem
GPUreadStatus
GPUreadData
GPUreadDataMem
GPUdmaChain
GPUupdateLace
GPUdisplayText
GPUfreeze
GPUvBlank
*/

#define PLUGIN_SLOT_0 EMPTY_PLUGIN
#define PLUGIN_SLOT_1 PAD1_PLUGIN
#define PLUGIN_SLOT_2 PAD2_PLUGIN
#define PLUGIN_SLOT_3 EMPTY_PLUGIN//CDR_PLUGIN
#define PLUGIN_SLOT_4 SPU_DFSOUND_PLUGIN
#define PLUGIN_SLOT_5 GPU_PEOPS_PLUGIN
#define PLUGIN_SLOT_6 EMPTY_PLUGIN
#define PLUGIN_SLOT_7 EMPTY_PLUGIN


#endif

