#include <iostream>
#include <iomanip>
#include <bitset>
#include <bit>
#include <format>
#include <random>

#include "ValuePack.h"

int main()
{
	ValuePack<int32_t, 4> vals{ 25 };
	ValuePack<int32_t, 4> shift{ 1, 2, 3, 4 };

	vals >>= shift;

	std::cout << vals << '\n';
}