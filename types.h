#ifndef GEO_TYPES_H
#define GEO_TYPES_H

#include "float.h"
#include "matrix.h"

namespace geo
{
	template<typename T>
	using basic_mat2 = mat<2, 2, T>;

	template<typename T = platform_type>
	using mat2 = basic_mat2<T>;

	template<typename T>
	using basic_mat3 = mat<3, 3, T>;

	template<typename T = platform_type>
	using mat3 = basic_mat3<T>;

	template<typename T>
	using basic_mat4 = mat<4, 4, T>;

	template<typename T = platform_type>
	using mat4 = basic_mat4<T>;


	template<typename T>
	using basic_vec2 = vec<2, T>;

	template<typename T = platform_type>
	using vec2 = basic_vec2<T>;

	template<typename T>
	using basic_vec3 = vec<3, T>;

	template<typename T = platform_type>
	using vec3 = basic_vec3<T>;

	template<typename T>
	using basic_vec4 = vec<4, T>;

	template<typename T = platform_type>
	using vec4 = basic_vec4<T>;
}

#endif
