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
#include "bmstatus.h"

bmStatus::bmStatus(wxWindow *parent, wxConfig *config, wxWindowID id)
	: wxPanel(parent, id)
{

	int conf_valid = 0;
	wxString path;
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
	
	bmsizer = new wxFlexGridSizer(4, 5, 5);

	wxSizerFlags datafl(0);
	datafl.Expand().Right();
	wxSizerFlags labelfl(0);
	labelfl.Centre();

#define LABELTEXT(t) \
    new wxStaticText(this, -1, wxT(t))

	bmsizer->Add(LABELTEXT("instance"), labelfl);
	bmsizer->Add(LABELTEXT("voltage(V)"), labelfl);
	bmsizer->Add(LABELTEXT("courant(A)"), labelfl);
	bmsizer->Add(LABELTEXT("temperature(C)"), labelfl);
	for(int i = 0; i < NINST; i++) {
		Tvolts[i] = new wxStaticText(this, -1, wxT(""));
		Tamps[i] = new wxStaticText(this, -1, wxT(""));
		Ttemp[i] = new wxStaticText(this, -1, wxT(""));
		label[i] = new wxButton(this, labelidx2id(i));
		wxString type = Tname[i].BeforeFirst(':');
		wxString name = Tname[i].AfterFirst(':');
		if (type.IsSameAs(_T("string"))) {
			label[i]->SetLabel(name);
		} else if (type.IsSameAs(_T("img"))) {
			wxBitmap bitmap(name, wxBITMAP_TYPE_ANY);
			label[i]->SetBitmap(bitmap);
		} else  {
			delete label[i];
			label[i] = NULL;
			continue;
		}
		this->Bind(wxEVT_BUTTON, &bmStatus::showStat, this, labelidx2id(i));
		bmsizer->Add(label[i], datafl);
		bmsizer->Add(Tvolts[i], datafl);
		bmsizer->Add(Tamps[i], datafl);
		bmsizer->Add(Ttemp[i], datafl);
	}

	wxSizerFlags bmfl(0);
	bmfl.Left();
	mainsizer = new wxFlexGridSizer(1, 0, 0);
	mainsizer->Add(bmsizer, bmfl);

	SetSizerAndFit(mainsizer);
}

void bmStatus::showStat(wxCommandEvent& event)
{
	printf("showStat %d\n", labelid2idx(event.GetId()));
}

void
bmStatus::address(int a)
{
#if 0
	if (a == -1)
		Taddr->SetLabel(_T(""));
	else
		Taddr->SetLabel(wxString::Format(_T("%d"), a));
#endif
}

void
bmStatus::values(int i, double v, double a, double t, bool valid)
{
	if (valid) {
		Tvolts[i]->SetLabel(wxString::Format(_T("%.2f"), v));
		Tamps[i]->SetLabel(wxString::Format(_T("%.2f"), a));
		if (t > -100) 
			Ttemp[i]->SetLabel(wxString::Format(_T("%.1f"), t));
		else
			Ttemp[i]->SetLabel(wxString::Format(_T("")));
	} else {
		Tvolts[i]->SetLabel(wxString::Format(_T("")));
		Tamps[i]->SetLabel(wxString::Format(_T("")));
		Ttemp[i]->SetLabel(wxString::Format(_T("")));
	}
}
