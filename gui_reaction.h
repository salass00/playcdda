/*
 * PlayCDDA - AmigaOS/AROS native CD audio player
 * Copyright (C) 2017 Fredrik Wikstrom <fredrik@a500.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GUI_REACTION_H
#define GUI_REACTION_H 1

#define ENABLE_REACTION_GUI

#include <proto/intuition.h>
#include <proto/speedbar.h>

enum {
	OID_MENUSTRIP,
	OID_WINDOW,
	OID_ROOT_LAYOUT,
	OID_STATUS_DISPLAY,
	OID_SEEK_BAR,
	OID_BUTTON_BAR,
	OID_TRACK01,
	OID_TRACK32 = OID_TRACK01 + 31,
	OID_VOLUME_SLIDER,
	OID_MAX
};

struct PlayCDDAGUI {
	struct Library        *pcg_IntuitionBase;
	struct IntuitionIFace *pcg_IIntuition;
	struct SpeedBarIFace  *pcg_ISpeedBar;

	struct ClassLibrary   *pcg_WindowBase;
	struct ClassLibrary   *pcg_LayoutBase;
	struct ClassLibrary   *pcg_ButtonBase;
	struct ClassLibrary   *pcg_SliderBase;
	struct ClassLibrary   *pcg_SpeedBarBase;
	struct ClassLibrary   *pcg_LabelBase;
	struct ClassLibrary   *pcg_BitMapBase;

	Class                 *pcg_WindowClass;
	Class                 *pcg_LayoutClass;
	Class                 *pcg_ButtonClass;
	Class                 *pcg_SliderClass;
	Class                 *pcg_SpeedBarClass;
	Class                 *pcg_LabelClass;
	Class                 *pcg_BitMapClass;

	struct MsgPort        *pcg_AppPort;
	struct Screen         *pcg_Screen;
	struct List            pcg_ButtonList;

	Object                *pcg_Obj[OID_MAX];
};

#define IntuitionBase (pcg->pcg_IntuitionBase)
#define IIntuition    (pcg->pcg_IIntuition)
#define ISpeedBar     (pcg->pcg_ISpeedBar)

#define WindowBase    (pcg->pcg_WindowBase)
#define LayoutBase    (pcg->pcg_LayoutBase)
#define ButtonBase    (pcg->pcg_ButtonBase)
#define SliderBase    (pcg->pcg_SliderBase)
#define SpeedBarBase  (pcg->pcg_SpeedBarBase)
#define LabelBase     (pcg->pcg_LabelBase)
#define BitMapBase    (pcg->pcg_BitMapBase)

#define WindowClass   (pcg->pcg_WindowClass)
#define LayoutClass   (pcg->pcg_LayoutClass)
#define ButtonClass   (pcg->pcg_ButtonClass)
#define SliderClass   (pcg->pcg_SliderClass)
#define SpeedBarClass (pcg->pcg_SpeedBarClass)
#define LabelClass    (pcg->pcg_LabelClass)
#define BitMapClass   (pcg->pcg_BitMapClass)

#endif /* GUI_REACTION_H */

