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

/* event sent when X range (possibly) changed */
wxDECLARE_EVENT(SCALEX_EVENT, wxCommandEvent);

/* bmInfoCoords reimplementation appropriate for the bmlog
   draw X/Y lines and send coords to parent
*/
class bmInfoCoords : public mpLayer
{
public:
    inline bmInfoCoords(bmLog *bmlog, wxPen pen, wxString fmt = "%f") : mpLayer() {
	m_fmt = fmt;
	m_bmlog = bmlog;
	SetPen(pen);
    };

    virtual void UpdateInfo(mpWindow& w, wxEvent& event);
    virtual void Plot(wxDC & dc, mpWindow & w);
    void UpdateX(time_t time, mpWindow *w);
    virtual bool HasBBox() { return false; };
    virtual bool IsInfo() { return true; };

protected:
    wxString m_fmt;
    bmLog *m_bmlog;
    time_t m_time;
};

/* mpFXYVector reimplementation which allows to read the data vectors
 * and keep a pointer to its mpwindow */
class bmFXYVector : public mpFXYVector
{
	public:
		inline bmFXYVector(mpWindow *w = NULL, wxString name = wxEmptyString, int flags = mpALIGN_NE) : mpFXYVector(name, flags) {
			m_w = w;
		}
		inline void GetData(const std::vector<double> *&x, const std::vector<double> *&y) {
			x = &m_xs;
			y = &m_ys;
		}
		inline mpWindow *GetWindow(void) {
			return m_w;
		}
	DECLARE_DYNAMIC_CLASS(bmFXYVector)
	private:
		mpWindow *m_w;
};

#endif // _BM_MATHPLOT_H_
