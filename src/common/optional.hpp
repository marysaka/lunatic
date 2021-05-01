/*
 * Copyright (c) 2021 fleroviux
 */

#pragma once

#include <stdexcept>

#include "meta.hpp"

/// std::optional like class template which supports optional references (unlike std::optional)
template<typename T>
struct Optional {
  Optional() : is_null(true) {}

  Optional(T value) : is_null(false) {
    if constexpr (IsReference<T>::value) {
      this->value = &value;
    } else {
      this->value = value;
    }
  }

  bool IsNull() const {
    return is_null;
  }

  auto Unwrap() -> T {
    if (IsNull()) {
      throw std::runtime_error("attempt to unwrap empty Optional<T>");
    }
    if constexpr (IsReference<T>::value) {
      return *value;
    } else {
      return value;
    }
  }

private:
  bool is_null;
  typename Pointerify<T>::type value;
};
