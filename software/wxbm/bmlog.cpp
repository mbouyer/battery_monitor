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
#include "bmstatus.h"
#include "bmlog.h"
#include "bmlogstorage.h"

bmLog::bmLog(wxWindow* parent, wxConfig *config)
	: wxFrame(parent, wxID_ANY, _T("bmLog"))
{
	wxString logPath;

	if (!config || !config->Read("/Log/path", &logPath)) {
		const char *home = getenv("HOME");
		if (home != NULL) {
			logPath = wxString::Format(wxT("%s/.wxbm_log"), home);
			if (config) {
				config->Write("/Log/path", logPath);
			}
		} else {
			err(1, "can't get log file name ($HOME not set)");
		}
	}
	bmlog_s = new bmLogStorage(logPath);

	Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(bmLog::OnClose));

	mainsizer = new wxBoxSizer( wxVERTICAL );
	title = new wxStaticText(this, -1, _T("truc"));
	mainsizer->Add( title, 0, wxEXPAND | wxALL, 5 );
	SetSizerAndFit(mainsizer);
}


void
bmLog::OnClose(wxCloseEvent & WXUNUSED(event))
{
	Show(false);
}

void
bmLog::address(int a)
{
	bmlog_s->address(a);
}

void
bmLog::addLogEntry(int sid, double volts, double amps,
                 int temp, int instance, int idx, bool last)
{
	bmlog_s->addLogEntry(sid, volts, amps, temp, instance, idx, last);
}

void
bmLog::logError(int sid, int err)
{
	bmlog_s->logError(sid, err);
}

void
bmLog::tick(void)
{
	bmlog_s->tick();
}
