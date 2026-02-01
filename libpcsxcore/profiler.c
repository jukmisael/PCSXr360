// ============================================================================
// PCSXR Profiler - Simplificado (FPS + Latency em arquivo)
// Arquivo: libpcsxcore/profiler.c
// 
// Metricas: VBlank FPS e Frame Latency apenas
// Atalho: LB + RB + BACK (Select)
// Log: game:\profiling\[GAMEID]_profile.log
// ============================================================================

#include "profiler.h"
#include <stdio.h>
#include <string.h>

// Declaracao externa para GetTickCount (Windows/Xbox 360 API)
extern unsigned int GetTickCount(void);

// Declaracao externa para Config.PsxType (definido em psxcommon.h)
// 0 = NTSC (60 FPS), 1 = PAL (50 FPS)
typedef unsigned char u8;
extern struct {
    u8 PsxType;
} Config;

// ============================================================================
// Configuracoes
// ============================================================================
#define PROFILER_LOG_DIR        "game:\\profiling"
#define FPS_SAMPLE_INTERVAL     1000    // 1 segundo em ms

// Declaracao externa para CreateDirectoryA (Windows/Xbox 360 API)
extern int CreateDirectoryA(const char* lpPathName, void* lpSecurityAttributes);

// ============================================================================
// Estado
// ============================================================================

typedef struct {
    unsigned int active;
    unsigned int initialized;
    char game_id[16];
    
    FILE* log_file;
    char log_filename[256];
    
    // Frame counting
    unsigned int frame_count;
    unsigned int last_calc_time;
    unsigned int current_fps;
    
    // Latency
    unsigned int last_frame_time;
    unsigned int frame_time_ms;
    
    unsigned int start_time;
    
} ProfilerState;

static ProfilerState g_profiler;

// ============================================================================
// Funcoes Internas
// ============================================================================

static void CreateProfilingDir(void) {
    CreateDirectoryA(PROFILER_LOG_DIR, NULL);
}

static void GenerateLogFilename(void) {
    unsigned int ticks = GetTickCount();
    sprintf(g_profiler.log_filename, "%s\\%s_profile_%08x.log", 
            PROFILER_LOG_DIR, 
            g_profiler.game_id[0] ? g_profiler.game_id : "UNKNOWN",
            ticks);
}

// ============================================================================
// Funcoes Publicas
// ============================================================================

void Profiler_Init(const char* game_id, const char* game_name) {
    (void)game_name;
    
    if (g_profiler.log_file) {
        Profiler_Stop();
    }
    
    memset(&g_profiler, 0, sizeof(g_profiler));
    
    if (game_id) {
        strncpy(g_profiler.game_id, game_id, sizeof(g_profiler.game_id) - 1);
    }
    
    g_profiler.initialized = 1;
}

void Profiler_Start(void) {
    if (!g_profiler.initialized) {
        Profiler_Init("UNKNOWN", NULL);
    }
    
    if (g_profiler.active) {
        return;
    }
    
    CreateProfilingDir();
    GenerateLogFilename();
    
    g_profiler.log_file = fopen(g_profiler.log_filename, "w");
    if (!g_profiler.log_file) {
        sprintf(g_profiler.log_filename, "game:\\%s_profile.log", 
                g_profiler.game_id[0] ? g_profiler.game_id : "UNKNOWN");
        g_profiler.log_file = fopen(g_profiler.log_filename, "w");
    }
    
    if (g_profiler.log_file) {
        const char* region = (Config.PsxType == 0) ? "NTSC" : "PAL";
        int target_fps = (Config.PsxType == 0) ? 60 : 50;
        
        fprintf(g_profiler.log_file, 
                "=================================================================\n"
                "PCSXr360 Performance Profiler\n"
                "=================================================================\n"
                "Game ID:    %s\n"
                "Region:     %s (Target: %d FPS)\n"
                "Start Tick: %u\n"
                "=================================================================\n\n",
                g_profiler.game_id,
                region,
                target_fps,
                GetTickCount());
        fflush(g_profiler.log_file);
        
        g_profiler.active = 1;
        g_profiler.start_time = GetTickCount();
        g_profiler.last_calc_time = GetTickCount();
        g_profiler.last_frame_time = GetTickCount();
        g_profiler.frame_count = 0;
    }
}

void Profiler_Stop(void) {
    if (!g_profiler.active) {
        return;
    }
    
    if (g_profiler.log_file) {
        unsigned int elapsed = (GetTickCount() - g_profiler.start_time) / 1000;
        const char* region = (Config.PsxType == 0) ? "NTSC" : "PAL";
        int target_fps = (Config.PsxType == 0) ? 60 : 50;
        float performance = (g_profiler.current_fps * 100.0f) / target_fps;
        
        fprintf(g_profiler.log_file,
                "\n=================================================================\n"
                "Profiling End\n"
                "=================================================================\n"
                "Duration:   %u seconds\n"
                "Region:     %s (Target: %d FPS)\n"
                "Last FPS:   %u (%.1f%% of target)\n"
                "Status:     %s\n"
                "=================================================================\n",
                elapsed,
                region,
                target_fps,
                g_profiler.current_fps,
                performance,
                (performance >= 95.0f) ? "GOOD" : (performance >= 80.0f) ? "BELOW TARGET" : "POOR");
        fflush(g_profiler.log_file);
        
        fclose(g_profiler.log_file);
        g_profiler.log_file = NULL;
    }
    
    g_profiler.active = 0;
}

void Profiler_Toggle(void) {
    if (g_profiler.active) {
        Profiler_Stop();
    } else {
        Profiler_Start();
    }
}

unsigned int Profiler_IsActive(void) {
    return g_profiler.active;
}

// ============================================================================
// FPS e Latencia - Chamadas a cada frame
// ============================================================================

void Profiler_FrameBegin(void) {
    if (!g_profiler.active) {
        return;
    }
    
    g_profiler.frame_count++;
}

void Profiler_FrameEnd(void) {
    unsigned int now;
    unsigned int elapsed;
    
    if (!g_profiler.active) {
        return;
    }
    
    now = GetTickCount();
    
    // Calcula latencia do frame
    g_profiler.frame_time_ms = now - g_profiler.last_frame_time;
    g_profiler.last_frame_time = now;
    
    // Calcula e loga FPS a cada 1 segundo
    elapsed = now - g_profiler.last_calc_time;
    if (elapsed >= FPS_SAMPLE_INTERVAL) {
        g_profiler.current_fps = (g_profiler.frame_count * 1000) / elapsed;
        
        // Escreve no log
        fprintf(g_profiler.log_file, "[%5u] FPS: %3u | Latency: %3u ms\n",
                (now - g_profiler.start_time) / 1000,
                g_profiler.current_fps,
                g_profiler.frame_time_ms);
        fflush(g_profiler.log_file);
        
        // Reseta contador
        g_profiler.frame_count = 0;
        g_profiler.last_calc_time = now;
    }
}

// ============================================================================
// Getters para display (se necessario no futuro)
// ============================================================================

unsigned int Profiler_GetFPS(void) {
    return g_profiler.current_fps;
}

unsigned int Profiler_GetLatency(void) {
    return g_profiler.frame_time_ms;
}

// ============================================================================
// Cleanup
// ============================================================================

void Profiler_Shutdown(void) {
    Profiler_Stop();
    g_profiler.initialized = 0;
}
