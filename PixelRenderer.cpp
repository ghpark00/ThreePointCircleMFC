#include "pch.h"
#include "PixelRenderer.h"

#include <algorithm>
#include <cmath>
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

bool PixelRenderer::DrawCircleOutline(ATL::CImage& image, double centerX,
	double centerY, double radius, int thickness, BYTE value) noexcept
{
	BYTE* bits = nullptr;
	int pitch = 0;
	int width = 0;
	int height = 0;
	if (radius <= 0.0 || thickness <= 0 || !std::isfinite(centerX) ||
		!std::isfinite(centerY) || !std::isfinite(radius) ||
		!GetWritableImageInfo(image, bits, pitch, width, height))
	{
		return false;
	}

	const double halfThickness = static_cast<double>(thickness) / 2.0;
	const double outerRadius = radius + halfThickness;
	double innerRadius = radius - halfThickness;
	if (!std::isfinite(outerRadius))
	{
		return false;
	}

	// A centered stroke wider than the diameter would mathematically become a
	// disk. Preserve a 0.5-pixel-radius hollow core instead of filling the center.
	if (innerRadius <= 0.0)
	{
		innerRadius = 0.5;
	}

	const double leftBound = centerX - outerRadius;
	const double rightBound = centerX + outerRadius;
	const double topBound = centerY - outerRadius;
	const double bottomBound = centerY + outerRadius;
	if (rightBound < 0.0 || bottomBound < 0.0 ||
		leftBound > static_cast<double>(width - 1) ||
		topBound > static_cast<double>(height - 1))
	{
		return true;
	}

	const int left = leftBound <= 0.0 ? 0 : static_cast<int>(std::ceil(leftBound));
	const int right = rightBound >= static_cast<double>(width - 1) ? width - 1 :
		static_cast<int>(std::floor(rightBound));
	const int top = topBound <= 0.0 ? 0 : static_cast<int>(std::ceil(topBound));
	const int bottom = bottomBound >= static_cast<double>(height - 1) ? height - 1 :
		static_cast<int>(std::floor(bottomBound));
	const double innerRadiusSquared = innerRadius * innerRadius;
	const double outerRadiusSquared = outerRadius * outerRadius;
	if (!std::isfinite(innerRadiusSquared) || !std::isfinite(outerRadiusSquared))
	{
		return false;
	}

	for (int y = top; y <= bottom; ++y)
	{
		BYTE* const row = bits + static_cast<std::ptrdiff_t>(y) * pitch;
		const double dy = static_cast<double>(y) - centerY;
		for (int x = left; x <= right; ++x)
		{
			const double dx = static_cast<double>(x) - centerX;
			const double distanceSquared = dx * dx + dy * dy;
			if (distanceSquared >= innerRadiusSquared &&
				distanceSquared <= outerRadiusSquared)
			{
				row[x] = value;
			}
		}
	}
	return true;
}
