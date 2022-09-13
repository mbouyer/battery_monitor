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
#include <fstream>
#include <err.h>
#include <stdlib.h>
#include <inttypes.h>
#include <N2K/NMEA2000.h>
#include "bmlogstorage.h"
#include "bmlog.h"

bmLogStorage::bmLogStorage(wxString logPath)
{
	time_t previous_t = 0;
	FilePath = logPath;
	cur_log_entry = 0;
	log_req_state = LOG_REQ_IDLE;
	log_tx = (private_log_tx *)nmea2000P->get_frametx(nmea2000P->get_tx_bypgn(PRIVATE_LOG));
	if ((errno = pthread_mutex_init(&log_mtx, NULL)) != 0)
		err(1, "init log_mtx");
	log_state = LOG_INIT;
	log_update_state = LOG_UP_IDLE;
	last_write_entry = 0;

	/* get exising entries from log file */
	std::ifstream _log(FilePath);
	if(!_log.is_open()) {
		warn("failed to open %s", FilePath.c_str());
		return;
	}
	std::string line;
	std::getline(_log, line); /* first line is headers */
	bm_log_entry_t log_entry;
	while(std::getline(_log, line)) {
		char buf[160];
		char *l, *e;
		int s;

		strncpy(buf, line.c_str(), sizeof(buf));
		l = buf;

		e = strsep(&l, ",");
		if (e == NULL) {
			warnx("separator not found: %s", l);
			break;
		}
		log_entry.instance = strtoi(e, NULL, 0, 0, NINST - 1, &s);
		if (s) {
			warnx("instance: %s: conversion failed", e);
			break;
		}

		e = strsep(&l, ",");
		if (e == NULL) {
			warnx("separator not found: %s", l);
			break;
		}
		errno = 0;
		log_entry.id = strtol(e, NULL, 0);
		if (errno) {
			warn("id: %s: conversion failed", e);
			break;
		}

		e = strsep(&l, ",");
		if (e == NULL) {
			warnx("separator not found: %s", l);
			break;
		}
		errno = 0;
		log_entry.volts = strtod(e, NULL);
		if (errno) {
			warn("volts: %s: conversion failed", e);
			break;
		}

		e = strsep(&l, ",");
		if (e == NULL) {
			warnx("separator not found: %s", l);
			break;
		}
		errno = 0;
		log_entry.amps = strtod(e, NULL);
		if (errno) {
			warn("amps: %s: conversion failed", e);
			break;
		}

		e = strsep(&l, ",");
		if (e == NULL) {
			warnx("separator not found: %s", l);
			break;
		}
		errno = 0;
		log_entry.temp = strtol(e, NULL, 0);
		if (errno) {
			warn("temp: %s: conversion failed", e);
			break;
		}

		e = strsep(&l, ",");
		if (e == NULL) {
			warnx("separator not found: %s", l);
			break;
		}
		errno = 0;
		log_entry.time = strtol(e, NULL, 0);
		if (errno) {
			warn("time: %s: conversion failed", e);
			break;
		}

		e = strsep(&l, ",");
		if (e == NULL) {
			warnx("separator not found: %s", l);
			break;
		}
		errno = 0;
		log_entry.flags = strtol(e, NULL, 0);
		if (errno) {
			warn("flags: %s: conversion failed", e);
			break;
		}
#if 0
		printf("idx 0x%06x inst %2d volts %2.2f amps %3.3f temp %3d time %ld flags 0x%02x\n",
		    log_entry.id, log_entry.instance,
		    log_entry.volts, log_entry.amps,
		    log_entry.temp, log_entry.time, log_entry.flags);
#else
		if (previous_t != 0 && log_entry.time != 0 &&
		    previous_t != log_entry.time) {
			printf("idx 0x%06x time %ld delta %ld flags 0x%02x\n",
			    log_entry.id, log_entry.time, log_entry.time - previous_t,
			    log_entry.flags);
		}
		previous_t = log_entry.time;
#endif
		log_entries.push_back(log_entry);
	}
	_log.close();
	last_write_entry = log_entries.size() - 1;
}

void
bmLogStorage::address(int a)
{
	log_lock();
	if (log_req_state == LOG_REQ_IDLE && log_state == LOG_INIT) {
		if (last_write_entry == 0) {
			log_req.cmd = PRIVATE_LOG_REQUEST_FIRST;
			log_req.idx = 0;
			log_update_state = LOG_UP_DOUP;
		} else {
			log_req.cmd = PRIVATE_LOG_REQUEST;
			log_req.idx =
			  (log_entries.back().id & ID_IDX_MASK) >> ID_IDX_SHIFT;
			log_update_state = LOG_UP_SEARCH;
		}
		sid_inc();
		log_req_state = LOG_REQ_DOREQ;
	}
	log_unlock();
}

void
bmLogStorage::addLogEntry(int sid, double volts, double amps,
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
	received_log_entries[cur_log_entry].flags = 0;
	if (volts == 0 && amps == 0 && instance == 0 && temp == TEMP_NULL)
		received_log_entries[cur_log_entry].flags = LOGE_BOUNDARY;
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
				log_update_state = LOG_UP_DOUP_NEWDATA;
				/* FALLTHROUGGH */
			case LOG_UP_DOUP_NEWDATA:
				printf(" store\n");
				log_entries.push_back(received_log_entries[i]);
				break;
			}
		}
		cur_log_entry = 0;
		log_req.cmd = PRIVATE_LOG_REQUEST_NEXT;
		sid_inc();
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
bmLogStorage::logError(int sid, int err)
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
		sid_inc();
		log_req.idx = 0;
		log_req_state = LOG_REQ_DOREQ;
		log_update_state = LOG_UP_DOUP;
		break;
	case PRIVATE_LOG_ERROR_LAST:
		wxASSERT(log_req.cmd == PRIVATE_LOG_REQUEST_NEXT);
		log_req_state = LOG_REQ_IDLE;
		if (log_update_state == LOG_UP_DOUP_NEWDATA &&
		    log_state != LOG_INIT) {
			log_update();
		}
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
bmLogStorage::tick(void)
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
			sid_inc();
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

void
bmLogStorage::log_update(void)
{
	int laste = log_entries.size() - 1;
	int lasteinst = log_entries[laste].instance;
	time_t now = time(NULL);
	const char *home;
	bool trusted = 1;

	/*
	 * update log times for this log block. We know that the last entry
	 * if from current time (with one minute precision) and we have 10mn
	 * between entries so walk the log backware updating time, util
	 * we find a boundary (i.e. BM boot) or non-0 time.
	 * We have one entry per instance, each with the same time so we have
	 * to deal with that
	 */
	for (int i = laste; i >= 0; i--) {
		printf("entry %d fl 0x%x time %ld", i, log_entries[i].flags, log_entries[i].time);
		if (log_entries[i].flags & LOGE_BOUNDARY)
			break;
		if (log_entries[i].time != 0)
			break;
		if (i != laste && lasteinst == log_entries[i].instance) {
			now -= 600; /* one log every 10mn */
			trusted = 0;
		}
		log_entries[i].time = now;
		if (trusted)
			log_entries[i].flags |= LOGE_TRUSTTIME;
		printf(" now 0x%ld\n", log_entries[i].time);
		if (i < last_write_entry)
			last_write_entry = 0; /* need to rewrite whole file */
	}
	printf("\n");
	if (last_write_entry == 0) {
		(void)rename(FilePath, FilePath+"~");
		std::ofstream _logf(FilePath);
		_logf << "instance,id,volts,amps,temp,time,flags" << std::endl;
		for (int i = 0; i <= laste; i++) {
			_logf << log_entries[i].instance << ",";
			_logf << "0x" << std::hex << log_entries[i].id << std::dec <<",";
			_logf << log_entries[i].volts << ",";
			_logf << log_entries[i].amps << ",";
			_logf << log_entries[i].temp << ",";
			_logf << log_entries[i].time << ",";
			_logf << "0x" << std::hex << log_entries[i].flags << std::endl;
		}
		_logf.close();
		last_write_entry = laste;
	} else {
		std::ofstream _logf(FilePath, std::ios::out | std::ios::app);
		for (int i = last_write_entry + 1; i <= laste; i++) {
			_logf << log_entries[i].instance << ",";
			_logf << "0x" << std::hex << log_entries[i].id << std::dec <<",";
			_logf << log_entries[i].volts << ",";
			_logf << log_entries[i].amps << ",";
			_logf << log_entries[i].temp << ",";
			_logf << log_entries[i].time << ",";
			_logf << "0x" << std::hex << log_entries[i].flags << std::endl;
		}
		_logf.close();
		last_write_entry = laste;
	}
}

int
bmLogStorage::getLogBlock(int cookie, std::vector<bm_log_entry_t> &entries)
{
	log_lock();
	if (cookie == -1) {
		/* point to last block */
		cookie = log_entries.size() - 1;
	} else if (cookie < 0 || cookie >= log_entries.size()) {
		log_unlock();
		return -1; /* invalid cookie */
	}
	/* find start of block */
	for (; cookie >= 0; cookie--) {
		if (log_entries[cookie].flags & LOGE_BOUNDARY)
			break;
	}
	cookie++;
	/* copy block to provided vector */
	for (int i = cookie; i < log_entries.size(); i++) {
		if (log_entries[i].flags & LOGE_BOUNDARY)
			break;
		entries.push_back(log_entries[i]);
	}
	log_unlock();
	return cookie;
}

int
bmLogStorage::getNextLogBlock(int cookie, std::vector<bm_log_entry_t> &entries)
{
	bool found = 0;
	log_lock();
	if (cookie < 0 || cookie >= log_entries.size()) {
		log_unlock();
		return -1; /* invalid cookie */
	}
	/* find start of next block */
	for (; cookie < log_entries.size(); cookie++) {
		if (found && (log_entries[cookie].flags & LOGE_BOUNDARY) == 0)
			break;
		else if (log_entries[cookie].flags & LOGE_BOUNDARY) {
			found = 1;
		}
	}
	log_unlock();
	if (cookie == log_entries.size()) {
		return -1; /* no next block */
	}
	return getLogBlock(cookie, entries);
}

int
bmLogStorage::getPrevLogBlock(int cookie, std::vector<bm_log_entry_t> &entries)
{
	bool found = 0;
	log_lock();
	if (cookie < 0 || cookie >= log_entries.size()) {
		log_unlock();
		return -1; /* invalid cookie */
	}
	/* find start of previous block */
	for (; cookie >= 0; cookie--) {
		if (found && (log_entries[cookie].flags & LOGE_BOUNDARY) == 0)
			break;
		else if (log_entries[cookie].flags & LOGE_BOUNDARY) {
			found = 1;
		}
	}
	log_unlock();
	if (cookie < 0) {
		return -1; /* no previous block */
	}
	return getLogBlock(cookie, entries);
}
