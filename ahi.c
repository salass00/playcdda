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

#include "playcdda.h"

BOOL open_ahi(struct PlayCDDAData *pcd) {
	pcd->pcd_AHIPort = create_msgport();
	if (pcd->pcd_AHIPort == NULL)
		return FALSE;

	pcd->pcd_AHIReq[0] = (struct AHIRequest *)create_iorequest(pcd->pcd_AHIPort, sizeof(struct AHIRequest));
	if (pcd->pcd_AHIReq[0] == NULL)
		return FALSE;

	pcd->pcd_AHIReq[0]->ahir_Version = 4; /* Must be version 4 or newer */

	if (OpenDevice((CONST_STRPTR)AHINAME, AHI_DEFAULT_UNIT, (struct IORequest *)pcd->pcd_AHIReq[0], 0) != 0) {
		pcd->pcd_AHIReq[0]->ahir_Std.io_Device = NULL;
		return FALSE;
	}

	pcd->pcd_AHIReq[1] = (struct AHIRequest *)copy_iorequest((struct IORequest *)pcd->pcd_AHIReq[0]);
	if (pcd->pcd_AHIReq[1] == NULL)
		return FALSE;

	pcd->pcd_Volume = 0x10000; /* Full volume */

	return TRUE;
}

void close_ahi(struct PlayCDDAData *pcd) {
	stop_playback(pcd, TRUE);

	if (pcd->pcd_AHIReq[1] != NULL)
		delete_iorequest_copy((struct IORequest *)pcd->pcd_AHIReq[1]);

	if (pcd->pcd_AHIReq[0] != NULL) {
		if (pcd->pcd_AHIReq[0]->ahir_Std.io_Device != NULL)
			CloseDevice((struct IORequest *)pcd->pcd_AHIReq[0]);

		delete_iorequest((struct IORequest *)pcd->pcd_AHIReq[0]);
	}

	delete_msgport(pcd->pcd_AHIPort);
}

void play_pcm_data(struct PlayCDDAData *pcd, const WORD *pcm_data, ULONG pcm_size) {
	struct AHIRequest *ahireq, *linkreq;
	int current_req;

	current_req = pcd->pcd_CurrentReq;

	ahireq  = pcd->pcd_AHIReq[current_req];
	linkreq = pcd->pcd_LinkReq;

	ahireq->ahir_Std.io_Command = CMD_WRITE;
	ahireq->ahir_Std.io_Data    = (APTR)pcm_data;
	ahireq->ahir_Std.io_Length  = pcm_size;
	ahireq->ahir_Std.io_Offset  = 0;
	ahireq->ahir_Type           = AHIST_S16S;
	ahireq->ahir_Frequency      = 44100;
	ahireq->ahir_Volume         = pcd->pcd_Volume;
	ahireq->ahir_Position       = 0x08000; /* Center */
	ahireq->ahir_Link           = linkreq;

	SendIO((struct IORequest *)ahireq);

	/* Make sure previous request has finished */
	if (linkreq != NULL)
		WaitIO((struct IORequest *)linkreq);

	pcd->pcd_LinkReq = ahireq;

	pcd->pcd_CurrentReq = current_req ^ 1;
}

void stop_playback(struct PlayCDDAData *pcd, BOOL force) {
	struct AHIRequest *linkreq = pcd->pcd_LinkReq;

	if (linkreq != NULL) {
		if (force && CheckIO((struct IORequest *)linkreq) == NULL)
			AbortIO((struct IORequest *)linkreq);

		WaitIO((struct IORequest *)linkreq);

		pcd->pcd_LinkReq = NULL;
	}
}

