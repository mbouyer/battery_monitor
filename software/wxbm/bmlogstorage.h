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

#include <wx/combobox.h>
#include <N2K/nmea2000_defs_tx.h>
#include <pthread.h>
#include <err.h>
#include <vector>

#define NINST 4

/* max number of entries sent by bm per request */
#define LOG_ENTRIES 51

typedef struct bm_log_entry {
	double volts;
	double amps;
	int temp;
#define TEMP_INVAL (-1)
#define TEMP_NULL (233)
	u_int instance;
	u_int id; /* idx from packet | index in packet */
#define ID_IDX_MASK	0xffff
#define ID_IDX_SHIFT	0
#define ID_INDEX_MASK	0xffff0000
#define ID_INDEX_SHIFT	16
	time_t time;
	int flags;
#define LOGE_BOUNDARY	0x01
} bm_log_entry_t;

class bmLogStorage {
  public:
	bmLogStorage(wxString logPath);
	void address(int);
	void addLogEntry(int sid, double volts, double amps,
		       int temp, int instance, int idx, bool last);
	void logError(int sid, int err);
	void tick(void);
	int getLogBlock(int cookie, std::vector<bm_log_entry_t> &entries);
	int getNextLogBlock(int cookie, std::vector<bm_log_entry_t> &entries);
	int getPrevLogBlock(int cookie, std::vector<bm_log_entry_t> &entries);
  private:
	wxString FilePath;
	private_log_tx *log_tx;
	bm_log_entry_t received_log_entries[LOG_ENTRIES];
	int cur_log_entry;
	struct timeval last_ev;
	struct timeval last_data;
	std::vector<bm_log_entry_t> log_entries;
	int last_write_entry;
	pthread_mutex_t log_mtx;
	struct log_req {
		int cmd;
		int sid;
		int idx;
	} log_req;
	enum {
		LOG_REQ_IDLE,
		LOG_REQ_DOREQ,
		LOG_REQ_WAIT_BLOCK,
	} log_req_state;
	enum {
		LOG_UP_IDLE,
		LOG_UP_SEARCH,
		LOG_UP_DOUP,
		LOG_UP_DOUP_NEWDATA,
	} log_update_state;
	enum {
		LOG_INIT,
		LOG_IDLE,
	} log_state;

	inline void sid_inc(void) {
		log_req.sid++;
		if (log_req.sid == 0 || log_req.sid > 0xfd)
			log_req.sid = 1;
	}
	inline void sendreq(void) {
		log_tx->sendreq(log_req.cmd, log_req.sid, log_req.idx);
		gettimeofday(&last_ev, NULL);
		log_req_state = LOG_REQ_WAIT_BLOCK;
	};
	inline void log_lock(void) {
		if ((errno = pthread_mutex_lock(&log_mtx)) != 0)
			err(1, "lock log_mtx");
	};
	inline void log_unlock(void) {
		if ((errno = pthread_mutex_unlock(&log_mtx)) != 0)
			err(1, "lock log_mtx");
	};
	void log_update(void);
};
