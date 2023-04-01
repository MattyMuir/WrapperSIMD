#include <iostream>
#include <format>

#include "ValuePack.h"

int main()
{
	float first = rand();
	float incr = rand();

	ValuePack p = ValuePack<float, 8>::Range(first, incr);
	std::cout << p << '\n';
}