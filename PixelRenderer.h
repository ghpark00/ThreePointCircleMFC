#pragma once

#include <atlimage.h>

namespace PixelRenderer
{
	bool ClearGrayscale(ATL::CImage& image, BYTE value) noexcept;
	bool DrawFilledCircle(ATL::CImage& image, int centerX, int centerY,
		int radius, BYTE value) noexcept;
	bool DrawCircleOutline(ATL::CImage& image, double centerX, double centerY,
		double radius, int thickness, BYTE value) noexcept;
}
