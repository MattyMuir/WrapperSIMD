#pragma once
#include <cassert>
#include <iostream>
#include <type_traits>
#include <format>

#include <immintrin.h>

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

	IS_NOT_NAN = 0x7
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
inline ValuePack operator op (ValuePack other) const\
{\
	RETURN_OP(is256, mmOpName, ValTy, pack, other.pack);\
}

#define ADD_IN_PLACE_METHOD(op)\
inline ValuePack& operator op##=(ValuePack other)\
{\
	pack = ((*this) op other).pack;\
	return *this;\
}

#define ADD_SCALAR_METHOD(op)\
inline ValuePack operator op (ValTy x) const\
{\
	return (*this) op ValuePack::RepVal(x);\
}

#define ADD_IN_PLACE_SCALAR_METHOD(op)\
inline ValuePack& operator op##=(ValTy x)\
{\
	pack = ((*this) op ValuePack::RepVal(x)).pack;\
	return *this;\
}

#define ADD_BITWISE_METHOD(op, mmOpName)\
inline ValuePack operator op (ValuePack other) const\
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
inline BoolPack<PackSize, sizeof(ValTy)> operator op (ValuePack other)\
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
inline BoolPack<PackSize, sizeof(ValTy)> operator op (ValTy x)\
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
template <typename ValTy2, size_t PackSize2>\
friend ValuePack<ValTy2, PackSize2> funcName (ValuePack<ValTy2, PackSize2> pack);

#define ADD_FREE_FUNC_2ARG(funcName, mmOpName)\
template <typename ValTy, size_t PackSize>\
inline ValuePack<ValTy, PackSize> funcName (ValuePack<ValTy, PackSize> pack1, ValuePack<ValTy, PackSize> pack2)\
{\
	RETURN_OP(pack1.is256, mmOpName, ValTy, pack1.pack, pack2.pack);\
}

#define ADD_FREE_FRIEND_2ARG(funcName)\
template <typename ValTy2, size_t PackSize2>\
friend ValuePack<ValTy2, PackSize2> funcName (ValuePack<ValTy2, PackSize2> pack1, ValuePack<ValTy2, PackSize2> pack2);

template<typename ValTy, typename T>
concept IsValTy = std::is_convertible_v<T, ValTy>;

template <typename Arithmetic>
using SumType = decltype(Arithmetic{} + Arithmetic{});

// Pre declare ValuePack
template <typename ValTy, size_t PackSize>
class ValuePack;

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
			return (bool)_mm256_testc_si256((pack), _mm256_cmpeq_epi32((pack), (pack)));
		}
		else
		{
			__m128i& pack = *(__m128i*) & d;
			return (bool)_mm_testc_si128((pack), _mm_cmpeq_epi32((pack), (pack)));
		}
	}

	inline bool None() const
	{
		if constexpr (is256)
		{
			__m256i& pack = *(__m256i*) & d;
			return (bool)_mm256_testz_si256(pack, pack);
		}
		else
		{
			__m128i& pack = *(__m128i*) & d;
			return (bool)_mm_testz_si128(pack, pack);
		}
	}

	template <typename To>
	inline ValuePack<To, NumElem* ElemSize / sizeof(To)> Cast() const
	{
		using PackTy = ValuePack<To, NumElem* ElemSize / sizeof(To)>::PackTy;
		return std::bit_cast<PackTy>(d);
	}

	// Operators
	inline BoolPack operator!() const
	{
		if constexpr (is256)
		{
			__m256i& pack = *(__m256i*) & d;
			return _mm256_xor_si256(pack, _mm256_set1_epi64x(-1));
		}
		else
		{
			__m128i& pack = *(__m128i*) & d;
			return _mm_xor_si128(pack, _mm_set1_epi64x(-1));
		}
	}

	inline BoolPack operator||(BoolPack other)
	{
		if constexpr (is256)
		{
			__m256i& pack = *(__m256i*) & d;
			__m256i& otherPack = *(__m256i*) & other.d;
			return _mm256_or_si256(pack, otherPack);
		}
		else
		{
			__m128i& pack = *(__m128i*) & d;
			__m128i& otherPack = *(__m128i*) & other.d;
			return _mm_or_si128(pack, otherPack);
		}
	}

	inline BoolPack operator&&(BoolPack other)
	{
		if constexpr (is256)
		{
			__m256i& pack = *(__m256i*) & d;
			__m256i& otherPack = *(__m256i*) & other.d;
			return _mm256_and_si256(pack, otherPack);
		}
		else
		{
			__m128i& pack = *(__m128i*) & d;
			__m128i& otherPack = *(__m128i*) & other.d;
			return _mm_and_si128(pack, otherPack);
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
public:

	// == Constructors ==
#pragma warning(push)
#pragma warning(disable : 26495)
	ValuePack() {}
#pragma warning(pop)

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

		// MSVC uses 'epi64x' even for 128 bit packs, despite the intel intrinsics docs
		else if constexpr (!is256 && std::is_same_v<ValTy, int64_t>)
			return _mm_setr_epi64x(vals...);
		else if constexpr (!is256 && std::is_same_v<ValTy, uint64_t>)
			return _mm_setr_epi64x(static_cast<int64_t>(vals)...);

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

	template <IsValTy<ValTy>... Vals>
	static ValuePack SetReverse(Vals... vals)
	{
		// Large pack of int64_t or uint64_t, different 'set' function required
		if constexpr (is256 && std::is_same_v<ValTy, int64_t>)
			return _mm256_set_epi64x(vals...);
		else if constexpr (is256 && std::is_same_v<ValTy, uint64_t>)
			return _mm256_set_epi64x(static_cast<int64_t>(vals)...);

		// MSVC uses 'epi64x' even for 128 bit packs, despite the intel intrinsics docs
		else if constexpr (!is256 && std::is_same_v<ValTy, int64_t>)
			return _mm_set_epi64x(vals...);
		else if constexpr (!is256 && std::is_same_v<ValTy, uint64_t>)
			return _mm_set_epi64x(static_cast<int64_t>(vals)...);

		else if constexpr (std::is_unsigned_v<ValTy>)
		{
			// Type is unsigned, use the signed function with a cast
			using OpTy = std::make_signed_t<ValTy>;
			RETURN_OP(is256, set, OpTy, static_cast<OpTy>(vals)...);
		}
		else
		{
			RETURN_OP(is256, set, ValTy, vals...);
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

	static ValuePack Range(ValTy first, ValTy incr)
	{
		if constexpr (std::is_floating_point_v<ValTy>)
		{
			return RepVal(first) + Range<0, 1>() * incr;
		}
		else
			return RangeWithSet(first, incr);
	}

	template <ValTy first, ValTy incr>
	static ValuePack Range()
	{
		return RangeWithSet(first, incr);
	}

protected:
	template <typename... Args>
	static inline ValuePack DoRangeWithSet(ValTy incr, ValTy first, Args... others)
	{
		if constexpr (sizeof...(others) == PackSize - 1)
			return SetReverse(first, others...);
		else
			return DoRangeWithSet(incr, first + incr, first, others...);
	}

	static ValuePack RangeWithSet(ValTy first, ValTy incr)
	{
		return DoRangeWithSet(incr, first);
	}

	template <size_t ShiftAmount, size_t... Sources>
	static constexpr int32_t ToControlMask()
	{
		int32_t ret = 0;
		uint32_t i = 0;
		for (size_t source : { Sources... })
		{
			ret += source << (i * ShiftAmount);
			i++;
		}
		return ret;
	}

	template <size_t ShiftAmount, size_t... Sources>
	static constexpr int32_t HalfSizeControlMask()
	{
		int32_t ret = 0;
		uint32_t i = 0;
		for (size_t source : { Sources... })
		{
			source *= 2;
			ret += source << (i * ShiftAmount);
			i++;
			ret += (source + 1) << (i * ShiftAmount);
			i++;
		}
		return ret;
	}

public:
	// == Special members ==
	static consteval size_t Size()
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

	template<typename To>
	inline ValuePack<To, PackSize> Convert()
	{
		static constexpr bool cvtIs256 = is256 || (sizeof(To) * PackSize == 32);

		// Types are the same, no conversion necessary
		if constexpr (std::is_same_v<ValTy, To>) return (*this);

		// Converting (signed -> unsigned) or vice versa
		if constexpr (std::is_integral_v<ValTy>)
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

	template<typename To>
	inline ValuePack<To, PackSize> Cast()
	{
		// Types are the same, no cast necessary
		if constexpr (std::is_same_v<ValTy, To>) return (*this);

		// All integral types are stored in the same container, no cast necessary
		if constexpr (std::is_integral_v<ValTy> && std::is_integral_v<To>)
			return pack;

		// 128 bit
		if constexpr (!is256 && std::is_same_v<ValTy, float> && std::is_same_v<To, double>)
			return _mm_castps_pd(pack);
		if constexpr (!is256 && std::is_same_v<ValTy, float> && std::is_integral_v<To>)
			return _mm_castps_si128(pack);
		if constexpr (!is256 && std::is_same_v<ValTy, double> && std::is_same_v<To, float>)
			return _mm_castpd_ps(pack);
		if constexpr (!is256 && std::is_same_v<ValTy, double> && std::is_integral_v<To>)
			return _mm_castpd_si128(pack);
		if constexpr (!is256 && std::is_integral_v<ValTy> && std::is_same_v<To, float>)
			return _mm_castsi128_ps(pack);
		if constexpr (!is256 && std::is_integral_v<ValTy> && std::is_same_v<To, double>)
			return _mm_castsi128_pd(pack);

		// 256 bit
		if constexpr (is256 && std::is_same_v<ValTy, float> && std::is_same_v<To, double>)
			return _mm256_castps_pd(pack);
		if constexpr (is256 && std::is_same_v<ValTy, float> && std::is_integral_v<To>)
			return _mm256_castps_si256(pack);
		if constexpr (is256 && std::is_same_v<ValTy, double> && std::is_same_v<To, float>)
			return _mm256_castpd_ps(pack);
		if constexpr (is256 && std::is_same_v<ValTy, double> && std::is_integral_v<To>)
			return _mm256_castpd_si256(pack);
		if constexpr (is256 && std::is_integral_v<ValTy> && std::is_same_v<To, float>)
			return _mm256_castsi256_ps(pack);
		if constexpr (is256 && std::is_integral_v<ValTy> && std::is_same_v<To, double>)
			return _mm256_castsi256_pd(pack);
	}

	template <size_t... Sources>
	inline ValuePack Permute() const
	{
		static_assert(sizeof...(Sources) == PackSize, "Permute sources must have same size as pack");
		static_assert((... && (Sources < PackSize)), "Permute sources out of range");

		// 128 bit
		if constexpr (!is256)
		{
			// int32 and uint32
			if constexpr (std::is_integral_v<ValTy> && sizeof(ValTy) == 4)
			{
				return _mm_shuffle_epi32(pack, ToControlMask<2, Sources...>());
			}

			// int8 and uint8
			if constexpr (std::is_integral_v<ValTy> && sizeof(ValTy) == 1)
			{
				return _mm_shuffle_epi8(pack, ValuePack<int8_t, 16>{ static_cast<uint8_t>(Sources)... }.pack);
			}

			// int64 and uint64
			if constexpr (std::is_integral_v<ValTy> && sizeof(ValTy) == 8)
			{
				return _mm_shuffle_epi32(pack, HalfSizeControlMask<2, Sources...>());
			}

			// float
			if constexpr (std::is_same_v<ValTy, float>)
			{
				return _mm_permute_ps(pack, ToControlMask<2, Sources...>());
			}

			// double
			if constexpr (std::is_same_v<ValTy, double>)
			{
				// The following code should work, but _mm_permute_pd is very buggy in MSVC
				//return _mm_permute_pd(pack, ToControlMask<1, Sources...>());

				// Using _mm_shuffle_pd instead
				return _mm_shuffle_pd(pack, pack, ToControlMask<1, Sources...>());
			}

			// TODO int16 and uint16
		}

		if constexpr (is256)
		{
			// double
			if constexpr (std::is_same_v<ValTy, double>)
			{
				return _mm256_permute4x64_pd(pack, ToControlMask<2, Sources...>());
			}

			if constexpr (std::is_same_v<ValTy, float>)
			{
				return _mm256_permutevar8x32_ps(pack, ValuePack<int32_t, 8>{ static_cast<int32_t>(Sources)... }.pack);
			}
		}

		throw;
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

	// Unary minus
	ValuePack operator-() const
	{
		static_assert(!std::is_unsigned_v<ValTy>, "Cannot apply unary minus to unsigned type");
		if constexpr (std::is_floating_point_v<ValTy>)
		{
			RETURN_OP(is256, xor, ValTy, pack, RepVal(-0.0).pack);
		}
		else
		{
			// TODO - Unary minus on integer types
		}
	}

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
	ADD_FREE_FRIEND_2ARG(atan2);
	ADD_FREE_FRIEND(sinh);
	ADD_FREE_FRIEND(cosh);
	ADD_FREE_FRIEND(tanh);
	ADD_FREE_FRIEND(asinh);
	ADD_FREE_FRIEND(acosh);
	ADD_FREE_FRIEND(atanh);

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

	template <size_t NumElem, size_t ElemSize>
	friend class BoolPack;
	
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
ADD_FREE_FUNC_2ARG(atan2, atan2);
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

// 'sum' default algorithm
template <typename ValTy, size_t PackSize>
inline SumType<ValTy> sum(ValuePack<ValTy, PackSize> pack)
{
	SumType<ValTy> sum = pack[0];
	for (size_t i = 1; i < PackSize; i++)
		sum += pack[i];
	return sum;
}

// ===== 'sum' Specializations =====
template <>
inline int64_t sum<int64_t, 2>(ValuePack<int64_t, 2> pack)
{
	__m128i shuffled = _mm_shuffle_epi32(pack.pack, 0b01'00'11'10);
	__m128i sum = _mm_add_epi64(pack.pack, shuffled);
	return sum.m128i_i64[0];
}

template <>
inline int sum<uint8_t, 32>(ValuePack<uint8_t, 32> pack)
{
	__m256i sum4 = _mm256_sad_epu8(pack.pack, _mm256_setzero_si256());
	__m128i low = _mm256_extracti128_si256(sum4, 0);
	__m128i high = _mm256_extracti128_si256(sum4, 1);
	__m128i sum2 = _mm_add_epi64(low, high);
	__m128i shuffled = _mm_shuffle_epi32(sum2, 0b01'00'11'10);
	__m128i sum1 = _mm_add_epi64(sum2, shuffled);
	return (int)sum1.m128i_i64[0];
}

template <>
inline int64_t sum<int64_t, 4>(ValuePack<int64_t, 4> pack)
{
	__m128i low = _mm256_extracti128_si256(pack.pack, 0);
	__m128i high = _mm256_extracti128_si256(pack.pack, 1);
	__m128i sum2 = _mm_add_epi64(low, high);
	__m128i shuffled = _mm_shuffle_epi32(sum2, 0b01'00'11'10);
	__m128i sum1 = _mm_add_epi64(sum2, shuffled);
	return sum1.m128i_i64[0];
}

template <>
inline double sum<double, 2>(ValuePack<double, 2> pack)
{
	__m128d high64 = _mm_unpackhi_pd(pack.pack, pack.pack);
	return _mm_add_sd(pack.pack, high64).m128d_f64[0];
}

template <>
inline double sum<double, 4>(ValuePack<double, 4> pack)
{
	__m128d low = _mm256_extractf128_pd(pack.pack, 0);
	__m128d high = _mm256_extractf128_pd(pack.pack, 1);
	__m128d sum2 = _mm_add_pd(low, high);

	__m128d high64 = _mm_unpackhi_pd(sum2, sum2);
	return _mm_add_sd(sum2, high64).m128d_f64[0];
}

template <>
inline int sum<int8_t, 32>(ValuePack<int8_t, 32> pack)
{
	__m256i rangeShifted = _mm256_xor_si256(pack.pack, _mm256_set1_epi8(0b1000'0000i8));
	__m256i sum4 = _mm256_sad_epu8(rangeShifted, _mm256_setzero_si256());
	__m128i low = _mm256_extracti128_si256(sum4, 0);
	__m128i high = _mm256_extracti128_si256(sum4, 1);
	__m128i sum2 = _mm_add_epi64(low, high);
	__m128i shuffled = _mm_shuffle_epi32(sum2, 0b01'00'11'10);
	__m128i sum1 = _mm_add_epi64(sum2, shuffled);
	return (int)(sum1.m128i_i64[0] - 4096);
}

template <>
inline int sum<uint8_t, 16>(ValuePack<uint8_t, 16> pack)
{
	__m128i sum2 = _mm_sad_epu8(pack.pack, _mm_setzero_si128());
	__m128i shuffled = _mm_shuffle_epi32(sum2, 0b01'00'11'10);
	__m128i sum1 = _mm_add_epi64(sum2, shuffled);
	return (int)sum1.m128i_i64[0];
}

template <>
inline int sum<int8_t, 16>(ValuePack<int8_t, 16> pack)
{
	__m128i rangeShifted = _mm_xor_si128(pack.pack, _mm_set1_epi8(0b1000'0000i8));
	__m128i sum2 = _mm_sad_epu8(rangeShifted, _mm_setzero_si128());
	__m128i shuffled = _mm_shuffle_epi32(sum2, 0b01'00'11'10);
	__m128i sum1 = _mm_add_epi64(sum2, shuffled);
	return (int)(sum1.m128i_i64[0] - 2048);
}

// Special
template <typename ValTy, size_t PackSize>
inline BoolPack<PackSize, sizeof(ValTy)> isfinite(ValuePack<ValTy, PackSize> pack)
{
	static constexpr double Inf = std::numeric_limits<double>::infinity();
	return (pack < Inf) && (pack > -Inf);
}

inline ValuePack<int64_t, 4> exponent(ValuePack<double, 4> pack)
{
	ValuePack<uint64_t, 4> punn = pack.Cast<uint64_t>();
	punn >>= 52;
	return (punn & 0b111'1111'1111).Cast<int64_t>();
}

inline ValuePack<double, 4> next(ValuePack<double, 4> pack)
{
	// This function is broken with input -0.0
	ValuePack<int64_t, 4> incr = ((pack >= 0).Cast<int64_t>() & 2) - 1;
	ValuePack<int64_t, 4> isFinite = isfinite(pack).Cast<int64_t>();
	incr &= isFinite;

	return (pack.Cast<int64_t>() + incr).Cast<double>();
}

inline ValuePack<double, 4> prev(ValuePack<double, 4> pack)
{
	ValuePack<int64_t, 4> incr = ((pack <= 0).Cast<int64_t>() & 2) - 1;
	ValuePack<int64_t, 4> isFinite = isfinite(pack).Cast<int64_t>();
	incr &= isFinite;

	ValuePack<int64_t, 4> res = pack.Cast<int64_t>() + incr;
	ValuePack<int64_t, 4> signFlipMask = (pack == 0).Cast<int64_t>() & INT64_MIN;
	res ^= signFlipMask;

	return res.Cast<double>();
}

template <ComparisonOperator op, typename ValTy, size_t PackSize>
inline BoolPack<PackSize, sizeof(ValTy)> cmp(ValuePack<ValTy, PackSize> pack1, ValuePack<ValTy, PackSize> pack2)
{
	static_assert(std::is_floating_point_v<ValTy>, "Function cmp only supports floating point types.");
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

// === Formatter ===
template <typename ValTy, size_t PackSize>
struct std::formatter<ValuePack<ValTy, PackSize>> : std::formatter<const char*>
{
public:
	auto format(ValuePack<ValTy, PackSize> pack, format_context& context) const
	{
		auto it = context.out();
		*(it++) = '[';
		for (size_t i = 0; i < PackSize; i++)
		{
			if (i) std::format_to(it, ", ");
			std::format_to(it, "{}", pack[i]);
		}
		*(it++) = ']';
		return it;
	}
};

// === Deduction Guides ===
template <typename FirstTy, IsValTy<FirstTy>... OtherTy> ValuePack(FirstTy, OtherTy...)
-> ValuePack<FirstTy, sizeof...(OtherTy) + 1>;

// Guides for __m128d, __m256, and __m256d
// Guides cannot be provided for __m128, __m128i and __m256i as they contain unions, and the desired type is therefore unknowable
ValuePack(__m128d)
->ValuePack<double, 2>;

ValuePack(__m256)
->ValuePack<float, 8>;

ValuePack(__m256d)
->ValuePack<double, 4>;