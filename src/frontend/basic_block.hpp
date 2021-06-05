/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/integer.hpp>
#include <vector>

#include "decode/definition/common.hpp"
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
      Mode mode : 5;
    } field;
    u64 value = 0;
  } key;

  // TODO: handle timing based on conditional flow.
  int length = 0;

  struct MicroBlock {
    Condition condition;
    IREmitter emitter;
    int length = 0;
  };

  std::vector<MicroBlock> micro_blocks;

  // Function pointer to the compiled function.
  CompiledFn function = nullptr;

  BasicBlock() {}
  BasicBlock(Key key) : key(key) {}
};

} // namespace lunatic::frontend
} // namespace lunatic
