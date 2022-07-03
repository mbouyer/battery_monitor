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
#include <err.h>
#include <N2K/NMEA2000.h>
#include "bmstatus.h"
#include "bmlog.h"

bmLog::bmLog(wxWindow* parent)
	: wxFrame(parent, wxID_ANY, _T("bmLog"))
{
	cur_log_entry = 0;
	log_req_state = LOG_REQ_IDLE;
	log_tx = (private_log_tx *)nmea2000P->get_frametx(nmea2000P->get_tx_bypgn(PRIVATE_LOG));
	bmsizer = new wxFlexGridSizer(4, 5, 5);
	if ((errno = pthread_mutex_init(&log_mtx, NULL)) != 0)
		err(1, "init log_mtx");
	log_state = LOG_INIT;
}

void
bmLog::address(int a)
{
	log_lock();
	if (log_req_state == LOG_REQ_IDLE && log_state == LOG_INIT) {
		log_req.cmd = PRIVATE_LOG_REQUEST_FIRST;
		log_req.sid++;
		log_req.idx = 0;
		log_req_state = LOG_REQ_DOREQ;
		log_update_state = LOG_UP_DOUP;
	}
	log_unlock();
}

void
bmLog::addLogEntry(int sid, double volts, double amps,
                 int temp, int instance, int idx, bool last)
{
	if (log_req_state != LOG_REQ_WAIT_BLOCK || log_req.sid != sid)
		return; /* not waiting for that */
	wxASSERT_MSG(cur_log_entry < LOG_ENTRIES && cur_log_entry >= 0,
	    wxString::Format("cur_log_entry %d", cur_log_entry));
	received_log_entries[cur_log_entry].volts = volts;
	received_log_entries[cur_log_entry].amps = amps;
	received_log_entries[cur_log_entry].temp = temp;
	received_log_entries[cur_log_entry].instance = instance;
	received_log_entries[cur_log_entry].id = (idx << ID_IDX_SHIFT) |
	    (cur_log_entry << ID_INDEX_SHIFT);
	received_log_entries[cur_log_entry].time = 0;
	cur_log_entry++;
	if (last) {
		log_lock();
		for (int i = 0; i < cur_log_entry; i++) {
			printf("sid 0x%02x idx 0x%06x %3d inst %2d volts %2.2f amps %3.3f temp %3d",
			    sid, received_log_entries[i].id, i,
			    received_log_entries[i].instance,
			    received_log_entries[i].volts, received_log_entries[i].amps,
			    received_log_entries[i].temp);
			switch(log_update_state) {
			case LOG_UP_IDLE:
				wxASSERT_MSG(false, _T("log_update_state idle"));
				break;
			case LOG_UP_SEARCH:
				if (received_log_entries[i].id ==
				     log_entries.back().id) {
					printf(" found\n");
					/* up to now these are new entries */
					log_update_state = LOG_UP_DOUP;
				} else {
					printf("\n");
				}
				break;
			case LOG_UP_DOUP:
				printf(" store\n");
				log_entries.push_back(received_log_entries[i]);
				break;
			}
		}
		cur_log_entry = 0;
		log_req.cmd = PRIVATE_LOG_REQUEST_NEXT;
		log_req.sid++;
		log_req.idx = idx;
		log_req_state = LOG_REQ_DOREQ;
		// sendreq();
		log_unlock();
	} else {
		wxASSERT_MSG(cur_log_entry < LOG_ENTRIES && cur_log_entry >= 0,
		    wxString::Format("cur_log_entry %d", cur_log_entry));
	}
}

void
bmLog::logError(int sid, int err)
{
	if (log_req_state != LOG_REQ_WAIT_BLOCK || log_req.sid != sid)
		return; /* not for us */
	log_lock();
	switch(err) {
	case PRIVATE_LOG_ERROR_NOTFOUND:
		wxASSERT(log_req.cmd == PRIVATE_LOG_REQUEST);
		printf("log idx 0x%x not found\n", log_req.idx);
		/* assume the log was reset */
		log_req.cmd = PRIVATE_LOG_REQUEST_FIRST;
		log_req.sid++;
		log_req.idx = 0;
		log_req_state = LOG_REQ_DOREQ;
		log_update_state = LOG_UP_DOUP;
		break;
	case PRIVATE_LOG_ERROR_LAST:
		wxASSERT(log_req.cmd == PRIVATE_LOG_REQUEST_NEXT);
		log_req_state = LOG_REQ_IDLE;
		log_update_state = LOG_UP_IDLE;
		printf("log complete\n");
		if (log_state == LOG_INIT)
			log_state = LOG_IDLE;
		gettimeofday(&last_data, NULL);
		break;
	}
	log_unlock();
}

void
bmLog::tick(void)
{
	log_lock();
	switch(log_req_state) {
	case LOG_REQ_IDLE:
		break;
	case LOG_REQ_DOREQ:
		sendreq();
		break;
	case LOG_REQ_WAIT_BLOCK:
	    {
		struct timeval now, diff;
		gettimeofday(&now, NULL);
		timersub(&now, &last_ev, &diff);
		if (diff.tv_sec >= 1) {
			/* timeout, resend last command */
			printf("timeout cmd %d sid 0x%x idx 0x%x\n", 
			    log_req.cmd, log_req.sid, log_req.idx);
			cur_log_entry = 0;
			sendreq();
		}
		break;
	    }
	}
	if (log_state == LOG_IDLE && log_req_state == LOG_REQ_IDLE) {
		struct timeval now, diff;
		gettimeofday(&now, NULL);
		timersub(&now, &last_data, &diff);
		if (diff.tv_sec >= 60) {
			/* request new data */
			log_req.cmd = PRIVATE_LOG_REQUEST;
			log_req.sid++;
			log_req.idx =
			  (log_entries.back().id & ID_IDX_MASK) >> ID_IDX_SHIFT;
			log_req_state = LOG_REQ_DOREQ;
			log_update_state = LOG_UP_SEARCH;
			printf("request new from 0x%04x\n", log_req.idx);
			sendreq();
		}
	}
	log_unlock();
}
