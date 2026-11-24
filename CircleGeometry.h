#pragma once

namespace CircleGeometry
{
	struct PointD
	{
		double x = 0.0;
		double y = 0.0;
	};

	struct Circumcircle
	{
		double centerX = 0.0;
		double centerY = 0.0;
		double radius = 0.0;
	};

	enum class CircumcircleError
	{
		None,
		DuplicatePoints,
		CollinearPoints,
		NumericalFailure
	};

	struct CircumcircleResult
	{
		Circumcircle circle{};
		CircumcircleError error = CircumcircleError::None;

		bool IsValid() const noexcept
		{
			return error == CircumcircleError::None;
		}
	};

	CircumcircleResult CalculateCircumcircle(
		const PointD& first, const PointD& second, const PointD& third) noexcept;

#ifdef _DEBUG
	bool RunDebugVerification() noexcept;
#endif
}
