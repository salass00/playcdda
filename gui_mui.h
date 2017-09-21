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

#ifndef GUI_MUI_H
#define GUI_MUI_H 1

#define ENABLE_MUI_GUI

#include <intuition/classes.h>

enum {
	OID_APPLICATION,
	OID_WINDOW,
	OID_ROOT_LAYOUT,
	OID_STATUS_DISPLAY,
	OID_SEEK_BAR,
	OID_BUTTON_BAR,
	OID_EJECT,
	OID_STOP,
	OID_PAUSE,
	OID_PREV,
	OID_PLAY,
	OID_NEXT,
	OID_TRACK01,
	OID_TRACK32 = OID_TRACK01 + 31,
	OID_VOLUME_SLIDER,
	OID_MAX
};

struct PlayCDDAGUI {
	Object *pcg_Obj[OID_MAX];
};

#endif /* GUI_MUI_H */

