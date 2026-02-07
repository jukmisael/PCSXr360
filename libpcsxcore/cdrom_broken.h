/***************************************************************************
 *   Copyright (C) 2025                                                    *
 *   PSX CD-ROM Controller - Rewritten for accuracy                        *
 *   Based on DuckStation and Mednafen specifications                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef __CDROM_H__
#define __CDROM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "psxcommon.h"
#include "decode_xa.h"

// ========================================================================
// CONSTANTS - For backward compatibility
// ========================================================================

#define CD_FRAMESIZE_RAW        2352
#define DATA_SIZE               (CD_FRAMESIZE_RAW - 12)
#define SUB_FRAMESIZE           96

// Clock and base timing
#define PSXCLK                      33868800
#define CDROM_CLOCK                 (PSXCLK / 2)

// Seek timing (in CDROM clock cycles)
#define MIN_SEEK_TICKS              33868
#define SEEK_TICKS_SHORT            135472
#define SEEK_TICKS_MEDIUM_BASE      677360
#define SEEK_TICKS_LONG_BASE        3386880

// Sector timing
#define SECTOR_TICKS_1X             (PSXCLK / 75)
#define SECTOR_TICKS_2X             (PSXCLK / 150)

// Command delays
#define DELAY_NOP                   1000
#define DELAY_GETSTAT               1000
#define DELAY_SETLOC                1000
#define DELAY_PLAY                  2000
#define DELAY_SEEK                  10000
#define DELAY_READ                  2000
#define DELAY_PAUSE                 10000
#define DELAY_STOP                  15000
#define DELAY_INIT                  4000000
#define DELAY_RESET                 2000000
#define DELAY_MOTOR_ON              3000000
#define DELAY_READTOC               45000
#define DELAY_GETID                 33868
#define DELAY_STANDBY               2000000

// Status bits
#define STATUS_ERROR                (1 << 0)
#define STATUS_SPINNING             (1 << 1)
#define STATUS_READING              (1 << 2)
#define STATUS_SEEKING              (1 << 3)
#define STATUS_PLAYING              (1 << 4)
#define STATUS_CHECK_ERROR          (1 << 5)

// Mode bits
#define MODE_CDDA                   (1 << 0)
#define MODE_AUTO_PAUSE             (1 << 1)
#define MODE_REPORT_IRQ             (1 << 2)
#define MODE_XA_FILTER              (1 << 3)
#define MODE_IGNORE_BIT             (1 << 4)
#define MODE_SIZE_2340              (1 << 5)
#define MODE_SIZE_2328              (1 << 6)
#define MODE_SPEED                  (1 << 7)

// FIFO sizes
#define PARAM_FIFO_SIZE             16
#define RESPONSE_FIFO_SIZE          16
#define DATA_FIFO_SIZE              2352
#define NUM_SECTOR_BUFFERS          8

// Legacy CD-ROM commands (for backward compatibility)
#define CdlSync                     0
#define CdlNop                      1
#define CdlSetloc                   2
#define CdlPlay                     3
#define CdlForward                  4
#define CdlBackward                 5
#define CdlReadN                    6
#define CdlStandby                  7
#define CdlStop                     8
#define CdlPause                    9
#define CdlInit                     10
#define CdlMute                     11
#define CdlDemute                   12
#define CdlSetfilter                13
#define CdlSetmode                  14
#define CdlGetmode                  15
#define CdlGetlocL                  16
#define CdlGetlocP                  17
#define CdlReadT                    18
#define CdlGetTN                    19
#define CdlGetTD                    20
#define CdlSeekL                    21
#define CdlSeekP                    22
#define CdlSetclock                 23
#define CdlGetclock                 24
#define CdlTest                     25
#define CdlGetID                    26
#define CdlReadS                    27
#define CdlReset                    28
#define CdlGetQ                     29
#define CdlReadTOC                  30

// Legacy status values
#define NoIntr                      0
#define Acknowledge                 1
#define DataReady                   2
#define Complete                    3
#define DiskError                   5

// Legacy seek state
#define SEEK_PENDING                0
#define SEEK_DONE                   1

// ========================================================================
// TYPES
// ========================================================================

typedef struct {
    u8 data[PARAM_FIFO_SIZE];
    u8 read_pos;
    u8 write_pos;
    u8 size;
} FIFO_Param;

typedef struct {
    u8 data[RESPONSE_FIFO_SIZE];
    u8 read_pos;
    u8 write_pos;
    u8 size;
} FIFO_Response;

// Drive states
enum DriveState {
    DRIVESTATE_IDLE = 0,
    DRIVESTATE_OPENING_SHELL,
    DRIVESTATE_RESETTING,
    DRIVESTATE_SPINNING_UP,
    DRIVESTATE_SEEKING_PHYSICAL,
    DRIVESTATE_SEEKING_LOGICAL,
    DRIVESTATE_READING,
    DRIVESTATE_PLAYING,
    DRIVESTATE_PAUSING,
    DRIVESTATE_STOPPING,
    DRIVESTATE_ERROR
};

// Command states
enum CmdState {
    CMDSTATE_IDLE = 0,
    CMDSTATE_EXECUTING,
    CMDSTATE_WAITING_IRQ,
    CMDSTATE_SECOND_RESPONSE
};

// Seek types
enum SeekType {
    SEEKTYPE_NONE = 0,
    SEEKTYPE_FORWARD,
    SEEKTYPE_TRACK_JUMP,
    SEEKTYPE_MEDIUM,
    SEEKTYPE_LONG
};

// Legacy cdrStruct for backward compatibility
typedef struct {
    unsigned char OCUP;
    unsigned char Reg1Mode;
    unsigned char Reg2;
    unsigned char CmdProcess;
    unsigned char Ctrl;
    unsigned char Stat;
    unsigned char StatP;
    unsigned char Transfer[CD_FRAMESIZE_RAW];
    unsigned int transferIndex;
    unsigned char Prev[4];
    unsigned char Param[8];
    unsigned char Result[16];
    unsigned char ParamC;
    unsigned char ParamP;
    unsigned char ResultC;
    unsigned char ResultP;
    unsigned char ResultReady;
    unsigned char Cmd;
    unsigned char Readed;
    unsigned char SetlocPending;
    u32 Reading;
    unsigned char ResultTN[6];
    unsigned char ResultTD[4];
    unsigned char SetSectorPlay[4];
    unsigned char SetSectorEnd[4];
    unsigned char SetSector[4];
    unsigned char Track;
    boolean Play, Muted;
    int CurTrack;
    int Mode, File, Channel;
    int Reset;
    int RErr;
    int FirstSector;
    xa_decode_t Xa;
    int Init;
    u16 Irq;
    u8 IrqRepeated;
    u32 eCycle;
    u8 Seeked;
    u8 ReadRescheduled;
    u8 DriveState;
    u8 FastForward;
    u8 FastBackward;
    u8 AttenuatorLeftToLeft, AttenuatorLeftToRight;
    u8 AttenuatorRightToRight, AttenuatorRightToLeft;
    u8 AttenuatorLeftToLeftT, AttenuatorLeftToRightT;
    u8 AttenuatorRightToRightT, AttenuatorRightToLeftT;
    struct {
        unsigned char Track;
        unsigned char Index;
        unsigned char Relative[3];
        unsigned char Absolute[3];
    } subq;
    unsigned char TrackChanged;
    
    // New fields for enhanced timing
    u32 SeekTargetLBA;
    u32 SeekStartLBA;
    u32 SeekTicks;
    u32 CmdStartTime;
    u32 CurrentLBA;
    u8 CmdState;
    u8 LastCmd;
    u8 CurrentPriority;
    u8 PreemptedIrq;
    u32 PreemptedCycle;
    u8 StatP_History[8];
    u8 StatP_HistoryIdx;
    u32 NextInterruptTime;
    u8 SecondResponse;
} cdrStruct;

// CDROM State Structure (for new implementation)
typedef struct {
    // Position tracking
    u8 current_pos[3];
    u8 target_pos[3];
    u32 current_lba;
    u32 target_lba;
    
    // Seek information
    u32 seek_distance;
    enum SeekType seek_type;
    u32 seek_start_time;
    u32 seek_end_time;
    u8 seek_pending;
    
    // Status
    u8 stat;
    u8 stat_p;
    u8 mode;
    u8 ctrl;
    
    // Drive state
    enum DriveState drive_state;
    enum CmdState cmd_state;
    u8 motor_on;
    u8 shell_open;
    
    // Command processing
    u8 current_cmd;
    u8 cmd_param[PARAM_FIFO_SIZE];
    u8 cmd_param_count;
    u8 cmd_executing;
    u32 cmd_start_cycle;
    
    // Response
    FIFO_Response response_fifo;
    u8 irq_pending;
    u8 irq_type;
    
    // Data
    u8 transfer_buffer[DATA_FIFO_SIZE];
    u32 transfer_index;
    u32 transfer_size;
    u8 data_ready;
    
    // Sector buffering
    u8 sector_buffer[NUM_SECTOR_BUFFERS][DATA_FIFO_SIZE];
    u8 sector_buffer_valid[NUM_SECTOR_BUFFERS];
    u32 sector_buffer_lba[NUM_SECTOR_BUFFERS];
    u8 current_buffer;
    
    // XA Audio
    xa_decode_t xa;
    u8 xa_playing;
    u8 xa_channel;
    u8 xa_file;
    
    // CDDA
    u8 cdda_playing;
    u8 cdda_track;
    u8 cdda_report_count;
    
    // Timing
    u32 last_update_cycle;
    u32 next_event_cycle;
    u32 irq_delay;
    
    // Error tracking
    u8 error_code;
    u8 read_error_count;
    
    // Game ID
    char game_id[12];
} CDROM_State;

// ========================================================================
// GLOBALS
// ========================================================================

extern cdrStruct cdr;
extern char CdromId[12];
extern CDROM_State *cdr_new;

// ========================================================================
// FUNCTIONS
// ========================================================================

// Core functions
void cdrReset(void);
void cdrInterrupt(void);
void cdrReadInterrupt(void);
void cdrPlayInterrupt(void);
void cdrLidSeekInterrupt(void);

// Legacy interrupt functions
void LidInterrupt(void);
void CDRDBUF_INT(u32 eCycle);
void CDR_INT(u32 eCycle);
void CDREAD_INT(u32 eCycle);
void CDRPLAY_INT(u32 eCycle);
void CDRMISC_INT(u32 eCycle);
void CDRLID_INT(u32 eCycle);
void CDRDMA_INT(u32 eCycle);

// Register interface
u8 cdrRead0(void);
u8 cdrRead1(void);
u8 cdrRead2(void);
u8 cdrRead3(void);
void cdrWrite0(u8 value);
void cdrWrite1(u8 value);
void cdrWrite2(u8 value);
void cdrWrite3(u8 value);

// Plugin interface
void ReadTrack(u8 *time);
u8 * CDR_getBuffer(void);
s32 CDR_readCDDA(u8 min, u8 sec, u8 frame, u8 *buffer);
void CDR_stop(void);
s32 CDR_getStatus(void);
void CDR_getTN(u8 *buffer);
void CDR_getTD(u8 track, u8 *buffer);

// Save state
int cdrFreeze(gzFile f, int Mode);

// Utility
void cdrGetGameID(char *game_id);
void StopReading(void);
void StopCdda(void);
void Find_CurTrack(u8 *time);
void CheckPPFCache(u8 *buffer, u8 m, u8 s, u8 f);
void cdrAttenuate(s16 *buf, int samples, int stereo);

// Helper macros
#define btoi(b)     ((b) / 16 * 10 + (b) % 16)
#define itob(i)     ((i) / 10 * 16 + (i) % 10)
#define MSF2SECT(m, s, f)   (((m) * 60 + (s) - 2) * 75 + (f))

#ifdef __cplusplus
}
#endif

#endif
