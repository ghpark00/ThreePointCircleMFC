#pragma once

#include <atlimage.h>

namespace PixelRenderer
{
	bool ClearGrayscale(ATL::CImage& image, BYTE value) noexcept;
	bool DrawFilledCircle(ATL::CImage& image, int centerX, int centerY,
		int radius, BYTE value) noexcept;
}
