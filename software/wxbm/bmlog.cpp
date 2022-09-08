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
#include "bmmathplot.h"

static const int plotID_A = wxID_HIGHEST + 1;
static const int plotID_V = wxID_HIGHEST + 2;
static const int plotID_T = wxID_HIGHEST + 3;

const wxColour *instcolor[NINST] = {wxRED, wxGREEN, wxBLUE, wxBLACK};

bmLog::bmLog(wxWindow* parent, wxConfig *config)
	: wxFrame(parent, wxID_ANY, _T("bmLog"))
{
	wxString logPath;
	int x, y, w, h;

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

	if (config) {
		x = config->ReadLong("/Log/x", -1);
		y = config->ReadLong("/Log/Y", -1);
		w = config->ReadLong("/Log/w", -1);
		h = config->ReadLong("/Log/h", -1);
	} else {
		x = y = w = h = -1;
	}

	Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(bmLog::OnClose));
	Connect(wxEVT_SHOW, wxShowEventHandler(bmLog::OnShow));
	Connect(SCALEX_EVENT, wxCommandEventHandler(bmLog::OnScale));

	plotA = MakePlot(wxT("%.2fA"), plotID_A);
	plotV = MakePlot(wxT("%.2fV"), plotID_V);
	wchar_t degChar = 0x00B0;
	wxString degFmt = wxT("%.1f");
	plotT = MakePlot(degFmt + degChar, plotID_T);
	std::vector<bm_log_entry_t> entries;
	log_cookie = bmlog_s->getLogBlock(-1, entries);
	std::cout << "log_cookie " << log_cookie << std::endl;
	if (log_cookie >= 0) {
		for (int i = 0; i < NINST; i++) {
			std::vector<double> D;
			std::vector<double> V;
			std::vector<double> A;
			std::vector<double> T;
			logV2XY(entries, D, V, A, T, i);
			if (D.size() == 0)
				continue;
			std::cout << "log entries " << D.size() << " " << A.size() << " " << V.size() << " " << T.size() << " " << i << std::endl;
			wxPen vectorpen(*instcolor[i], 2, wxSOLID);

			mpFXYVector* Alayer = new mpFXYVector(_("Amps"));
			Alayer->SetData(D, A);
			Alayer->SetContinuity(true);
			Alayer->SetPen(vectorpen);
			Alayer->SetDrawOutsideMargins(false);
			plotA->AddLayer(Alayer);

			mpFXYVector* Vlayer = new mpFXYVector(_("Volts"));
			Vlayer->SetData(D, V);
			Vlayer->SetContinuity(true);
			Vlayer->SetPen(vectorpen);
			Vlayer->SetDrawOutsideMargins(false);
			plotV->AddLayer(Vlayer);

			if (T.size() == D.size()) {
				mpFXYVector* Tlayer = new mpFXYVector(_("Temp"));
				Tlayer->SetData(D, T);
				Tlayer->SetContinuity(true);
				Tlayer->SetPen(vectorpen);
				Tlayer->SetDrawOutsideMargins(false);
				plotT->AddLayer(Tlayer);
			}
		}
	}
	mainsizer = new wxBoxSizer( wxVERTICAL );
	mainsizer->Add( plotA, 1, wxEXPAND | wxALL, 5 );
	mainsizer->Add( plotV, 1, wxEXPAND | wxALL, 5 );
	mainsizer->Add( plotT, 1, wxEXPAND | wxALL, 5 );
	SetAutoLayout(true);
	SetSizer(mainsizer);
	this->SetSize(x, y, w, h);
}

void
bmLog::logV2XY(std::vector<bm_log_entry_t> &e, std::vector<double> &D, 
    std::vector<double> &V, std::vector<double> &A,
    std::vector<double> &T, int instance)
{
	std::cout << "logV2XY size " <<  e.size() << std::endl;
	for (int i = 0; i < e.size(); i++) {
		if (e[i].instance != instance)
			continue;
		/*
		 * of time is known, use it
		 * otherwise, just compute time from start of block
		 */
		if (e[i].time != 0) {
			D.push_back(e[i].time);
		} else {
			D.push_back(i * 600);
		}
		V.push_back(e[i].volts);
		A.push_back(-e[i].amps);
		if (e[i].temp != TEMP_INVAL) 
			T.push_back(e[i].temp - 273);
	}
}

void
bmLog::OnClose(wxCloseEvent & WXUNUSED(event))
{
	Show(false);
}

void
bmLog::OnShow(wxShowEvent &event)
{
	plotA->Fit();
	plotV->Fit();
	plotT->Fit();
	event.Skip();
}

void
bmLog::OnScale(wxCommandEvent &event)
{
	double n_scaleX, n_posX;
	switch(event.GetId()) {
	case plotID_A:
		n_scaleX = plotA->GetScaleX();
		n_posX = plotA->GetXpos();
		break;
	case plotID_V:
		n_scaleX = plotV->GetScaleX();
		n_posX = plotV->GetXpos();
		break;
	case plotID_T:
		n_scaleX = plotT->GetScaleX();
		n_posX = plotT->GetXpos();
		break;
	default:
		return;
	}
	if (n_scaleX == mp_scaleX && n_posX == mp_posX)
		return;
	mp_scaleX = n_scaleX;
	mp_posX = n_posX;

	plotA->SetPosX(n_posX);
	plotA->SetScaleX(n_scaleX);
	plotV->SetPosX(n_posX);
	plotV->SetScaleX(n_scaleX);
	plotT->SetPosX(n_posX);
	plotT->SetScaleX(n_scaleX);
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

mpWindow *
bmLog::MakePlot(wxString yFormat, wxWindowID id)
{
	mpWindow *plot;
	plot = new mpWindow( this, id, wxPoint(0,0), wxSize(500,500), wxSUNKEN_BORDER );
	bmScaleX* xaxis = new bmScaleX(wxT(""), mpALIGN_BOTTOM);
	mpScaleY* yaxis = new mpScaleY(wxT(""), mpALIGN_LEFT, true);
	wxFont graphFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	xaxis->SetFont(graphFont);
	yaxis->SetFont(graphFont);
	xaxis->SetDrawOutsideMargins(false);
	yaxis->SetDrawOutsideMargins(false);
	xaxis->SetLabelMode(mpX_DATETIME, mpX_LOCALTIME);
	yaxis->SetLabelFormat(yFormat);
	plot->SetMargins(20, 20, 20, 70);
	plot->AddLayer(xaxis);
	plot->AddLayer(yaxis);
	bmInfoCoords *nfo;
	nfo = new bmInfoCoords(wxRect(80,20,10,10), wxTRANSPARENT_BRUSH, yFormat); 
	nfo->SetLabelMode(mpX_DATETIME, mpX_LOCALTIME);
	plot->AddLayer(nfo);
	nfo->SetVisible(true);
	return plot;
}
