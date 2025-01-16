#include "test.h"
#include "types.h"
#include "float.h"
#include "base.h"
#include "vector.h"
#include "matrix.h"

#include <Windows.h>

//#define VALIDATE(x, y, z, ...) geo::validate(x, y, z, __VA_ARGS)
#define VALIDATE(x) MessageBoxA(NULL, x, "Validation Error", MB_OK)

int APIENTRY wWinMain(_In_     HINSTANCE hinstance,
					  _In_opt_ HINSTANCE hprevinstance,
					  _In_     LPWSTR    lpcmdline,
					  _In_     INT       ncmdshow)
{
#pragma region VECTOR_VALIDATION
	if (geo::broadcast(1.0f) != geo::vec4{ 1.0f, 1.0f, 1.0f, 1.0f })
	{
		VALIDATE("broadcast(1)");
		return -1;
	}

	else if (geo::broadcast(2.0f) != geo::vec4{ 2.0f, 2.0f, 2.0f, 2.0f })
	{
		VALIDATE("broadcast(2)");
		return 1;
	}


	else if (geo::truncate(geo::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }) != geo::vec3{ 1.0f, 1.0f, 1.0f })
	{
		VALIDATE("truncate({ 1, 1, 1, 1 })");
		return 2;
	}

	else if (geo::truncate(geo::vec4{ 1.0f, 2.0f, 3.0f, 4.0f }) != geo::vec3{ 1.0f, 2.0f, 3.0f })
	{
		VALIDATE("truncate({ 1, 2, 3, 4 })");
		return 3;
	}


	else if (geo::extend(geo::vec3{ 1.0f, 2.0f, 3.0f }) != geo::vec4{ 1.0f, 2.0f, 3.0f, 1.0f })
	{
		VALIDATE("extend({ 1, 2, 3 })");
		return 4;
	}

	else if (geo::extend(geo::vec3{ 2.0f, 3.0f, 4.0f }) != geo::vec4{ 2.0f, 3.0f, 4.0f, 1.0f })
	{
		VALIDATE("extend({ 2, 3, 4 })");
		return 5;
	}


	else if (geo::scale(geo::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }, 2.0f) != geo::vec4{ 2.0f, 2.0f, 2.0f, 2.0f })
	{
		VALIDATE("scale({ 1, 1, 1, 1 }, 2)");
		return 6;
	}

	else if (geo::scale(geo::vec4{ 1.0f, 2.0f, 3.0f, 4.0f }, 2.0f) != geo::vec4{ 2.0f, 4.0f, 6.0f, 8.0f })
	{
		VALIDATE("scale({ 1, 2, 3, 4 }, 2)");
		return 7;
	}


	else if (geo::invert(geo::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }) != geo::vec4{ -1.0f, -1.0f, -1.0f, -1.0f })
	{
		VALIDATE("invert({ 1, 1, 1, 1 })");
		return 8;
	}

	else if (geo::invert(geo::vec4{ 1.0f, 2.0f, 3.0f, 4.0f }) != geo::vec4{ -1.0f, -2.0f, -3.0f, -4.0f })
	{
		VALIDATE("invert({ 1, 2, 3, 4 })");
		return 9;
	}


	else if (geo::add(geo::vec4{ 1.0f, 1.0f, 1.0f, 1.0f },
				      geo::vec4{ 1.0f, 2.0f, 3.0f, 4.0f }) != geo::vec4{ 2.0f, 3.0f, 4.0f, 5.0f })
	{
		VALIDATE("add({ 1, 1, 1, 1 }, { 1, 2, 3, 4 })");
		return 10;
	}

	else if (geo::add(geo::vec4{ 1.0f, 1.0f, 1.0f, 1.0f },
					  geo::vec4{ 2.0f, 3.0f, 4.0f, 5.0f }) != geo::vec4{ 3.0f, 4.0f, 5.0f, 6.0f })
	{
		VALIDATE("add({ 1, 1, 1, 1 }, { 2, 3, 4, 5 })");
		return 11;
	}


	else if (geo::subtract(geo::vec4{ 1.0f, 1.0f, 1.0f, 1.0f },
						   geo::vec4{ 1.0f, 2.0f, 3.0f, 4.0f }) != geo::vec4{ 0.0f, -1.0f, -2.0f, -3.0f })
	{
		VALIDATE("subtract({ 1, 1, 1, 1 }, { 1, 2, 3, 4 })");
		return 12;
	}

	else if (geo::subtract(geo::vec4{ 1.0f, 2.0f, 3.0f, 4.0f },
						   geo::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }) != geo::vec4{ 0.0f, 1.0f, 2.0f, 3.0f })
	{
		VALIDATE("subtract({ 1, 2, 3, 4 }, { 1, 1, 1, 1 })");
		return 13;
	}


	else if (geo::multiply(geo::vec4{ 1.0f, 1.0f, 1.0f, 1.0f },
						   geo::vec4{ 1.0f, 2.0f, 3.0f, 4.0f }) != geo::vec4{ 1.0f, 2.0f, 3.0f, 4.0f })
	{
		VALIDATE("multiply({ 1, 1, 1, 1 }, { 1, 2, 3, 4 })");
		return 14;
	}

	else if (geo::multiply(geo::vec4{ 2.0f, 2.0f, 2.0f, 2.0f },
						   geo::vec4{ 1.0f, 2.0f, 3.0f, 4.0f }) != geo::vec4{ 2.0f, 4.0f, 6.0f, 8.0f })
	{
		VALIDATE("multiply({ 2, 2, 2, 2 }, { 1, 2, 3, 4 })");
		return 15;
	}


	else if (geo::total(geo::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }) != 4.0f)
	{
		VALIDATE("total({ 1, 1, 1, 1 })");
		return 16;
	}

	else if (geo::total(geo::vec4{ 1.0f, 2.0f, 3.0f, 4.0f }) != 10.0f)
	{
		VALIDATE("total({ 1, 2, 3, 4 })");
		return 17;
	}


	else if (geo::magnitude(geo::vec4{ 1.0f, 0.0f, 0.0f, 0.0f }) != 1.0f)
	{
		VALIDATE("magnitude({ 1, 0, 0, 0 })");
		return 18;
	}

	else if (geo::magnitude(geo::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }) != 2.0f)
	{
		VALIDATE("magnitude({ 1, 1, 1, 1 })");
		return 19;
	}


	else if (geo::normalize(geo::vec4{ 1.0f, 0.0f, 0.0f, 0.0f }) != geo::vec4{ 1.0f, 0.0f, 0.0f, 0.0f })
	{
		VALIDATE("normalize({ 1, 0, 0, 0 })");
		return 20;
	}

	else if (geo::normalize(geo::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }) != geo::vec4{ 0.5f, 0.5f, 0.5f, 0.5f })
	{
		VALIDATE("normalize({ 1, 1, 1, 1 })");
		return 21;
	}


	else if (geo::distance(geo::vec4{ 1.0f, 1.0f, 1.0f, 1.0f },
						   geo::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }) != 0.0f)
	{
		VALIDATE("distance({ 1, 1, 1, 1 }, { 1, 1, 1, 1 })");
		return 22;
	}

	else if (geo::distance(geo::vec4{ 1.0f, 1.0f, 1.0f, 1.0f },
						   geo::vec4{ 3.0f, 3.0f, 3.0f, 3.0f }) != 4.0f)
	{
		VALIDATE("distance({ 1, 1, 1, 1 }, { 3, 3, 3, 3 })");
		return 23;
	}
	

	else if (geo::dot(geo::vec4{ 0.0f, 0.0f, 0.0f, 0.0f },
					  geo::vec4{ 1.0f, 1.0f, 1.0f, 1.0f }) != 0.0f)
	{
		VALIDATE("dot({ 0, 0, 0, 0 }, { 1, 1, 1, 1 })");
		return 24;
	}

	else if (geo::dot(geo::vec4{ 1.0f, 1.0f, 1.0f, 1.0f },
					  geo::vec4{ 1.0f, 2.0f, 3.0f, 4.0f }) != 10.0f)
	{
		VALIDATE("dot({ 1, 1, 1, 1 }, { 1, 2, 3, 4 })");
		return 25;
	}


	else if (geo::cross(geo::vec3{ 1.0f, 1.0f, 1.0f },
						geo::vec3{ 1.0f, 2.0f, 3.0f }) != geo::vec3{ 1.0f, -2.0f, 1.0f })
	{
		VALIDATE("cross({ 1, 1, 1 }, { 1, 2, 3 })");
		return 26;
	}

	else if (geo::cross(geo::vec3{ 1.0f, 0.0f, 0.0f },
						geo::vec3{ 1.0f, 1.0f, 1.0f }) != geo::vec3{ 0.0f, -1.0f, 1.0f })
	{
		VALIDATE("cross({ 1, 0, 0 }, { 1, 1, 1 })");
		return 27; 
	}
#pragma endregion

#define A std::array<float, 4>

#pragma region MATRIX_VALIDATION
	if (geo::identity() != geo::mat4{ A{ 1.0f, 0.0f, 0.0f, 0.0f },
									  A{ 0.0f, 1.0f, 0.0f, 0.0f },
									  A{ 0.0f, 0.0f, 1.0f, 0.0f },
									  A{ 0.0f, 0.0f, 0.0f, 1.0f } })
	{
		VALIDATE("identity()");
		return 28;
	}


	else if (geo::null() != geo::mat4{ A{ 0.0f, 0.0f, 0.0f, 0.0f },
									   A{ 0.0f, 0.0f, 0.0f, 0.0f },
									   A{ 0.0f, 0.0f, 0.0f, 0.0f },
									   A{ 0.0f, 0.0f, 0.0f, 0.0f } })
	{
		VALIDATE("null()");
		return 29;
	}

	return 0;
}