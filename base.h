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
			return !operator==(other);
		}
	};

	// represents an M x N matrix
	template<std::size_t M = 4, std::size_t N = 4, typename T = platform_type>
	class mat
	{
	private:
		std::array<std::array<T, N>, M> _data;

	public:
		mat(const std::array<std::array<T, N>, M>& data)
		{
			_data = data;
		}

		template<typename... Ts>
		mat(Ts&&... ts)
		{
			// const std::array<T, N>
			auto i = 0;
			
			([&]()
			{
				_data[i] = ts;
				++i;
			}(), ...);
		}

	public:
		inline constexpr auto& operator[](std::size_t i)
		{
			return _data[i];
		}

		inline constexpr const auto& operator[](std::size_t i) const
		{
			return _data[i];
		}

	public:
		constexpr bool operator==(const auto& other) const
		{
			for (auto i = 0; i < M; i++)
			{
				if (_data[i] != other[i])
				{
					return false;
				}
			}

			return true;
		}

		constexpr bool operator!=(const auto& other) const
		{
			return !operator==(other);
		}
	};
}

#endif
