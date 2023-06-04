#include <format>
#include <print>

#include "ValuePack.h"
#include "Timer.h"

double ReciprocalCubesSum1(uint64_t start, uint64_t end)
{
	double sum = 0;
	for (uint64_t n = start; n <= end; n++)
		sum += pow((double)n, -1.0 / 3.0);

	return sum;
}

double ReciprocalCubesSum2(uint64_t start, uint64_t end)
{
	ValuePack pack = ValuePack<double, 4>::Range(start, 1.0);
	ValuePack<double, 4> sums{ 0.0 };
	for (uint64_t i = 0; i < (end + 1 - start) / 4; i++)
	{
		sums += invcbrt(pack);
		pack += 4.0;
	}

	return sum(sums);
}

double ReciprocalCubesSumApprox(uint64_t start, uint64_t end)
{
	static constexpr double zetaOneThird = -0.9733602483507827154688869;
	double sum = zetaOneThird + 1.5 * pow(end, 2.0 / 3.0) + 0.5 * pow(end, -1.0 / 3.0);

	for (uint64_t n = 1; n < start; n++)
		sum -= pow((double)n, -1.0 / 3.0);

	return sum;
}

int main()
{
	/*
	static constexpr uint64_t start = 1;
	static constexpr uint64_t end = 1e6;

	TIMER(naive);
	double sum1 = ReciprocalCubesSum1(start, end);
	STOP_LOG(naive);

	TIMER(simd);
	double sum2 = ReciprocalCubesSum2(start, end);
	STOP_LOG(simd);

	TIMER(approx);
	double approxSum = ReciprocalCubesSumApprox(start, end);
	STOP_LOG(approx);

	std::print("===========\n{:.15}\n{:.15}\n===========\n", sum1, sum2);
	std::print("Approx: {:.15}\n", approxSum);
	*/

	ValuePack<float, 8> pack{ -0.1f, 0.2f, 0.3f, 0.4f, 0.5f, -0.6f, 0.7f, 0.8f };
	auto inv = -pack;

	std::cout << inv << '\n';
}