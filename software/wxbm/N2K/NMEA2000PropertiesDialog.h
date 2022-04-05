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

#ifndef __NMEA2000PropertiesDialog__
#define __NMEA2000PropertiesDialog__

#include <wx/bmpcbox.h>
#include <wx/notebook.h>
#include <wx/arrstr.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

class NMEA2000PropertiesDialog : public wxDialog
{
protected:
    void OnNMEA2000PropertiesOKClick( wxCommandEvent& event );
    void OnNMEA2000PropertiesCancelClick( wxCommandEvent& event );
    void NMEA2000PropertiesGetIfList(void);
    wxBoxSizer* m_SizerDialogBox;
    wxNotebook* m_notebookProperties;
    wxPanel* m_panelBasicProperties;
    wxFlexGridSizer* m_SizerBasicProperties;
    wxPanel* m_panelIfSelectProperties;
    wxFlexGridSizer* m_SizerIfSelectProperties;
    wxListBox* m_ListIfSelectProperties;
    wxStaticText* m_TextIfSelectProperties;
    wxBoxSizer *bSizer;
    wxButton* m_buttonOK;
    wxButton* m_Cancel;

    wxArrayString *ifList;

public:
    NMEA2000PropertiesDialog( wxWindow* parent );
    ~NMEA2000PropertiesDialog( void );

private:
    void NMEA2000PropertiesConfig( void );
};

#endif // __NMEA2000PropertiesDialog__
