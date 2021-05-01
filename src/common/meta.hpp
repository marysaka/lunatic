/*
 * Copyright (c) 2021 fleroviux
 */

#pragma once

template<typename T>
struct Pointerify {
  using type = T;
};

template<typename T>
struct Pointerify<T&> {
  using type = T*;
};

template<typename T>
struct IsReference {
  static constexpr bool value = false;
};

template<typename T>
struct IsReference<T&> {
  static constexpr bool value = true;
};
