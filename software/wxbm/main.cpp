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

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/cmdline.h>
#include "wxbm.h"
#include "bmstatus.h"
#include "bmlog.h"
#include "icons/icons8-car-battery-30.xpm"
#include <N2K/NMEA2000.h>
#include <N2K/NMEA2000Properties.h>
#include <N2K/NMEA2000PropertiesDialog.h>

#ifdef DEBUG            
#define DBG(a) {a;}
#else
#define DBG(a) /* */
#endif

const int myID_F_N2KCONF =	wxID_HIGHEST + 10;
const int myID_F_PLOAD =	wxID_HIGHEST + 11;
const int myID_F_PSAVE =	wxID_HIGHEST + 12;
const int myID_F_SHOWLOG =	wxID_HIGHEST + 13;
const int myID_DATAUP =		wxID_HIGHEST + 100;

class bmFrame : public wxFrame
{
public:
	bmFrame(const wxString& title);
	typedef enum dataup {
		data_values = 0,
		data_status,
	} dataup_t;
	void wake(dataup_t);
	int addr;
	int group;
	wxString mode;
	double volts[NINST];
	double amps[NINST];
	double temp[NINST];
	bool instV[NINST];

private:
	wxPanel *mainpanel;
	wxBoxSizer *mainsizer;
	wxMenuBar *menubar;
	wxMenu *file;
	wxMenu *view;
	bmStatus *bmstatus;

	void OnN2KConfig(wxCommandEvent & event);
	void OnDataUpdate(wxCommandEvent & event);
	void OnQuit(wxCommandEvent & event);
	void OnClose(wxCloseEvent & event);
	void OnShowLog(wxCommandEvent & event);
};

bmFrame::bmFrame(const wxString& title)
	: wxFrame(NULL, wxID_ANY, title)
{
	int x, y, w, h;

	wxConfig *config = wxp->getConfig();
	if (config) {
		x = config->ReadLong("/Position/X", -1);
		y = config->ReadLong("/Position/Y", -1);
		w = config->ReadLong("/Position/W", -1);
		h = config->ReadLong("/Position/H", -1);
	} else {
		x = y = w = h = -1;
	}
	for (int i = 0; i < NINST; i++)
		instV[i] = false;
	nmea2000P = new nmea2000;
	NMEA2000PropertiesP = new NMEA2000Properties(config);
	//mainpanel = new wxPanel(this, wxID_ANY);
	mainsizer = new wxBoxSizer( wxVERTICAL );
	menubar = new wxMenuBar;
	file = new wxMenu;
	file->Append(myID_F_N2KCONF, _T("N2k Config"));
	file->Append(wxID_EXIT, _T("&Quit"));
	menubar->Append(file, _T("&File"));
	view = new wxMenu;
	view->Append(myID_F_SHOWLOG, _T("&Log\tCTRL+L"));
	menubar->Append(view, _T("&View"));
	SetMenuBar(menubar);
	Connect(wxEVT_CLOSE_WINDOW,
		wxCloseEventHandler(bmFrame::OnClose));
	Connect(myID_F_N2KCONF, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler(bmFrame::OnN2KConfig));
	Connect(wxID_EXIT, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler(bmFrame::OnQuit));
	Connect(myID_DATAUP, wxEVT_COMMAND_TEXT_UPDATED,
		wxCommandEventHandler(bmFrame::OnDataUpdate));
	Connect(myID_F_SHOWLOG, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler(bmFrame::OnShowLog));

	bmstatus = new bmStatus(this);
	mainsizer->Add( bmstatus, 0, wxEXPAND | wxALL, 5 );

	SetSizerAndFit(mainsizer);
	this->SetSize(x, y, w, h);

	wxp->bmlog = new bmLog(this);
	wxIcon icon(icons8_car_battery_30);
	wxp->bmlog->SetIcon(icon);
	wxp->bmlog->Show(false);
}

void bmFrame::OnN2KConfig(wxCommandEvent & WXUNUSED(event))
{
	NMEA2000PropertiesDialog *N2KPropDialog = new NMEA2000PropertiesDialog(this);
	N2KPropDialog->ShowModal();
	delete N2KPropDialog;

}

void bmFrame::wake(dataup_t t)
{
	wxCommandEvent event(wxEVT_COMMAND_TEXT_UPDATED, myID_DATAUP);
	event.SetInt(t);
	GetEventHandler()->AddPendingEvent( event );
}

void bmFrame::OnDataUpdate(wxCommandEvent & event)
{
	dataup_t t;
	t = (dataup_t)event.GetInt();
	switch(t) {
	case bmFrame::dataup_t::data_status:
		bmstatus->address(addr);
		break;
	case bmFrame::dataup_t::data_values:
		for (int i = 0; i < NINST; i++) {
			bmstatus->values(i, volts[i], amps[i],
			    temp[i], instV[i]);
		}
		break;
	}
}

void bmFrame::OnQuit(wxCommandEvent & WXUNUSED(event))
{
	Close(true);
}

void bmFrame::OnClose(wxCloseEvent & event)
{
	wxConfig *config = wxp->getConfig();
	if (config) {
		int x, y, w, h;
		GetClientSize(&w, &h);
		GetPosition(&x, &y);
		config->Write(_T("/Position/x"), (long) x);
		config->Write(_T("/Position/y"), (long) y);
		config->Write(_T("/Position/w"), (long) w);
		config->Write(_T("/Position/h"), (long) h);
		wxp->bmlog->GetClientSize(&w, &h);
		wxp->bmlog->GetPosition(&x, &y);
		config->Write(_T("/Log/x"), (long) x);
		config->Write(_T("/Log/y"), (long) y);
		config->Write(_T("/Log/w"), (long) w);
		config->Write(_T("/Log/h"), (long) h);
		config->Flush();
		std::cout <<  "saved config file ... " << std::endl;
	}
	delete nmea2000P;
	std::cout <<  "Exiting ... " << std::endl;
	event.Skip();
}

void bmFrame::OnShowLog(wxCommandEvent & WXUNUSED(event))
{

	DBG(std::cout <<  "show log ... " << std::endl);
	wxp->bmlog->Show(true);

}

static const wxCmdLineEntryDesc g_cmdLineDesc [] =
{
	{ wxCMD_LINE_SWITCH, "h", "help",
	    "displays help on the command line parameters",
	    wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
	{ wxCMD_LINE_SWITCH, "L", "nolog",
	    "disables reading log from device"},
	{ wxCMD_LINE_NONE }
};

IMPLEMENT_APP(wxbm)

wxbm *wxp;

bool wxbm::OnInit()
{
	if (!wxApp::OnInit())
		return false;

	config = new wxConfig(AppName());
	wxIcon icon(icons8_car_battery_30);
	int conf_valid = 0;
	wxString path;

	wxp = this;

	for(int i = 0; i < NINST; i++) {
		if (config) {
			path = wxString::Format(wxT("/Instance/%d"), i);
			if (config->Read(path, &Tname[i])) {
				conf_valid++;
			}
		}
	}
	if (conf_valid == 0 && config) {
		for(int i = 0; i < NINST; i++) {
			Tname[i] = wxString::Format(wxT("string:%d"), i);
			path = wxString::Format(wxT("/Instance/%d"), i);
			config->Write(path, Tname[i]);
		}
	}

	frame = new bmFrame(AppName());
	frame->SetIcon(icon);
	frame->Show(true);
	nmea2000P->Init();

	return true;
}

void wxbm::OnInitCmdLine(wxCmdLineParser& parser)
{
	parser.SetDesc (g_cmdLineDesc);
	parser.SetSwitchChars (_T("-"));
}

bool wxbm::OnCmdLineParsed(wxCmdLineParser& parser)
{
	getlog = !parser.Found(_T("L"));
	if (!getlog)
		printf("log disabled\n");
	return true;
}

#define APPNAME "wxbm"

wxString wxbm::AppName()
{
	static wxString s(APPNAME);
	return s;
}
wxString wxbm::ErrMsgPrefix()
{
	static wxString s(APPNAME ": ");
	return s;
}

void wxbm::setStatus(int addr, int group, const wxString &mode)
{
	frame->addr = addr;
	frame->group = group;
	frame->mode = mode;
	frame->wake(bmFrame::dataup_t::data_status);
}

void wxbm::setBatt(int inst, double v, double i, double t, bool valid)
{
	if (valid) {
		if (inst < 0 || inst >= NINST)
			return;
		frame->volts[inst] = v;
		frame->amps[inst] = i;
		frame->temp[inst] = t;
		frame->instV[inst] = true;
	} else {
		for (int i = 0; i < NINST; i++) {
			frame->instV[i] = false;
		}
	}
	frame->wake(bmFrame::dataup_t::data_values);
}

void
wxbm::setBmAddress(int a)
{
	if (getlog)
		bmlog->address(a);
}

void
wxbm::addLogEntry(int sid, double volts, double amps,
                 int temp, int instance, int idx)
{
	if (getlog)
		bmlog->addLogEntry(sid, volts, amps, temp, instance, idx);
}

void
wxbm::logComplete(int sid)
{
	if (getlog)
		bmlog->logComplete(sid);
}

void
wxbm::logError(int sid, int err)
{
	if (getlog)
		bmlog->logError(sid, err);
}

void
wxbm::logTick(void)
{
	if (getlog)
		bmlog->tick();
}


wxWindow *
wxbm::getTlabel(int i, wxWindow * parent)
{
	wxString type = Tname[i].BeforeFirst(':');
	wxString name = Tname[i].AfterFirst(':');

	DBG(std::cout << "getTlabel " << i << " " << Tname[i] << " " << type << " " << name << std::endl);
	if (type.IsSameAs(_T("string"))) {
		return new wxStaticText(parent, -1, name);
	} else if (type.IsSameAs(_T("img"))) {
		wxBitmap bitmap(name, wxBITMAP_TYPE_ANY);
		return new wxStaticBitmap(parent, -1, bitmap);
	} else  {
		return NULL;
	}
}
