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

#include <workbench/startup.h>

#include "PlayCDDA_rev.h"

const char verstag[] = VERSTAG;

#ifdef __amigaos4__

APTR alloc_shared_mem(ULONG size) {
	return AllocVecTags(size, AVT_Type, MEMF_SHARED, TAG_END);
}

void free_shared_mem(APTR memory, ULONG size) {
	FreeVec(memory);
}

#else

APTR alloc_shared_mem(ULONG size) {
	return AllocMem(size, MEMF_PUBLIC);
}

void free_shared_mem(APTR memory, ULONG size) {
	FreeMem(memory, size);
}

#endif

#if !defined(__amigaos4__) && !defined(__AROS__)

static struct Node *GetHead(const struct List *list) {
	struct Node *node = list->lh_Head;

	if (node->ln_Succ != NULL)
		return node;

	return NULL;
}

#endif

static BOOL get_icon(struct PlayCDDAData *pcd, int argc, char **argv) {
	IconBase = OpenLibrary((CONST_STRPTR)"icon.library", 0);
	if (IconBase == NULL)
		return FALSE;

#ifdef __amigaos4__
	IIcon = (struct IconIFace *)GetInterface(IconBase, "main", 1, NULL);
	if (IIcon == NULL)
		return FALSE;
#endif

	if (argc == 0) {
		/* WB startup */
		struct WBStartup *wbsm = (struct WBStartup *)argv;
		BPTR cd;

		cd = CurrentDir(wbsm->sm_ArgList[0].wa_Lock);
		pcd->pcd_Icon = GetDiskObject((STRPTR)wbsm->sm_ArgList[0].wa_Name);
		CurrentDir(cd);
	} else {
		/* CLI startup */
		BPTR cd;

		cd = CurrentDir(GetProgramDir());
		pcd->pcd_Icon = GetDiskObject(FilePart((STRPTR)argv[0]));
		CurrentDir(cd);
	}

	if (pcd->pcd_Icon == NULL)
		pcd->pcd_Icon = GetDefDiskObject(WBTOOL);

	return (pcd->pcd_Icon != NULL);
}

static void free_icon(struct PlayCDDAData *pcd) {
	if (pcd->pcd_Icon != NULL)
		FreeDiskObject(pcd->pcd_Icon);

#ifdef __amigaos4__
	DropInterface((struct Interface *)IIcon);
#endif

	CloseLibrary(IconBase);
}

int main(int argc, char **argv) {
	struct PlayCDDAData *pcd;
	int rc = RETURN_ERROR;

	pcd = alloc_shared_mem(sizeof(*pcd));
	if (pcd == NULL)
		goto cleanup;

	memset(pcd, 0, sizeof(*pcd));

	pcd->pcd_MainProc = (struct Process *)FindTask(NULL);

	open_catalog(pcd, "PlayCDDA.catalog");

	if (!get_icon(pcd, argc, argv))
		goto cleanup;

	if (!open_ahi(pcd))
		goto cleanup;

	if (!get_cdrom_drives(pcd, &pcd->pcd_CDDrives))
		goto cleanup;

	if (IsListEmpty(&pcd->pcd_CDDrives))
		goto cleanup;

	/* Default to drive CD0: */
	pcd->pcd_CurrentDrive = (struct CDROMDrive *)FindName(&pcd->pcd_CDDrives, (CONST_STRPTR)"CD0");

	/* If not available use the first CD drive in the list */
	if (pcd->pcd_CurrentDrive == NULL)
		pcd->pcd_CurrentDrive = (struct CDROMDrive *)GetHead(&pcd->pcd_CDDrives);

	if (!open_cdrom_drive(pcd, pcd->pcd_CurrentDrive))
		goto cleanup;

	set_volume(pcd, 64); /* Full volume */

	if (!create_gui(pcd))
		goto cleanup;

	rc = main_loop(pcd);

cleanup:
	if (pcd != NULL) {
		destroy_gui(pcd);

		close_cdrom_drive(pcd);

		free_cdrom_drives(pcd, &pcd->pcd_CDDrives);

		close_ahi(pcd);

		free_icon(pcd);

		close_catalog(pcd);

		free_shared_mem(pcd, sizeof(*pcd));
	}

	return rc;
}

