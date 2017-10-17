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

#ifdef ENABLE_MUI_GUI

#define MUI_OBSOLETE

#include <libraries/mui.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#ifndef __amigaos4__
#include <clib/alib_protos.h>
#endif

#include <stdio.h>

#include "PlayCDDA_rev.h"

#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS

#include "locale.h"

#define OBJ(id) pcg->pcg_Obj[OID_ ## id]

#define STR(id) get_catalog_string(pcd, MSG_ ## id, MSG_ ## id ## _STR)

#ifndef __amigaos4__
static APTR SetProcWindow(APTR new_win) {
	struct Process *me = (struct Process *)FindTask(NULL);
	APTR old_win;

	old_win = me->pr_WindowPtr;
	me->pr_WindowPtr = new_win;

	return old_win;
}
#endif

static Object *create_menu(struct PlayCDDAData *pcd) {
	struct List        *list;
	struct Node        *node;
	int                 index;
	struct CDROMDrive  *cdd;
	char                label[256];
	Object             *about_item;
	Object             *separator_1;
	Object             *iconify_item;
	Object             *separator_2;
	Object             *cdrom_menu;
	Object             *menu_item;
	Object             *separator_3;
	Object             *quit_item;
	Object             *project_menu;
	Object             *menustrip;

	about_item = MUI_NewObject(MUIC_Menuitem,
		MUIA_Menuitem_Title,    STR(PROJECT_ABOUT),
		MUIA_Menuitem_Shortcut, "?",
		TAG_END);

	separator_1 = MUI_NewObject(MUIC_Menuitem,
		MUIA_Menuitem_Title,    NM_BARLABEL,
		TAG_END);

	iconify_item = MUI_NewObject(MUIC_Menuitem,
		MUIA_Menuitem_Title,    STR(PROJECT_ICONIFY),
		MUIA_Menuitem_Shortcut, "I",
		TAG_END);

	separator_2 = MUI_NewObject(MUIC_Menuitem,
		MUIA_Menuitem_Title,    NM_BARLABEL,
		TAG_END);

	cdrom_menu = MUI_NewObject(MUIC_Menuitem,
		MUIA_Menuitem_Title,    STR(PROJECT_CDROMDRIVE),
		TAG_END);

	separator_3 = MUI_NewObject(MUIC_Menuitem,
		MUIA_Menuitem_Title,    NM_BARLABEL,
		TAG_END);

	quit_item = MUI_NewObject(MUIC_Menuitem,
		MUIA_Menuitem_Title,    STR(PROJECT_QUIT),
		MUIA_Menuitem_Shortcut, "Q",
		TAG_END);

	project_menu = MUI_NewObject(MUIC_Menu,
		MUIA_Menu_Title,   "PlayCDDA",
		MUIA_Family_Child, about_item,
		MUIA_Family_Child, separator_1,
		MUIA_Family_Child, iconify_item,
		MUIA_Family_Child, separator_2,
		MUIA_Family_Child, cdrom_menu,
		MUIA_Family_Child, separator_3,
		MUIA_Family_Child, quit_item,
		TAG_END);

	menustrip = MUI_NewObject(MUIC_Menustrip,
		MUIA_Family_Child, project_menu,
		TAG_END);

	list  = &pcd->pcd_CDDrives;
	index = 0;

	for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ) {
		cdd = (struct CDROMDrive *)node;

		snprintf(label, sizeof(label), "%s [%s:%lu]", node->ln_Name, cdd->cdd_Device, (unsigned long)cdd->cdd_Unit);

		menu_item = MUI_NewObject(MUIC_Menuitem,
			MUIA_UserData,         cdd,
			MUIA_Menuitem_Title,   strdup(label),
			MUIA_Menuitem_Exclude, ~(1 << index),
			MUIA_Menuitem_Checkit, TRUE,
			MUIA_Menuitem_Checked, (cdd == pcd->pcd_CurrentDrive),
			TAG_END);
		if (menu_item == NULL) {
			DisposeObject(menustrip);
			return NULL;
		}

		DoMethod(cdrom_menu, MUIM_Family_AddTail, menu_item);

		if (++index >= 32)
			break;
	}

	return menustrip;
}

static BOOL file_exists(const char *path) {
	APTR window;
	BPTR file;

	/* Disable "Please insert volume ..." requesters */
	window = SetProcWindow((APTR)~0);

	file = Open((CONST_STRPTR)path, MODE_OLDFILE);

	SetProcWindow(window);

	if (file == 0)
		return FALSE;

	Close(file);
	return TRUE;
}

static BOOL find_image(const char *name, char *path, int path_size) {
	static const char *search_path[] = {
		"PROGDIR:",
		"PROGDIR:Images",
		"SYS:Prefs/Presets/Images",
		"TBImages:"
	};
	int i;

	for (i = 0; i < (sizeof(search_path) / sizeof(search_path[0])); i++) {
		strlcpy(path, search_path[i], path_size);
		AddPart((STRPTR)path, (CONST_STRPTR)name, path_size);

		if (file_exists(path))
			return TRUE;
	}

	return FALSE;
}

static Object *create_image_button(struct PlayCDDAData *pcd, const char *help_text, BOOL enabled, const char *image_name) {
	Object *button;
	char image_path[64];

	if (!find_image(image_name, image_path, sizeof(image_path)))
		return NULL;

	button = MUI_NewObject(MUIC_Dtpic,
		MUIA_Frame,      MUIV_Frame_ImageButton,
		MUIA_InputMode,  MUIV_InputMode_RelVerify,
		MUIA_ShortHelp,  help_text,
		MUIA_Disabled,   !enabled,
		MUIA_Dtpic_Name, strdup(image_path),
		TAG_END);

	return button;
}

static Object *create_track_buttons(struct PlayCDDAData *pcd, int columns, int rows) {
	char *buffer;
	Object *table_layout;
	Object *column_layout;
	Object *button;
	int i, j, index;

	buffer = malloc(columns * rows * 3);
	if (buffer == NULL)
		return NULL;

	table_layout = MUI_NewObject(MUIC_Group, MUIA_Group_Horiz, TRUE, TAG_END);
	if (table_layout == NULL)
		goto cleanup;

	for (i = 0; i < columns; i++) {
		column_layout = MUI_NewObject(MUIC_Group, TAG_END);
		if (column_layout == NULL)
			goto cleanup;

		DoMethod(table_layout, MUIM_Group_AddTail, column_layout);

		for (j = 0; j < rows; j++) {
			index = (j * columns) + i;

			snprintf(&buffer[index * 3], 3, "%d", index + 1);

			button = MUI_NewObject(MUIC_Text,
				MUIA_Frame,         MUIV_Frame_Button,
				MUIA_InputMode,     MUIV_InputMode_RelVerify,
				MUIA_Text_PreParse, "\33c",
				MUIA_Text_Contents, &buffer[index * 3],
				TAG_END);
			if (button == NULL)
				goto cleanup;

			DoMethod(column_layout, MUIM_Group_AddTail, button);
		}
	}

	return table_layout;

cleanup:
	if (table_layout != NULL)
		MUI_DisposeObject(table_layout);

	return NULL;
}

BOOL create_gui(struct PlayCDDAData *pcd) {
	struct PlayCDDAGUI *pcg = &pcd->pcd_GUIData;
	Object             *sub_layout_1;
	Object             *sub_layout_2;
	Object             *sub_layout_3, *volume_label;

	OBJ(MENUSTRIP) = create_menu(pcd);
	if (OBJ(MENUSTRIP) == NULL)
		return FALSE;

	OBJ(STATUS_DISPLAY) = MUI_NewObject(MUIC_Text,
		MUIA_Frame,         MUIV_Frame_Text,
		MUIA_Text_SetVMax,  FALSE,
		MUIA_Text_Contents, STR(NODISC),
		TAG_END);

	OBJ(SEEK_BAR) = MUI_NewObject(MUIC_Slider,
		MUIA_InputMode,    MUIV_InputMode_RelVerify,
		MUIA_Disabled,     TRUE,
		MUIA_Slider_Quiet, TRUE,
		TAG_END);

	OBJ(EJECT) = create_image_button(pcd, STR(EJECT_GAD), FALSE, "tapeeject");
	OBJ(STOP)  = create_image_button(pcd, STR(STOP_GAD),  FALSE, "tapestop" );
	OBJ(PAUSE) = create_image_button(pcd, STR(PAUSE_GAD), FALSE, "tapepause");
	OBJ(PREV)  = create_image_button(pcd, STR(PREV_GAD),  FALSE, "tapelast" );
	OBJ(PLAY)  = create_image_button(pcd, STR(PLAY_GAD),  FALSE, "tapeplay" );
	OBJ(NEXT)  = create_image_button(pcd, STR(NEXT_GAD),  FALSE, "tapenext" );

	OBJ(BUTTON_BAR) = MUI_NewObject(MUIC_Group,
		MUIA_Group_Horiz, TRUE,
		MUIA_Group_Child, OBJ(EJECT),
		MUIA_Group_Child, OBJ(STOP),
		MUIA_Group_Child, OBJ(PAUSE),
		MUIA_Group_Child, OBJ(PREV),
		MUIA_Group_Child, OBJ(PLAY),
		MUIA_Group_Child, OBJ(NEXT),
		TAG_END);

	sub_layout_1 = MUI_NewObject(MUIC_Group,
		MUIA_Group_Child, OBJ(STATUS_DISPLAY),
		MUIA_Group_Child, OBJ(SEEK_BAR),
		MUIA_Group_Child, OBJ(BUTTON_BAR),
		TAG_END);

	sub_layout_2 = create_track_buttons(pcd, 8, 4);

	OBJ(VOLUME_SLIDER) = MUI_NewObject(MUIC_Slider,
		MUIA_Slider_Horiz,   FALSE,
		MUIA_Slider_Reverse, TRUE,
		MUIA_Slider_Min,     0,
		MUIA_Slider_Max,     64,
		MUIA_Slider_Level,   64,
		TAG_END);

	volume_label = MUI_NewObject(MUIC_Text,
		MUIA_Text_Contents, STR(VOLUME_GAD),
		TAG_END);

	sub_layout_3 = MUI_NewObject(MUIC_Group,
		MUIA_Group_Child, OBJ(VOLUME_SLIDER),
		MUIA_Group_Child, volume_label,
		TAG_END);

	OBJ(ROOT_LAYOUT) = MUI_NewObject(MUIC_Group,
		MUIA_Group_Horiz, TRUE,
		MUIA_Group_Child, sub_layout_1,
		MUIA_Group_Child, sub_layout_2,
		MUIA_Group_Child, sub_layout_3,
		TAG_END);

	OBJ(WINDOW) = MUI_NewObject(MUIC_Window,
		MUIA_Window_Title,      VERS,
		MUIA_Window_Menustrip,  OBJ(MENUSTRIP),
		MUIA_Window_RootObject, OBJ(ROOT_LAYOUT),
		TAG_END);

	OBJ(APPLICATION) = MUI_NewObject(MUIC_Application,
		MUIA_Application_Title,      "PlayCDDA",
		MUIA_Application_Version,    &verstag[1],
		MUIA_Application_DiskObject, pcd->pcd_Icon,
		MUIA_Application_Window,     OBJ(WINDOW),
		TAG_END);
	if (OBJ(APPLICATION) == NULL)
		return FALSE;

	DoMethod(OBJ(WINDOW), MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		OBJ(APPLICATION), 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

	set(OBJ(WINDOW), MUIA_Window_Open, TRUE);
	if (XGET(OBJ(WINDOW), MUIA_Window_Open) == FALSE)
		return FALSE;

	return TRUE;
}

void destroy_gui(struct PlayCDDAData *pcd) {
	struct PlayCDDAGUI *pcg = &pcd->pcd_GUIData;

	if (OBJ(APPLICATION) != NULL)
		MUI_DisposeObject(OBJ(APPLICATION));
}

int main_loop(struct PlayCDDAData *pcd) {
	struct PlayCDDAGUI *pcg = &pcd->pcd_GUIData;
	ULONG sigmask, signals;

	while (DoMethod(OBJ(APPLICATION), MUIM_Application_NewInput, &sigmask) != MUIV_Application_ReturnID_Quit) {
		signals = Wait(sigmask | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);

		if (signals & SIGBREAKF_CTRL_C)
			break;

		if (signals & SIGBREAKF_CTRL_F)
			set(OBJ(WINDOW), MUIA_Window_Open, TRUE);
	}

	return RETURN_OK;
}

#endif /* ENABLE_MUI_GUI */

