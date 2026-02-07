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

#ifndef __CDROM_NEW_H__
#define __CDROM_NEW_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "psxcommon.h"
#include "decode_xa.h"

// ========================================================================
// CONSTANTS
// ========================================================================

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

// CDROM State Structure
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

extern CDROM_State *cdr;

// ========================================================================
// FUNCTIONS
// ========================================================================

// Core functions
void cdrReset(void);
void cdrInterrupt(void);
void cdrReadInterrupt(void);
void cdrPlayInterrupt(void);
void cdrLidSeekInterrupt(void);

// Register interface
u8 cdrRead0(void);
u8 cdrRead1(void);
u8 cdrRead2(void);
u8 cdrRead3(void);
void cdrWrite0(u8 value);
void cdrWrite1(u8 value);
void cdrWrite2(u8 value);
void cdrWrite3(u8 value);

// Save state
int cdrFreeze(gzFile f, int Mode);

// Utility
void cdrGetGameID(char *game_id);

// Helper macros
#define btoi(b)     ((b) / 16 * 10 + (b) % 16)
#define itob(i)     ((i) / 10 * 16 + (i) % 10)
#define MSF2SECT(m, s, f)   (((m) * 60 + (s) - 2) * 75 + (f))

#ifdef __cplusplus
}
#endif

#endif
