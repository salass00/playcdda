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

#ifndef PLAYCDDA_H
#define PLAYCDDA_H 1

#ifdef __amigaos4__
#define __USE_INLINE__
#define DEFAULT_CODESET 4
#endif

#if !defined(__AROS__) || AROS_BIG_ENDIAN
#define WORDS_BIGENDIAN 1
#endif

#include <devices/ahi.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>
#include <proto/icon.h>

#include <stdlib.h>
#include <string.h>

#ifdef __AROS__
#include "gui_mui.h"
#else
#include "gui_reaction.h"
#endif

#define CDDA_FRAME_SIZE 2352

#define CDDA_BUF_FRAMES 150
#define CDDA_BUF_SIZE   (CDDA_BUF_FRAMES*CDDA_FRAME_SIZE)

#define PCM_BUF_FRAMES  5
#define PCM_BUF_SIZE    (PCM_BUF_FRAMES*CDDA_FRAME_SIZE)

#define PLAYER_TASK_PRI 5

#ifdef __amigaos4__
#define CurrentDir(dir) SetCurrentDir(dir)
#endif

struct PlayCDDAData {
	struct Library     *pcd_LocaleBase;
	struct Library     *pcd_IconBase;

#ifdef __amigaos4__
	struct LocaleIFace *pcd_ILocale;
	struct IconIFace   *pcd_IIcon;
#endif

	struct Catalog     *pcd_Catalog;

	struct DiskObject  *pcd_Icon;

	struct MsgPort     *pcd_AHIPort;
	struct AHIRequest  *pcd_AHIReq[2];
	struct AHIRequest  *pcd_LinkReq;
	int                 pcd_CurrentReq;
	Fixed               pcd_Volume;

	struct List         pcd_CDDrives;

	struct MsgPort     *pcd_CDPort;
	struct IOStdReq    *pcd_CDReq;

	WORD               *pcd_PCMBuf[2];
	UWORD              *pcd_CDDABuf[2];

	struct PlayCDDAGUI  pcd_GUIData;
};

#define LocaleBase (pcd->pcd_LocaleBase)
#define IconBase   (pcd->pcd_IconBase)

#define ILocale    (pcd->pcd_ILocale)
#define IIcon      (pcd->pcd_IIcon)

struct CDROMDrive {
	struct Node  cdd_Node;
	CONST_STRPTR cdd_Device;
	ULONG        cdd_Unit;
	ULONG        cdd_Flags;
};

extern const char verstag[];

BOOL open_catalog(struct PlayCDDAData *pcd, const char *catalog_name);
void close_catalog(struct PlayCDDAData *pcd);
const char *get_catalog_string(struct PlayCDDAData *pcd, int id, const char *builtin);

BOOL open_ahi(struct PlayCDDAData *pcd);
void close_ahi(struct PlayCDDAData *pcd);
void play_pcm_data(struct PlayCDDAData *pcd, const WORD *pcm_data, ULONG pcm_size);
void stop_playback(struct PlayCDDAData *pcd, BOOL force);

BOOL get_cdrom_drives(struct PlayCDDAData *pcd, struct List *list);
void free_cdrom_drives(struct PlayCDDAData *pcd, struct List *list);
BOOL open_cdrom_drive(struct PlayCDDAData *pcd, const struct CDROMDrive *cdd);
void close_cdrom_drive(struct PlayCDDAData *pcd);

BOOL create_gui(struct PlayCDDAData *pcd);
void destroy_gui(struct PlayCDDAData *pcd);
int main_loop(struct PlayCDDAData *pcd);

#endif /* PLAYCDDA_H */

