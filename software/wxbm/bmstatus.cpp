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

bmStatus::bmStatus(wxWindow *parent, wxWindowID id)
	: wxPanel(parent, id)
{
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
		bmsizer->Add(new wxStaticText(this, -1,
		    wxString::Format(wxT("%d"), i)), datafl);

		Tvolts[i] = new wxStaticText(this, -1, wxT(""));
		bmsizer->Add(Tvolts[i], datafl);

		Tamps[i] = new wxStaticText(this, -1, wxT(""));
		bmsizer->Add(Tamps[i], datafl);

		Ttemp[i] = new wxStaticText(this, -1, wxT(""));
		bmsizer->Add(Ttemp[i], datafl);
	}

	wxSizerFlags bmfl(0);
	bmfl.Left();
	mainsizer = new wxFlexGridSizer(1, 0, 0);
	mainsizer->Add(bmsizer, bmfl);

	SetSizerAndFit(mainsizer);
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
		Ttemp[i]->SetLabel(wxString::Format(_T("%.1f"), t));
	} else {
		Tvolts[i]->SetLabel(wxString::Format(_T("")));
		Tamps[i]->SetLabel(wxString::Format(_T("")));
		Ttemp[i]->SetLabel(wxString::Format(_T("")));
	}
}
