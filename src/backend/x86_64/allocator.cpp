/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <cstdlib>
#include <fmt/format.h>

#include "allocator.hpp"

namespace lunatic {
namespace backend {

Allocator::Allocator() {
  buffer = reinterpret_cast<u8*>(std::aligned_alloc(4096, kBufferSize));
  if (buffer == nullptr) {
    // ...
  }
  Xbyak::CodeArray::protect(buffer, kBufferSize, Xbyak::CodeArray::PROTECT_RWE);

  auto chunk = (Chunk*)buffer;
  chunk->prev = nullptr;
  chunk->next = nullptr;
  chunk->size = kBufferSize - sizeof(Chunk);
  chunk->free = true;

  hint = chunk;
}

Allocator::~Allocator() {
  if (buffer != nullptr) {
    std::free(buffer);
    buffer = nullptr;
  }
}

auto Allocator::alloc(size_t size) -> u8* {
  auto chunk = (Chunk*)buffer;

  // Round requested size up to a 64-bit boundary.
  auto rem = size & (sizeof(u64) - 1);
  if (rem != 0) {
    size += sizeof(u64) - rem;
  }

  if (hint && hint->free && hint->size >= size) {
    chunk = hint;
    a++;
    //fmt::print("used hint\n");
  } else {
    //fmt::print("could not use hint\n");
  }
  b++;

  //fmt::print("statistics: {}%\n", a * 100.0 / b);

  do {
    if (chunk->free && chunk->size >= size) {
      auto result = (u8*)chunk + sizeof(Chunk);
      auto unused = chunk->size - (s32)size;

      if (unused >= kMinChunkCapacity) {
        auto new_chunk = (Chunk*)(result + size);
        new_chunk->size = unused - sizeof(Chunk);
        new_chunk->free = true;
        new_chunk->prev = chunk;
        new_chunk->next = chunk->next;
        if (new_chunk->next) {
          new_chunk->next->prev = new_chunk;
        }
        chunk->next = new_chunk;
      }

      chunk->free = false;
      chunk->size = size;

      auto prev = chunk->prev;
      auto next = chunk->next;

      //if (prev && (!hint || prev->size > hint->size)) {
      //  hint = prev;
      //}

      if (next && (!hint || next->size > hint->size)) {
        hint = next;
      }

      return result;
    }

    chunk = chunk->next;
  } while (chunk != nullptr);

  return nullptr;
}

void Allocator::free(u8* data) {
  auto chunk = (Chunk*)(data - sizeof(Chunk));

  auto prev_chunk = chunk->prev;
  auto next_chunk = chunk->next;

  chunk->free = true;

  if (next_chunk && next_chunk->free) {
    merge(chunk, next_chunk);
  }

  if (prev_chunk && prev_chunk->free) {
    merge(prev_chunk, chunk);
  }
}

void Allocator::resize(u8* data, size_t size) {
  // TODO: messy, duplicated code. fixme.

  auto chunk = (Chunk*)(data - sizeof(Chunk));

  // TODO: probably should round size up

  auto unused = chunk->size - (s32)size;

  if (unused >= kMinChunkCapacity) {
    auto new_chunk = (Chunk*)(data + size);
    new_chunk->size = unused - sizeof(Chunk);
    new_chunk->free = true;
    new_chunk->prev = chunk;
    new_chunk->next = chunk->next;
    if (new_chunk->next) {
      new_chunk->next->prev = new_chunk;
      if (new_chunk->next->free) {
        merge(new_chunk, new_chunk->next);
      }
    }
    chunk->next = new_chunk;
  }

  chunk->size = size;

  if (!hint || !hint->free || chunk->size > hint->size) {
    hint = chunk;
  }
}

void Allocator::merge(Chunk* a, Chunk* b) {
  auto c = b->next;
  a->size += b->size + sizeof(Chunk);
  a->next = c;
  if (c) {
    c->prev = a;
  }

  if (b == hint) {
    hint = a;
  }
}

} // namespace lunatic::backend
} // namespace lunatic
