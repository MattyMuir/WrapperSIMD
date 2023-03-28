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
	ValuePack<float, 8> b { 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f };
}