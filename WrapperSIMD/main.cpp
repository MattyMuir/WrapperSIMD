#include <print>
#include <random>
#include <limits>
#include <compare>

#include "ValuePack.h"

int main()
{
	ValuePack<double, 4> pack{ 1.0, 2.0, -3.0, 4.0 };
	ValuePack<double, 4> nudged = next(pack);

	std::println("{}", nudged);
}