/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/integer.hpp>

#include "ir/emitter.hpp"
#include "state.hpp"

namespace lunatic {
namespace frontend {

// TODO: how to handle conditional flow (predicated instructions) inside a basic block?
struct BasicBlock {
  typedef void (*CompiledFn)();

  union Key {
    Key() {}
    Key(State& state);
    struct Struct {
      u32 address : 32;
      State::Mode mode : 5;
    } field;
    u64 value = 0;
  } key;

  int cycle_count = 0;

  IREmitter emitter;

  // Function pointer to the compiled function.
  CompiledFn function = nullptr;

  BasicBlock() {}
  BasicBlock(Key key) : key(key) {}
};

} // namespace lunatic::frontend
} // namespace lunatic
