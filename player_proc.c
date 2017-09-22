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

#ifndef __amigaos4__
#include <clib/alib_protos.h>
#endif

static void init_msgport(struct MsgPort *port) {
	port->mp_Node.ln_Type = NT_MSGPORT;
	port->mp_Flags        = PA_SIGNAL;
	port->mp_SigBit       = AllocSignal(-1);
	port->mp_SigTask      = FindTask(NULL);

	NewList(&port->mp_MsgList);
}

static void deinit_msgport(struct MsgPort *port) {
	if (port->mp_Node.ln_Type == NT_MSGPORT) {
		if ((BYTE)port->mp_SigBit != -1)
			FreeSignal(port->mp_SigBit);

		memset(port, 0, sizeof(*port));
	}
}

static void init_player_message(struct PlayCDDAData *pcd, struct PlayCDDAMsg *pcm, struct MsgPort *reply_port) {
	pcm->pcm_Msg.mn_Node.ln_Type = NT_MESSAGE;
	pcm->pcm_Msg.mn_ReplyPort    = reply_port;
	pcm->pcm_Msg.mn_Length       = sizeof(*pcm);

	pcm->pcm_GlobalData = pcd;
	pcm->pcm_Command    = PCC_INVALID;
}

static BOOL valid_player_message(const struct PlayCDDAData *pcd, const struct PlayCDDAMsg *pcm) {
	if (pcm->pcm_Msg.mn_Node.ln_Type == NT_MESSAGE && pcm->pcm_Msg.mn_Length == sizeof(*pcm)) {
		if (pcm->pcm_GlobalData == pcd)
			return TRUE;
	}
	return FALSE;
}

#ifdef __amigaos4__

static int send_message_func(struct Hook *hook, ULONG pid, struct Process *proc) {
	struct Message *msg = hook->h_Data;

	if (proc->pr_ProcessID == pid) {
		PutMsg(&proc->pr_MsgPort, msg);
		return TRUE;
	}

	return FALSE;
}

static BOOL send_message_to_pid(ULONG pid, struct Message *msg) {
	struct Hook hook;

	memset(&hook, 0, sizeof(hook));

	hook.h_Entry = (HOOKFUNC)send_message_func;
	hook.h_Data  = msg;

	if (ProcessScan(&hook, (APTR)pid, 0))
		return TRUE;

	return FALSE;
}

static int find_proc_func(struct Hook *hook, ULONG pid, const struct Process *proc) {
	if (proc->pr_ProcessID == pid)
		return TRUE;

	return FALSE;
}

static BOOL find_proc_by_pid(ULONG pid) {
	struct Hook hook;

	memset(&hook, 0, sizeof(hook));

	hook.h_Entry = (HOOKFUNC)find_proc_func;

	if (ProcessScan(&hook, (APTR)pid, 0))
		return TRUE;

	return FALSE;
}

#else

static BOOL is_in_list(const struct List *list, const struct Node *node) {
	const struct Node *listnode;

	for (listnode = list->lh_Head; listnode->ln_Succ; listnode = listnode->ln_Succ) {
		if (listnode == node)
			return TRUE;
	}

	return FALSE;
}

static BOOL send_message_to_pid(struct Process *proc, struct Message *msg) {
	const struct ExecBase *exec = (const struct ExecBase *)SysBase;
	BOOL                   result = FALSE;

	Forbid();

	if (is_in_list(&exec->TaskReady, &proc->pr_Task.tc_Node) ||
		is_in_list(&exec->TaskWait, &proc->pr_Task.tc_Node))
	{
		PutMsg(&proc->pr_MsgPort, msg);
		result = TRUE;
	}

	Permit();

	return result;
}

static BOOL find_proc_by_pid(struct Process *proc) {
	const struct ExecBase *exec = (const struct ExecBase *)SysBase;
	BOOL                   result = FALSE;

	Forbid();

	if (is_in_list(&exec->TaskReady, &proc->pr_Task.tc_Node) ||
		is_in_list(&exec->TaskWait, &proc->pr_Task.tc_Node))
	{
		result = TRUE;
	}

	Permit();

	return result;
}

#endif

static BOOL do_player_command(struct PlayCDDAData *pcd, pcm_command_t command) {
	struct PlayCDDAPlayerData *pcpd = &pcd->pcd_PlayerData;
	struct PlayCDDAMsg        *pcm  = &pcpd->pcpd_PlayerMsg;

	pcm->pcm_Command = command;

	if (!send_message_to_pid(pcpd->pcpd_ProcessID, &pcm->pcm_Msg))
		return FALSE;

	WaitPort(&pcpd->pcpd_ReplyPort);

	pcm = (struct PlayCDDAMsg *)GetMsg(&pcpd->pcpd_ReplyPort);

	return pcm->pcm_Result;
}

static void wait_for_death(pcpd_proc_id_t pid) {
	while (find_proc_by_pid(pid)) Delay(10);
}

static int player_proc_entry(void) {
	struct Process            *me;
	struct MsgPort            *myport;
	struct PlayCDDAData       *pcd;
	struct PlayCDDAMsg        *pcm;
	struct PlayCDDAPlayerData *pcpd;
	struct MsgPort             ioport;
	struct AHIRequest         *ahireq[2];
	struct IOStdReq           *cdreq;
	BOOL                       playing;
	BOOL                       done;
	int                        rc = RETURN_ERROR;

	me     = (struct Process *)FindTask(NULL);
	myport = &me->pr_MsgPort;
	pcd    = me->pr_Task.tc_UserData;

	WaitPort(myport);
	pcm = (struct PlayCDDAMsg *)GetMsg(myport);

	if (!valid_player_message(pcd, pcm) || pcm->pcm_Command != PCC_STARTUP)
		return RETURN_FAIL;

	pcpd = &pcd->pcd_PlayerData;

	init_msgport(&ioport);

	ahireq[0] = (struct AHIRequest *)copy_iorequest((struct IORequest *)pcd->pcd_AHIReq);
	ahireq[1] = (struct AHIRequest *)copy_iorequest((struct IORequest *)pcd->pcd_AHIReq);
	if (ahireq[0] == NULL || ahireq[1] == NULL)
		goto cleanup;

	cdreq = (struct IOStdReq *)copy_iorequest((struct IORequest *)pcd->pcd_CDReq);
	if (cdreq == NULL)
		goto cleanup;

	ahireq[0]->ahir_Std.io_Message.mn_ReplyPort = &ioport;
	ahireq[1]->ahir_Std.io_Message.mn_ReplyPort = &ioport;

	cdreq->io_Message.mn_ReplyPort = &ioport;

	pcm->pcm_Result = TRUE;
	ReplyMsg(&pcm->pcm_Msg);

	playing = FALSE;
	done = FALSE;

	while (!done) {
		if (!playing) {
			WaitPort(myport);
		}

		while ((pcm = (struct PlayCDDAMsg *)GetMsg(myport)) != NULL) {
			if (valid_player_message(pcd, pcm)) {
				switch (pcm->pcm_Command) {
					/* FIXME: Add support for player commands */

					case PCC_DIE:
						if (playing) {
							pcm->pcm_Result = FALSE;
							break;
						}

						done = TRUE;
						pcm->pcm_Result = TRUE;
						break;

					default:
						pcm->pcm_Result = FALSE;
						break;
				}
			}
			ReplyMsg(&pcm->pcm_Msg);
		}

		if (playing) {
			/* FIXME: Add CDDA playback code */
		}
	}

cleanup:
	delete_iorequest_copy((struct IORequest *)cdreq);

	delete_iorequest_copy((struct IORequest *)ahireq[0]);
	delete_iorequest_copy((struct IORequest *)ahireq[1]);

	deinit_msgport(&ioport);

	return rc;
}

static BOOL start_player_proc(struct PlayCDDAData *pcd) {
	struct PlayCDDAPlayerData *pcpd = &pcd->pcd_PlayerData;
	struct Process            *proc;

	if (pcpd->pcpd_ProcessID != 0)
		return TRUE;

	init_msgport(&pcpd->pcpd_ReplyPort);

	init_player_message(pcd, &pcpd->pcpd_PlayerMsg, &pcpd->pcpd_ReplyPort);

	proc = CreateNewProcTags(
		NP_Name,        "PlayCDDA Audio Process",
		NP_Entry,       &player_proc_entry,
		NP_StackSize,   8192,
		NP_Priority,    PLAYER_PROC_PRI,
		NP_CurrentDir,  0,
		NP_Path,        0,
		NP_CopyVars,    FALSE,
		NP_Input,       0,
		NP_Output,      0,
		NP_Error,       0,
		NP_CloseInput,  FALSE,
		NP_CloseOutput, FALSE,
		NP_CloseError,  FALSE,
		TAG_END);
	if (proc == NULL)
		goto cleanup;

#ifdef __amigaos4__
	/* IoErr() returns the PID if CreateNewProcTags() was successful */
	pcpd->pcpd_ProcessID = IoErr();
#else
	/* No PID here, so use process pointer */
	pcpd->pcpd_ProcessID = proc;
#endif

	if (!do_player_command(pcd, PCC_STARTUP)) {
		wait_for_death(pcpd->pcpd_ProcessID);
		pcpd->pcpd_ProcessID = 0;
		goto cleanup;
	}

	return TRUE;

cleanup:
	deinit_msgport(&pcpd->pcpd_ReplyPort);

	return FALSE;
}

static void kill_player_proc(struct PlayCDDAData *pcd) {
	struct PlayCDDAPlayerData *pcpd = &pcd->pcd_PlayerData;

	if (pcpd->pcpd_ProcessID == 0)
		return;

	if (!do_player_command(pcd, PCC_DIE))
		return;

	wait_for_death(pcpd->pcpd_ProcessID);
	pcpd->pcpd_ProcessID = 0;

	deinit_msgport(&pcpd->pcpd_ReplyPort);
}

void set_volume(struct PlayCDDAData *pcd, int volume) {
	struct PlayCDDAPlayerData *pcpd = &pcd->pcd_PlayerData;

	if (volume >= 0 && volume <= 64) {
		pcpd->pcpd_Volume = volume << 10;
	}
}

int get_volume(const struct PlayCDDAData *pcd) {
	const struct PlayCDDAPlayerData *pcpd = &pcd->pcd_PlayerData;

	return pcpd->pcpd_Volume >> 10;
}

