/*
 * Copyright (C) 2022 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#if defined(__GNUC__)
  #define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#elif defined(_MSC_VER)
  #define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#else
  #error "Unsupported compiler"
#endif