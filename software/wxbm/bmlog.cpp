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
#include "wxbm.h"
#include "bmlog.h"
#include "bmlogstorage.h"
#include "bmmathplot.h"

static const int plotID_A = wxID_HIGHEST + 1;
static const int plotID_V = wxID_HIGHEST + 2;
static const int plotID_T = wxID_HIGHEST + 3;
static const int scaleID = wxID_HIGHEST + 7;
static const int nextID = wxID_HIGHEST + 8;
static const int prevID = wxID_HIGHEST + 9;

const wxColour *instcolor[NINST] = {wxRED, wxGREEN, wxBLUE, wxBLACK};

static wxString
date2string(time_t date)
{
	struct tm tm;
	wxString fmt = (wxT("%04d-%02d-%02d %02d:%02d"));

	localtime_r(&date, &tm);
	tm.tm_mon++;
	tm.tm_year+= 1900;

	return wxString::Format(fmt, tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min);
}

static wxString
time2string(time_t duration)
{
	wxString fmt;
	int days, hours, minutes;
	days = duration / 86400;
	duration -= days * 86400;
	hours = duration / 3600;
	duration -= hours * 3600;
	minutes = duration / 60;

	if (days > 0) {
		return wxString::Format(_T("%dj%02dh%02dm"), days, hours, minutes);
	} else {
		return wxString::Format(_T("%dh%02dm"), hours, minutes);
	}
}

bmLog::bmLog(wxWindow* parent)
	: wxFrame(parent, wxID_ANY, _T("bmLog"))
{
	wxConfig *config = wxp->getConfig();
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

	wxBoxSizer *mainsizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *topsizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton *prev = new wxButton(this, prevID, _T("<-"));
	prev->Connect(wxEVT_BUTTON,
	    wxCommandEventHandler(bmLog::OnPrevious), NULL, this);
	wxButton *next = new wxButton(this, prevID, _T("->"));
	next->Connect(wxEVT_BUTTON,
	    wxCommandEventHandler(bmLog::OnNext), NULL, this);
	timescale = new wxTextCtrl(this, scaleID, _T("--d--h--m"),
	    wxDefaultPosition, wxDefaultSize,
	    wxTE_PROCESS_ENTER | wxTE_CENTRE);
	timescale->Connect(wxEVT_TEXT_ENTER,
	    wxCommandEventHandler(bmLog::OnScaleChange), NULL, this);
	timerange = new wxStaticText(this, -1,
	    _T("-------- --:-- -------- --:--"),
	    wxDefaultPosition, wxDefaultSize,
	    wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE);
	topsizer->Add(prev, 0, wxEXPAND | wxALL, 5 );
	topsizer->Add(timescale, 1, wxEXPAND | wxALL, 5 );
	topsizer->Add(timerange, 1, wxEXPAND | wxALL, 5 );
	topsizer->Add(next, 0, wxEXPAND | wxALL, 5 );
	mainsizer->Add(topsizer, 0, wxEXPAND | wxALL, 1 );

	plotA = MakePlot(wxT("%.2fA"), plotID_A);
	plotV = MakePlot(wxT("%.2fV"), plotID_V);
	wchar_t degChar = 0x00B0;
	wxString degFmt = wxT("%.1f");
	plotT = MakePlot(degFmt + degChar, plotID_T);

	for (int i = 0; i < NINST; i++) {
		InstLabel[i] = wxp->getTlabel(i, this);
		if (InstLabel[i] == NULL)
			continue;
		wxPen vectorpen(*instcolor[i], 2, wxSOLID);

		Alayer[i] = new bmFXYVector(_("Amps"));
		Alayer[i]->Clear();
		Alayer[i]->SetContinuity(true);
		Alayer[i]->SetPen(vectorpen);
		Alayer[i]->SetDrawOutsideMargins(false);
		plotA->AddLayer(Alayer[i]);

		Vlayer[i] = new bmFXYVector(_("Volts"));
		Vlayer[i]->Clear();
		Vlayer[i]->SetContinuity(true);
		Vlayer[i]->SetPen(vectorpen);
		Vlayer[i]->SetDrawOutsideMargins(false);
		plotV->AddLayer(Vlayer[i]);

		Tlayer[i] = new bmFXYVector(_("Temp"));
		Tlayer[i]->Clear();
		Tlayer[i]->SetContinuity(true);
		Tlayer[i]->SetPen(vectorpen);
		Tlayer[i]->SetDrawOutsideMargins(false);
		plotT->AddLayer(Tlayer[i]);
	}
	wxFlexGridSizer *graphsizer = new wxFlexGridSizer(2, 3, 5);
	wxFlexGridSizer *lsizerA = new wxFlexGridSizer(3, NINST, 5);
	wxFlexGridSizer *lsizerV = new wxFlexGridSizer(2, NINST, 5);
	wxFlexGridSizer *lsizerT = new wxFlexGridSizer(2, NINST, 5);
	wxSizerFlags datafl(0);
	datafl.Expand().Right();
	wxSizerFlags labelfl(0);
	labelfl.Centre();
	for (int i = 0; i < NINST; i++) {
		if (InstLabel[i] == NULL)
			continue;
		lsizerA->Add(InstLabel[i], labelfl);
		InstAh[i] = new wxStaticText(this, -1, wxT("XXXX.XXAh"),
		    wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
		InstAh[i]->SetForegroundColour(*instcolor[i]);
		lsizerA->Add(InstAh[i], datafl);
		InstA[i] = new wxStaticText(this, -1, wxT("XX.XXA"),
		    wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
		InstA[i]->SetForegroundColour(*instcolor[i]);
		lsizerA->Add(InstA[i], datafl);
		/* each time we add a wxWindow we need to allocate a new one */
		InstLabel[i] = wxp->getTlabel(i, this);
		lsizerV->Add(InstLabel[i], labelfl);
		InstV[i] = new wxStaticText(this, -1, wxT("XX.XXV XX.XXV"));
		InstV[i]->SetForegroundColour(*instcolor[i]);
		lsizerV->Add(InstV[i], datafl);
		InstLabel[i] = wxp->getTlabel(i, this);
		lsizerT->Add(InstLabel[i], labelfl);
		InstT[i] = new wxStaticText(this, -1, wxT("XX.XXC XX.XXC"));
		InstT[i]->SetForegroundColour(*instcolor[i]);
		lsizerT->Add(InstT[i], datafl);
	}
	graphsizer->Add(lsizerA, 0,  wxALIGN_CENTER_VERTICAL | wxALL, 5);
	graphsizer->Add(plotA, 1, wxEXPAND | wxALL, 5);
	graphsizer->Add(lsizerV, 0,  wxALIGN_CENTER_VERTICAL | wxALL, 5);
	graphsizer->Add(plotV, 1, wxEXPAND | wxALL, 5);
	graphsizer->Add(lsizerT, 0,  wxALIGN_CENTER_VERTICAL | wxALL, 5);
	graphsizer->Add(plotT, 1, wxEXPAND | wxALL, 5);

	graphsizer->AddGrowableCol(1, 1);
	graphsizer->AddGrowableRow(0, 1);
	graphsizer->AddGrowableRow(1, 1);
	graphsizer->AddGrowableRow(2, 1);

	mainsizer->Add( graphsizer, 1, wxEXPAND | wxALL, 5 );
	SetAutoLayout(true);
	SetSizer(mainsizer);
	this->SetSize(x, y, w, h);
}

void
bmLog::logV2XY(std::vector<double> &D, std::vector<double> &V,
    std::vector<double> &A, std::vector<double> &T, int instance)
{
	std::cout << "logV2XY size " <<  log_entries.size() << std::endl;
	for (int i = 0; i < log_entries.size(); i++) {
		if (log_entries[i].instance != instance)
			continue;
		/*
		 * of time is known, use it
		 * otherwise, just compute time from start of block
		 */
		if (log_entries[i].time != 0) {
			D.push_back(log_entries[i].time);
		} else {
			D.push_back(i * 600);
		}
		V.push_back(log_entries[i].volts);
		A.push_back(-log_entries[i].amps);
		if (log_entries[i].temp != TEMP_INVAL)
			T.push_back(log_entries[i].temp - 273);
	}
}

void
bmLog::showGraphs(void)
{
	for (int i = 0; i < NINST; i++) {
		if (InstLabel[i] == NULL)
			continue;
		std::vector<double> D;
		std::vector<double> V;
		std::vector<double> A;
		std::vector<double> T;
		logV2XY(D, V, A, T, i);
		if (D.size() == 0)
			continue;
		std::cout << "log entries " << D.size() << " " << A.size() << " " << V.size() << " " << T.size() << " " << i << std::endl;
		Alayer[i]->Clear();
		Vlayer[i]->Clear();
		Tlayer[i]->Clear();
		Alayer[i]->SetData(D, A);
		Vlayer[i]->SetData(D, V);
		if (T.size() == D.size()) {
			Tlayer[i]->SetData(D, T);
			Tlayer[i]->SetVisible(true);
		} else {
			/* set up a fake, invisible data layer at 20C*/
			T.clear();
			for (int e = 0; e < D.size(); e++)
				T.push_back(20);
			Tlayer[i]->SetData(D, T);
			Tlayer[i]->SetVisible(false);
		}
	}
	plotA->Fit();
	plotV->Fit();
	plotT->Fit();
	mp_scaleX = plotA->GetScaleX();
	mp_posX = plotA->GetXpos();
	updateStats();
}

void
bmLog::OnClose(wxCloseEvent & WXUNUSED(event))
{
	Show(false);
}

void
bmLog::OnShow(wxShowEvent &event)
{
	log_cookie = bmlog_s->getLogBlock(-1, log_entries);
	std::cout << "log_cookie " << log_cookie << std::endl;
	if (log_cookie >= 0) {
		showGraphs();
	}
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
	updateStats();
}

void
bmLog::OnScaleChange(wxCommandEvent &event)
{
	wxString value = timescale->GetValue();
	std::cout << "scale text " << value << std::endl;
	char buf[80];
	char *l, *e;
	strncpy(buf, value.c_str(), sizeof(buf));
	time_t duration = 0;
	l = buf;
	e = strsep(&l, "j");
	if (l != NULL) {
		printf(" j %s\n", e);
		errno = 0;
		duration += strtol(e, NULL, 0) * 86400;
		if (errno)
			goto error;
	} else
		l = buf;
	e = strsep(&l, "h");
	if (l != NULL) {
		printf(" h %s\n", e);
		errno = 0;
		duration += strtol(e, NULL, 0) * 3600;
		if (errno)
			goto error;
	} else
		l = buf;
	e = strsep(&l, "m");
	if (l != NULL) {
		printf(" m %s\n", e);
		errno = 0;
		duration += strtol(e, NULL, 0) * 60;
		if (errno)
			goto error;
	}
	if (duration == 0) {
error:
		timescale->ChangeValue(time2string(mp_endX - mp_startX));
		return;
	}
	std::cout << "scale seconds " << duration << std::endl;
	double startY, endY, centerX;
	startY = plotA->GetDesiredYmin();
	endY = plotA->GetDesiredYmax();
	centerX = (mp_startX + mp_endX) / 2.0;
	plotA->Fit(centerX - duration / 2, centerX + duration / 2,
	    startY, endY, NULL);
	/* setting plotA will trigger a OnScale event */
}

void
bmLog::OnPrevious(wxCommandEvent &event)
{
	time_t duration = mp_endX - mp_startX;
	time_t log_start = log_entries[0].time;
	double startY, endY, centerX;
	startY = plotA->GetDesiredYmin();
	endY = plotA->GetDesiredYmax();
	centerX = (mp_startX + mp_endX) / 2.0;
	centerX -= duration;
	if (centerX <= log_start) {
		centerX = (mp_startX + mp_endX) / 2.0;
		/* previous entry */
		int new_coockie = bmlog_s->getPrevLogBlock(log_cookie, log_entries);
		std::cout << "previous " << new_coockie << std::endl;
		if (new_coockie < 0) {
			/* no previous block */
			centerX = (mp_startX + mp_endX) / 2.0;
		} else {
			log_cookie = new_coockie;
			showGraphs();
			return;
		}
	} else if (centerX < log_start + duration / 2.0) {
		centerX = log_start + duration / 2.0;
	}
	plotA->Fit(centerX - duration / 2, centerX + duration / 2,
	    startY, endY, NULL);
	/* setting plotA will trigger a OnScale event */
}

void
bmLog::OnNext(wxCommandEvent &event)
{
	time_t duration = mp_endX - mp_startX;
	time_t log_end = log_entries[log_entries.size() - 1].time;
	double startY, endY, centerX;
	startY = plotA->GetDesiredYmin();
	endY = plotA->GetDesiredYmax();
	centerX = (mp_startX + mp_endX) / 2.0;
	centerX += duration;
	if (centerX >= log_end) {
		/* next entry */
		int new_coockie = bmlog_s->getNextLogBlock(log_cookie, log_entries);
		std::cout << "next " << new_coockie << std::endl;
		if (new_coockie < 0) {
			/* no next block */
			centerX = (mp_startX + mp_endX) / 2.0;
		} else {
			log_cookie = new_coockie;
			showGraphs();
			return;
		}
	} else if (centerX > log_end - duration / 2.0) {
		centerX = log_end - duration / 2.0;
	}
	plotA->Fit(centerX - duration / 2, centerX + duration / 2,
	    startY, endY, NULL);
	/* setting plotA will trigger a OnScale event */
}

void
bmLog::updateStats(void)
{
	mp_startX = plotA->GetDesiredXmin();
	mp_endX = plotA->GetDesiredXmax();

	std::cout << " start " << mp_startX << " end " << mp_endX << std::endl;
	timerange->SetLabel(date2string(mp_startX) + _T(" ") + date2string(mp_endX));
	timescale->ChangeValue(time2string(mp_endX - mp_startX));

	for (int i = 0; i < NINST; i++) {
		if (InstLabel[i] == NULL)
			continue;

		const std::vector<double> *D;
		const std::vector<double> *A;
		const std::vector<double> *V;
		const std::vector<double> *T;
		double duration = 0;
		int nentries = 0;
		time_t prev_time = -1;
		time_t time;
		double Ah = 0;
		double Aav;
		double Vmin = 10000;
		double Vmax = -10000;
		double Tmin = 10000;
		double Tmax = -100000;

		Alayer[i]->GetData(D, A);
		Vlayer[i]->GetData(D, V);
		if (Tlayer[i]->IsVisible()) {
			Tlayer[i]->GetData(D, T);
		} else {
			T = NULL;
		}

		for (int e = 0; e < D->size(); e++) {
			time_t time;
			time = (*D)[e];
			if (time < mp_startX || time > mp_endX)
				continue;
			if (prev_time >= 0) {
				duration += time - prev_time;
			}
			prev_time = time;
			nentries++;

			Ah += (*A)[e];
			if (Vmin > (*V)[e])
				Vmin = (*V)[e];
			if (Vmax < (*V)[e])
				Vmax = (*V)[e];
			if (T != NULL) {
				if (Tmin > (*T)[e])
					Tmin = (*T)[e];
				if (Tmax < (*T)[e])
					Tmax = (*T)[e];
			}
		}
		std::cout << "duration " << duration << std::endl;
		Aav =  Ah / nentries;
		Ah = Ah / 3600.0 * 600.0;
		wxString Aformat;
		if (Ah >= 100 || Ah <= -100)
			Aformat = _T("%.1fAh");
		else
			Aformat = _T("%.2fAh");
		InstAh[i]->SetLabel(wxString::Format(Aformat, Ah));
		if (Aav >= 10 || Aav <= -10)
			Aformat = _T("%.1fA");
		else
			Aformat = _T("%.2fA");
		InstA[i]->SetLabel(wxString::Format(Aformat, Aav));
		InstV[i]->SetLabel(wxString::Format(_T("%.2fV %.2fV"), Vmin, Vmax));
		if (T != NULL) {
			wchar_t degChar = 0x00B0;
			wxString degFmt = wxT("%.1f");
			InstT[i]->SetLabel(wxString::Format(
			    degFmt + degChar + _T(" ") + degFmt + degChar,
			    Tmin, Tmax));
		} else {
			InstT[i]->SetLabel(_T(""));
		}
	}
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
