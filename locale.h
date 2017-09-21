#ifndef LOCALE_H
#define LOCALE_H

/* This file was created automatically by catcomp v1.4.
 * Do NOT edit by hand!
 */

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifdef CATCOMP_ARRAY
#ifndef CATCOMP_NUMBERS
#define CATCOMP_NUMBERS
#endif
#ifndef CATCOMP_STRINGS
#define CATCOMP_STRINGS
#endif
#endif

#ifdef CATCOMP_BLOCK
#ifndef CATCOMP_STRINGS
#define CATCOMP_STRINGS
#endif
#endif

#ifdef CATCOMP_NUMBERS

#define MSG_PROJECT_ABOUT 0
#define MSG_PROJECT_ICONIFY 1
#define MSG_PROJECT_QUIT 2
#define MSG_ABOUT_WND 3
#define MSG_ABOUT_REQ 4
#define MSG_OK_GAD 5
#define MSG_PLAYING 6
#define MSG_STOPPED 7
#define MSG_NODISC 8
#define MSG_NOTRACK 9
#define MSG_EJECT_GAD 10
#define MSG_STOP_GAD 11
#define MSG_PAUSE_GAD 12
#define MSG_PREV_GAD 13
#define MSG_PLAY_GAD 14
#define MSG_NEXT_GAD 15
#define MSG_VOLUME_GAD 16

#endif

#ifdef CATCOMP_STRINGS

#define MSG_PROJECT_ABOUT_STR "About..."
#define MSG_PROJECT_ICONIFY_STR "Iconify"
#define MSG_PROJECT_QUIT_STR "Quit"
#define MSG_ABOUT_WND_STR "About - %s"
#define MSG_ABOUT_REQ_STR "%s version %ld.%ld\n\nCopyright (C) 2010-2017 Fredrik Wikstrom\nWebsite: http://www.a500.org\nE-mail: fredrik@a500.org"
#define MSG_OK_GAD_STR "_Ok"
#define MSG_PLAYING_STR "Track %02ld - %02ld:%02ld"
#define MSG_STOPPED_STR "Track %02ld - STOPPED"
#define MSG_NODISC_STR "No disc in drive"
#define MSG_NOTRACK_STR "No track selected"
#define MSG_EJECT_GAD_STR "Eject"
#define MSG_STOP_GAD_STR "Stop"
#define MSG_PAUSE_GAD_STR "Pause"
#define MSG_PREV_GAD_STR "Previous"
#define MSG_PLAY_GAD_STR "Play"
#define MSG_NEXT_GAD_STR "Next"
#define MSG_VOLUME_GAD_STR "Volume"

#endif

#ifdef CATCOMP_ARRAY

struct CatCompArrayType
{
	LONG         cca_ID;
	CONST_STRPTR cca_Str;
};

STATIC CONST struct CatCompArrayType CatCompArray[] =
{
	{MSG_PROJECT_ABOUT,(CONST_STRPTR)MSG_PROJECT_ABOUT_STR},
	{MSG_PROJECT_ICONIFY,(CONST_STRPTR)MSG_PROJECT_ICONIFY_STR},
	{MSG_PROJECT_QUIT,(CONST_STRPTR)MSG_PROJECT_QUIT_STR},
	{MSG_ABOUT_WND,(CONST_STRPTR)MSG_ABOUT_WND_STR},
	{MSG_ABOUT_REQ,(CONST_STRPTR)MSG_ABOUT_REQ_STR},
	{MSG_OK_GAD,(CONST_STRPTR)MSG_OK_GAD_STR},
	{MSG_PLAYING,(CONST_STRPTR)MSG_PLAYING_STR},
	{MSG_STOPPED,(CONST_STRPTR)MSG_STOPPED_STR},
	{MSG_NODISC,(CONST_STRPTR)MSG_NODISC_STR},
	{MSG_NOTRACK,(CONST_STRPTR)MSG_NOTRACK_STR},
	{MSG_EJECT_GAD,(CONST_STRPTR)MSG_EJECT_GAD_STR},
	{MSG_STOP_GAD,(CONST_STRPTR)MSG_STOP_GAD_STR},
	{MSG_PAUSE_GAD,(CONST_STRPTR)MSG_PAUSE_GAD_STR},
	{MSG_PREV_GAD,(CONST_STRPTR)MSG_PREV_GAD_STR},
	{MSG_PLAY_GAD,(CONST_STRPTR)MSG_PLAY_GAD_STR},
	{MSG_NEXT_GAD,(CONST_STRPTR)MSG_NEXT_GAD_STR},
	{MSG_VOLUME_GAD,(CONST_STRPTR)MSG_VOLUME_GAD_STR},
};

#endif

#ifdef CATCOMP_BLOCK

STATIC CONST UBYTE CatCompBlock[] =
{
	"\x00\x00\x00\x00\x00\x0A"
	MSG_PROJECT_ABOUT_STR "\x00\x00"
	"\x00\x00\x00\x01\x00\x08"
	MSG_PROJECT_ICONIFY_STR "\x00"
	"\x00\x00\x00\x02\x00\x06"
	MSG_PROJECT_QUIT_STR "\x00\x00"
	"\x00\x00\x00\x03\x00\x0C"
	MSG_ABOUT_WND_STR "\x00\x00"
	"\x00\x00\x00\x04\x00\x74"
	MSG_ABOUT_REQ_STR "\x00\x00"
	"\x00\x00\x00\x05\x00\x04"
	MSG_OK_GAD_STR "\x00"
	"\x00\x00\x00\x06\x00\x1A"
	MSG_PLAYING_STR "\x00"
	"\x00\x00\x00\x07\x00\x16"
	MSG_STOPPED_STR "\x00"
	"\x00\x00\x00\x08\x00\x12"
	MSG_NODISC_STR "\x00\x00"
	"\x00\x00\x00\x09\x00\x12"
	MSG_NOTRACK_STR "\x00"
	"\x00\x00\x00\x0A\x00\x06"
	MSG_EJECT_GAD_STR "\x00"
	"\x00\x00\x00\x0B\x00\x06"
	MSG_STOP_GAD_STR "\x00\x00"
	"\x00\x00\x00\x0C\x00\x06"
	MSG_PAUSE_GAD_STR "\x00"
	"\x00\x00\x00\x0D\x00\x0A"
	MSG_PREV_GAD_STR "\x00\x00"
	"\x00\x00\x00\x0E\x00\x06"
	MSG_PLAY_GAD_STR "\x00\x00"
	"\x00\x00\x00\x0F\x00\x06"
	MSG_NEXT_GAD_STR "\x00\x00"
	"\x00\x00\x00\x10\x00\x08"
	MSG_VOLUME_GAD_STR "\x00\x00"
};

#endif

#endif