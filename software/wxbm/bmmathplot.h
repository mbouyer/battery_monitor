#ifndef _BM_MATHPLOT_H_
#define _BM_MATHPLOT_H_

#include <mathplot.h>

/* mpScaleX reimplementation appropriate for the bmlog
   round ticks to minutes and display time in a more concise way
*/
class bmScaleX : public mpScaleX
{
public:
    bmScaleX(wxString name = wxT("X"), int flags = mpALIGN_CENTER);

    virtual void Plot(wxDC & dc, mpWindow & w);
};

/* mpScaleX reimplementation appropriate for the bmlog
   round ticks to minutes and display time in a more concise way
   Allow Y format to be set
*/
class bmInfoCoords : public mpInfoCoords
{
public:
    inline bmInfoCoords(wxRect rect, const wxBrush* brush = wxTRANSPARENT_BRUSH, wxString fmt = "%f") : mpInfoCoords(rect, brush) {
	m_fmt = fmt;
    };

    virtual void UpdateInfo(mpWindow& w, wxEvent& event);

protected:
    wxString m_fmt;
};
#endif // _BM_MATHPLOT_H_
