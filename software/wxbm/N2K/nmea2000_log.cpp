/*
 * Copyright (c) 2022 Manuel Bouyer
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "NMEA2000.h"
#include "nmea2000_defs_tx.h"
#include "nmea2000_defs_rx.h"
#include "../wxbm.h"

bool
private_log_tx::sendreq(uint8_t cmd, uint8_t sid, uint16_t idx)
{
	bool ret;
	uint82frame(cmd, 0);
	uint82frame(sid, 1);
	uint162frame(idx, 2);
	valid = true;
	ret = nmea2000P->send_bypgn(PRIVATE_LOG, true);
	valid = false;
	return ret;
}

#define PRIVATE_LOG_RESET_MAGIC 0x18e1

bool
private_log_tx::sendreset(uint8_t sid)
{
	bool ret;
	uint82frame(PRIVATE_LOG_RESET, 0);
	uint82frame(sid, 1);
	uint162frame(0x18e1, 2);
	valid = true;
	ret = nmea2000P->send_bypgn(PRIVATE_LOG, true);
	valid = false;
	return ret;
}


bool
nmea2000_private_log_rx::fast_handle(const nmea2000_frame &f)
{
	int len = getlen();
	uint8_t cmd = f.frame2uint8(0);
	uint8_t sid = f.frame2uint8(1);

	switch(cmd) {
	case PRIVATE_LOG_REPLY:
		{
		uint16_t idx = f.frame2uint16(2);
		if (len <= 4) {
			wxp->logComplete(sid);
			return true;
		}
		for (int i = 4; i < len; ) {
			u_int temp = f.frame2uint8(i);
			u_int volts = f.frame2uint8(i+1);
			int32_t amps = f.frame2int16(i+2);
			u_int data = f.frame2uint8(i+4);
			bool nvalid = (data & 0x8);
			u_int instance = ((data >> 6) & 0x3);
			volts = volts | (data & 0x7) << 8;
			amps = amps | ((data >> 4) & 0x3) << 8;
			/* expand sign */
			if (amps & 0x20000) {
				amps |= 0xfffc0000;
			}
			i += 5;
			wxp->addLogEntry(sid, (double)volts / 100.0,
			    (double)amps / 1000,
			    (temp == 0xff) ? -1 : (temp + 233),
			    instance, (idx & ~0x100));
			if ((idx & 0x100) != 0 && i >= len)
				wxp->logComplete(sid);
		}
		return true;
		}
	case PRIVATE_LOG_ERROR:
		{
		uint8_t err = f.frame2uint8(2);
		printf("log_rx error %d sid %d\n", err, sid);
		wxp->logError(sid, err);
		return true;
		}
	default:
		printf("private log cmd %d sid %d\n");
	}
	return false;
}

void
nmea2000_private_log_rx::tick()
{
	wxp->logTick();
}
