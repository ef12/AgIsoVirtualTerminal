//================================================================================================
/// @file NumericValueConversion.hpp
///
/// @brief Conversion helpers for ISO 11783 Virtual Terminal numeric objects.
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef NUMERIC_VALUE_CONVERSION_HPP
#define NUMERIC_VALUE_CONVERSION_HPP

#include <cstdint>

namespace NumericValueConversion
{
	/// @brief Converts a raw unsigned VT numeric value into its displayed value.
	/// @details The signed offset must be applied without unsigned integer promotion.
	constexpr double to_displayed_value(std::uint32_t rawValue, std::int32_t offset, float scale) noexcept
	{
		const auto offsetValue = static_cast<std::int64_t>(rawValue) + static_cast<std::int64_t>(offset);
		return static_cast<double>(offsetValue) * static_cast<double>(scale);
	}

	static_assert((-1.000001 < to_displayed_value(214748363U, -214748364, 1.0f)) &&
	                (-0.999999 > to_displayed_value(214748363U, -214748364, 1.0f)),
	              "Negative VT numeric values must not wrap to an unsigned value.");
} // namespace NumericValueConversion

#endif // NUMERIC_VALUE_CONVERSION_HPP
