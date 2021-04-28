/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <utility>
#include <type_traits>

// TODO: consider moving this to lunatic::detail

namespace lunatic {

template<typename T, typename... Args>
struct is_one_of {
  static constexpr bool value = (... || std::is_same_v<T, Args>);
};

template<typename T, typename... Args>
inline constexpr bool is_one_of_v = is_one_of<T, Args...>::value;

} // namespace lunatic
