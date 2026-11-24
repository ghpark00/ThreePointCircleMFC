#pragma once

#include "CircleGeometry.h"

#include <array>
#include <atlimage.h>
#include <optional>

inline constexpr UINT WM_APP_CANVAS_EVENT = WM_APP + 1;

enum class CanvasEvent
{
	PointsChanged = 1
};

class CCanvasWnd : public CWnd
{
public:
	static constexpr int kMaxPointCount = 3;

	BOOL AttachCanvas(CWnd* parent, UINT controlId);
	void SetRenderSettings(int pointRadius, int outlineThickness);
	void ResetPoints();

	int GetPointCount() const noexcept;
	bool GetPoint(int index, CPoint& point) const noexcept;
	bool GetCircumcircle(CircleGeometry::Circumcircle& circle) const noexcept;
	CircleGeometry::CircumcircleError GetCircumcircleError() const noexcept;

protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* dc);
	afx_msg void OnLButtonDown(UINT flags, CPoint point);
	afx_msg void OnLButtonUp(UINT flags, CPoint point);
	afx_msg void OnMouseMove(UINT flags, CPoint point);
	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg void OnCancelMode();
	afx_msg void OnKeyDown(UINT character, UINT repeatCount, UINT flags);
	afx_msg void OnNcDestroy();
	afx_msg LRESULT OnCaptureChanged(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
	bool RecreateImage(int width, int height);
	void RecalculateCircumcircle();
	void RenderScene();
	int HitTestPoint(CPoint point) const noexcept;
	bool UpdateDraggedPoint(CPoint point);
	void EndDrag(bool releaseCapture) noexcept;
	void NotifyParent(CanvasEvent event) const;

	ATL::CImage m_image;
	std::array<CPoint, kMaxPointCount> m_points{};
	std::optional<CircleGeometry::Circumcircle> m_circumcircle;
	CircleGeometry::CircumcircleError m_circumcircleError =
		CircleGeometry::CircumcircleError::None;
	int m_pointCount = 0;
	int m_pointRadius = 10;
	int m_outlineThickness = 2;
	int m_dragPointIndex = -1;
	bool m_isDragging = false;
};
