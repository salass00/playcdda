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

#if defined(AMIGA) && !defined(__amigaos4__)

size_t strlcpy(char *dst, const char *src, size_t size) {
	char       *d = dst;
	const char *s = src;
	size_t      n = size;

	/* Copy as many characters as will fit while leaving
	 * room for the nul-terminator.
	 */
	if (n != 0) {
		n--; /* Leave space for nul-terminator. */
		do {
			if ((*d++ = *s++) == '\0')
				break;
		} while (--n != 0);
	}

	if (n == 0) {
		/* Not enough room, so cut string off. */
		if (size != 0)
			*d = '\0';
		s += strlen(s) + 1;
	}

	return s - src - 1;
}

size_t strlcat(char *dst, const char *src, size_t size) {
	char       *d = dst;
	const char *s = src;
	size_t      n = size;

	/* Find end of dst but don't read more than size characters */
	if (n != 0) {
		do {
			if (*d == '\0')
				break;
			d++;
		} while (--n != 0);
	}

	return (d - dst) + strlcpy(d, s, n);
}

#endif

