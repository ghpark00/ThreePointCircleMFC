#include "pch.h"
#include "PixelRenderer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace
{
	bool GetWritableImageInfo(ATL::CImage& image, BYTE*& bits, int& pitch,
		int& width, int& height) noexcept
	{
		if (image.IsNull() || image.GetBPP() != 8)
		{
			return false;
		}

		bits = static_cast<BYTE*>(image.GetBits());
		pitch = image.GetPitch();
		width = image.GetWidth();
		height = image.GetHeight();
		return bits != nullptr && pitch != 0 && width > 0 && height > 0;
	}
}

bool PixelRenderer::ClearGrayscale(ATL::CImage& image, BYTE value) noexcept
{
	BYTE* bits = nullptr;
	int pitch = 0;
	int width = 0;
	int height = 0;
	if (!GetWritableImageInfo(image, bits, pitch, width, height))
	{
		return false;
	}

	for (int y = 0; y < height; ++y)
	{
		BYTE* const row = bits + static_cast<std::ptrdiff_t>(y) * pitch;
		std::fill_n(row, width, value);
	}
	return true;
}

bool PixelRenderer::DrawFilledCircle(ATL::CImage& image, int centerX, int centerY,
	int radius, BYTE value) noexcept
{
	BYTE* bits = nullptr;
	int pitch = 0;
	int width = 0;
	int height = 0;
	if (radius <= 0 || !GetWritableImageInfo(image, bits, pitch, width, height))
	{
		return false;
	}

	const int left = (std::max)(0, centerX - radius);
	const int right = (std::min)(width - 1, centerX + radius);
	const int top = (std::max)(0, centerY - radius);
	const int bottom = (std::min)(height - 1, centerY + radius);
	const std::int64_t radiusSquared =
		static_cast<std::int64_t>(radius) * radius;

	for (int y = top; y <= bottom; ++y)
	{
		BYTE* const row = bits + static_cast<std::ptrdiff_t>(y) * pitch;
		const std::int64_t dy = static_cast<std::int64_t>(y) - centerY;
		for (int x = left; x <= right; ++x)
		{
			const std::int64_t dx = static_cast<std::int64_t>(x) - centerX;
			if (dx * dx + dy * dy <= radiusSquared)
			{
				row[x] = value;
			}
		}
	}
	return true;
}
