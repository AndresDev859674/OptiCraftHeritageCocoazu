#pragma once

#include <memory>
#include <cstdint>
#include <cmath>

typedef std::int8_t byte_t;
typedef std::uint8_t ubyte_t;

typedef std::uint16_t char_t;
typedef std::uint16_t uchar_t;

typedef std::int16_t short_t;
typedef std::uint16_t ushort_t;

#ifndef PS2_PLATFORM
typedef std::int32_t int_t;
typedef std::uint32_t uint_t;
typedef std::int64_t long_t;
typedef std::uint64_t ulong_t;
#else
// On the PS2 EE compiler, std::int32_t resolves to 'long int' which breaks
// std::max(int_t, int) deduction. Use plain int/unsigned int instead — both
// are 32-bit on MIPS and match Java's int/long semantics closely enough.
typedef int                int_t;
typedef unsigned int       uint_t;
typedef long long          long_t;
typedef unsigned long long ulong_t;
#endif

#ifndef float_t
typedef float float_t;
#endif

#ifndef double_t
typedef double double_t;
#endif

typedef bool bool_t;
