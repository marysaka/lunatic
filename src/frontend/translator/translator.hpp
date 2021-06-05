/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/memory.hpp>

#include "frontend/decode/arm.hpp"
#include "frontend/decode/thumb.hpp"
#include "frontend/basic_block.hpp"

namespace lunatic {
namespace frontend {

// TODO: rename me :)
enum class Status {
  Continue,
  BreakBasicBlock,
  BreakMicroBlock,
  Unimplemented
};

struct Translator final : ARMDecodeClient<Status> {
  void Translate(BasicBlock& basic_block, Memory& memory);

  auto Handle(ARMDataProcessing const& opcode) -> Status override;
  auto Handle(ARMMoveStatusRegister const& opcode) -> Status override;
  auto Handle(ARMMoveRegisterStatus const& opcode) -> Status override;
  auto Handle(ARMMultiply const& opcode) -> Status override;
  auto Handle(ARMMultiplyLong const& opcode) -> Status override;
  auto Handle(ARMSingleDataSwap const& opcode) -> Status override;
  auto Handle(ARMBranchExchange const& opcode) -> Status override;
  auto Handle(ARMHalfwordSignedTransfer const& opcode) -> Status override;
  auto Handle(ARMSingleDataTransfer const& opcode) -> Status override;
  auto Handle(ARMBlockDataTransfer const& opcode) -> Status override;
  auto Handle(ARMBranchRelative const& opcode) -> Status override;
  auto Handle(ARMException const& opcode) -> Status override;
  auto Handle(ThumbBranchLinkSuffix const& opcode) -> Status override;
  auto Undefined(u32 opcode) -> Status override;

private:
  static constexpr int kMaxBlockSize = 32;

  void TranslateARM(BasicBlock& basic_block, Memory& memory);
  void TranslateThumb(BasicBlock& basic_block, Memory& memory);

  void EmitUpdateNZ();
  void EmitUpdateNZC();
  void EmitUpdateNZCV();
  void EmitAdvancePC();
  void EmitFlush();
  void EmitFlushExchange(IRVariable const& address);
  void EmitFlushNoSwitch();
  void EmitLoadSPSRToCPSR();

  // TODO: deduce opcode_size from thumb_mode. this is redundant.
  u32 code_address;
  bool thumb_mode;
  u32 opcode_size;
  Mode mode;
  bool armv5te = false;
  IREmitter* emitter = nullptr;
};

} // namespace lunatic::frontend
} // namespace lunatic
