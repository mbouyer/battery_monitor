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

#define NINST 4

/* max number of entries sent by bm per request */
#define LOG_ENTRIES 51

struct bm_log_entry {
	double volts;
	double amps;
	int temp;
#define TEMP_INVAL (-1)
	u_int instance;
	u_int idx;
};

class bmLog: public wxFrame
{
  public:
	bmLog(wxWindow* parent);
	void address(int);
	void addLogEntry(int sid, double volts, double amps,
		       int temp, int instance, int idx, bool last);
  private:
	wxFlexGridSizer *mainsizer, *bmsizer;
	private_log_tx *log_tx;
	wxStaticText *Tinst[NINST];
	wxStaticText *Tvolts[NINST];
	wxStaticText *Tamps[NINST];
	wxStaticText *Ttemp[NINST];
	struct bm_log_entry log_entries[LOG_ENTRIES];
	int cur_log_entry;
};
