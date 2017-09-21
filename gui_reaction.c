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

#ifdef ENABLE_REACTION_GUI

#include <intuition/menuclass.h>
#include <classes/window.h>
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <gadgets/slider.h>
#include <gadgets/speedbar.h>
#include <images/label.h>
#include <images/bitmap.h>

#include <stdarg.h>
#include <stdio.h>

#include "PlayCDDA_rev.h"

#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS

#include "locale.h"

#define OBJ(id) pcg->pcg_Obj[OID_ ## id]

#define STR(id) get_catalog_string(pcd, MSG_ ## id, MSG_ ## id ## _STR)

enum {
	MID_PROJECT_MENU,
	MID_PROJECT_ABOUT,
	MID_PROJECT_ICONIFY,
	MID_PROJECT_CDROMDRIVE,
	MID_PROJECT_CDROMDRIVE_01,
	MID_PROJECT_CDROMDRIVE_32 = MID_PROJECT_CDROMDRIVE_01 + 31,
	MID_PROJECT_QUIT
};

enum {
	SBID_DUMMY,
	SBID_EJECT,
	SBID_STOP,
	SBID_PAUSE,
	SBID_PREV,
	SBID_PLAY,
	SBID_NEXT
};

static BOOL VARARGS68K create_menu(struct PlayCDDAData *pcd, ...) {
	struct PlayCDDAGUI *pcg = &pcd->pcd_GUIData;
	va_list tags;
	BOOL    result;

	OBJ(MENUSTRIP) = NewObject(NULL, "menuclass", MA_Type, T_ROOT, TAG_END);
	if (OBJ(MENUSTRIP) == NULL)
		return FALSE;

	va_startlinear(tags, pcd);
	result = DoMethod(OBJ(MENUSTRIP), MM_NEWMENU, 0, va_getlinearva(tags, struct TagItem *));
	va_end(tags);

	return result;
}

static BOOL file_exists(const char *path) {
	APTR window;
	BPTR file;

	/* Disable "Please insert volume ..." requesters */
	window = SetProcWindow((APTR)~0);

	file = Open(path, MODE_OLDFILE);

	SetProcWindow(window);

	if (file == ZERO)
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
		AddPart(path, name, path_size);

		if (file_exists(path))
			return TRUE;
	}

	return FALSE;
}

static Object *load_image(struct PlayCDDAData *pcd, const char *name) {
	struct PlayCDDAGUI *pcg = &pcd->pcd_GUIData;
	char    normal_path[64];
	char    selected_path[64];
	char    disabled_path[64];
	Object *image;

	if (!find_image(name, normal_path, sizeof(normal_path)))
		return NULL;

	strlcpy(selected_path, normal_path, sizeof(selected_path));
	strlcat(selected_path, "_s", sizeof(selected_path));

	strlcpy(disabled_path, normal_path, sizeof(disabled_path));
	strlcat(disabled_path, "_g", sizeof(disabled_path));

	image = NewObject(BitMapClass, NULL,
		BITMAP_Screen,             pcg->pcg_Screen,
		BITMAP_SourceFile,         normal_path,
		BITMAP_SelectSourceFile,   selected_path,
		BITMAP_DisabledSourceFile, disabled_path,
		BITMAP_Masking,            TRUE,
		TAG_END);

	return image;
}

static BOOL add_speed_button(struct PlayCDDAData *pcd, UWORD id, BOOL enabled, const char *image_name) {
	struct PlayCDDAGUI *pcg = &pcd->pcd_GUIData;
	struct List *sb_list = &pcg->pcg_ButtonList;
	Object      *sb_image;
	struct Node *sb_node;

	sb_image = load_image(pcd, image_name);
	if (sb_image == NULL)
		return FALSE;

	sb_node = AllocSpeedButtonNode(id,
		SBNA_Image,     sb_image,
		SBNA_Highlight, SBH_IMAGE,
		SBNA_Disabled,  !enabled,
		TAG_END);
	if (sb_node == NULL) {
		DisposeObject(sb_image);
		return FALSE;
	}

	AddTail(sb_list, sb_node);
	return TRUE;
}

static Object *create_track_buttons(struct PlayCDDAData *pcd, int columns, int rows) {
	struct PlayCDDAGUI *pcg = &pcd->pcd_GUIData;
	Object *table_layout;
	Object *column_layout;
	Object *button;
	int     i, j, index;

	table_layout = NewObject(LayoutClass, NULL, TAG_END);
	if (table_layout == NULL)
		goto cleanup;

	for (i = 0; i < columns; i++) {
		column_layout = NewObject(LayoutClass, NULL, LAYOUT_Orientation, LAYOUT_ORIENT_VERT, TAG_END);
		if (column_layout == NULL)
			goto cleanup;

		SetAttrs(table_layout, LAYOUT_AddChild, column_layout, TAG_END);

		for (j = 0; j < rows; j++) {
			index = (j * columns) + i;

			button = NewObject(ButtonClass, NULL,
				GA_ID,          OID_TRACK01 + index,
				GA_RelVerify,   TRUE,
				GA_Disabled,    TRUE,
				BUTTON_Integer, index + 1,
				TAG_END);
			if (button == NULL)
				goto cleanup;

			SetAttrs(column_layout, LAYOUT_AddChild, button, TAG_END);
		}
	}

	return table_layout;

cleanup:
	if (table_layout != NULL)
		DisposeObject(table_layout);

	return NULL;
}

BOOL create_gui(struct PlayCDDAData *pcd) {
	struct PlayCDDAGUI *pcg = &pcd->pcd_GUIData;
	Object             *sub_layout_1;
	Object             *sub_layout_2;
	Object             *sub_layout_3, *volume_label;
	BOOL                menu_done;
	struct List        *list;
	struct Node        *node;
	int                 index;
	struct CDROMDrive  *cdd;
	char                label[256];
	Object             *sub_menu, *menu_item;
	int                 num_buttons;

	IntuitionBase = OpenLibrary("intuition.library", 53);
	if (IntuitionBase == NULL)
		return FALSE;

	IIntuition = (struct IntuitionIFace *)GetInterface(IntuitionBase, "main", 1, NULL);
	if (IIntuition == NULL)
		return FALSE;

	WindowBase = OpenClass("window.class", 53, &WindowClass);
	if (WindowBase == NULL)
		return FALSE;

	LayoutBase = OpenClass("gadgets/layout.gadget", 53, &LayoutClass);
	if (LayoutBase == NULL)
		return FALSE;

	ButtonBase = OpenClass("gadgets/button.gadget", 53, &ButtonClass);
	if (ButtonBase == NULL)
		return FALSE;

	SliderBase = OpenClass("gadgets/slider.gadget", 53, &SliderClass);
	if (SliderBase == NULL)
		return FALSE;

	SpeedBarBase = OpenClass("gadgets/speedbar.gadget", 53, &SpeedBarClass);
	if (SpeedBarBase == NULL)
		return FALSE;

	LabelBase = OpenClass("images/label.image", 53, &LabelClass);
	if (LabelBase == NULL)
		return FALSE;

	BitMapBase = OpenClass("images/bitmap.image", 53, &BitMapClass);
	if (BitMapBase == NULL)
		return FALSE;

	ISpeedBar = (struct SpeedBarIFace *)GetInterface(&SpeedBarBase->cl_Lib, "main", 1, NULL);
	if (ISpeedBar == NULL)
		return FALSE;

	pcg->pcg_AppPort = AllocSysObject(ASOT_PORT, NULL);
	if (pcg->pcg_AppPort == NULL)
		return FALSE;

	pcg->pcg_Screen = LockPubScreen(NULL);
	if (pcg->pcg_Screen == NULL)
		return FALSE;

	menu_done = create_menu(pcd,
		NM_Menu, "PlayCDDA", MA_ID, MID_PROJECT_MENU,
			NM_Item, STR(PROJECT_ABOUT), MA_Key, "?", MA_ID, MID_PROJECT_ABOUT,
			NM_Item, ML_SEPARATOR,
			NM_Item, STR(PROJECT_ICONIFY), MA_Key, "I", MA_ID, MID_PROJECT_ICONIFY,
			NM_Item, ML_SEPARATOR,
			NM_Item, STR(PROJECT_CDROMDRIVE), MA_ID, MID_PROJECT_CDROMDRIVE,
			NM_Item, ML_SEPARATOR,
			NM_Item, STR(PROJECT_QUIT), MA_Key, "Q", MA_ID, MID_PROJECT_QUIT,
		TAG_END);
	if (!menu_done)
		return FALSE;

	list     = &pcd->pcd_CDDrives;
	index    = 0;
	sub_menu = (Object *)DoMethod(OBJ(MENUSTRIP), MM_FINDID, 0, MID_PROJECT_CDROMDRIVE);
	if (sub_menu == NULL)
		return FALSE;

	for (node = list->lh_Head; node->ln_Succ; node = node->ln_Succ) {
		cdd = (struct CDROMDrive *)node;

		snprintf(label, sizeof(label), "%s [%s:%lu]", node->ln_Name, cdd->cdd_Device, cdd->cdd_Unit);

		menu_item = NewObject(NULL, "menuclass",
			MA_Type,     T_ITEM,
			MA_Label,    strdup(label),
			MA_ID,       MID_PROJECT_CDROMDRIVE_01 + index,
			MA_MX,       ~(1 << index),
			MA_Selected, (cdd == pcd->pcd_CurrentDrive),
			MA_UserData, cdd,
			TAG_END);
		if (menu_item == NULL)
			return FALSE;

		SetAttrs(sub_menu, MA_AddChild, menu_item, TAG_END);

		if (++index >= 32)
			break;
	}

	NewList(&pcg->pcg_ButtonList);

	num_buttons  = add_speed_button(pcd, SBID_EJECT, FALSE, "tapeeject");
	num_buttons += add_speed_button(pcd, SBID_STOP,  FALSE, "tapestop" );
	num_buttons += add_speed_button(pcd, SBID_PAUSE, FALSE, "tapepause");
	num_buttons += add_speed_button(pcd, SBID_PREV,  FALSE, "tapelast" );
	num_buttons += add_speed_button(pcd, SBID_PLAY,  FALSE, "tapeplay" );
	num_buttons += add_speed_button(pcd, SBID_NEXT,  FALSE, "tapenext" );
	if (num_buttons != SBID_NEXT)
		return FALSE;

	OBJ(STATUS_DISPLAY) = NewObject(ButtonClass, NULL,
		GA_ID,       OID_STATUS_DISPLAY,
		GA_ReadOnly, TRUE,
		GA_Text,     STR(NODISC),
		TAG_END);

	OBJ(SEEK_BAR) = NewObject(SliderClass, NULL,
		GA_ID,              OID_SEEK_BAR,
		GA_RelVerify,       TRUE,
		GA_Disabled,        TRUE,
		SLIDER_Orientation, SLIDER_HORIZONTAL,
		TAG_END);

	OBJ(BUTTON_BAR) = NewObject(SpeedBarClass, NULL,
		GA_ID,               OID_BUTTON_BAR,
		GA_RelVerify,        TRUE,
		SPEEDBAR_Buttons,    &pcg->pcg_ButtonList,
		SPEEDBAR_BevelStyle, BVS_NONE,
		TAG_END);

	sub_layout_1 = NewObject(LayoutClass, NULL,
		LAYOUT_Orientation,   LAYOUT_ORIENT_VERT,
		LAYOUT_AddChild,      OBJ(STATUS_DISPLAY),
		LAYOUT_AddChild,      OBJ(SEEK_BAR),
		CHILD_WeightedHeight, 0,
		LAYOUT_AddChild,      OBJ(BUTTON_BAR),
		CHILD_NominalSize,    TRUE,
		CHILD_WeightedHeight, 0,
		TAG_END);

	sub_layout_2 = create_track_buttons(pcd, 8, 4);

	OBJ(VOLUME_SLIDER) = NewObject(SliderClass, NULL,
		GA_ID,              OID_VOLUME_SLIDER,
		GA_RelVerify,       TRUE,
		SLIDER_Orientation, SLIDER_VERTICAL,
		SLIDER_Min,         0,
		SLIDER_Max,         64,
		SLIDER_Level,       64,
		SLIDER_Invert,      TRUE,
		TAG_END);

	volume_label = NewObject(LabelClass, NULL,
		LABEL_Text, STR(VOLUME_GAD),
		TAG_END);

	sub_layout_3 = NewObject(LayoutClass, NULL,
		LAYOUT_Orientation,    LAYOUT_ORIENT_VERT,
		LAYOUT_HorizAlignment, LALIGN_CENTER,
		LAYOUT_AddChild,       OBJ(VOLUME_SLIDER),
		CHILD_WeightedWidth,   0,
		LAYOUT_AddImage,       volume_label,
		TAG_END);

	OBJ(ROOT_LAYOUT) = NewObject(LayoutClass, NULL,
		LAYOUT_AddChild, sub_layout_1,
		LAYOUT_AddChild, sub_layout_2,
		LAYOUT_AddChild, sub_layout_3,
		TAG_END);

	OBJ(WINDOW) = NewObject(WindowClass, NULL,
		WA_PubScreen, pcg->pcg_Screen,
		WA_Title,     VERS,
		WA_Flags,     WFLG_CLOSEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_NOCAREREFRESH | WFLG_NEWLOOKMENUS,
		WA_IDCMP,     IDCMP_CLOSEWINDOW | IDCMP_MENUPICK | IDCMP_GADGETUP,
		WINDOW_MenuStrip,     OBJ(MENUSTRIP),
		WINDOW_Position,      WPOS_CENTERMOUSE,
		WINDOW_AppPort,       pcg->pcg_AppPort,
		WINDOW_IconifyGadget, TRUE,
		WINDOW_IconTitle,     "PlayCDDA",
		WINDOW_Icon,          pcd->pcd_Icon,
		WINDOW_IconNoDispose, TRUE,
		WINDOW_Layout,        OBJ(ROOT_LAYOUT),
		TAG_END);
	if (OBJ(WINDOW) == NULL)
		return FALSE;

	if ((struct Window *)DoMethod(OBJ(WINDOW), WM_OPEN, NULL) == NULL)
		return FALSE;

	return TRUE;
}

void destroy_gui(struct PlayCDDAData *pcd) {
	struct PlayCDDAGUI *pcg = &pcd->pcd_GUIData;

	if (OBJ(WINDOW) != NULL)
		DisposeObject(OBJ(WINDOW));

	if (pcg->pcg_ButtonList.lh_Head != NULL) {
		struct List *sb_list = &pcg->pcg_ButtonList;
		struct Node *sb_node;
		Object      *sb_image;

		while ((sb_node = RemHead(sb_list)) != NULL) {
			GetSpeedButtonNodeAttrs(sb_node, SBNA_Image, &sb_image, TAG_END);

			FreeSpeedButtonNode(sb_node);

			DisposeObject(sb_image);
		}
	}

	if (OBJ(MENUSTRIP) != NULL)
		DisposeObject(OBJ(MENUSTRIP));

	if (pcg->pcg_Screen != NULL)
		UnlockPubScreen(NULL, pcg->pcg_Screen);

	if (pcg->pcg_AppPort != NULL)
		FreeSysObject(ASOT_PORT, pcg->pcg_AppPort);

	if (ISpeedBar != NULL)
		DropInterface((struct Interface *)ISpeedBar);

	if (BitMapBase != NULL)
		CloseClass(BitMapBase);

	if (LabelBase != NULL)
		CloseClass(LabelBase);

	if (SpeedBarBase != NULL)
		CloseClass(SpeedBarBase);

	if (SliderBase != NULL)
		CloseClass(SliderBase);

	if (ButtonBase != NULL)
		CloseClass(ButtonBase);

	if (LayoutBase != NULL)
		CloseClass(LayoutBase);

	if (WindowBase != NULL)
		CloseClass(WindowBase);

	if (IIntuition != NULL)
		DropInterface((struct Interface *)IIntuition);

	if (IntuitionBase != NULL)
		CloseLibrary(IntuitionBase);
}

int main_loop(struct PlayCDDAData *pcd) {
	struct PlayCDDAGUI *pcg = &pcd->pcd_GUIData;
	struct Window *window;
	ULONG sigmask, signals, result;
	UWORD code;
	BOOL  done = FALSE;
	int   menu_id;
	int   gadget_id;

	while (!done) {
		GetAttr(WINDOW_SigMask, OBJ(WINDOW), &sigmask);
		signals = Wait(sigmask | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);

		if (signals & SIGBREAKF_CTRL_C)
			done = TRUE;

		if (signals & SIGBREAKF_CTRL_F) {
			window = (struct Window *)DoMethod(OBJ(WINDOW), WM_OPEN, NULL);
			if (window != NULL)
				ScreenToFront(window->WScreen);
		}

		if (signals & sigmask) {
			while ((result = DoMethod(OBJ(WINDOW), WM_HANDLEINPUT, &code)) != WMHI_LASTMSG) {
				switch (result & WMHI_CLASSMASK) {
					case WMHI_MENUPICK:
						menu_id = NO_MENU_ID;
						while ((menu_id = DoMethod(OBJ(MENUSTRIP), MM_NEXTSELECT, 0, menu_id)) != NO_MENU_ID) {
							switch (menu_id) {
								case MID_PROJECT_ABOUT:
									/* FIXME: Implement about window */
									break;

								case MID_PROJECT_ICONIFY:
									DoMethod(OBJ(WINDOW), WM_ICONIFY, NULL);
									break;

								case MID_PROJECT_QUIT:
									done = TRUE;
									break;
							}
						}
						break;

					case WMHI_GADGETUP:
						gadget_id = result & WMHI_GADGETMASK;
						switch (gadget_id) {
							case OID_BUTTON_BAR:
								switch (code) {
									/* FIXME: Implement tapedeck controls */
								}
								break;

							case OID_SEEK_BAR:
								/* FIXME: Implement seeking */
								break;

							case OID_VOLUME_SLIDER:
								pcd->pcd_Volume = (ULONG)code << 10;
								break;

							default:
								if (gadget_id >= OID_TRACK01 && gadget_id <= OID_TRACK32) {
									/* FIXME: Implement track selection */
								}
								break;
						}
						break;

					case WMHI_ICONIFY:
						DoMethod(OBJ(WINDOW), WM_ICONIFY, NULL);
						break;

					case WMHI_UNICONIFY:
						window = (struct Window *)DoMethod(OBJ(WINDOW), WM_OPEN, NULL);
						if (window != NULL)
							ScreenToFront(window->WScreen);
						break;

					case WMHI_CLOSEWINDOW:
						done = TRUE;
						break;
				}
			}
		}
	}

	return RETURN_OK;
}

#endif /* ENABLE_REACTION_GUI */

