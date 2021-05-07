/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/memory.hpp>
#include <fmt/format.h>

#include "backend/backend.hpp"
#include "frontend/ir/emitter.hpp"
#include "frontend/state.hpp"

namespace lunatic {
namespace backend {

struct X64Backend : Backend {
  void Run(
    Memory& memory,
    lunatic::frontend::State& state,
    lunatic::frontend::IREmitter const& emitter,
    bool int3
  );

private:
  Memory* memory = nullptr;

  // TODO: get rid of the thunks eventually.

  auto ReadByte(u32 address, Memory::Bus bus) -> u8 {
    return memory->ReadByte(address, bus);
  }

  auto ReadHalf(u32 address, Memory::Bus bus) -> u16 {
    return memory->ReadHalf(address, bus);
  }

  auto ReadWord(u32 address, Memory::Bus bus) -> u32 {
    return memory->ReadWord(address, bus);
  }
};

} // namespace lunatic::backend
} // namespace lunatic
