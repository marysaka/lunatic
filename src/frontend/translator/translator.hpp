/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <lunatic/memory.hpp>

#include "frontend/decode/arm.hpp"
#include "frontend/basic_block.hpp"

namespace lunatic {
namespace frontend {

struct Translator final : ARMDecodeClient<bool> {
  auto translate(BasicBlock& block, Memory& memory) -> bool;

  auto handle(ARMDataProcessing const& opcode) -> bool override;
  auto undefined(u32 opcode) -> bool override;

  Mode mode;
  IREmitter* emitter = nullptr;
};

} // namespace lunatic::frontend
} // namespace lunatic
