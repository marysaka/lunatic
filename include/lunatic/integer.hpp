/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <cstdint>

#ifdef LUNATIC_NO_CUSTOM_INT_TYPES
namespace lunatic {
#endif

using u8 = std::uint8_t;
using s8 = std::int8_t;
using u16 = std::uint16_t;
using s16 = std::int16_t;
using u32 = std::uint32_t;
using s32 = std::int32_t;
using u64 = std::uint64_t;
using s64 = std::int64_t;

using uint = unsigned int;
using uintptr = std::uintptr_t;

#ifdef LUNATIC_NO_CUSTOM_INT_TYPES
} // namespace lunatic
#endif
