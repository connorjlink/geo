#ifndef GEO_VECTOR_H
#define GEO_VECTOR_H

#include "base.h"
#include "matrix.h"

#include <algorithm>
#include <numeric>

namespace geo
{
	template<std::size_t M = 4, typename T = platform_type>
	constexpr vec<M, T> broadcast(T scalar)
	{
		vec<M, T> out{};

		for (auto i = 0; i < M; i++)
		{
			out[i] = scalar;
		}

		return out;
	}

	template<std::size_t M2 = 3, std::size_t M1 = 4, typename T = platform_type>
		requires (M2 < M1)
	constexpr vec<M2, T> truncate(const vec<M1, T>& vector)
	{
		vec<M2, T> out{};

		for (auto i = 0; i < M2; i++)
		{
			out[i] = vector[i];
		}

		return out;
	}

	template<std::size_t M2 = 4, std::size_t M1, typename T = platform_type>
		requires (M2 > M1)
	constexpr vec<M2, T> extend(const vec<M1, T>& vector, T scalar = T{ 1 })
	{
		vec<M2, T> out{};

		for (auto i = 0; i < M1; i++)
		{
			out[i] = vector[i];
		}

		for (auto i = M1; i < M2; i++)
		{
			out[i] = scalar;
		}

		return out;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr vec<M, T> scale(const vec<M, T>& vector, T scalar)
	{
		vec<M, T> out{};

		for (auto i = 0; i < M; i++)
		{
			out[i] = vector[i] * scalar;
		}

		return out;
	}


	// reverse the direction of the given input vector
	template<std::size_t M = 4, typename T = platform_type>
	constexpr vec<M, T> invert(const vec<M, T>& vector)
	{
		const auto result = scale(vector, T{ -1 });
		return result;
	}

	template<std::size_t M = 4, typename T = platform_type, typename U>
	constexpr vec<M, T> _accumulate(const vec<M, T>& vector1, const vec<M, T>& vector2)
	{
		vec<M, T> out{};

		for (auto i = 0; i < M; i++)
		{
			out[i] = U{}(vector1[i], vector2[i]);
		}

		return out;
	}

	// element-wise sum of all components of two vectors
	template<std::size_t M = 4, typename T = platform_type>
	constexpr vec<M, T> add(const vec<M, T>& vector1, const vec<M, T>& vector2)
	{
		return _accumulate<M, T, std::plus<T>>(vector1, vector2);
	}

	// element-wise difference of all components of two vectors
	template<std::size_t M = 4, typename T = platform_type>
	constexpr vec<M, T> subtract(const vec<M, T>& vector1, const vec<M, T>& vector2)
	{
		return _accumulate<M, T, std::minus<T>>(vector1, vector2);
	}

	// element-wise product of all components of two vectors
	template<std::size_t M = 4, typename T = platform_type>
	constexpr vec<M, T> multiply(const vec<M, T>& vector1, const vec<M, T>& vector2)
	{
		return _accumulate<M, T, std::multiplies<T>>(vector1, vector2);
	}

	// element-wise summation of all components of a vector
	template<std::size_t M = 4, typename T = platform_type>
	constexpr T total(const vec<M, T>& vector)
	{
		auto out = T{ 0 };

		for (auto i = 0; i < M; i++)
		{
			out += vector[i];
		}

		return out;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr T magnitude(const vec<M, T>& vector)
	{
		const auto product = multiply(vector, vector);
		const auto sum = total(product);
		const auto root = std::sqrt(sum);

		return root;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr vec<M, T> normalize(const vec<M, T>& vector)
	{
		const auto length = magnitude(vector);
		const auto factor = 1 / length;
		const auto result = scale(vector, factor);

		return result;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr T distance(const vec<M, T>& vector1, const vec<M, T>& vector2)
	{
		const auto difference = subtract(vector2, vector1);
		const auto length = magnitude(difference);

		return length;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr T dot(const vec<M, T>& vector1, const vec<M, T>& vector2)
	{
		const auto product = multiply(vector1, vector2);
		const auto sum = total(product);

		return sum;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr void _update_js(std::array<std::size_t, M>& js)
	{
		for (auto k = 0; k < M; k++)
		{
			// increment and clamp the range so indices wrap back around
			js[k]++;
			js[k] %= M;
		}
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr T _diagonal_product(const mat<M, M, T>& matrix,
		const std::array<std::size_t, M>& is, const std::array<std::size_t, M>& js)
	{
		auto product = T{ 1 };

		// for each element each diagonal
		for (auto k = 0; k < M; k++)
		{
			const auto i = is[k];
			const auto j = js[k];

			product *= matrix[i][j];
		}

		return product;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr vec<M, T> _forward_diagonals(const mat<M, M, T>& matrix)
	{
		vec<M, T> out{};

		std::array<std::size_t, M> is;
		std::iota(is.begin(), is.end(), 0);

		std::array<std::size_t, M> js;
		std::iota(js.begin(), js.end(), 0);

		for (auto k = 0; k < M; k++)
		{
			out[k] = _diagonal_product(matrix, is, js);
			_update_js(js);
		}

		return out;
	}

	template<std::size_t M = 4, typename T = platform_type>
	constexpr vec<M, T> _backward_diagonals(const mat<M, M, T>& matrix)
	{
		vec<M, T> out;

		std::array<std::size_t, M> is;
		// NOTE: the following is a hack to emulate iota behavior for a decreasing range
		auto value = M - 1;
		std::generate(is.begin(), is.end(), [&value]() { return value--; });

		std::array<std::size_t, M> js;
		std::iota(js.begin(), js.end(), 0);
		// rotate left so it starts at 1 and wraps back at end
		std::rotate(std::begin(js), std::begin(js) + 1, std::end(js));

		for (auto k = 0; k < M; k++)
		{
			out[k] = _diagonal_product(matrix, is, js);
			_update_js(js);
		}

		return out;
	}

	template<std::size_t M = 4, typename T = platform_type>
		requires (M >= 3)
	constexpr vec<M, T> cross(const vec<M, T>& vector1, const vec<M, T>& vector2)
	{
		auto matrix = constant<M, M, T>(1);
		matrix[M - 2] = vector1;
		matrix[M - 1] = vector2;

		const auto forward = _forward_diagonals(matrix);
		const auto backward = _backward_diagonals(matrix);

		return subtract(forward, backward);
	}
}

#endif
