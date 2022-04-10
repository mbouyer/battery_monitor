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

#include <wx/wx.h>
#include <iostream>
#include <N2K/NMEA2000.h>
#include "bmstatus.h"
#include "bmlog.h"

bmLog::bmLog(wxWindow* parent)
	: wxFrame(parent, wxID_ANY, _T("bmLog"))
{
	cur_log_entry = 0;
	log_tx = (private_log_tx *)nmea2000P->get_frametx(nmea2000P->get_tx_bypgn(PRIVATE_LOG));
	bmsizer = new wxFlexGridSizer(4, 5, 5);
}

void
bmLog::address(int a)
{
	log_tx->sendreq(PRIVATE_LOG_REQUEST_FIRST, 10, 0xaa15);

}

void
bmLog::addLogEntry(int sid, double volts, double amps,
                 int temp, int instance, int idx, bool last)
{
	wxASSERT(cur_log_entry < LOG_ENTRIES);
	log_entries[cur_log_entry].volts = volts;
	log_entries[cur_log_entry].amps = amps;
	log_entries[cur_log_entry].temp = temp;
	log_entries[cur_log_entry].instance = instance;
	log_entries[cur_log_entry].idx = idx;
	cur_log_entry++;
	if (last) {
		for (int i = 0; i < cur_log_entry; i++) {
			printf("sid 0x%02x idx 0x%02x %3d inst %2d volts %2.2f amps %3.3f temp %3d\n",
			    sid, log_entries[i].idx, i,
			    log_entries[i].instance,
			    log_entries[i].volts, log_entries[i].amps,
			    log_entries[i].temp);
		}
		cur_log_entry = 0;
		log_tx->sendreq(PRIVATE_LOG_REQUEST_NEXT, sid + 1, idx);
	}
}
