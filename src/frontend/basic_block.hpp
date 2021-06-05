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

struct BasicBlock {
  // TODO: consider using uintptr_t
  typedef void (*CompiledFn)();

  union Key {
    Key() {}

    Key(State& state) {
      value  = state.GetGPR(Mode::User, GPR::PC) >> 1;
      value |= u64(state.GetCPSR().v & 0x3F) << 31; // mode and thumb bit
    }

    auto Address() -> u32 { return (value & 0x7FFFFFFF) << 1; }
    auto Mode() -> Mode { return static_cast<lunatic::Mode>((value >> 31) & 0x1F); }
    bool Thumb() { return value & (1ULL << 36); }
    
    // bits  0 - 30: address[31:1]
    // bits 31 - 35: CPU mode
    // bit       36: thumb-flag
    u64 value = 0;
  } key;

  int length = 0;

  struct MicroBlock {
    Condition condition;
    IREmitter emitter;
    int length = 0;
  };

  std::vector<MicroBlock> micro_blocks;

  // Pointer to the compiled code.
  CompiledFn function = nullptr;

  BasicBlock() {}
  BasicBlock(Key key) : key(key) {}
};

} // namespace lunatic::frontend
} // namespace lunatic
