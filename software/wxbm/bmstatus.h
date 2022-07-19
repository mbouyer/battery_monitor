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
#include <wx/windowid.h>

#define NINST 4

class bmStatus: public wxPanel
{
  public:
	bmStatus(wxWindow *parent, wxConfig *config, wxWindowID id=wxID_ANY);
	void address(int);
	void values(int, double, double, double, bool);
  private:
	wxFlexGridSizer *mainsizer, *bmsizer;
	wxString Tname[NINST];
	wxStaticText *Tvolts[NINST];
	wxStaticText *Tamps[NINST];
	wxStaticText *Ttemp[NINST];
	wxButton *label[NINST];
	void showStat(wxCommandEvent& event);
	static inline wxWindowID labelidx2id(int idx)
	{
		return (wxID_HIGHEST + 1) + idx;
	}
	static inline int labelid2idx(wxWindowID id)
	{
		return id - (wxID_HIGHEST + 1);
	}
};
