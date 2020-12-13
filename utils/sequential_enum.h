#pragma once
#include <array>
#include <algorithm>

#ifdef _MSC_VER
#define SequentialEnum(Name,...) \
enum Name { __VA_ARGS__ }; \

#else
#define NUMARGS(...)  (sizeof((int[]){__VA_ARGS__})/sizeof(int))

#define SequentialEnum(Name,...) \
enum Name { __VA_ARGS__ }; \
namespace \
{ \
    constexpr std::array<Name, NUMARGS(__VA_ARGS__)> Name##List { __VA_ARGS__ }; \
};
#endif
