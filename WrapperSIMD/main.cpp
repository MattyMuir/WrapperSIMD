#include <print>

#include "ValuePack.h"

int main()
{
	ValuePack<float, 8> pack{ 3.0f, -6.0f, 9.0f, 12.0f, 15.0f, 18.0f, 21.0f, 24.0f };
	pack = abs(pack);
	for (size_t i = 0; i < pack.Size(); i++)
		std::println("{}", pack[i]);
}