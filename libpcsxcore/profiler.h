// ============================================================================
// PCSXR Profiler Header - Simplificado
// Arquivo: libpcsxcore/profiler.h
// ============================================================================

#ifndef PROFILER_H
#define PROFILER_H

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Controle
// ============================================================================

void Profiler_Init(const char* game_id, const char* game_name);
void Profiler_Start(void);
void Profiler_Stop(void);
void Profiler_Toggle(void);
unsigned int Profiler_IsActive(void);
void Profiler_Shutdown(void);

// ============================================================================
// Frame Tracking
// ============================================================================

void Profiler_FrameBegin(void);
void Profiler_FrameEnd(void);

// ============================================================================
// Getters (para display OSD)
// ============================================================================

unsigned int Profiler_GetFPS(void);
unsigned int Profiler_GetLatency(void);

// ============================================================================
// Macros
// ============================================================================

#define PROFILER_FRAME_START()  Profiler_FrameBegin()
#define PROFILER_FRAME_END()    Profiler_FrameEnd()
#define PROFILER_TOGGLE()       Profiler_Toggle()

#ifdef __cplusplus
}
#endif

#endif // PROFILER_H
