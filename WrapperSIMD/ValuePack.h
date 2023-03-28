#pragma once
#include <cassert>
#include <iostream>
#include <type_traits>
#include <format>

#include <intrin.h>

enum ComparisonOperator
{
	EQUAL = 0x0,
	LESS = 0x11,
	LESS_EQUAL = 0x12,
	GREATER = 0x1E,
	GREATER_EQUAL = 0x1D,

	EQUAL_NAN_TRUE = 0x8,
	LESS_NAN_TRUE = 0x9,
	LESS_EQUAL_NAN_TRUE = 0xA,
	GREATER_NAN_TRUE = 0x6,
	GREATER_EQUAL_NAN_TRUE = 0x5,

	IS_FINITE = 0x7
};

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

#define ADD_BITWISE_METHOD(op, mmOpName)\
inline ValuePack operator##op##(ValuePack other) const\
{\
	if constexpr (std::is_integral_v<ValTy>)\
	{\
		if constexpr (is256)\
			return _mm256_##mmOpName##_si256(pack, other.pack);\
		else\
			return _mm_##mmOpName##_si128(pack, other.pack);\
	}\
	if constexpr (std::is_same_v<ValTy, float>)\
	{\
		if constexpr (is256)\
			return _mm256_##mmOpName##_ps(pack, other.pack);\
		else\
			return _mm_##mmOpName##_ps(pack, other.pack);\
	}\
	if constexpr (std::is_same_v<ValTy, double>)\
	{\
		if constexpr (is256)\
			return _mm256_##mmOpName##_pd(pack, other.pack);\
		else\
			return _mm_##mmOpName##_pd(pack, other.pack);\
	}\
}

#define ADD_COMP_OP(op, mmOpName, opCode)\
inline BoolPack<PackSize, sizeof(ValTy)> operator##op##(ValuePack other)\
{\
	if constexpr (std::is_integral_v<ValTy>)\
	{\
		RETURN_OP(is256, mmOpName, ValTy, pack, other.pack);\
	}\
	else\
	{\
		RETURN_OP(is256, cmp, ValTy, pack, other.pack, opCode);\
	}\
}

#define ADD_COMP_OP_SCALAR(op)\
inline BoolPack<PackSize, sizeof(ValTy)> operator##op##(ValTy x)\
{\
	return (*this) op RepVal(x);\
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

template <size_t NumElem, size_t ElemSize>
class BoolPack
{
protected:
	static_assert(NumElem * ElemSize == 16 || NumElem * ElemSize == 32, "Invalid BoolPack size");
	static_assert(ElemSize == 1 || ElemSize == 2 || ElemSize == 4 || ElemSize == 8, "Invalid element size in BoolPack");
	using ElemType =	std::conditional_t<ElemSize == 1, uint8_t,
						std::conditional_t<ElemSize == 2, uint16_t,
						std::conditional_t<ElemSize == 4, uint32_t, uint64_t>>>;

	static constexpr bool is256 = (NumElem * ElemSize == 32);

public:
	template<typename PackType>
	BoolPack(PackType pack)
		: d(std::bit_cast<Data>(pack)) {}

	bool operator[](size_t idx) const
	{
		return (bool)d.vals[idx];
	}

	inline operator bool() const
	{
		return All();
	}

	inline bool All() const
	{
		if constexpr (is256)
		{
			__m256i& pack = *(__m256i*) & d;
			return (bool)_mm256_test_all_ones(pack);
		}
		else
		{
			__m128i& pack = *(__m128i*) & d;
			return (bool)_mm_test_all_ones(std::bit_cast<__m128i>(d));
		}
	}

	inline bool None() const
	{
		if constexpr (is256)
		{
			__m256i& pack = *(__m256i*) & d;
			return (bool)_mm256_testz_si256(pack, _mm256_cmpeq_epi32(pack, pack));
		}
		else
		{
			__m128i& pack = *(__m128i*) & d;
			return (bool)_mm_testz_si128(pack, _mm_cmpeq_epi32(pack, pack));
		}
	}

protected:
	struct Data
	{
		ElemType vals[NumElem];
	};

	alignas(NumElem* ElemSize) Data d;
};

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

	using SumType = decltype(ValTy{} + ValTy{});

public:

	// == Constructors ==
	ValuePack() {}

	ValuePack(PackTy pack_)
		: pack(pack_) {}

	template <IsValTy<ValTy>... Vals>
	ValuePack(ValTy first, Vals... others)
	{
		static_assert(sizeof...(others) == PackSize - 1, "Incorrect number of initializer values for ValuePack");
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
		// Large pack of int64_t or uint64_t, different 'set' function required
		if constexpr (is256 && std::is_same_v<ValTy, int64_t>)
			return _mm256_setr_epi64x(vals...);
		else if constexpr (is256 && std::is_same_v<ValTy, uint64_t>)
			return _mm256_setr_epi64x(static_cast<int64_t>(vals)...);

		else if constexpr (std::is_unsigned_v<ValTy>)
		{
			// Type is unsigned, use the signed function with a cast
			using OpTy = std::make_signed_t<ValTy>;
			RETURN_OP(is256, setr, OpTy, static_cast<OpTy>(vals)...);
		}
		else
		{
			RETURN_OP(is256, setr, ValTy, vals...);
		}
	}

	static ValuePack RepVal(ValTy x)
	{
		// Large pack of int64_t or uint64_t, different 'set' function required
		if constexpr (is256 && std::is_same_v<ValTy, int64_t>)
			return _mm256_set1_epi64x(x);
		else if constexpr (is256 && std::is_same_v<ValTy, uint64_t>)
			return _mm256_set1_epi64x(static_cast<int64_t>(x));

		else if constexpr (std::is_unsigned_v<ValTy>)
		{
			// Type is unsigned, use the signed function with a cast
			using OpTy = std::make_signed_t<ValTy>;
			RETURN_OP(is256, set1, OpTy, static_cast<OpTy>(x));
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
	ValTy& operator[](size_t idx) const
	{
#ifdef _DEBUG
		assert(idx >= 0 && idx < PackSize);
#endif
		return ((ValTy*)&pack)[idx];
	}

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

		// Converting (signed -> unsigned) or vice versa
		if constexpr (std::is_same_v<std::make_unsigned_t<ValTy>, std::make_unsigned_t<To>>) return pack;

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
	ADD_BITWISE_METHOD(&, and);
	ADD_BITWISE_METHOD(|, or);
	ADD_BITWISE_METHOD(^, xor);

	// In place with packs
	ADD_IN_PLACE_METHOD(+);
	ADD_IN_PLACE_METHOD(-);
	ADD_IN_PLACE_METHOD(*);
	ADD_IN_PLACE_METHOD(/);
	ADD_IN_PLACE_METHOD(%);
	ADD_IN_PLACE_METHOD(<<);
	ADD_IN_PLACE_METHOD(>>);
	ADD_IN_PLACE_METHOD(&);
	ADD_IN_PLACE_METHOD(|);
	ADD_IN_PLACE_METHOD(^);

	// With scalars
	ADD_SCALAR_METHOD(+);
	ADD_SCALAR_METHOD(-);
	ADD_SCALAR_METHOD(*);
	ADD_SCALAR_METHOD(/);
	ADD_SCALAR_METHOD(%);
	ADD_SCALAR_METHOD(&);
	ADD_SCALAR_METHOD(|);
	ADD_SCALAR_METHOD(^);

	// In place with scalars
	ADD_IN_PLACE_SCALAR_METHOD(+);
	ADD_IN_PLACE_SCALAR_METHOD(-);
	ADD_IN_PLACE_SCALAR_METHOD(*);
	ADD_IN_PLACE_SCALAR_METHOD(/);
	ADD_IN_PLACE_SCALAR_METHOD(%);
	ADD_IN_PLACE_SCALAR_METHOD(&);
	ADD_IN_PLACE_SCALAR_METHOD(|);
	ADD_IN_PLACE_SCALAR_METHOD(^);

	// === Comparison operators ===
	ADD_COMP_OP(==, cmpeq, EQUAL);
	ADD_COMP_OP(>, cmpgt, GREATER);
	ADD_COMP_OP(<, cmplt, LESS);
	ADD_COMP_OP(>=, cmpge, GREATER_EQUAL);
	ADD_COMP_OP(<=, cmple, LESS_EQUAL);

	ADD_COMP_OP_SCALAR(==);
	ADD_COMP_OP_SCALAR(>);
	ADD_COMP_OP_SCALAR(<);
	ADD_COMP_OP_SCALAR(>=);
	ADD_COMP_OP_SCALAR(<=);

	// Shifting
	inline ValuePack operator<<(ValuePack other) const
	{
		using UValTy = std::make_signed_t<ValTy>;
		RETURN_OP(is256, sllv, UValTy, pack, other.pack);
	}
	inline ValuePack operator>>(ValuePack other) const
	{
		static_assert(!std::is_same_v<ValTy, int64_t>, "Arithmetic right shift is not supported on signed 64 bit integers in SSE / AVX");

		if constexpr (std::is_unsigned_v<ValTy>)
		{
			// epu32, epu64
			using UValTy = std::make_signed_t<ValTy>;
			RETURN_OP(is256, srlv, UValTy, pack, other.pack);
		}
		else
		{
			// epi32
			RETURN_OP(is256, srav, ValTy, pack, other.pack);
		}
	}
	inline ValuePack operator<<(int x) const
	{
		using UValTy = std::make_signed_t<ValTy>;
		RETURN_OP(is256, slli, UValTy, pack, x);
	}
	inline ValuePack operator>>(int x) const
	{
		static_assert(!std::is_same_v<ValTy, int64_t>, "Arithmetic right shift is not supported on signed 64 bit integers in SSE / AVX");

		if constexpr (std::is_unsigned_v<ValTy>)
		{
			// epu16, epu32, epu64
			using UValTy = std::make_signed_t<ValTy>;
			RETURN_OP(is256, srli, UValTy, pack, x);
		}
		else
		{
			// epi16, epi32
			RETURN_OP(is256, srai, ValTy, pack, x);
		}
	}
	inline ValuePack& operator<<=(int x)
	{
		pack = ((*this) << x).pack;
		return *this;
	}
	inline ValuePack& operator>>=(int x)
	{
		pack = ((*this) >> x).pack;
		return *this;
	}

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
	ADD_FREE_FRIEND(cbrt);
	ADD_FREE_FRIEND(invsqrt);
	ADD_FREE_FRIEND(invsqrt_approx);
	ADD_FREE_FRIEND(invcbrt);
	ADD_FREE_FRIEND_2ARG(pow);

	// Other functions
	// Rounding
	ADD_FREE_FRIEND(floor);
	ADD_FREE_FRIEND(round);
	ADD_FREE_FRIEND(ceil);

	// Simple
	ADD_FREE_FRIEND(abs);
	ADD_FREE_FRIEND_2ARG(min);
	ADD_FREE_FRIEND_2ARG(max);
	ADD_FREE_FRIEND_2ARG(avg);
	ADD_FREE_FRIEND_2ARG(adds);
	ADD_FREE_FRIEND_2ARG(subs);

	// Special
	ADD_FREE_FRIEND(erf);

	template <ComparisonOperator op, typename ValTy, size_t PackSize>
	friend BoolPack<PackSize, sizeof(ValTy)> cmp(ValuePack<ValTy, PackSize> pack1, ValuePack<ValTy, PackSize> pack2);
	
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
ADD_FREE_FUNC(sinh, sinh);
ADD_FREE_FUNC(cosh, cosh);
ADD_FREE_FUNC(tanh, tanh);
ADD_FREE_FUNC(asinh, asinh);
ADD_FREE_FUNC(acosh, acosh);
ADD_FREE_FUNC(atanh, atanh);

// Exp functions
ADD_FREE_FUNC(exp, exp);
ADD_FREE_FUNC(log, log);
ADD_FREE_FUNC(log2, log2);
ADD_FREE_FUNC(log10, log10);
ADD_FREE_FUNC(sqrt, sqrt);
ADD_FREE_FUNC(cbrt, cbrt);
ADD_FREE_FUNC(invsqrt, invsqrt);
ADD_FREE_FUNC(invsqrt_approx, rsqrt);
ADD_FREE_FUNC(invcbrt, invcbrt);
ADD_FREE_FUNC_2ARG(pow, pow);

// Other functions
// Rounding
ADD_FREE_FUNC(floor, floor);
ADD_FREE_FUNC(round, round);
ADD_FREE_FUNC(ceil, ceil);

// Simple
ADD_FREE_FUNC(abs, abs);
ADD_FREE_FUNC_2ARG(min, min);
ADD_FREE_FUNC_2ARG(max, max);
ADD_FREE_FUNC_2ARG(avg, avg);
ADD_FREE_FUNC_2ARG(adds, adds);
ADD_FREE_FUNC_2ARG(subs, subs);

// Special
ADD_FREE_FUNC(erf, erf);

template <typename ValTy, size_t PackSize>
inline BoolPack<PackSize, sizeof(ValTy)> isfinite(ValuePack<ValTy, PackSize> pack)
{
	return cmp<IS_FINITE>(pack, pack);
}

template <ComparisonOperator op, typename ValTy, size_t PackSize>
inline BoolPack<PackSize, sizeof(ValTy)> cmp(ValuePack<ValTy, PackSize> pack1, ValuePack<ValTy, PackSize> pack2)
{
	RETURN_OP(pack1.is256, cmp, ValTy, pack1.pack, pack2.pack, op);
}

// === Ostream operators ===
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

template <size_t NumElem, size_t ElemSize>
std::ostream& operator<<(std::ostream& os, const BoolPack<NumElem, ElemSize>& pack)
{
	os << '[';
	for (size_t i = 0; i < NumElem; i++)
	{
		if (i) os << ", ";
		os << std::format("{}", pack[i]);
	}
	os << ']';
	return os;
}