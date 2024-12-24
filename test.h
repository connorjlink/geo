#ifndef GEO_TEST_H
#define GEO_TEST_H

#include <iostream>
#include <functional>

#include <Windows.h>

namespace geo
{
	template<typename T, typename... Ts>
	bool validate(const std::string& name, T(*func)(Ts...), T expected, Ts... args)
	{
		const auto actual = func(args...);

		if (actual != expected)
		{
			auto message = std::format("Test validation failed: `{}`", name);
			MessageBoxA(NULL, message.c_str(), "Validation Failure", MB_OK);
			return false;
		}

		return true;
	}
}

#endif
