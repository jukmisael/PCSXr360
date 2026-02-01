/***************************************************************************
                          fps.h -  description
                             -------------------
    begin                : Sun Oct 28 2001
    copyright            : (C) 2001 by Pete Bernert
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

#ifndef _FPS_INTERNALS_H
#define _FPS_INTERNALS_H

#ifdef __cplusplus
extern "C" {
#endif

void FrameCap(void);
void FrameSkip(void); 
void calcfps(void);
void PCFrameCap (void);
void PCcalcfps(void);
void SetAutoFrameCap(void);
void SetFPSHandler(void);
void InitFPS(void);
void CheckFrameRate(void);

// Variáveis para acesso externo (Xbox 360)
extern int iFrameLimit;
extern int UseFrameLimit;
extern int UseFrameSkip;
extern int s_adaptive_skip_enabled;

#ifdef __cplusplus
}
#endif

#endif // _FPS_INTERNALS_H
