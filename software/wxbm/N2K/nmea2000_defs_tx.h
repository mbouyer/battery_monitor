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

#ifndef NMEA2000_FRAME_TX_H_
#define NMEA2000_FRAME_TX_H_
#include "nmea2000_frame.h"
#include "nmea2000_defs.h"
#include <wxbm.h>
#include <array>
#include <time.h>

class nmea2000_frame_tx : public nmea2000_frame, public nmea2000_desc {
    public:
	bool  valid;

	inline nmea2000_frame_tx() :
	    nmea2000_frame(),
	    nmea2000_desc(NULL, false, -1)
	    {valid = 0; }

	inline nmea2000_frame_tx(const char *desc, bool isuser, u_int pgn, u_int pri, u_int len) : nmea2000_frame(), nmea2000_desc(desc, isuser, pgn)
	    {
		valid = 0;
		wxASSERT((len & 0xff) <= 8);
		frame->can_id = ((pri & 0x7) << 26) |
		    (pgn << 8);
		frame->can_id |= CAN_EFF_FLAG;
		frame->can_dlc = len;
	    }

	 virtual ~nmea2000_frame_tx() {};

	inline void setsrc(int src)
	    {
		frame->can_id = (frame->can_id & ~0xff) | (src & 0xff);
	    }
	inline void setdst(int dst) {
		wxASSERT(is_pdu1());
		frame->can_id =
		   (frame->can_id & ~0xff00) | ((dst & 0xff) << 8);
	}

	virtual bool send(int);
};

class nmea2000_fastframe_tx : public nmea2000_frame_tx {
public:
	inline nmea2000_fastframe_tx() : nmea2000_frame_tx(), fastlen(223) { init(); }
	inline nmea2000_fastframe_tx(const char *desc, bool isuser, u_int pgn, u_int pri, u_int len) : nmea2000_frame_tx(desc, isuser, pgn, pri, 8), fastlen(len) { init(); }
	virtual ~nmea2000_fastframe_tx();
	virtual bool send(int);
protected:
	const int fastlen;
private:
	uint8_t *userdata;
	uint8_t ident;
	inline void init()
	    { userdata = (uint8_t *)malloc(fastlen);
	      data = userdata; ; 
	      ident = 0;
	    }
};

class iso_address_claim_tx : public nmea2000_frame_tx {
    public:
	inline iso_address_claim_tx() : nmea2000_frame_tx("ISO address claim", false, ISO_ADDRESS_CLAIM, NMEA2000_PRIORITY_REQUEST, 8) {} ;

	inline void setdata(u_int uniquenumn, u_int manuf, u_int devfunc, u_int devclass, u_int devinst, u_int systinst)
	{
	    data[0] = (uniquenumn >> 0) & 0xff;
	    data[1] = (uniquenumn >> 8) & 0xff;
	    data[2] = (uniquenumn >> 16) & 0x1f;
	    data[2] |= (manuf << 5) & 0xe0;
	    data[3] = manuf >> 3;
	    data[4] = devinst;
	    data[5] = devfunc;
	    data[6] = devclass << 1;
	    data[7] = 0x80 | (NMEA2000_INDUSTRY_GROUP << 4) | systinst;
	};
};

class private_log_tx : public nmea2000_fastframe_tx {
    public:
	    inline private_log_tx() : nmea2000_fastframe_tx("private log", true, PRIVATE_LOG, NMEA2000_PRIORITY_INFO, 4) { };

	    bool sendreq(uint8_t cmd, uint8_t sid, uint16_t idx);
	    bool sendreset(uint8_t sid);
};


class nmea2000_tx {
    public:
	nmea2000_tx();
	~nmea2000_tx();

	const nmea2000_desc *get_byindex(u_int);
	nmea2000_frame_tx *get_frametx(u_int);
	int get_bypgn(int);
	void enable(u_int, bool);

	bool send_frame(int sock, int pgn, bool force = false);
	void setsrc(int);

	iso_address_claim_tx iso_address_claim;
	private_log_tx private_log;

    private:

	std::array<nmea2000_frame_tx *,2> frames_tx = { {
		&iso_address_claim,
		&private_log,
	} };
	uint8_t sid;
};

#endif // NMEA2000_FRAME_TX_H_
