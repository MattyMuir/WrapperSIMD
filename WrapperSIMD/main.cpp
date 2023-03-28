#include <iostream>
#include <iomanip>
#include <bitset>
#include <bit>
#include <format>
#include <random>

#include "ValuePack.h"

int main()
{
	ValuePack<float, 8> a { 2.0f };

	auto r1 = invsqrt(a);
	auto r2 = invsqrt_approx(a);

	std::cout << std::format("{}\n", r1[0]);
	std::cout << std::format("{}\n", r2[0]);
}