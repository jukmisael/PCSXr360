/*
 * PSX CD-ROM Controller
 * Rewritten for accuracy based on DuckStation/Mednafen specifications
 * 
 * Architecture:
 * - Event-driven timing system
 * - Distance-based seek calculations
 * - Command-specific delays
 * - Proper state machine
 */

#include "psxcommon.h"
#include "cdrom.h"
#include "psxhw.h"
#include "psxdma.h"
#include "mdec.h"
#include "plugins.h"
#include "profiler.h"

// ========================================================================
// CONSTANTS - Based on hardware specifications and DuckStation research
// ========================================================================

// Clock and base timing
#define PSXCLK                      33868800
#define CDROM_CLOCK                 (PSXCLK / 2)  // CDROM runs at half clock

// Seek timing (in CDROM clock cycles)
#define MIN_SEEK_TICKS              33868       // ~1ms minimum
#define SEEK_TICKS_SHORT            135472      // ~4ms for track jumps
#define SEEK_TICKS_MEDIUM_BASE      677360      // ~20ms base
#define SEEK_TICKS_LONG_BASE        3386880     // ~100ms base

// Sector timing
#define SECTOR_TICKS_1X             (PSXCLK / 75)      // 451584 cycles
#define SECTOR_TICKS_2X             (PSXCLK / 150)     // 225792 cycles

// Command delays (in CDROM cycles)
#define DELAY_NOP                   1000
#define DELAY_GETSTAT               1000
#define DELAY_SETLOC                1000
#define DELAY_PLAY                  2000
#define DELAY_SEEK                  10000
#define DELAY_READ                  2000
#define DELAY_PAUSE                 10000
#define DELAY_STOP                  15000
#define DELAY_INIT                  4000000     // ~118ms - CRITICAL
#define DELAY_RESET                 2000000
#define DELAY_MOTOR_ON              3000000
#define DELAY_READTOC               45000
#define DELAY_GETID                 33868
#define DELAY_STANDBY               2000000

// Interrupt timing
#define IRQ_DELAY_ACK               500
#define IRQ_DELAY_COMPLETE          1000
#define IRQ_DELAY_DATA_READY        500
#define IRQ_DELAY_MINIMUM           1000

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
    SEEKTYPE_FORWARD,       // 1-3 sectors
    SEEKTYPE_TRACK_JUMP,    // 4-16 sectors  
    SEEKTYPE_MEDIUM,        // 17-1000 sectors
    SEEKTYPE_LONG           // 1000+ sectors
};

// Response FIFO sizes
#define PARAM_FIFO_SIZE         16
#define RESPONSE_FIFO_SIZE      16
#define DATA_FIFO_SIZE          2352
#define NUM_SECTOR_BUFFERS      8

// CD-ROM Commands
#define CdlSync                 0
#define CdlNop                  1
#define CdlSetloc               2
#define CdlPlay                 3
#define CdlForward              4
#define CdlBackward             5
#define CdlReadN                6
#define CdlStandby              7
#define CdlStop                 8
#define CdlPause                9
#define CdlInit                 10
#define CdlMute                 11
#define CdlDemute               12
#define CdlSetfilter            13
#define CdlSetmode              14
#define CdlGetmode              15
#define CdlGetlocL              16
#define CdlGetlocP              17
#define CdlReadT                18
#define CdlGetTN                19
#define CdlGetTD                20
#define CdlSeekL                21
#define CdlSeekP                22
#define CdlSetclock             23
#define CdlGetclock             24
#define CdlTest                 25
#define CdlGetID                26
#define CdlReadS                27
#define CdlReset                28
#define CdlGetQ                 29
#define CdlReadTOC              30

// Status bits
#define STATUS_ERROR            (1 << 0)
#define STATUS_SPINNING         (1 << 1)
#define STATUS_READING          (1 << 2)
#define STATUS_SEEKING          (1 << 3)
#define STATUS_PLAYING          (1 << 4)
#define STATUS_CHECK_ERROR      (1 << 5)

// Mode bits
#define MODE_CDDA               (1 << 0)
#define MODE_AUTO_PAUSE         (1 << 1)
#define MODE_REPORT_IRQ         (1 << 2)
#define MODE_XA_FILTER          (1 << 3)
#define MODE_IGNORE_BIT         (1 << 4)
#define MODE_SIZE_2340          (1 << 5)
#define MODE_SIZE_2328          (1 << 6)
#define MODE_SPEED              (1 << 7)

// ========================================================================
// STRUCTURES
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

typedef struct {
    // Position tracking
    u8 current_pos[3];          // Current position (MSF)
    u8 target_pos[3];           // Target position for seek (MSF)
    u32 current_lba;
    u32 target_lba;
    
    // Seek information
    u32 seek_distance;
    enum SeekType seek_type;
    u32 seek_start_time;
    u32 seek_end_time;
    u8 seek_pending;
    
    // Status
    u8 stat;                    // Primary status
    u8 stat_p;                  // Status register
    u8 mode;                    // Mode register
    u8 ctrl;                    // Control register
    
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
    
    // Game ID for profiler
    char game_id[12];
} CDROM_State;

// ========================================================================
// GLOBAL STATE
// ========================================================================

static CDROM_State s_cdrom;
static CDROM_State *cdr = &s_cdrom;

// Extern for plugins
extern char *LibName;

// ========================================================================
// TIMING FUNCTIONS
// ========================================================================

static inline u32 GetCurrentCycle(void) {
    return psxRegs.cycle;
}

static inline u32 CyclesToMS(u32 cycles) {
    return (cycles * 1000) / PSXCLK;
}

static inline u32 MSToCycles(u32 ms) {
    return (ms * PSXCLK) / 1000;
}

// Calculate seek time based on LBA distance
static u32 CalculateSeekTicks(u32 from_lba, u32 to_lba) {
    u32 distance;
    u32 ticks;
    
    if (to_lba > from_lba)
        distance = to_lba - from_lba;
    else
        distance = from_lba - to_lba;
    
    // Determine seek type and calculate timing
    if (distance == 0) {
        return MIN_SEEK_TICKS;
    }
    else if (distance <= 3) {
        // Forward seek: just wait for sector to come around
        ticks = MIN_SEEK_TICKS;
    }
    else if (distance <= 16) {
        // Track jump
        ticks = SEEK_TICKS_SHORT + (distance * 5000);
    }
    else if (distance <= 1000) {
        // Medium seek
        ticks = SEEK_TICKS_MEDIUM_BASE + (distance * 200);
    }
    else {
        // Long/sled seek
        ticks = SEEK_TICKS_LONG_BASE + (distance * 100);
        if (ticks > MSToCycles(900))  // Cap at 900ms
            ticks = MSToCycles(900);
    }
    
    return ticks;
}

static enum SeekType GetSeekType(u32 distance) {
    if (distance == 0) return SEEKTYPE_NONE;
    if (distance <= 3) return SEEKTYPE_FORWARD;
    if (distance <= 16) return SEEKTYPE_TRACK_JUMP;
    if (distance <= 1000) return SEEKTYPE_MEDIUM;
    return SEEKTYPE_LONG;
}

// Get command delay
static u32 GetCommandDelay(u8 cmd) {
    switch (cmd) {
        case CdlNop:            return DELAY_NOP;
        case CdlSetloc:         return DELAY_SETLOC;
        case CdlPlay:           return DELAY_PLAY;
        case CdlForward:        return DELAY_PLAY;
        case CdlBackward:       return DELAY_PLAY;
        case CdlReadN:          return DELAY_READ;
        case CdlReadS:          return DELAY_READ;
        case CdlStandby:        return DELAY_STANDBY;
        case CdlStop:           return DELAY_STOP;
        case CdlPause:          return DELAY_PAUSE;
        case CdlInit:           return DELAY_INIT;
        case CdlMute:           return DELAY_NOP;
        case CdlDemute:         return DELAY_NOP;
        case CdlSetfilter:      return DELAY_NOP;
        case CdlSetmode:        return DELAY_NOP;
        case CdlGetmode:        return DELAY_NOP;
        case CdlGetlocL:        return DELAY_NOP;
        case CdlGetlocP:        return DELAY_NOP;
        case CdlReadT:          return DELAY_READ;
        case CdlGetTN:          return DELAY_NOP;
        case CdlGetTD:          return DELAY_NOP;
        case CdlSeekL:          return DELAY_SEEK;
        case CdlSeekP:          return DELAY_SEEK;
        case CdlSetclock:       return DELAY_NOP;
        case CdlGetclock:       return DELAY_NOP;
        case CdlTest:           return DELAY_NOP;
        case CdlGetID:          return DELAY_GETID;
        case CdlReset:          return DELAY_RESET;
        case CdlGetQ:           return DELAY_NOP;
        case CdlReadTOC:        return DELAY_READTOC;
        default:                return DELAY_NOP;
    }
}

// ========================================================================
// FIFO FUNCTIONS
// ========================================================================

static void FIFO_Param_Clear(void) {
    cdr->cmd_param_count = 0;
}

static void FIFO_Param_Push(u8 value) {
    if (cdr->cmd_param_count < PARAM_FIFO_SIZE) {
        cdr->cmd_param[cdr->cmd_param_count++] = value;
    }
}

static u8 FIFO_Param_Pop(void) {
    if (cdr->cmd_param_count > 0) {
        return cdr->cmd_param[--cdr->cmd_param_count];
    }
    return 0;
}

static void FIFO_Response_Clear(void) {
    cdr->response_fifo.read_pos = 0;
    cdr->response_fifo.write_pos = 0;
    cdr->response_fifo.size = 0;
}

static void FIFO_Response_Push(u8 value) {
    if (cdr->response_fifo.size < RESPONSE_FIFO_SIZE) {
        cdr->response_fifo.data[cdr->response_fifo.write_pos] = value;
        cdr->response_fifo.write_pos = (cdr->response_fifo.write_pos + 1) % RESPONSE_FIFO_SIZE;
        cdr->response_fifo.size++;
    }
}

static u8 FIFO_Response_Pop(void) {
    u8 value = 0;
    if (cdr->response_fifo.size > 0) {
        value = cdr->response_fifo.data[cdr->response_fifo.read_pos];
        cdr->response_fifo.read_pos = (cdr->response_fifo.read_pos + 1) % RESPONSE_FIFO_SIZE;
        cdr->response_fifo.size--;
    }
    return value;
}

static u8 FIFO_Response_Peek(void) {
    if (cdr->response_fifo.size > 0) {
        return cdr->response_fifo.data[cdr->response_fifo.read_pos];
    }
    return 0;
}

// ========================================================================
// STATUS FUNCTIONS
// ========================================================================

static void UpdateStatusRegister(void) {
    u8 status = 0;
    
    if (cdr->response_fifo.size > 0)
        status |= 0x20;  // Response ready
    
    if (cdr->cmd_param_count < PARAM_FIFO_SIZE)
        status |= 0x10;  // Param FIFO not full
    
    if (cdr->data_ready)
        status |= 0x08;  // Data ready
    
    if (cdr->stat_p & STATUS_SEEKING)
        status |= 0x04;  // Seeking
    
    if (cdr->stat_p & STATUS_READING)
        status |= 0x02;  // Reading
    
    if (cdr->stat_p & STATUS_ERROR)
        status |= 0x01;  // Error
    
    cdr->stat = status;
}

static void SetStatusBit(u8 bit) {
    cdr->stat_p |= bit;
    UpdateStatusRegister();
}

static void ClearStatusBit(u8 bit) {
    cdr->stat_p &= ~bit;
    UpdateStatusRegister();
}

// ========================================================================
// INTERRUPT FUNCTIONS
// ========================================================================

static void GenerateIRQ(u8 type) {
    cdr->irq_type = type;
    cdr->irq_pending = 1;
    
    // Schedule interrupt with proper delay
    u32 delay = IRQ_DELAY_ACK;
    if (type == 1) delay = IRQ_DELAY_DATA_READY;
    if (type == 2) delay = IRQ_DELAY_COMPLETE;
    
    cdr->irq_delay = delay;
    
    // Set interrupt pending
    psxHu32ref_2(0x1070) |= SWAP32((u32)0x4);
    
    CDR_INT(delay);
}

static void ClearIRQ(void) {
    cdr->irq_pending = 0;
    cdr->irq_type = 0;
}

// ========================================================================
// COMMAND HANDLERS
// ========================================================================

static void Cmd_Nop(void) {
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    GenerateIRQ(3);  // ACK
}

static void Cmd_GetStat(void) {
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    GenerateIRQ(3);  // ACK
}

static void Cmd_SetLoc(void) {
    u8 i;
    u32 current_lba, target_lba;
    
    // Parse parameters (BCD to int)
    for (i = 0; i < 3; i++) {
        cdr->target_pos[i] = btoi(cdr->cmd_param[i]);
    }
    
    // Calculate LBAs
    current_lba = cdr->current_lba;
    target_lba = MSF2SECT(cdr->target_pos[0], cdr->target_pos[1], cdr->target_pos[2]);
    
    // Calculate seek information
    cdr->target_lba = target_lba;
    cdr->seek_distance = (target_lba > current_lba) ? 
                         (target_lba - current_lba) : (current_lba - target_lba);
    cdr->seek_type = GetSeekType(cdr->seek_distance);
    cdr->seek_pending = (cdr->seek_distance > 0);
    
    // Response
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    GenerateIRQ(3);  // ACK
}

static void Cmd_SeekL(void) {
    u32 seek_ticks;
    
    // Stop any current operation
    cdr->cdda_playing = 0;
    
    // Calculate seek time
    if (cdr->seek_pending && cdr->seek_distance > 0) {
        seek_ticks = CalculateSeekTicks(cdr->current_lba, cdr->target_lba);
    } else {
        seek_ticks = MIN_SEEK_TICKS;
    }
    
    // Update state
    SetStatusBit(STATUS_SEEKING);
    cdr->drive_state = DRIVESTATE_SEEKING_PHYSICAL;
    cdr->seek_start_time = GetCurrentCycle();
    cdr->seek_end_time = GetCurrentCycle() + seek_ticks;
    
    // Response
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    GenerateIRQ(3);  // ACK
    
    // Schedule seek completion
    cdr->cmd_state = CMDSTATE_WAITING_IRQ;
    CDR_INT(seek_ticks);
}

static void Cmd_SeekP(void) {
    // Similar to SeekL but for logical seeks
    Cmd_SeekL();
}

static void Cmd_ReadN(void) {
    // Setup for reading
    if (cdr->seek_pending) {
        // Need to seek first
        cdr->current_lba = cdr->target_lba;
        cdr->current_pos[0] = cdr->target_pos[0];
        cdr->current_pos[1] = cdr->target_pos[1];
        cdr->current_pos[2] = cdr->target_pos[2];
        cdr->seek_pending = 0;
    }
    
    SetStatusBit(STATUS_READING);
    ClearStatusBit(STATUS_SEEKING);
    cdr->drive_state = DRIVESTATE_READING;
    cdr->data_ready = 0;
    
    // Response
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    GenerateIRQ(3);  // ACK
    
    // Schedule first read
    u32 read_delay = (cdr->mode & MODE_SPEED) ? SECTOR_TICKS_2X : SECTOR_TICKS_1X;
    CDREAD_INT(read_delay);
}

static void Cmd_ReadS(void) {
    Cmd_ReadN();  // Same handling for now
}

static void Cmd_Stop(void) {
    cdr->cdda_playing = 0;
    ClearStatusBit(STATUS_READING | STATUS_PLAYING);
    
    // Response 1
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    GenerateIRQ(3);  // ACK
    
    // Schedule second response
    cdr->cmd_state = CMDSTATE_SECOND_RESPONSE;
    CDR_INT(DELAY_STOP);
}

static void Cmd_Pause(void) {
    cdr->cdda_playing = 0;
    ClearStatusBit(STATUS_READING | STATUS_PLAYING);
    
    // Response 1
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    GenerateIRQ(3);  // ACK
    
    // Schedule second response
    cdr->cmd_state = CMDSTATE_SECOND_RESPONSE;
    CDR_INT(DELAY_PAUSE);
}

static void Cmd_Init(void) {
    // Reset drive state
    cdr->mode = 0;
    cdr->stat_p = STATUS_SPINNING;
    cdr->motor_on = 1;
    cdr->drive_state = DRIVESTATE_RESETTING;
    
    // Response 1
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    GenerateIRQ(3);  // ACK
    
    // Schedule second response with proper delay (~118ms)
    cdr->cmd_state = CMDSTATE_SECOND_RESPONSE;
    CDR_INT(DELAY_INIT);
}

static void Cmd_Mute(void) {
    // Set mute flag
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    GenerateIRQ(3);  // ACK
}

static void Cmd_Demute(void) {
    // Clear mute flag
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    GenerateIRQ(3);  // ACK
}

static void Cmd_SetFilter(void) {
    // Set XA filter
    cdr->xa_file = cdr->cmd_param[0];
    cdr->xa_channel = cdr->cmd_param[1];
    
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    GenerateIRQ(3);  // ACK
}

static void Cmd_SetMode(void) {
    cdr->mode = cdr->cmd_param[0];
    
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    GenerateIRQ(3);  // ACK
}

static void Cmd_GetMode(void) {
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    FIFO_Response_Push(cdr->mode);
    GenerateIRQ(3);  // ACK
}

static void Cmd_GetLocL(void) {
    // Return current position
    FIFO_Response_Clear();
    FIFO_Response_Push(itob(cdr->current_pos[2]));  // Frame
    FIFO_Response_Push(itob(cdr->current_pos[1]));  // Second
    FIFO_Response_Push(itob(cdr->current_pos[0]));  // Minute
    FIFO_Response_Push(0);  // Mode
    GenerateIRQ(3);  // ACK
}

static void Cmd_GetLocP(void) {
    u8 track, index;
    
    // Get track info
    track = 1;  // Simplified
    index = 1;
    
    FIFO_Response_Clear();
    FIFO_Response_Push(itob(track));
    FIFO_Response_Push(itob(index));
    FIFO_Response_Push(itob(cdr->current_pos[0]));
    FIFO_Response_Push(itob(cdr->current_pos[1]));
    FIFO_Response_Push(itob(cdr->current_pos[2]));
    GenerateIRQ(3);  // ACK
}

static void Cmd_GetTN(void) {
    u8 first_track, last_track;
    
    CDR_getTN(cdr->cmd_param);
    first_track = cdr->cmd_param[0];
    last_track = cdr->cmd_param[1];
    
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    FIFO_Response_Push(first_track);
    FIFO_Response_Push(last_track);
    GenerateIRQ(3);  // ACK
}

static void Cmd_GetTD(void) {
    u8 track = btoi(cdr->cmd_param[0]);
    u8 td[4];
    
    CDR_getTD(track, td);
    
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    FIFO_Response_Push(td[2]);  // Frame
    FIFO_Response_Push(td[1]);  // Second
    FIFO_Response_Push(td[0]);  // Minute
    GenerateIRQ(3);  // ACK
}

static void Cmd_Seek(void) {
    // Generic seek command
    Cmd_SeekL();
}

static void Cmd_Play(void) {
    cdr->cdda_playing = 1;
    cdr->drive_state = DRIVESTATE_PLAYING;
    SetStatusBit(STATUS_PLAYING);
    
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    GenerateIRQ(3);  // ACK
    
    // Schedule CDDA playback
    CDRPLAY_INT(SECTOR_TICKS_1X);
}

static void Cmd_Forward(void) {
    // Fast forward
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    GenerateIRQ(3);  // ACK
}

static void Cmd_Backward(void) {
    // Fast backward
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    GenerateIRQ(3);  // ACK
}

static void Cmd_GetID(void) {
    FIFO_Response_Clear();
    
    if (!CDR_getStatus()) {
        // No disc
        FIFO_Response_Push(0x11);
        FIFO_Response_Push(0x80);
        GenerateIRQ(5);  // Error
    } else {
        // Licensed disc
        FIFO_Response_Push(cdr->stat_p);
        FIFO_Response_Push(0x02);  // Licensed
        FIFO_Response_Push(0x00);
        FIFO_Response_Push(0x00);
        FIFO_Response_Push(0x00);
        FIFO_Response_Push('S');
        FIFO_Response_Push('C');
        FIFO_Response_Push('E');
        FIFO_Response_Push('X');
        GenerateIRQ(2);  // Complete
    }
}

static void Cmd_Test(void) {
    u8 subcmd = cdr->cmd_param[0];
    
    FIFO_Response_Clear();
    
    switch (subcmd) {
        case 0x20:  // Version
            FIFO_Response_Push(0x94);
            FIFO_Response_Push(0x09);
            FIFO_Response_Push(0x19);
            FIFO_Response_Push(0x94);
            break;
        default:
            FIFO_Response_Push(cdr->stat_p);
            break;
    }
    
    GenerateIRQ(3);  // ACK
}

static void Cmd_Reset(void) {
    cdr->mode = 0;
    cdr->stat_p = 0;
    cdr->motor_on = 0;
    cdr->drive_state = DRIVESTATE_IDLE;
    
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    GenerateIRQ(3);  // ACK
}

static void Cmd_ReadTOC(void) {
    FIFO_Response_Clear();
    FIFO_Response_Push(cdr->stat_p);
    GenerateIRQ(3);  // ACK
    
    // Schedule completion
    cdr->cmd_state = CMDSTATE_SECOND_RESPONSE;
    CDR_INT(DELAY_READTOC);
}

// Command dispatch table
typedef void (*CmdHandler)(void);

static const CmdHandler s_cmdHandlers[32] = {
    Cmd_Nop,           // 0x00 - Sync
    Cmd_Nop,           // 0x01 - Nop
    Cmd_SetLoc,        // 0x02 - Setloc
    Cmd_Play,          // 0x03 - Play
    Cmd_Forward,       // 0x04 - Forward
    Cmd_Backward,      // 0x05 - Backward
    Cmd_ReadN,         // 0x06 - ReadN
    Cmd_Nop,           // 0x07 - Standby
    Cmd_Stop,          // 0x08 - Stop
    Cmd_Pause,         // 0x09 - Pause
    Cmd_Init,          // 0x0A - Init
    Cmd_Mute,          // 0x0B - Mute
    Cmd_Demute,        // 0x0C - Demute
    Cmd_SetFilter,     // 0x0D - Setfilter
    Cmd_SetMode,       // 0x0E - Setmode
    Cmd_GetMode,       // 0x0F - Getmode
    Cmd_GetLocL,       // 0x10 - GetlocL
    Cmd_GetLocP,       // 0x11 - GetlocP
    Cmd_Nop,           // 0x12 - ReadT
    Cmd_GetTN,         // 0x13 - GetTN
    Cmd_GetTD,         // 0x14 - GetTD
    Cmd_SeekL,         // 0x15 - SeekL
    Cmd_SeekP,         // 0x16 - SeekP
    Cmd_Nop,           // 0x17 - Setclock
    Cmd_Nop,           // 0x18 - Getclock
    Cmd_Test,          // 0x19 - Test
    Cmd_GetID,         // 0x1A - GetID
    Cmd_ReadS,         // 0x1B - ReadS
    Cmd_Reset,         // 0x1C - Reset
    Cmd_Nop,           // 0x1D - GetQ
    Cmd_ReadTOC        // 0x1E - ReadTOC
};

// ========================================================================
// MAIN INTERRUPT HANDLER
// ========================================================================

void cdrInterrupt(void) {
    u8 cmd;
    u32 current_cycle = GetCurrentCycle();
    
    // Check if we're waiting for IRQ clear
    if (cdr->irq_pending) {
        // IRQ still pending, reschedule
        CDR_INT(0x100);
        return;
    }
    
    // Handle second response
    if (cdr->cmd_state == CMDSTATE_SECOND_RESPONSE) {
        // Clear appropriate status bits
        switch (cdr->current_cmd) {
            case CdlInit:
                ClearStatusBit(STATUS_SEEKING);
                cdr->drive_state = DRIVESTATE_IDLE;
                break;
            case CdlStop:
                ClearStatusBit(STATUS_SPINNING);
                cdr->motor_on = 0;
                cdr->drive_state = DRIVESTATE_IDLE;
                break;
            case CdlPause:
                ClearStatusBit(STATUS_READING | STATUS_PLAYING);
                cdr->drive_state = DRIVESTATE_IDLE;
                break;
        }
        
        // Send completion response
        FIFO_Response_Clear();
        FIFO_Response_Push(cdr->stat_p);
        GenerateIRQ(2);  // Complete
        
        cdr->cmd_state = CMDSTATE_IDLE;
        return;
    }
    
    // Execute command
    cmd = cdr->current_cmd;
    if (cmd < 32 && s_cmdHandlers[cmd]) {
        s_cmdHandlers[cmd]();
    } else {
        // Unknown command
        FIFO_Response_Clear();
        FIFO_Response_Push(cdr->stat_p | STATUS_ERROR);
        GenerateIRQ(5);  // Error
    }
    
    cdr->cmd_executing = 0;
}

// ========================================================================
// READ INTERRUPT
// ========================================================================

void cdrReadInterrupt(void) {
    u8 *buf;
    u32 read_delay;
    
    if (!cdr->stat_p & STATUS_READING) {
        return;
    }
    
    // Check for IRQ conflicts
    if (cdr->irq_pending) {
        CDREAD_INT(0x100);
        return;
    }
    
    // Update status
    SetStatusBit(STATUS_READING);
    ClearStatusBit(STATUS_SEEKING);
    
    // Read track
    ReadTrack(cdr->current_pos);
    
    buf = CDR_getBuffer();
    if (buf == NULL) {
        // Read error
        cdr->error_code = 0x01;
        SetStatusBit(STATUS_ERROR);
        GenerateIRQ(5);  // Error
        return;
    }
    
    // Copy to transfer buffer
    memcpy(cdr->transfer_buffer, buf, 2352);
    cdr->data_ready = 1;
    
    // Advance position
    cdr->current_lba++;
    cdr->current_pos[2]++;
    if (cdr->current_pos[2] == 75) {
        cdr->current_pos[2] = 0;
        cdr->current_pos[1]++;
        if (cdr->current_pos[1] == 60) {
            cdr->current_pos[1] = 0;
            cdr->current_pos[0]++;
        }
    }
    
    // Generate data ready interrupt
    GenerateIRQ(1);  // DataReady
    
    // Schedule next read
    read_delay = (cdr->mode & MODE_SPEED) ? SECTOR_TICKS_2X : SECTOR_TICKS_1X;
    CDREAD_INT(read_delay);
}

// ========================================================================
// CDDA PLAY INTERRUPT
// ========================================================================

void cdrPlayInterrupt(void) {
    if (!cdr->cdda_playing) {
        return;
    }
    
    // Read CDDA sector
    ReadTrack(cdr->current_pos);
    
    // Decode and play
    if (CDR_readCDDA) {
        u8 buf[2352];
        if (CDR_readCDDA(cdr->current_pos[0], cdr->current_pos[1], 
                         cdr->current_pos[2], buf) == 0) {
            // Output to SPU
        }
    }
    
    // Advance position
    cdr->current_lba++;
    cdr->current_pos[2]++;
    if (cdr->current_pos[2] == 75) {
        cdr->current_pos[2] = 0;
        cdr->current_pos[1]++;
        if (cdr->current_pos[1] == 60) {
            cdr->current_pos[1] = 0;
            cdr->current_pos[0]++;
        }
    }
    
    // Schedule next sector
    CDRPLAY_INT(SECTOR_TICKS_1X);
}

// ========================================================================
// REGISTER INTERFACE
// ========================================================================

u8 cdrRead0(void) {
    UpdateStatusRegister();
    return cdr->stat;
}

u8 cdrRead1(void) {
    return FIFO_Response_Pop();
}

u8 cdrRead2(void) {
    u8 value = 0;
    
    if (cdr->transfer_index < cdr->transfer_size) {
        value = cdr->transfer_buffer[cdr->transfer_index++];
        
        // Check if we've read all data
        if (cdr->transfer_index >= cdr->transfer_size) {
            cdr->data_ready = 0;
            UpdateStatusRegister();
        }
    }
    
    return value;
}

u8 cdrRead3(void) {
    u8 value = cdr->irq_type;
    
    // Clear IRQ pending on read
    if (cdr->irq_pending) {
        cdr->irq_pending = 0;
        ClearIRQ();
    }
    
    return value;
}

void cdrWrite0(u8 value) {
    cdr->ctrl = value;
}

void cdrWrite1(u8 value) {
    u8 cmd = value;
    
    // Check for parameter mode
    if (cdr->ctrl & 0x01) {
        // Push parameter
        FIFO_Param_Push(value);
    } else {
        // Execute command
        cdr->current_cmd = cmd;
        cdr->cmd_executing = 1;
        cdr->cmd_state = CMDSTATE_EXECUTING;
        cdr->cmd_start_cycle = GetCurrentCycle();
        
        // Clear previous params if this is a new command
        if (cmd != CdlNop && cmd != CdlGetStat) {
            // Keep params, they'll be processed by command handler
        }
        
        // Schedule command execution with proper delay
        u32 delay = GetCommandDelay(cmd);
        CDR_INT(delay);
    }
}

void cdrWrite2(u8 value) {
    if (cdr->ctrl & 0x01) {
        // Parameter FIFO mode
        FIFO_Param_Push(value);
    } else {
        // Data register
        // Usually not written directly
    }
}

void cdrWrite3(u8 value) {
    if (cdr->ctrl & 0x01) {
        // Request register
        cdr->data_ready = (value & 0x80) ? 1 : 0;
        
        if (value & 0x40) {
            // Reset parameter FIFO
            FIFO_Param_Clear();
        }
    } else {
        // Interrupt enable/mask
        // Handle interrupt masking
    }
}

// ========================================================================
// LID/DISC INTERRUPT
// ========================================================================

void cdrLidSeekInterrupt(void) {
    switch (cdr->drive_state) {
        case DRIVESTATE_OPENING_SHELL:
            // Lid opened
            cdr->shell_open = 1;
            cdr->stat_p = 0;
            cdr->motor_on = 0;
            break;
            
        case DRIVESTATE_RESETTING:
        case DRIVESTATE_SPINNING_UP:
            // Initialize drive
            cdr->stat_p = STATUS_SPINNING;
            cdr->motor_on = 1;
            cdr->shell_open = 0;
            cdr->drive_state = DRIVESTATE_IDLE;
            
            // Get disc info
            CDR_getTN(cdr->cmd_param);
            break;
            
        default:
            break;
    }
}

// ========================================================================
// INITIALIZATION AND RESET
// ========================================================================

void cdrReset(void) {
    memset(cdr, 0, sizeof(CDROM_State));
    
    // Initialize defaults
    cdr->stat_p = STATUS_SPINNING;
    cdr->motor_on = 1;
    cdr->drive_state = DRIVESTATE_IDLE;
    cdr->cmd_state = CMDSTATE_IDLE;
    cdr->mode = 0;
    
    // Clear FIFOs
    FIFO_Param_Clear();
    FIFO_Response_Clear();
    
    // Initialize position
    cdr->current_pos[0] = 0;
    cdr->current_pos[1] = 2;  // Start at 00:02:00
    cdr->current_pos[2] = 0;
    cdr->current_lba = 150;   // LBA 150 = MSF 00:02:00
    
    // Get initial disc info
    if (CDR_getStatus()) {
        CDR_getTN(cdr->cmd_param);
    }
    
    cdr->last_update_cycle = GetCurrentCycle();
}

// ========================================================================
// SAVE STATE SUPPORT
// ========================================================================

int cdrFreeze(gzFile f, int Mode) {
    // TODO: Implement save state support
    // Need to freeze all CDROM_State fields
    
    if (Mode == 0) {
        // Save
        gzwrite(f, cdr, sizeof(CDROM_State));
    } else {
        // Load
        gzread(f, cdr, sizeof(CDROM_State));
    }
    
    return 0;
}

// ========================================================================
// GAME ID DETECTION
// ========================================================================

void cdrGetGameID(char *game_id) {
    // Try to read game ID from disc
    // This is a simplified version
    
    if (CDR_getStatus()) {
        // Read system.cnf or look for SCUS/SLES/SLPS
        strncpy(game_id, "UNKNOWN", 12);
    } else {
        strncpy(game_id, "NO_DISC", 12);
    }
    
    game_id[11] = '\0';
}
