#include <print>
#include <random>
#include <limits>

#include "ValuePack.h"

// === Parameters ===
#define OP *
#define FUNC sqrt
// ==================

#define stringify_impl(x) #x
#define stringify(x) stringify_impl(x)

double RandomDouble()
{
	static std::mt19937_64 gen{ std::random_device{}() };
	static std::uniform_real_distribution<double> dist { log(DBL_MIN), log(DBL_MAX) };

	double x = exp(dist(gen));
	return (gen() % 2) ? x : -x;
}

ValuePack<int64_t, 4> GetExponent(ValuePack<double, 4> pack)
{
	ValuePack<uint64_t, 4> punt = pack.Cast<uint64_t>();
	punt >>= 52;
	return (punt & ValuePack<uint64_t, 4>::RepVal(0b111'1111'1111)).Cast<int64_t>();
}

ValuePack<double, 4> Next(ValuePack<double, 4> pack)
{
	// This function is broken with input -0.0
	ValuePack<int64_t, 4> incr = ((pack >= 0).Cast<int64_t>() & 2) - 1;
	ValuePack<int64_t, 4> isFinite = (GetExponent(pack) == 2047).Cast<int64_t>() ^ -1;
	incr &= isFinite;

	return (pack.Cast<int64_t>() + incr).Cast<double>();
}

ValuePack<double, 4> Prev(ValuePack<double, 4> pack)
{
	// This function is broken with input -0.0
	ValuePack<int64_t, 4> incr = ((pack < 0).Cast<int64_t>() & 2) - 1;
	ValuePack<int64_t, 4> isFinite = (GetExponent(pack) == 2047).Cast<int64_t>() ^ -1;
	incr &= isFinite;

	return (pack.Cast<int64_t>() + incr).Cast<double>();
}

double Next(double x)
{
	return std::nextafter(x, std::numeric_limits<double>::infinity());
}

double Prev(double x)
{
	return std::nextafter(x, -std::numeric_limits<double>::infinity());
}

int main()
{
	for (;;)
	{
		ValuePack<double, 4> a{ RandomDouble(), RandomDouble(), RandomDouble(), RandomDouble() };

		ValuePack<double, 4> res1 = Prev(a);
		ValuePack<double, 4> res2 = { Prev(a[0]), Prev(a[1]), Prev(a[2]), Prev(a[3]) };
		if (!(res1 == res2)) std::println("Error: {}", a);
	}
}