/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <lunatic/integer.hpp>
#include <cstring>

// TODO: consider moving this to lunatic::detail

namespace lunatic {

template<typename T>
auto read(void* data, uint offset) -> T {
  T value;
  std::memcpy(&value, (u8*)data + offset, sizeof(T));
  return value;
}

template<typename T>
void write(void* data, uint offset, T value) {
  std::memcpy((u8*)data + offset, &value, sizeof(T));
}

} // namespace lunatic
