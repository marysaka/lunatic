/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
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
