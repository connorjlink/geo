#ifndef GEO_BASE_H
#define GEO_BASE_H

#include "float.h"

#include <cstdlib>
#include <array>

namespace geo
{
	template<std::size_t M = 4, typename T = platform_type>
	class vec
	{
	private:
		std::array<T, M> _data;

	public:
		template<typename... Ts>
		vec(Ts... ts)
		{
			_data = { static_cast<T>(ts)... };
		}

	public:
		constexpr operator std::array<T, M>() const
		{
			return _data;
		}

	public:
		constexpr auto& operator[](std::size_t i)
		{
			return _data[i];
		}

		constexpr const auto& operator[](std::size_t i) const
		{
			return _data[i];
		}

	public:
		constexpr auto operator==(const auto& other) const
		{
			return _data == other._data;
		}

		constexpr auto operator!=(const auto& other) const
		{
			return _data != other._data;
		}
	};

	template<std::size_t M = 4, std::size_t N = 4, typename T = platform_type>
	class mat
	{
	private:
		std::array<std::array<T, N>, M> _data;

	public:
		template<typename... Ts>
		mat(Ts... ts)
		{
			_data = { static_cast<T>(ts)... };
		}

	public:
		constexpr auto& operator[](std::size_t i)
		{
			return _data[i];
		}

		constexpr const auto& operator[](std::size_t i) const
		{
			return _data[i];
		}
	};
}

#endif
