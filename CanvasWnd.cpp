#include "pch.h"
#include "CanvasWnd.h"

#include "PixelRenderer.h"

#include <array>
#include <cstdint>

BEGIN_MESSAGE_MAP(CCanvasWnd, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL CCanvasWnd::AttachCanvas(CWnd* parent, UINT controlId)
{
	if (!SubclassDlgItem(controlId, parent))
	{
		return FALSE;
	}
	SetWindowText(_T("Drawing canvas"));

	CRect clientRectangle;
	GetClientRect(&clientRectangle);
	if (!RecreateImage(clientRectangle.Width(), clientRectangle.Height()))
	{
		UnsubclassWindow();
		return FALSE;
	}
	return TRUE;
}

void CCanvasWnd::SetRenderSettings(int pointRadius, int outlineThickness)
{
	if (pointRadius == m_pointRadius && outlineThickness == m_outlineThickness)
	{
		return;
	}

	m_pointRadius = pointRadius;
	m_outlineThickness = outlineThickness;
	RenderScene();
	Invalidate(FALSE);
}

void CCanvasWnd::ResetPoints()
{
	m_pointCount = 0;
	m_circumcircle.reset();
	m_circumcircleError = CircleGeometry::CircumcircleError::None;
	RenderScene();
	Invalidate(FALSE);
}

int CCanvasWnd::GetPointCount() const noexcept
{
	return m_pointCount;
}

bool CCanvasWnd::GetPoint(int index, CPoint& point) const noexcept
{
	if (index < 0 || index >= m_pointCount)
	{
		return false;
	}

	point = m_points[static_cast<std::size_t>(index)];
	return true;
}

bool CCanvasWnd::GetCircumcircle(CircleGeometry::Circumcircle& circle) const noexcept
{
	if (!m_circumcircle.has_value())
	{
		return false;
	}

	circle = *m_circumcircle;
	return true;
}

CircleGeometry::CircumcircleError CCanvasWnd::GetCircumcircleError() const noexcept
{
	return m_circumcircleError;
}

void CCanvasWnd::OnPaint()
{
	CPaintDC dc(this);
	if (!m_image.IsNull())
	{
		// BitBlt only transfers the already-rendered bitmap. Shape creation is
		// performed separately by PixelRenderer through direct memory writes.
		m_image.BitBlt(dc.GetSafeHdc(), 0, 0, SRCCOPY);
	}
	else
	{
		CRect clientRectangle;
		GetClientRect(&clientRectangle);
		dc.FillSolidRect(clientRectangle, RGB(255, 255, 255));
	}
}

BOOL CCanvasWnd::OnEraseBkgnd(CDC* /*dc*/)
{
	// The persistent CImage covers the complete client area in OnPaint.
	return TRUE;
}

void CCanvasWnd::OnLButtonDown(UINT flags, CPoint point)
{
	SetFocus();

	CRect clientRectangle;
	GetClientRect(&clientRectangle);
	if (!clientRectangle.PtInRect(point))
	{
		CWnd::OnLButtonDown(flags, point);
		return;
	}

	// This branch is intentionally separate from the capacity check. The next
	// stage can start mouse capture here without restructuring the 3-point case.
	if (m_pointCount == kMaxPointCount && HitTestPoint(point) >= 0)
	{
		NotifyParent(CanvasEvent::ExistingPointPressed);
		CWnd::OnLButtonDown(flags, point);
		return;
	}

	if (m_pointCount >= kMaxPointCount)
	{
		NotifyParent(CanvasEvent::PointLimitReached);
		CWnd::OnLButtonDown(flags, point);
		return;
	}

	m_points[static_cast<std::size_t>(m_pointCount)] = point;
	++m_pointCount;
	RecalculateCircumcircle();
	RenderScene();
	Invalidate(FALSE);
	NotifyParent(CanvasEvent::PointsChanged);

	CWnd::OnLButtonDown(flags, point);
}

void CCanvasWnd::OnSize(UINT type, int cx, int cy)
{
	CWnd::OnSize(type, cx, cy);
	if (cx > 0 && cy > 0 &&
		(m_image.IsNull() || m_image.GetWidth() != cx || m_image.GetHeight() != cy))
	{
		if (RecreateImage(cx, cy))
		{
			Invalidate(FALSE);
		}
	}
}

bool CCanvasWnd::RecreateImage(int width, int height)
{
	if (width <= 0 || height <= 0)
	{
		return false;
	}
	if (!m_image.IsNull() && m_image.GetWidth() == width && m_image.GetHeight() == height)
	{
		return true;
	}

	m_image.Destroy();
	if (!m_image.Create(width, height, 8) || m_image.IsNull())
	{
		return false;
	}

	std::array<RGBQUAD, 256> grayscalePalette{};
	for (int value = 0; value < 256; ++value)
	{
		grayscalePalette[static_cast<std::size_t>(value)].rgbBlue =
			static_cast<BYTE>(value);
		grayscalePalette[static_cast<std::size_t>(value)].rgbGreen =
			static_cast<BYTE>(value);
		grayscalePalette[static_cast<std::size_t>(value)].rgbRed =
			static_cast<BYTE>(value);
	}
	m_image.SetColorTable(0, static_cast<UINT>(grayscalePalette.size()),
		grayscalePalette.data());

	RenderScene();
	return true;
}

void CCanvasWnd::RecalculateCircumcircle()
{
	m_circumcircle.reset();
	m_circumcircleError = CircleGeometry::CircumcircleError::None;
	if (m_pointCount != kMaxPointCount)
	{
		return;
	}

	const CPoint& first = m_points[0];
	const CPoint& second = m_points[1];
	const CPoint& third = m_points[2];
	const CircleGeometry::CircumcircleResult result =
		CircleGeometry::CalculateCircumcircle(
			{ static_cast<double>(first.x), static_cast<double>(first.y) },
			{ static_cast<double>(second.x), static_cast<double>(second.y) },
			{ static_cast<double>(third.x), static_cast<double>(third.y) });
	m_circumcircleError = result.error;
	if (result.IsValid())
	{
		m_circumcircle = result.circle;
	}
}

void CCanvasWnd::RenderScene()
{
	if (!PixelRenderer::ClearGrayscale(m_image, 255))
	{
		return;
	}

	if (m_circumcircle.has_value())
	{
		PixelRenderer::DrawCircleOutline(m_image,
			m_circumcircle->centerX, m_circumcircle->centerY,
			m_circumcircle->radius, m_outlineThickness, 0);
	}

	for (int index = 0; index < m_pointCount; ++index)
	{
		const CPoint& point = m_points[static_cast<std::size_t>(index)];
		PixelRenderer::DrawFilledCircle(
			m_image, point.x, point.y, m_pointRadius, 0);
	}
}

int CCanvasWnd::HitTestPoint(CPoint point) const noexcept
{
	const std::int64_t radiusSquared =
		static_cast<std::int64_t>(m_pointRadius) * m_pointRadius;
	for (int index = m_pointCount - 1; index >= 0; --index)
	{
		const CPoint& candidate = m_points[static_cast<std::size_t>(index)];
		const std::int64_t dx = static_cast<std::int64_t>(point.x) - candidate.x;
		const std::int64_t dy = static_cast<std::int64_t>(point.y) - candidate.y;
		if (dx * dx + dy * dy <= radiusSquared)
		{
			return index;
		}
	}
	return -1;
}

void CCanvasWnd::NotifyParent(CanvasEvent event) const
{
	const HWND parentWindow = ::GetParent(GetSafeHwnd());
	if (::IsWindow(parentWindow))
	{
		::SendMessage(parentWindow, WM_APP_CANVAS_EVENT,
			static_cast<WPARAM>(event), 0);
	}
}
