#include "pch.h"
#include "CircleGeometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
	constexpr double kCollinearityEpsilonMultiplier = 64.0;
	constexpr double kDistanceValidationEpsilonMultiplier = 512.0;

	bool IsFinitePoint(const CircleGeometry::PointD& point) noexcept
	{
		return std::isfinite(point.x) && std::isfinite(point.y);
	}

	bool IsSamePoint(const CircleGeometry::PointD& left,
		const CircleGeometry::PointD& right) noexcept
	{
		return left.x == right.x && left.y == right.y;
	}

	double Distance(const CircleGeometry::PointD& point,
		const CircleGeometry::Circumcircle& circle) noexcept
	{
		return std::hypot(point.x - circle.centerX, point.y - circle.centerY);
	}

	bool DistancesMatch(const CircleGeometry::PointD& first,
		const CircleGeometry::PointD& second,
		const CircleGeometry::PointD& third,
		const CircleGeometry::Circumcircle& circle) noexcept
	{
		const double firstDistance = Distance(first, circle);
		const double secondDistance = Distance(second, circle);
		const double thirdDistance = Distance(third, circle);
		const double tolerance = kDistanceValidationEpsilonMultiplier *
			std::numeric_limits<double>::epsilon() *
			(std::max)(1.0, circle.radius);

		return std::isfinite(firstDistance) && std::isfinite(secondDistance) &&
			std::isfinite(thirdDistance) &&
			std::abs(firstDistance - circle.radius) <= tolerance &&
			std::abs(secondDistance - circle.radius) <= tolerance &&
			std::abs(thirdDistance - circle.radius) <= tolerance;
	}

#ifdef _DEBUG
	bool NearlyEqual(double left, double right, double relativeTolerance = 1.0e-10) noexcept
	{
		return std::abs(left - right) <= relativeTolerance *
			(std::max)(1.0, (std::max)(std::abs(left), std::abs(right)));
	}
#endif
}

CircleGeometry::CircumcircleResult CircleGeometry::CalculateCircumcircle(
	const PointD& first, const PointD& second, const PointD& third) noexcept
{
	CircumcircleResult result;
	if (!IsFinitePoint(first) || !IsFinitePoint(second) || !IsFinitePoint(third))
	{
		result.error = CircumcircleError::NumericalFailure;
		return result;
	}

	if (IsSamePoint(first, second) || IsSamePoint(first, third) ||
		IsSamePoint(second, third))
	{
		result.error = CircumcircleError::DuplicatePoints;
		return result;
	}

	// Translate P1 to the origin. This avoids squaring the absolute canvas
	// coordinates and solves the two perpendicular-bisector equations directly.
	const double ax = second.x - first.x;
	const double ay = second.y - first.y;
	const double bx = third.x - first.x;
	const double by = third.y - first.y;
	const double cx = third.x - second.x;
	const double cy = third.y - second.y;
	const double aLengthSquared = ax * ax + ay * ay;
	const double bLengthSquared = bx * bx + by * by;
	const double cLengthSquared = cx * cx + cy * cy;
	const double determinant = ax * by - ay * bx;
	const double maximumEdgeSquared = (std::max)(aLengthSquared,
		(std::max)(bLengthSquared, cLengthSquared));
	const double determinantTolerance = kCollinearityEpsilonMultiplier *
		std::numeric_limits<double>::epsilon() *
		(std::max)(1.0, maximumEdgeSquared);

	if (!std::isfinite(determinant) ||
		std::abs(determinant) <= determinantTolerance)
	{
		result.error = CircumcircleError::CollinearPoints;
		return result;
	}

	const double twiceDeterminant = 2.0 * determinant;
	const double offsetX =
		(aLengthSquared * by - bLengthSquared * ay) / twiceDeterminant;
	const double offsetY =
		(ax * bLengthSquared - bx * aLengthSquared) / twiceDeterminant;
	result.circle.centerX = first.x + offsetX;
	result.circle.centerY = first.y + offsetY;
	result.circle.radius = std::hypot(offsetX, offsetY);

	if (!std::isfinite(result.circle.centerX) ||
		!std::isfinite(result.circle.centerY) ||
		!std::isfinite(result.circle.radius) || result.circle.radius <= 0.0 ||
		!DistancesMatch(first, second, third, result.circle))
	{
		result.circle = {};
		result.error = CircumcircleError::NumericalFailure;
	}
	return result;
}

#ifdef _DEBUG
bool CircleGeometry::RunDebugVerification() noexcept
{
	const PointD rightFirst{ 0.0, 0.0 };
	const PointD rightSecond{ 10.0, 0.0 };
	const PointD rightThird{ 0.0, 10.0 };
	const CircumcircleResult rightResult =
		CalculateCircumcircle(rightFirst, rightSecond, rightThird);
	if (!rightResult.IsValid() || !NearlyEqual(rightResult.circle.centerX, 5.0) ||
		!NearlyEqual(rightResult.circle.centerY, 5.0) ||
		!NearlyEqual(rightResult.circle.radius, std::sqrt(50.0)) ||
		!DistancesMatch(rightFirst, rightSecond, rightThird, rightResult.circle))
	{
		return false;
	}

	const PointD isoscelesFirst{ 0.0, 0.0 };
	const PointD isoscelesSecond{ 10.0, 0.0 };
	const PointD isoscelesThird{ 5.0, 10.0 };
	const CircumcircleResult isoscelesResult =
		CalculateCircumcircle(isoscelesFirst, isoscelesSecond, isoscelesThird);
	if (!isoscelesResult.IsValid() ||
		!DistancesMatch(isoscelesFirst, isoscelesSecond, isoscelesThird,
			isoscelesResult.circle))
	{
		return false;
	}

	const CircumcircleResult collinearResult = CalculateCircumcircle(
		{ 0.0, 0.0 }, { 10.0, 10.0 }, { 20.0, 20.0 });
	if (collinearResult.error != CircumcircleError::CollinearPoints)
	{
		return false;
	}

	const CircumcircleResult duplicateResult = CalculateCircumcircle(
		{ 10.0, 10.0 }, { 10.0, 10.0 }, { 20.0, 30.0 });
	return duplicateResult.error == CircumcircleError::DuplicatePoints;
}
#endif
