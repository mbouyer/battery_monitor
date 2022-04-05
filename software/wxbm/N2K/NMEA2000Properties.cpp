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

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif //precompiled headers

#include "NMEA2000Properties.h"
#include "NMEA2000.h"

NMEA2000Properties *NMEA2000PropertiesP;

NMEA2000Properties::NMEA2000Properties(wxConfig *c)
{
	wxString s;
	int uniquenumber, deviceinstance, manufcode;
	long l;

	config = c;

	if (config->Read("/NMEA2000/Interface", &s))
		nmea2000P->setcanif(s);

	nmea2000P->getconfig(&uniquenumber, &deviceinstance, &manufcode);
	if (config->Read("/NMEA2000/UniqueNumber", &l))
		uniquenumber = l;
	if (config->Read("/NMEA2000/DeviceInstance", &l))
		deviceinstance = l;
	if (config->Read("/NMEA2000/ManufacturerCode", &l))
		manufcode = l;
	nmea2000P->setconfig(uniquenumber, deviceinstance, manufcode);
}

NMEA2000Properties::~NMEA2000Properties()
{
}

void NMEA2000Properties::SaveConfig()
{
	int uniquenumber, deviceinstance, manufcode;
	wxString s;

	s = nmea2000P->getcanif();
	if (s.Len() > 0)
		config->Write("/NMEA2000/Interface", s);

	nmea2000P->getconfig(&uniquenumber, &deviceinstance, &manufcode);
	config->Write("/NMEA2000/UniqueNumber", (long)uniquenumber);
	config->Write("/NMEA2000/DeviceInstance", (long)deviceinstance);
	config->Write("/NMEA2000/ManufacturerCode", (long)manufcode);
	config->Flush();
}
