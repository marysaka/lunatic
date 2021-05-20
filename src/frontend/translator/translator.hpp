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

// TODO: rename me :)
enum class Status {
  Continue,
  BreakBasicBlock,
  Unimplemented
};

struct Translator final : ARMDecodeClient<Status> {
  auto Translate(BasicBlock& block, Memory& memory) -> bool;

  auto Handle(ARMDataProcessing const& opcode) -> Status override;
  auto Handle(ARMMoveStatusRegister const& opcode) -> Status override;
  auto Handle(ARMMoveRegisterStatus const& opcode) -> Status override;
  auto Handle(ARMBranchExchange const& opcode) -> Status override;
  auto Handle(ARMHalfwordSignedTransfer const& opcode) -> Status override;
  auto Handle(ARMSingleDataTransfer const& opcode) -> Status override;
  auto Handle(ARMBlockDataTransfer const& opcode) -> Status override;
  auto Handle(ARMBranchRelative const& opcode) -> Status override;
  auto Undefined(u32 opcode) -> Status override;

  void EmitUpdateNZC();
  void EmitUpdateNZCV();
  void EmitAdvancePC();
  void EmitFlush();
  void EmitFlushExchange(IRVariable const& address);
  void EmitFlushNoSwitch();
  void EmitLoadSPSRToCPSR();

  Mode mode;
  u32 opcode_size;
  u32 code_address;
  bool armv5te = false;
  IREmitter* emitter = nullptr;
};

} // namespace lunatic::frontend
} // namespace lunatic
