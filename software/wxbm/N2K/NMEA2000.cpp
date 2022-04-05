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

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <net/if.h>

#include <wx/wx.h>

#include <wxbm.h>
#include "NMEA2000.h"
#include "nmea2000_defs_tx.h"
#include "nmea2000_defs_rx.h"
#include "NMEA2000Properties.h" 

nmea2000 *nmea2000P;

nmea2000::nmea2000(void) {
    myaddress = 0x80;
    srandom(time(NULL));
    // the following may be overriden by the config file
    uniquenumber = random() & 0x1fffff;
    deviceinstance = 0;
    manufcode = 0x7ff;

    nmea2000_rxP = new nmea2000_rx;
    nmea2000_txP = new nmea2000_tx;
}

nmea2000::~nmea2000(void)
{
    if (GetThread() &&      
	GetThread()->IsRunning()) {
	GetThread()->Delete();
	GetThread()->Wait();
    }
    close(sock);
    delete nmea2000_rxP;
    delete nmea2000_txP;
}

void nmea2000::Init() {

    state = UNCONF;

    if ((sock = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
	wxLogSysError(wxbm::ErrMsgPrefix() + _T("create CAN socket"));
	return;
    }
    nmea2000_txP->setsrc(myaddress);
    nmea2000_txP->iso_address_claim.setdst(NMEA2000_ADDR_GLOBAL);
    nmea2000_txP->iso_address_claim.setdata(uniquenumber, manufcode, 130, 120, deviceinstance, 0);
    nmea2000_txP->iso_address_claim.enabled = 1;
    nmea2000_txP->iso_address_claim.valid = 1;
    if (CreateThread(wxTHREAD_JOINABLE) != wxTHREAD_NO_ERROR) {
        wxLogError(wxbm::ErrMsgPrefix() + _T("Could not create the management thread"));
	return;
    }
   if (GetThread()->Run() != wxTHREAD_NO_ERROR) {
        wxLogError(wxbm::ErrMsgPrefix() + _T("Could not run the management thread!"));
	return;
    }
}

wxThread::ExitCode nmea2000::Entry()
{

    while (!GetThread()->TestDestroy()) {
	switch(state) {
	case UNCONF:
	case DOINGCONF:
	        if (configure()) {
	            state = DOCLAIM;
	        } else {
	            state = DOINGCONF;
		    sleep(1);
	        }
	        break;
	case DOCLAIM:
		if (nmea2000_txP->iso_address_claim.send(sock)) {
			state = CLAIMING;
			gettimeofday(&claim_date, NULL);
		} else {
			std::cerr << "failed to send claim " << std::endl;
			sleep(1);
		}
		break;
	case CLAIMING:
		struct timeval tv;
		struct timeval tv2;
		// poll checking for addr claim, and timeout
		gettimeofday(&tv, NULL);
		timersub(&tv, &claim_date, &tv2);
		if (tv2.tv_sec >= 1) {
			std::cout << "NMEA200 address " << myaddress << std::endl;
			state = CLAIMED;
		}
		/* FALLTHROUGH */
	case CLAIMED:
		// normal operation
		struct timeval timeout;
		fd_set read_set;
		int sret;

		timeout.tv_sec=1;
		timeout.tv_usec = 0;

		FD_ZERO(&read_set);
		FD_SET(sock, &read_set);
		sret = select(sock + 1, &read_set, NULL, NULL, &timeout);
		switch(sret) {
		case -1:
			wxLogSysError(wxbm::ErrMsgPrefix() + _T("select"));
			break;
		case 0:
			nmea2000_rxP->tick();
			break;
		default:
			nmea2000_frame n2kframe;
			int rret = n2kframe.readframe(sock);
			switch(rret) {
			case -1:
				wxLogSysError(wxbm::ErrMsgPrefix() + _T("read CAN socket"));
				break;
			case 0:
				/* EOF ? */
				break;
			default:
				parse_frame(n2kframe);
				break;
			}
			nmea2000_rxP->tick();
		}
		break;
	default:
		sleep(1);
	}
			
    }
    return (wxThread::ExitCode)0;
}

bool nmea2000::configure()
{
    struct ifreq ifr;
    struct sockaddr_can addr;

    memset(&ifr, 0, sizeof(ifr));
    if (canif.Len() > 0) {
	strcpy(ifr.ifr_name, canif.c_str());
	if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
	    if (state == UNCONF) {
		wxLogSysError(wxbm::ErrMsgPrefix() + _T("can't get index for CAN interface %s"), canif.c_str());
	    }
	    return false;
        }
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;
        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	    if (state == UNCONF) {
	        wxLogSysError(wxbm::ErrMsgPrefix() + _T("can't bind CAN socket to %s"), canif.c_str());
	    }
	    return false;
        }
        return true;
    }
    return false;
}

void nmea2000::parse_frame(const nmea2000_frame &n2kf)
{
	if (n2kf.is_pdu1() &&
	    n2kf.getdst() != myaddress &&
	    n2kf.getdst() != NMEA2000_ADDR_GLOBAL) {
		return;
	}
	switch(n2kf.getpgn()) {
	case ISO_ADDRESS_CLAIM:
		handle_address_claim(n2kf);
		break;
	case ISO_REQUEST:
		handle_iso_request(n2kf);
		break;
	default:
		nmea2000_rxP->handle(n2kf);
		break;
	}
	return;
}

void nmea2000::handle_address_claim(const nmea2000_frame &n2kf)
{
	if (n2kf.getsrc() != myaddress)
		return;
	for (int i = 7; i >= 0; i--) {
	    if (n2kf.getdata()[i] < nmea2000_txP->iso_address_claim.getdata()[i]) {
		// we loose
		myaddress++;
		if (myaddress >= NMEA2000_ADDR_MAX)
			myaddress = 0;
		nmea2000_txP->setsrc(myaddress);
		state = DOCLAIM;
		return;
	    }
	    if (n2kf.getdata()[i] > nmea2000_txP->iso_address_claim.getdata()[i])
		break;
	}
	// defend our address. if we can't right now restart the whole process
	if (!nmea2000_txP->iso_address_claim.send(sock)) 
		state = DOCLAIM;
}

void nmea2000::handle_iso_request(const nmea2000_frame &n2kf)
{
	int pgn;
	if (n2kf.getlen() < 3)
		return;

	pgn = n2kf.frame2uint24(0);
	if (nmea2000_txP->get_bypgn(pgn) < 0) {
		return;
	}
	nmea2000_txP->send_frame(sock, pgn);
}

void nmea2000::OnThreadUpdate(wxThreadEvent& evt) {
}

const nmea2000_desc *nmea2000::get_tx_byindex(int i) {
	return nmea2000_txP->get_byindex(i);
}

int nmea2000::get_tx_bypgn(int pgn) {
	return nmea2000_txP->get_bypgn(pgn);
}

nmea2000_frame_tx *nmea2000::get_frametx(u_int i) {
    return nmea2000_txP->get_frametx(i);
}

bool nmea2000::send_bypgn(int pgn, bool force) {
	if (state != CLAIMED)
		return false;

	return nmea2000_txP->send_frame(sock, pgn, force);
}

void nmea2000::tx_enable(int i, bool en) {
	nmea2000_txP->enable(i, en);
}
	
const nmea2000_desc *nmea2000::get_rx_byindex(int i) {
	return nmea2000_rxP->get_byindex(i);
}

int nmea2000::get_rx_bypgn(int pgn) {
	return nmea2000_rxP->get_bypgn(pgn);
}

void nmea2000::rx_enable(int i, bool en) {
	nmea2000_rxP->enable(i, en);
}
