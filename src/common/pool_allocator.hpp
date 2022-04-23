/*
 * Copyright (C) 2022 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <fmt/format.h>
#include <lunatic/integer.hpp>

namespace lunatic {

// T = data-type for object IDs (local to a pool)
// capacity = number of objects in a pool
// size = size of each object
template<typename T, size_t capacity, size_t size>
struct PoolAllocator {
  static constexpr size_t max_size = size;

  auto Allocate() -> void* {
    if (free_pools.head == nullptr) {
      free_pools.head = new Pool{};
      free_pools.tail = free_pools.head;
    }

    auto pool = free_pools.head;
    auto object = pool->Pop();

    if (pool->IsFull()) {
      // Remove pool from the front of the free list.
      if (pool->next == nullptr) {
        free_pools.head = nullptr;
        free_pools.tail = nullptr;
      } else {
        auto new_head = pool->next;

        new_head->prev = nullptr;
        pool->next = nullptr;
        free_pools.head = new_head;
      }

      // Add pool to the end of the full list.
      if (full_pools.head == nullptr) {
        full_pools.head = pool;
        full_pools.tail = pool;
      } else {
        auto old_tail = full_pools.tail;

        old_tail->next = pool;
        pool->prev = old_tail;
        full_pools.tail = pool;
      }
    }

    return object;
  }

  void Release(void* object) {
    auto obj = (typename Pool::Object*)object;
    auto pool = (Pool*)(obj - obj->id);

    if (pool->IsFull()) {
      // Remove pool from the full list.
      if (pool->next == nullptr) {
        full_pools.tail = pool->prev;
      } else {
        pool->next->prev = pool->prev;
      }
      if (pool->prev == nullptr) {
        full_pools.head = pool->next;
      } else {
        pool->prev->next = pool->next;
      }

      // Add pool to the end of the free list.
      if (free_pools.head == nullptr) {
        free_pools.head = pool;
        free_pools.tail = pool;
        pool->prev = nullptr;
      } else {
        auto old_tail = free_pools.tail;

        old_tail->next = pool;
        pool->prev = old_tail;
        free_pools.tail = pool;
      }
      pool->next = nullptr;
    }

    pool->Push(obj);
  }

private:
  struct Pool {
    Pool() {
      T invert = capacity - 1;

      for (size_t id = 0; id < capacity; id++) {
        objects[id].id = id;
        
        stack.data[id] = invert - static_cast<T>(id);
      }

      stack.length = capacity;
    }

    bool IsFull() const { return stack.length == 0; }

    void Push(void* object) {
      stack.data[stack.length++] = ((Object*)object)->id;
    }

    auto Pop() -> void* {
      return &objects[stack.data[--stack.length]];
    }

    struct Object {
      u8 data[size];
      T id;
    } __attribute__((packed)) objects[capacity];

    struct Stack {
      T data[capacity];
      size_t length;
    } stack;

    Pool* prev = nullptr;
    Pool* next = nullptr;
  };

  struct List {
   ~List() {
      Pool* pool = head;

      while (pool != nullptr) {
        Pool* next_pool = pool->next;
        delete pool;
        pool = next_pool;
      }
    }

    Pool* head = nullptr;
    Pool* tail = nullptr;
  };

  List free_pools;
  List full_pools;
};

extern PoolAllocator<u16, 65536, 96 - sizeof(u16)> g_pool_alloc;

struct PoolObject {
  auto operator new(size_t size) -> void* {
#ifndef NDEBUG
    if (size > decltype(g_pool_alloc)::max_size) {
      throw std::runtime_error(
        fmt::format("PoolObject: requested size ({}) is larger than the supported maximum ({})",
          size, decltype(g_pool_alloc)::max_size));
    }
#endif
    return g_pool_alloc.Allocate();
  }

  void operator delete(void* object) {
    g_pool_alloc.Release(object);
  }
};

// Wrapper around our g_pool_alloc that implements the 'Allocator' named requirements:
// https://en.cppreference.com/w/cpp/named_req/Allocator
template<typename T>
struct StdPoolAlloc {
  static_assert(sizeof(T) <= decltype(g_pool_alloc)::max_size,
    "StdPoolAlloc: type exceeds maxmimum supported allocation size");

  using value_type = T;

  auto allocate(std::size_t n) -> T* {
    return (T*)g_pool_alloc.Allocate();
  }

  void deallocate(T* p, size_t n) {
    g_pool_alloc.Release(p);
  }

  auto max_size() const -> size_t {
    return 1;
  }
};

} // namespace lunatic
