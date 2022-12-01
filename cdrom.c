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
#include <dos/filehandler.h>
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

	port = create_msgport();
	if (port == NULL)
		goto cleanup;

	ioreq = (struct IOStdReq *)create_iorequest(port, sizeof(struct IOStdReq));
	if (ioreq == NULL)
		goto cleanup;

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
	if (ioreq != NULL) {
		if (ioreq->io_Device != NULL)
			CloseDevice((struct IORequest *)ioreq);

		delete_iorequest((struct IORequest *)ioreq);
	}

	if (port != NULL)
		delete_msgport(port);

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

	pcd->pcd_CDPort = create_msgport();
	if (pcd->pcd_CDPort == NULL)
		goto cleanup;

	pcd->pcd_CDReq = (struct IOStdReq *)create_iorequest(pcd->pcd_CDPort, sizeof(struct IOStdReq));
	if (pcd->pcd_CDReq == NULL)
		goto cleanup;

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
	if (pcd->pcd_CDReq != NULL) {
		if (pcd->pcd_CDReq->io_Device != NULL)
			CloseDevice((struct IORequest *)pcd->pcd_CDReq);

		delete_iorequest((struct IORequest *)pcd->pcd_CDReq);
		pcd->pcd_CDReq = NULL;
	}

	if (pcd->pcd_CDPort != NULL) {
		delete_msgport(pcd->pcd_CDPort);
		pcd->pcd_CDPort = NULL;
	}
}

BOOL read_toc(struct PlayCDDAData *pcd, struct PlayCDDATOC *toc) {
	struct IOStdReq   *ioreq;
	struct SCSICmd     scsicmd;
	UBYTE              buffer[4 + (MAX_TRACKS + 1) * 8];
	UBYTE              sensebuffer[128];
	static const UBYTE cmd[10] = { 0x43, 0, 0, 0, 0, 0, 0, 0x03, 0x24, 0 };
	int                tocsize, tracks, i;
	int                bp;

	memset(toc, 0, sizeof(*toc));

	ioreq = pcd->pcd_CDReq;
	if (ioreq == NULL)
		return FALSE;

	memset(&scsicmd, 0, sizeof(scsicmd));

	scsicmd.scsi_Data        = (UWORD *)buffer;
	scsicmd.scsi_Length      = sizeof(buffer);
	scsicmd.scsi_SenseData   = sensebuffer;
	scsicmd.scsi_SenseLength = sizeof(sensebuffer);
	scsicmd.scsi_Command     = (UBYTE *)cmd;
	scsicmd.scsi_CmdLength   = sizeof(cmd);
	scsicmd.scsi_Flags       = SCSIF_READ | SCSIF_AUTOSENSE;

	ioreq->io_Command = HD_SCSICMD;
	ioreq->io_Data    = &scsicmd;
	ioreq->io_Length  = sizeof(scsicmd);

	if (DoIO((struct IORequest *)ioreq) != 0)
		return FALSE;

	tocsize = ((unsigned)buffer[0] << 8)
	        |  (unsigned)buffer[1];
	if (tocsize < 10)
		return FALSE;

	tracks = (unsigned)(tocsize - 10) >> 3;
	toc->toc_NumTracks = tracks;

	bp = 4;
	for (i = 0; i < tracks; i++, bp += 8) {
		toc->toc_Type[i] = (buffer[bp + 1] & 4) ? TRACK_DATA : TRACK_CDDA;

		toc->toc_Addr[i] = ((ULONG)buffer[bp + 4] << 24)
		                 | ((ULONG)buffer[bp + 5] << 16)
		                 | ((ULONG)buffer[bp + 6] << 8)
		                 |  (ULONG)buffer[bp + 7];
	}

	toc->toc_Addr[i] = ((ULONG)buffer[bp + 4] << 24)
	                 | ((ULONG)buffer[bp + 5] << 16)
	                 | ((ULONG)buffer[bp + 6] << 8)
	                 |  (ULONG)buffer[bp + 7];

	return TRUE;
}

