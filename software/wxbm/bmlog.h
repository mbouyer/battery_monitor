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
#include <wx/config.h>
#include "bmmathplot.h"

class bmLogStorage;
struct bm_log_entry;

class bmLog: public wxFrame
{
  public:
	bmLog(wxWindow* parent);
	void address(int);
	void addLogEntry(int sid, double volts, double amps,
		       int temp, int instance, int idx, bool last);
	void logError(int sid, int err);
	void tick(void);
  private:
	wxPanel *mainpanel;
	wxTextCtrl *timescale;
	wxStaticText *timerange;
	wxWindow *InstLabel[NINST];
	wxStaticText *InstAh[NINST];
	wxStaticText *InstA[NINST];
	wxStaticText *InstV[NINST];
	wxStaticText *InstT[NINST];
	bmFXYVector *Alayer[NINST];
	bmFXYVector *Vlayer[NINST];
	bmFXYVector *Tlayer[NINST];
	mpWindow *plotA;
	mpWindow *plotV;
	mpWindow *plotT;
	double mp_scaleX;
	double mp_posX;
	time_t mp_startX, mp_endX;
	bmLogStorage *bmlog_s;
	int log_cookie;
	std::vector<struct bm_log_entry> log_entries;
	void OnClose(wxCloseEvent & event);
	void OnShow(wxShowEvent & event);
	void OnScale(wxCommandEvent & event);
	void OnScaleChange(wxCommandEvent & event);
	void OnPrevious(wxCommandEvent & event);
	void OnNext(wxCommandEvent & event);
	void OnFit(wxCommandEvent & event);
	void updateStats(void);
	void logV2XY(std::vector<double> &, std::vector<double> &,
	             std::vector<double> &, std::vector<double> &, int);
	void showGraphs(void);
	mpWindow *MakePlot(wxString, wxWindowID);
};
