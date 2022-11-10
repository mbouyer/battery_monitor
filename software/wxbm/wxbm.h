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

#ifndef _WXbm_H_
#define _WXbm_H_

#include <wx/config.h>

class bmFrame;
class bmLog;

#define NINST 4

class wxbm : public wxApp
{
  public:
	virtual bool OnInit();
	virtual void OnInitCmdLine(wxCmdLineParser& parser);
	virtual bool OnCmdLineParsed(wxCmdLineParser& parser);
	static wxString AppName();
	static wxString ErrMsgPrefix();
	void setBatt(int instance, double v, double i, double t, bool);
	void setStatus(int, int, const wxString &);
	void setBmAddress(int);
	void addLogEntry(int sid, double volts, double amps,
	    int temp, int instance, int idx);
	void logComplete(int sid);
	void logError(int sid, int err);
	void logTick(void);
	wxWindow *getTlabel(int, wxWindow *);
	inline wxConfig *getConfig(void) { return config; };
	inline int getBmAddress(void) { return bmAddress; };

	bmLog *bmlog;
  private:
	wxConfig *config;
	bmFrame *frame;
	bool getlog;
	wxString Tname[NINST];
	int bmAddress;
};

extern wxbm *wxp;

#endif /* _WXbm_H_ */
