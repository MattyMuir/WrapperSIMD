#include <iostream>

#include "ValuePack.h"

int main()
{
	ValuePack<double, 4> pack{ 1.7 };

	auto res = cos(pack) * sin(pack);
	std::cout << res << '\n';
}