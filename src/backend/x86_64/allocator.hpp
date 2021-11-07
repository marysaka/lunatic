/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/integer.hpp>
#include <xbyak/xbyak.h>

namespace lunatic {
namespace backend {

struct Allocator : Xbyak::Allocator {
  Allocator();
 ~Allocator();

  auto alloc(size_t size) -> u8* override;
  void free(u8* data) override;
  void resize(u8* data, size_t size);
  bool useProtect() const override { return false; }

private:
  struct Chunk {
    Chunk* prev;
    Chunk* next;
    s32 size;
    bool free;
  };

  static constexpr u32 kBufferSize = 33554432;
  static constexpr u32 kMinChunkCapacity = sizeof(Chunk) + 128;

  void merge(Chunk* a, Chunk* b);

  u8* buffer = nullptr;
  Chunk* hint = nullptr;

  int a = 0;
  int b = 0;
};

} // namespace lunatic::backend
} // namespace lunatic
