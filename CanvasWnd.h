#pragma once

#include <array>
#include <atlimage.h>

inline constexpr UINT WM_APP_CANVAS_EVENT = WM_APP + 1;

enum class CanvasEvent
{
	PointsChanged = 1,
	PointLimitReached,
	ExistingPointPressed
};

class CCanvasWnd : public CWnd
{
public:
	static constexpr int kMaxPointCount = 3;

	BOOL AttachCanvas(CWnd* parent, UINT controlId);
	void SetPointRadius(int radius);
	void ResetPoints();

	int GetPointCount() const noexcept;
	bool GetPoint(int index, CPoint& point) const noexcept;

protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* dc);
	afx_msg void OnLButtonDown(UINT flags, CPoint point);
	afx_msg void OnSize(UINT type, int cx, int cy);
	DECLARE_MESSAGE_MAP()

private:
	bool RecreateImage(int width, int height);
	void RenderScene();
	int HitTestPoint(CPoint point) const noexcept;
	void NotifyParent(CanvasEvent event) const;

	ATL::CImage m_image;
	std::array<CPoint, kMaxPointCount> m_points{};
	int m_pointCount = 0;
	int m_pointRadius = 10;
};
