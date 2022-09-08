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

#endif // _BM_MATHPLOT_H_
