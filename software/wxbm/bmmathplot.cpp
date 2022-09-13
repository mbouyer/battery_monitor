/////////////////////////////////////////////////////////////////////////////
// Name:            mathplot.cpp
// Purpose:         Framework for plotting in wxWindows
// Original Author: David Schalig
// Maintainer:      Davide Rondini
// Contributors:    Jose Luis Blanco, Val Greene
// Created:         21/07/2003
// Last edit:       09/09/2007
// Copyright:       (c) David Schalig, Davide Rondini
// Licence:         wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include <bmmathplot.h>

#define mpLN10 2.3025850929940456840179914546844

bmScaleX::bmScaleX(wxString name, int flags) :
    mpScaleX(name, flags, true, mpX_DATETIME)
{
}

// Minimum axis label separation
#define mpMIN_X_AXIS_LABEL_SEPARATION 64

wxDEFINE_EVENT(SCALEX_EVENT, wxCommandEvent);

void bmScaleX::Plot(wxDC & dc, mpWindow & w)
{
	wxCommandEvent event(SCALEX_EVENT, w.GetId());
	event.SetEventObject(&w);
	w.GetEventHandler()->AddPendingEvent( event );

	if (!m_visible) {
		return;
	}
	dc.SetPen( m_pen);
	dc.SetFont( m_font);
	int orgy=0;

	const int extend = w.GetScrX();
	if (m_flags == mpALIGN_CENTER)
	orgy   = w.y2p(0); //(int)(w.GetPosY() * w.GetScaleY());
	if (m_flags == mpALIGN_TOP) {
		if (m_drawOutsideMargins)
			orgy = X_BORDER_SEPARATION;
		else
			orgy = w.GetMarginTop();
	}
	if (m_flags == mpALIGN_BOTTOM) {
		if (m_drawOutsideMargins)
			orgy = X_BORDER_SEPARATION;
		else
			orgy = w.GetScrY() - w.GetMarginBottom();
	}
	if (m_flags == mpALIGN_BORDER_BOTTOM )
	orgy = w.GetScrY() - 1;//dc.LogicalToDeviceY(0) - 1;
	if (m_flags == mpALIGN_BORDER_TOP )
	orgy = 1;//-dc.LogicalToDeviceY(0);

	dc.DrawLine( 0, orgy, w.GetScrX(), orgy);

	const double dig  = floor( log( 128.0 / w.GetScaleX() ) / mpLN10 );
	/* round step to 10mn */
	int step = floor(exp( mpLN10 * dig) / 600) * 600;
	const double end  = w.GetPosX() + (double)extend / w.GetScaleX();

	/* adjust step for large values */
	if (step > 3600 * 24) {
		step = floor(step / 86400.0) * 86400;
	} else if (step > 3600 * 3) {
		step = floor(step / 10800.0) * 10800;
	} else if (step > 3600) {
		step = floor(step / 3600.0) * 3600;
	} else if (step > 1800) {
		step = floor(step / 1800.0) * 1800;
	} else if (step < 600) {
		step = 600;
	}

	wxCoord tx, ty;
	wxString s;
	wxString fmt;
	int tmp = (int)dig;
	// Date and/or time axis representation
	if (m_labelType == mpX_DATETIME) {
		fmt = (wxT("%04.0f-%02.0f-%02.0f %02.0f:%02.0f"));
	} else if (m_labelType == mpX_DATE) {
		fmt = (wxT("%04.0f-%02.0f-%02.0f"));
	} else if ((m_labelType == mpX_TIME) && (end/60 < 2)) {
		fmt = (wxT("%02.0f:%02.3f"));
	} else {
		fmt = (wxT("%02.0f:%02.0f"));
	}

	time_t n0 = floor( (w.GetPosX() /* - (double)(extend - w.GetMarginLeft() - w.GetMarginRight())/ w.GetScaleX() */) / step ) * step ;
	time_t n = 0;
#ifdef MATHPLOT_DO_LOGGING
	wxLogMessage(wxT("bmScaleX::Plot: dig: %f , step: %f, end: %f, n: %f ex: %d"), dig, step, end, n0, extend);
#endif
	wxCoord startPx = m_drawOutsideMargins ? 0 : w.GetMarginLeft();
	wxCoord endPx   = m_drawOutsideMargins ? w.GetScrX() : w.GetScrX() - w.GetMarginRight();
	wxCoord minYpx  = m_drawOutsideMargins ? 0 : w.GetMarginTop();
	wxCoord maxYpx  = m_drawOutsideMargins ? w.GetScrY() : w.GetScrY() - w.GetMarginBottom();

	tmp=-65535;
	int labelH = 0; // Control labels heigth to decide where to put axis name (below labels or on top of axis)
	int maxExtent = 0;
	time_t when = 0;
	struct tm timestruct;
	for (n = n0; n < end; n += step) {
		const int p = (int)((n - w.GetPosX()) * w.GetScaleX());
#ifdef MATHPLOT_DO_LOGGING
		wxLogMessage(wxT("bmScaleX::Plot: n: %f -> p = %d"), n, p);
#endif
		if ((p >= startPx) && (p <= endPx)) {
			// Write ticks labels in s string
			if (m_labelType == mpX_DATETIME) {
				when = (time_t)n;
				if (when > 0) {
					if (m_timeConv == mpX_LOCALTIME) {
						timestruct = *localtime(&when);
					} else {
						timestruct = *gmtime(&when);
					}
					s.Printf(fmt, (double)timestruct.tm_year+1900, (double)timestruct.tm_mon+1, (double)timestruct.tm_mday, (double)timestruct.tm_hour, (double)timestruct.tm_min);
				}
			} else if (m_labelType == mpX_DATE) {
				when = (time_t)n;
				if (when > 0) {
					if (m_timeConv == mpX_LOCALTIME) {
						timestruct = *localtime(&when);
					} else {
						timestruct = *gmtime(&when);
					}
					s.Printf(fmt, (double)timestruct.tm_year+1900, (double)timestruct.tm_mon+1, (double)timestruct.tm_mday);
				}
			} else if ((m_labelType == mpX_TIME) || (m_labelType == mpX_HOURS)) {
				double modulus = fabs(n);
				double sign = n/modulus;
				double hh = floor(modulus/3600);
				double mm = floor((modulus - hh*3600)/60);
				double ss = modulus - hh*3600 - mm*60;
#ifdef MATHPLOT_DO_LOGGING
				wxLogMessage(wxT("%02.0f Hours, %02.0f minutes, %02.0f seconds"), sign*hh, mm, ss);
#endif // MATHPLOT_DO_LOGGING
				if (fmt.Len() == 20) // Format with hours has 11 chars
					s.Printf(fmt, sign*hh, mm);
				else
					s.Printf(fmt, sign*mm, ss);
			}
			dc.GetTextExtent(s, &tx, &ty);
			labelH = (labelH <= ty) ? ty : labelH;
			maxExtent = (tx > maxExtent) ? tx : maxExtent; // Keep in mind max label width
		}
	}
	// Actually draw ticks and labels, taking care of not overlapping them, and distributing them regularly
	int labelStep = ceil((maxExtent + mpMIN_X_AXIS_LABEL_SEPARATION)/(w.GetScaleX()*step))*step;
	for (n = n0; n < end; n += step) {
		const int p = (int)((n - w.GetPosX()) * w.GetScaleX());
#ifdef MATHPLOT_DO_LOGGING
	wxLogMessage(wxT("bmScaleX::Plot: n_label = %f -> p_label = %d"), n, p);
#endif
		if ((p < startPx) || (p > endPx))
			continue;
		if ((n - n0) % labelStep == 0) {
			// Write ticks labels in s string
			if (m_labelType == mpX_DATETIME) {
				when = (time_t)n;
				if (when > 0) {
					if (m_timeConv == mpX_LOCALTIME) {
						timestruct = *localtime(&when);
					} else {
						timestruct = *gmtime(&when);
					}
					s.Printf(fmt, (double)timestruct.tm_year+1900, (double)timestruct.tm_mon+1, (double)timestruct.tm_mday, (double)timestruct.tm_hour, (double)timestruct.tm_min);
				}
			} else if (m_labelType == mpX_DATE) {
				when = (time_t)n;
				if (when > 0) {
					if (m_timeConv == mpX_LOCALTIME) {
						timestruct = *localtime(&when);
					} else {
						timestruct = *gmtime(&when);
					}
					s.Printf(fmt, (double)timestruct.tm_year+1900, (double)timestruct.tm_mon+1, (double)timestruct.tm_mday);
				}
			} else if ((m_labelType == mpX_TIME) || (m_labelType == mpX_HOURS)) {
				double modulus = fabs(n);
				double sign = n/modulus;
				double hh = floor(modulus/3600);
				double mm = floor((modulus - hh*3600)/60);
				double ss = modulus - hh*3600 - mm*60;
	#ifdef MATHPLOT_DO_LOGGING
				wxLogMessage(wxT("%02.0f Hours, %02.0f minutes, %02.0f seconds"), sign*hh, mm, ss);
	#endif // MATHPLOT_DO_LOGGING
				if (fmt.Len() == 20) // Format with hours has 11 chars
					s.Printf(fmt, sign*hh, mm);
				else
					s.Printf(fmt, sign*mm, ss);
			}
			dc.GetTextExtent(s, &tx, &ty);
			if ((m_flags == mpALIGN_BORDER_BOTTOM) || (m_flags == mpALIGN_TOP)) {
				dc.DrawText( s, p-tx/2, orgy-4-ty);
			} else {
				dc.DrawText( s, p-tx/2, orgy+4);
			}
			m_pen.SetColour(*wxRED);
			dc.SetPen(m_pen);
		}
		if (m_ticks) { // draw axis ticks
			if (m_flags == mpALIGN_BORDER_BOTTOM)
				dc.DrawLine( p, orgy, p, orgy-4);
			else
				dc.DrawLine( p, orgy, p, orgy+4);
		} else { // draw grid dotted lines
			m_pen.SetStyle(wxDOT);
			dc.SetPen(m_pen);
			if ((m_flags == mpALIGN_BOTTOM) && !m_drawOutsideMargins) {
				dc.DrawLine( p, orgy+4, p, minYpx );
			} else {
				if ((m_flags == mpALIGN_TOP) && !m_drawOutsideMargins) {
					dc.DrawLine( p, orgy-4, p, maxYpx );
				} else {
					dc.DrawLine( p, 0/*-w.GetScrY()*/, p, w.GetScrY() );
				}
			}
			m_pen.SetStyle(wxSOLID);
		}
		m_pen.SetColour(*wxBLACK);
		dc.SetPen(m_pen);
	}

	// Draw axis name
	dc.GetTextExtent(m_name, &tx, &ty);
	switch (m_flags) {
		case mpALIGN_BORDER_BOTTOM:
			dc.DrawText( m_name, extend - tx - 4, orgy - 8 - ty - labelH);
		break;
		case mpALIGN_BOTTOM: {
			if ((!m_drawOutsideMargins) && (w.GetMarginBottom() > (ty + labelH + 8))) {
				dc.DrawText( m_name, (endPx - startPx - tx)>>1, orgy + 6 + labelH);
			} else {
				dc.DrawText( m_name, extend - tx - 4, orgy - 4 - ty);
			}
		} break;
		case mpALIGN_CENTER:
			dc.DrawText( m_name, extend - tx - 4, orgy - 4 - ty);
		break;
		case mpALIGN_TOP: {
			if ((!m_drawOutsideMargins) && (w.GetMarginTop() > (ty + labelH + 8))) {
				dc.DrawText( m_name, (endPx - startPx - tx)>>1, orgy - 6 - ty - labelH);
			} else {
				dc.DrawText( m_name, extend - tx - 4, orgy + 4);
			}
		} break;
		case mpALIGN_BORDER_TOP:
			dc.DrawText( m_name, extend - tx - 4, orgy + 6 + labelH);
		break;
		default:
		break;
	}
}

void bmInfoCoords::UpdateInfo(mpWindow& w, wxEvent& event)
{
	time_t when = 0;
	double xVal = 0.0, yVal = 0.0;
	struct tm timestruct;
	if (event.GetEventType() == wxEVT_MOTION) {
		wxString yStr;
		int mouseX = ((wxMouseEvent&)event).GetX();
		int mouseY = ((wxMouseEvent&)event).GetY();
		xVal = w.p2x(mouseX);
		yVal = w.p2y(mouseY);

		m_content.Clear();

		yStr.Printf(m_fmt, yVal);
		if (m_labelType == mpX_DATETIME) {
			when = (time_t) xVal;
			if (when > 0) {
				if (m_timeConv == mpX_LOCALTIME) {
					timestruct = *localtime(&when);
				} else {
					timestruct = *gmtime(&when);
				}
				m_content.Printf(wxT("%04.0f-%02.0f-%02.0f %02.0f:%02.0f\n%s"), (double)timestruct.tm_year+1900, (double)timestruct.tm_mon+1, (double)timestruct.tm_mday, (double)timestruct.tm_hour, (double)timestruct.tm_min, yStr.c_str());
			}
		} else if (m_labelType == mpX_DATE) {
			when = (time_t) xVal;
			if (when > 0) {
				if (m_timeConv == mpX_LOCALTIME) {
					timestruct = *localtime(&when);
				} else {
					timestruct = *gmtime(&when);
				}
				m_content.Printf(wxT("%04.0f-%02.0f-%02.0f\n%s"), (double)timestruct.tm_year+1900, (double)timestruct.tm_mon+1, (double)timestruct.tm_mday, yStr.c_str());
			}
		} else if ((m_labelType == mpX_TIME) || (m_labelType == mpX_HOURS)) {
			double modulus = fabs(xVal);
			double sign = xVal/modulus;
			double hh = floor(modulus/3600);
			double mm = floor((modulus - hh*3600)/60);
			double ss = modulus - hh*3600 - mm*60;
			m_content.Printf(wxT("%02.0f:%02.0f\n%s"), sign*hh, mm, yStr.c_str());
		}

	}
}
