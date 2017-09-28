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

#ifdef __amigaos4__

struct MsgPort *create_msgport(void) {
	return AllocSysObject(ASOT_PORT, NULL);
}

void delete_msgport(struct MsgPort *port) {
	FreeSysObject(ASOT_PORT, port);
}

struct IORequest *create_iorequest(struct MsgPort *port, ULONG size) {
	return AllocSysObjectTags(ASOT_IOREQUEST,
		ASOIOR_ReplyPort, port,
		ASOIOR_Size,      size,
		TAG_END);
}

struct IORequest *copy_iorequest(const struct IORequest *original) {
	return AllocSysObjectTags(ASOT_IOREQUEST,
		ASOIOR_Duplicate, original,
		TAG_END);
}

void delete_iorequest(struct IORequest *ioreq) {
	FreeSysObject(ASOT_IOREQUEST, ioreq);
}

void delete_iorequest_copy(struct IORequest *ioreq) {
	delete_iorequest(ioreq);
}

#else

struct MsgPort *create_msgport(void) {
	return CreateMsgPort();
}

void delete_msgport(struct MsgPort *port) {
	DeleteMsgPort(port);
}

struct IORequest *create_iorequest(struct MsgPort *port, ULONG size) {
	return CreateIORequest(port, size);
}

struct IORequest *copy_iorequest(const struct IORequest *original) {
	ULONG             size = original->io_Message.mn_Length;
	struct IORequest *ioreq;

	ioreq = alloc_shared_mem(size);

	if (ioreq != NULL)
		CopyMem(original, ioreq, size);

	return ioreq;
}

void delete_iorequest(struct IORequest *ioreq) {
	DeleteIORequest(ioreq);
}

void delete_iorequest_copy(struct IORequest *ioreq) {
	if (ioreq != NULL) {
		free_shared_mem(ioreq, ioreq->io_Message.mn_Length);
	}
}

#endif

