/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <cstdlib>

namespace memory {

#ifdef _WIN32
  void* aligned_alloc(std::size_t alignment, std::size_t size) {
    return _aligned_malloc(size, alignment);
  }
  
  void free(void* ptr) {
    _aligned_free(ptr);
  }
#else
  void* aligned_alloc(std::size_t alignment, std::size_t size) {
    return std::aligned_alloc(alignment, size);
  }
  
  void free(void* ptr) {
    std::free(ptr);
  }
#endif

} // namespace memory
