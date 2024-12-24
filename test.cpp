#include "test.h"
#include "types.h"
#include "float.h"
#include "base.h"
#include "vector.h"
#include "matrix.h"

#include <Windows.h>

#define VALIDATE(x, y, z, ...) geo::validate(x, y, z, __VA_ARGS)

int APIENTRY wWinMain(_In_     HINSTANCE hinstance,
					  _In_opt_ HINSTANCE hprevinstance,
					  _In_     LPWSTR    lpcmdline,
					  _In_     INT       ncmdshow)
{
	geo::validate("broadcast@vector", geo::broadcast, geo::vec{ 1.0f, 1.0f, 1.0f, 1.0f }, 1.0f);
	geo::validate("broadcast@vector", geo::broadcast, geo::vec{ 2.0f, 2.0f, 2.0f, 3.0f }, 2.0f);


	return 0;
}