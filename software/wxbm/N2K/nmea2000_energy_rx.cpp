/*
 * Copyright (c) 2019 Manuel Bouyer
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
#include "nmea2000_defs_rx.h"
#include <wxbm.h>

bool nmea2000_battery_status_rx::handle(const nmea2000_frame &f)
{
	int instance = f.frame2uint8(0);
	int volt = f.frame2int16(1);
	int current = f.frame2int16(3);
	int temp = f.frame2int16(5);
	gettimeofday(&last_rx, NULL);

	if (addr != f.getsrc()) {
		n2k_set_bm_address(f.getsrc());
		addr = f.getsrc(); 
	}


	wxp->setBatt(instance, volt / 100.0, current / 1000.0,
	    temp / 100.0 - 273.15, true);
	return true;
}

void nmea2000_battery_status_rx::tick()
{
	struct timeval now, diff;

	gettimeofday(&now, NULL);
	timersub(&now, &last_rx, &diff);
	if (diff.tv_sec >= 5)
		wxp->setBatt(-1, -1, -1, -1, false);
}
