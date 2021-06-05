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
      auto const& cpsr = state.GetCPSR();
      field.mode = cpsr.f.mode;
      field.thumb = cpsr.f.thumb;
      field.address = state.GetGPR(field.mode, GPR::PC) >> 1;
    }

    struct Struct {
      u32 address : 31;
      Mode mode : 5;
      uint thumb : 1;
    } field;
    
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
