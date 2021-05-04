/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/memory.hpp>

#include "frontend/decode/arm.hpp"
#include "frontend/basic_block.hpp"

namespace lunatic {
namespace frontend {

struct Translator final : ARMDecodeClient<bool> {
  auto Translate(BasicBlock& block, Memory& memory) -> bool;

  auto Handle(ARMDataProcessing const& opcode) -> bool override;
  auto Handle(ARMSingleDataTransfer const& opcode) -> bool override;
  auto Undefined(u32 opcode) -> bool override;

  void EmitUpdateNZC();
  void EmitUpdateNZCV();
  void EmitAdvancePC();
  void EmitFlushPipeline();
  void EmitLoadSPSRToCPSR();

  Mode mode;
  u32 opcode_size;
  IREmitter* emitter = nullptr;
};

} // namespace lunatic::frontend
} // namespace lunatic
