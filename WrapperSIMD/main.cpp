#include <format>
#include <print>

#include "ValuePack.h"

int main()
{
	ValuePack<float, 8> pack{ -0.1f, 0.2f, 0.3f, 0.4f, 0.5f, -0.6f, 0.7f, 0.8f };
	auto inv = -pack;

	std::print("{}", inv);
}