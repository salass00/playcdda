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

BOOL open_catalog(struct PlayCDDAData *pcd, const char *catalog_name) {
	LocaleBase = OpenLibrary((CONST_STRPTR)"locale.library", 0);
	if (LocaleBase == NULL)
		return FALSE;

#ifdef __amigaos4__
	ILocale = (struct LocaleIFace *)GetInterface(LocaleBase, "main", 1, NULL);
	if (ILocale == NULL)
		return FALSE;
#endif

#ifdef __amigaos4__
	pcd->pcd_Catalog = OpenCatalog(NULL, catalog_name,
		OC_BuiltInCodeSet, DEFAULT_CODESET,
		TAG_END);
#else
	pcd->pcd_Catalog = OpenCatalogA(NULL, (STRPTR)catalog_name, NULL);
#endif
	if (pcd->pcd_Catalog == NULL)
		return FALSE;

	return TRUE;
}

void close_catalog(struct PlayCDDAData *pcd) {
	if (pcd->pcd_Catalog != NULL)
		CloseCatalog(pcd->pcd_Catalog);

#ifdef __amigaos4__
	DropInterface((struct Interface *)ILocale);
#endif

	CloseLibrary(LocaleBase);
}

const char *get_catalog_string(struct PlayCDDAData *pcd, int id, const char *builtin) {
	if (pcd->pcd_Catalog == NULL)
		return builtin;

	return (const char *)GetCatalogStr(pcd->pcd_Catalog, id, (STRPTR)builtin);
}

