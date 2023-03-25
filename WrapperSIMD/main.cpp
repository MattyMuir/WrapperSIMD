#include <iostream>
#include <bitset>
#include <bit>
#include <format>

#include "ValuePack.h"

int main()
{
	ValuePack<double, 4> p1{ 17, -5, 3, 8 };
	ValuePack<double, 4> p2{ 17, -4, 3, -8 };

	auto bools = cmp<GREATER_EQUAL_NAN_TRUE>(p1, p2);

	std::cout << bools << '\n';
}