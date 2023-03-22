#pragma once
#include <cassert>
#include <iostream>
#include <type_traits>
#include <format>

#include <intrin.h>

#define RETURN_OP_WITH_SIZE(bits, op, type, ...) {\
if constexpr (std::is_same_v<type, int8_t>)\
	return _mm##bits##_##op##_epi8(__VA_ARGS__);\
if constexpr (std::is_same_v<type, uint8_t>)\
	return _mm##bits##_##op##_epu8(__VA_ARGS__);\
if constexpr (std::is_same_v<type, int16_t>)\
	return _mm##bits##_##op##_epi16(__VA_ARGS__);\
if constexpr (std::is_same_v<type, uint16_t>)\
	return _mm##bits##_##op##_epu16(__VA_ARGS__);\
if constexpr (std::is_same_v<type, int32_t>)\
	return _mm##bits##_##op##_epi32(__VA_ARGS__);\
if constexpr (std::is_same_v<type, uint32_t>)\
	return _mm##bits##_##op##_epu32(__VA_ARGS__);\
if constexpr (std::is_same_v<type, int64_t>)\
	return _mm##bits##_##op##_epi64(__VA_ARGS__);\
if constexpr (std::is_same_v<type, uint64_t>)\
	return _mm##bits##_##op##_epu64(__VA_ARGS__);\
if constexpr (std::is_same_v<type, float>)\
	return _mm##bits##_##op##_ps(__VA_ARGS__);\
if constexpr (std::is_same_v<type, double>)\
	return _mm##bits##_##op##_pd(__VA_ARGS__);}

#define RETURN_OP(is256, op, type, ...)\
if constexpr (is256)\
{\
	RETURN_OP_WITH_SIZE(256, op, type, __VA_ARGS__);\
}\
else\
{\
	RETURN_OP_WITH_SIZE(, op, type, __VA_ARGS__);\
}\

#define ADD_OP_METHOD(op, mmOpName)\
inline ValuePack operator##op##(ValuePack other) const\
{\
	RETURN_OP(is256, mmOpName, ValTy, pack, other.pack);\
}

#define ADD_IN_PLACE_METHOD(op)\
inline ValuePack& operator##op##=(ValuePack other)\
{\
	pack = ((*this) op other).pack;\
	return *this;\
}

#define ADD_SCALAR_METHOD(op)\
inline ValuePack operator##op##(ValTy x) const\
{\
	return (*this) op ValuePack::RepVal(x);\
}

#define ADD_IN_PLACE_SCALAR_METHOD(op)\
inline ValuePack& operator##op##=(ValTy x)\
{\
	pack = ((*this) op ValuePack::RepVal(x)).pack;\
	return *this;\
}

#define ADD_FREE_FUNC(funcName, mmOpName)\
template <typename ValTy, size_t PackSize>\
inline ValuePack<ValTy, PackSize> funcName (ValuePack<ValTy, PackSize> pack)\
{\
	RETURN_OP(pack.is256, mmOpName, ValTy, pack.pack);\
}

#define ADD_FREE_FRIEND(funcName)\
template <typename ValTy, size_t PackSize>\
friend ValuePack<ValTy, PackSize> funcName (ValuePack<ValTy, PackSize> pack);

#define ADD_FREE_FUNC_2ARG(funcName, mmOpName)\
template <typename ValTy, size_t PackSize>\
inline ValuePack<ValTy, PackSize> funcName (ValuePack<ValTy, PackSize> pack1, ValuePack<ValTy, PackSize> pack2)\
{\
	RETURN_OP(pack1.is256, mmOpName, ValTy, pack1.pack, pack2.pack);\
}

#define ADD_FREE_FRIEND_2ARG(funcName)\
template <typename ValTy, size_t PackSize>\
friend ValuePack<ValTy, PackSize> funcName (ValuePack<ValTy, PackSize> pack1, ValuePack<ValTy, PackSize> pack2);

template<typename ValTy, typename T>
concept IsValTy = std::is_convertible_v<T, ValTy>;

template <typename ValTy, size_t PackSize>
class ValuePack
{
protected:
	// Check if type is supported
	static_assert(std::is_integral_v<ValTy> || std::is_same_v<ValTy, float> || std::is_same_v<ValTy, double>,
		"Only integral, float, and double types are supported");

	// Check if pack size is valid
	static_assert(sizeof(ValTy)* PackSize == 16 || sizeof(ValTy) * PackSize == 32, "Total pack size must be 128 or 256 bits");

	static constexpr bool is256 = (sizeof(ValTy) * PackSize == 32);

	using PackTy = std::conditional_t<std::is_integral_v<ValTy>,
		// Integers
		std::conditional_t<!is256, __m128i, __m256i>,
		
		// Floating point
		std::conditional_t<std::is_same_v<ValTy, float>,
			// Float
			std::conditional_t<PackSize == 4, __m128, __m256>,

			// Double
			std::conditional_t<PackSize == 2, __m128d, __m256d>>>;

public:

	// == Constructors ==
	ValuePack() {}

	ValuePack(PackTy pack_)
		: pack(pack_) {}

	template <IsValTy<ValTy>... Vals>
	ValuePack(ValTy first, Vals... others)
	{
		pack = Set(first, others...).pack;
	}

	ValuePack(ValTy x)
	{
		pack = ValuePack::RepVal(x).pack;
	}

	// == Generators ==
	template <IsValTy<ValTy>... Vals>
	static ValuePack Set(Vals... vals)
	{
		RETURN_OP(is256, set, ValTy, vals...);
	}

	static ValuePack RepVal(ValTy x)
	{
		if constexpr (std::is_unsigned_v<ValTy>)
		{
			// Type is unsigned, use the signed function with a bit_cast
			using OpTy = std::make_signed_t<ValTy>;
			RETURN_OP(is256, set1, OpTy, std::bit_cast<OpTy>(x));
		}
		else
		{
			RETURN_OP(is256, set1, ValTy, x);
		}
	}

	// == Special members ==
	consteval size_t Size()
	{
		return PackSize;
	}

	// Array access operator
	ValTy operator[](size_t idx) const
	{
#ifdef _DEBUG
		assert(idx >= 0 && idx < PackSize);
#endif
		return ((ValTy*)&pack)[idx];
	}

	using SumType = decltype(ValTy{} + ValTy{});
	SumType Sum() const
	{
		SumType sum = (*this)[0];
		for (size_t i = 1; i < PackSize; i++)
			sum += (*this)[i];
		return sum;
	}

	template<typename To>
	ValuePack<To, PackSize> Convert()
	{
		static constexpr bool cvtIs256 = is256 || (sizeof(To) * PackSize == 32);

		// Types are the same, no conversion necessary
		if constexpr (std::is_same_v<ValTy, To>) return (*this);

		if constexpr (std::is_same_v<ValTy, int8_t>)
			RETURN_OP(cvtIs256, cvtepi8, To, pack);
		if constexpr (std::is_same_v<ValTy, uint8_t>)
			RETURN_OP(cvtIs256, cvtepu8, To, pack);
		if constexpr (std::is_same_v<ValTy, int16_t>)
			RETURN_OP(cvtIs256, cvtepi16, To, pack);
		if constexpr (std::is_same_v<ValTy, uint16_t>)
			RETURN_OP(cvtIs256, cvtepu16, To, pack);
		if constexpr (std::is_same_v<ValTy, int32_t>)
			RETURN_OP(cvtIs256, cvtepi32, To, pack);
		if constexpr (std::is_same_v<ValTy, uint32_t>)
			RETURN_OP(cvtIs256, cvtepu32, To, pack);
		if constexpr (std::is_same_v<ValTy, int64_t>)
			RETURN_OP(cvtIs256, cvtepi64, To, pack);
		if constexpr (std::is_same_v<ValTy, uint64_t>)
			RETURN_OP(cvtIs256, cvtepu64, To, pack);
		if constexpr (std::is_same_v<ValTy, float>)
			RETURN_OP(cvtIs256, cvtps, To, pack);
		if constexpr (std::is_same_v<ValTy, double>)
			RETURN_OP(cvtIs256, cvtpd, To, pack);
	}

	// == Numerical operations ==
	// With other packs
	ADD_OP_METHOD(+, add);
	ADD_OP_METHOD(-, sub);
	ADD_OP_METHOD(*, mul);
	ADD_OP_METHOD(/, div);
	ADD_OP_METHOD(%, rem);

	// In place with packs
	ADD_IN_PLACE_METHOD(+);
	ADD_IN_PLACE_METHOD(-);
	ADD_IN_PLACE_METHOD(*);
	ADD_IN_PLACE_METHOD(/);
	ADD_IN_PLACE_METHOD(%);

	// With scalars
	ADD_SCALAR_METHOD(+);
	ADD_SCALAR_METHOD(-);
	ADD_SCALAR_METHOD(*);
	ADD_SCALAR_METHOD(/);
	ADD_SCALAR_METHOD(%);

	// In place with scalars
	ADD_IN_PLACE_SCALAR_METHOD(+);
	ADD_IN_PLACE_SCALAR_METHOD(-);
	ADD_IN_PLACE_SCALAR_METHOD(*);
	ADD_IN_PLACE_SCALAR_METHOD(/ );
	ADD_IN_PLACE_SCALAR_METHOD(%);

	// == Free function friends ==
	// Trig functions
	ADD_FREE_FRIEND(sin);
	ADD_FREE_FRIEND(cos);
	ADD_FREE_FRIEND(tan);
	ADD_FREE_FRIEND(asin);
	ADD_FREE_FRIEND(acos);
	ADD_FREE_FRIEND(atan);

	// Exp functions
	ADD_FREE_FRIEND(exp);
	ADD_FREE_FRIEND(log);
	ADD_FREE_FRIEND(log2);
	ADD_FREE_FRIEND(log10);
	ADD_FREE_FRIEND(sqrt);
	ADD_FREE_FRIEND_2ARG(pow);

	// Other functions
	ADD_FREE_FRIEND(abs);

	ADD_FREE_FRIEND(floor);
	ADD_FREE_FRIEND(round);
	ADD_FREE_FRIEND(ceil);

	ADD_FREE_FRIEND_2ARG(min);
	ADD_FREE_FRIEND_2ARG(max);
	
	PackTy pack;
};

// == Free functions ==
// Trig functions
ADD_FREE_FUNC(sin, sin);
ADD_FREE_FUNC(cos, cos);
ADD_FREE_FUNC(tan, tan);
ADD_FREE_FUNC(asin, asin);
ADD_FREE_FUNC(acos, acos);
ADD_FREE_FUNC(atan, atan);

// Exp functions
ADD_FREE_FUNC(exp, exp);
ADD_FREE_FUNC(log, log);
ADD_FREE_FUNC(log2, log2);
ADD_FREE_FUNC(log10, log10);
ADD_FREE_FUNC(sqrt, sqrt);
ADD_FREE_FUNC_2ARG(pow, pow);

// Other functions
ADD_FREE_FUNC(abs, abs);

ADD_FREE_FUNC(floor, floor);
ADD_FREE_FUNC(round, round);
ADD_FREE_FUNC(ceil, ceil);

ADD_FREE_FUNC_2ARG(min, min);
ADD_FREE_FUNC_2ARG(max, max);

// Ostream operator
template <typename ValTy, size_t PackSize>
std::ostream& operator<<(std::ostream& os, const ValuePack<ValTy, PackSize>& pack)
{
	os << '[';
	for (size_t i = 0; i < PackSize; i++)
	{
		if (i > 0) os << ", ";
		if constexpr (std::is_integral_v<ValTy> && sizeof(ValTy) == 1)
			os << (int)pack[i];
		else
			os << pack[i];
	}
	os << ']';
	return os;
}