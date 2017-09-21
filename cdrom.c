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

#include <exec/errors.h>
#include <devices/trackdisk.h>
#include <devices/scsidisk.h>
#include <devices/newstyle.h>
#ifndef __amigaos4__
#include <clib/alib_protos.h>
#endif

#include <stdio.h>

#if defined(__AROS__) && defined(AROS_FAST_BPTR)
#define IS_VALID_BPTR(bptr) (TRUE)
#else
#define IS_VALID_BPTR(bptr) ((bptr & 0xC0000000) == 0)
#endif

#ifndef __amigaos4__
#define RELAXED_NSD_CHECK
#endif

static APTR convert_bptr(BPTR bptr) {
	APTR cptr;

	if (!IS_VALID_BPTR(bptr))
		return NULL;

	cptr = BADDR(bptr);
	if (cptr == NULL)
		return NULL;

	if (TypeOfMem(cptr) == 0)
		return NULL;

	return cptr;
}

static int copy_bstr_to_c(BSTR bstr, char *dst, int dst_len) {
#ifdef __AROS__
	STRPTR src     = AROS_BSTR_ADDR(bstr);
	int    src_len = AROS_BSTR_strlen(bstr);
#else
	UBYTE *src     = BADDR(bstr);
	int    src_len = *src++;
#endif
	int    copy_len;

	dst_len--; /* Reserve space for nul terminator */

	copy_len = src_len;
	if (copy_len > dst_len)
		copy_len = dst_len;

	memcpy(dst, src, copy_len);

	dst[copy_len] = '\0';

	return src_len;
}

static BOOL has_command(UWORD *cmd_list, UWORD cmd) {
	int i;

	for (i = 0; cmd_list[i]; i++) {
		if (cmd_list[i] == cmd)
			return TRUE;
	}

	return FALSE;
}

static BOOL is_cdrom_drive(struct PlayCDDAData *pcd, const char *device, ULONG unit, ULONG flags) {
	struct MsgPort             *port;
	struct IOStdReq            *ioreq = NULL;
	struct NSDeviceQueryResult  nsdqr;
	struct DriveGeometry        dg;
	BOOL                        result = FALSE;

#ifdef __amigaos4__
	port = AllocSysObject(ASOT_PORT, NULL);
	if (port == NULL)
		goto cleanup;

	ioreq = AllocSysObjectTags(ASOT_IOREQUEST,
		ASOIOR_ReplyPort, port,
		ASOIOR_Size,      sizeof(struct IOStdReq),
		TAG_END);
	if (ioreq == NULL)
		goto cleanup;
#else
	port = CreateMsgPort();
	if (port == NULL)
		goto cleanup;

	ioreq = (struct IOStdReq *)CreateIORequest(port, sizeof(struct IOStdReq));
	if (ioreq == NULL)
		goto cleanup;
#endif

	if (OpenDevice((CONST_STRPTR)device, unit, (struct IORequest *)ioreq, flags) != 0) {
		ioreq->io_Device = NULL;
		goto cleanup;
	}

	nsdqr.DevQueryFormat = 0;
	nsdqr.SizeAvailable  = 0;

	ioreq->io_Command = NSCMD_DEVICEQUERY;
	ioreq->io_Data    = &nsdqr;
	ioreq->io_Length  = sizeof(nsdqr);

	if (DoIO((struct IORequest *)ioreq) == 0) {
		if (nsdqr.DeviceType != NSDEVTYPE_TRACKDISK)
			goto cleanup;

		if (!has_command(nsdqr.SupportedCommands, HD_SCSICMD))
			goto cleanup;
	} else {
#ifdef RELAXED_NSD_CHECK
		if (ioreq->io_Error != IOERR_NOCMD)
			goto cleanup;
#else
		goto cleanup;
#endif
	}

	ioreq->io_Command = TD_GETGEOMETRY;
	ioreq->io_Data    = &dg;
	ioreq->io_Length  = sizeof(dg);

	if (DoIO((struct IORequest *)ioreq) != 0)
		goto cleanup;

	if (dg.dg_DeviceType != DG_CDROM)
		goto cleanup;

	result = TRUE;

cleanup:
#ifdef __amigaos4__
	if (ioreq != NULL) {
		if (ioreq->io_Device != NULL)
			CloseDevice((struct IORequest *)ioreq);

		FreeSysObject(ASOT_IOREQUEST, ioreq);
	}

	if (port != NULL)
		FreeSysObject(ASOT_PORT, port);
#else
	if (ioreq != NULL) {
		if (ioreq->io_Device != NULL)
			CloseDevice((struct IORequest *)ioreq);

		DeleteIORequest((struct IORequest *)ioreq);
	}

	if (port != NULL)
		DeleteMsgPort(port);
#endif

	return result;
}

static void add_sorted(struct PlayCDDAData *pcd, struct List *list, struct Node *node) {
	struct Node *listnode;

	for (listnode = list->lh_TailPred; listnode->ln_Pred; listnode = listnode->ln_Pred) {
		if (strcasecmp(node->ln_Name, listnode->ln_Name) >= 0)
			break;
	}

	if (listnode->ln_Pred == NULL)
		listnode = NULL;

	Insert(list, node, listnode);
}

static BOOL add_cdrom_drive(struct PlayCDDAData *pcd, struct List *list, const char *drive,
	const char *device, ULONG unit, ULONG flags)
{
	struct CDROMDrive *cdd;
	int drive_len, device_len;

	drive_len = strlen(drive) + 1;
	device_len = strlen(device) + 1;

	cdd = malloc(sizeof(*cdd) + drive_len + device_len);
	if (cdd == NULL)
		return FALSE;

	cdd->cdd_Node.ln_Name = (char *)(cdd + 1);
	cdd->cdd_Device       = (CONST_STRPTR)(cdd->cdd_Node.ln_Name + drive_len);

	memcpy((APTR)cdd->cdd_Node.ln_Name, drive, drive_len);
	memcpy((APTR)cdd->cdd_Device, device, device_len);

	cdd->cdd_Unit  = unit;
	cdd->cdd_Flags = flags;

	add_sorted(pcd, list, &cdd->cdd_Node);

	return TRUE;
}

BOOL get_cdrom_drives(struct PlayCDDAData *pcd, struct List *list) {
	struct DosList           *dl;
	struct DeviceNode        *dn;
	struct FileSysStartupMsg *fssm;
	char                      drive[256];
	char                      device[256];
	ULONG                     unit;
	ULONG                     flags;
	int                       errors = 0;

	NewList(list);

	dl = LockDosList(LDF_DEVICES | LDF_READ);

	while ((dl = NextDosEntry(dl, LDF_DEVICES)) != NULL) {
		dn = (struct DeviceNode *)dl;

		/* Make sure that it's a valid FSSM */
		fssm = convert_bptr(dn->dn_Startup);
		if (fssm && convert_bptr(fssm->fssm_Device) && convert_bptr(fssm->fssm_Environ)) {
			copy_bstr_to_c(dn->dn_Name, drive, sizeof(drive));
			copy_bstr_to_c(fssm->fssm_Device, device, sizeof(device));

			unit  = fssm->fssm_Unit;
			flags = fssm->fssm_Flags;

			if (is_cdrom_drive(pcd, device, unit, flags)) {
				/* Add it to the list */
				if (!add_cdrom_drive(pcd, list, drive, device, unit, flags))
					errors++;
			}
		}
	}

	UnLockDosList(LDF_DEVICES | LDF_READ);

	return (errors == 0);
}

void free_cdrom_drives(struct PlayCDDAData *pcd, struct List *list) {
	if (list->lh_Head != NULL) {
		struct Node *node;

		while ((node = RemHead(list)) != NULL)
			free(node);
	}
}

BOOL open_cdrom_drive(struct PlayCDDAData *pcd, const struct CDROMDrive *cdd) {
	struct IOStdReq            *ioreq;
	struct NSDeviceQueryResult  nsdqr;
	struct DriveGeometry        dg;

	close_cdrom_drive(pcd);

#ifdef __amigaos4__
	pcd->pcd_CDPort = AllocSysObject(ASOT_PORT, NULL);
	if (pcd->pcd_CDPort == NULL)
		goto cleanup;

	pcd->pcd_CDReq = AllocSysObjectTags(ASOT_IOREQUEST,
		ASOIOR_ReplyPort, pcd->pcd_CDPort,
		ASOIOR_Size,      sizeof(struct IOStdReq),
		TAG_END);
	if (pcd->pcd_CDReq == NULL)
		goto cleanup;
#else
	pcd->pcd_CDPort = CreateMsgPort();
	if (pcd->pcd_CDPort == NULL)
		goto cleanup;

	pcd->pcd_CDReq = (struct IOStdReq *)CreateIORequest(pcd->pcd_CDPort, sizeof(struct IOStdReq));
	if (pcd->pcd_CDReq == NULL)
		goto cleanup;
#endif

	ioreq = pcd->pcd_CDReq;

	if (OpenDevice((CONST_STRPTR)cdd->cdd_Device, cdd->cdd_Unit, (struct IORequest *)ioreq, cdd->cdd_Flags) != 0) {
		ioreq->io_Device = NULL;
		goto cleanup;
	}

	nsdqr.DevQueryFormat = 0;
	nsdqr.SizeAvailable  = 0;

	ioreq->io_Command = NSCMD_DEVICEQUERY;
	ioreq->io_Data    = &nsdqr;
	ioreq->io_Length  = sizeof(nsdqr);

	if (DoIO((struct IORequest *)ioreq) == 0) {
		if (nsdqr.DeviceType != NSDEVTYPE_TRACKDISK)
			goto cleanup;

		if (!has_command(nsdqr.SupportedCommands, HD_SCSICMD))
			goto cleanup;
	} else {
#ifdef RELAXED_NSD_CHECK
		if (ioreq->io_Error != IOERR_NOCMD)
			goto cleanup;
#else
		goto cleanup;
#endif
	}

	ioreq->io_Command = TD_GETGEOMETRY;
	ioreq->io_Data    = &dg;
	ioreq->io_Length  = sizeof(dg);

	if (DoIO((struct IORequest *)ioreq) != 0)
		goto cleanup;

	if (dg.dg_DeviceType != DG_CDROM)
		goto cleanup;

	return TRUE;

cleanup:
	close_cdrom_drive(pcd);
	return FALSE;
}

void close_cdrom_drive(struct PlayCDDAData *pcd) {
#ifdef __amigaos4__
	if (pcd->pcd_CDReq != NULL) {
		if (pcd->pcd_CDReq->io_Device != NULL)
			CloseDevice((struct IORequest *)pcd->pcd_CDReq);

		FreeSysObject(ASOT_IOREQUEST, pcd->pcd_CDReq);
		pcd->pcd_CDReq = NULL;
	}

	if (pcd->pcd_CDPort != NULL) {
		FreeSysObject(ASOT_PORT, pcd->pcd_CDPort);
		pcd->pcd_CDPort = NULL;
	}
#else
	if (pcd->pcd_CDReq != NULL) {
		if (pcd->pcd_CDReq->io_Device != NULL)
			CloseDevice((struct IORequest *)pcd->pcd_CDReq);

		DeleteIORequest((struct IORequest *)pcd->pcd_CDReq);
		pcd->pcd_CDReq = NULL;
	}

	if (pcd->pcd_CDPort != NULL) {
		DeleteMsgPort(pcd->pcd_CDPort);
		pcd->pcd_CDPort = NULL;
	}
#endif
}

