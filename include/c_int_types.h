//
// Created by af on 12/10/23.
//

#ifndef TINY_CPP_C_INT_TYPES_H
#define TINY_CPP_C_INT_TYPES_H


#ifdef NO_STD

using uint64 = unsigned long long;
using uint32 = unsigned int;
using uint16 = unsigned short;
using uint8  = unsigned char;

using int64 = signed long long;
using int32 = signed int;
using int16 = signed short;
using int8  = signed char;

using float32 = float;
using float64 = double;

#else

#include <cstdint>


using uint64 = uint64_t;
using uint32 = uint32_t;
using uint16 = uint16_t;
using uint8  = uint8_t;

using int64 = int64_t;
using int32 = int32_t;
using int16 = int16_t;
using int8  = int8_t;

using float32 = float;
using float64 = double;


#endif

#endif //TINY_CPP_C_INT_TYPES_H
