/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMMoveStatusRegister const& opcode) -> Status {
  u32 mask = 0;

  if (opcode.fsxc & 1) mask |= 0x000000FF;
  if (opcode.fsxc & 2) mask |= 0x0000FF00;
  if (opcode.fsxc & 4) mask |= 0x00FF0000;
  if (opcode.fsxc & 8) mask |= 0xFF000000;

  auto& psr = emitter->CreateVar(IRDataType::UInt32, "psr");
  auto& psr_masked = emitter->CreateVar(IRDataType::UInt32, "psr_masked");
  auto& psr_result = emitter->CreateVar(IRDataType::UInt32, "psr_result");

  if (opcode.spsr) {
    emitter->LoadSPSR(psr, mode);
  } else {
    emitter->LoadCPSR(psr);
  }

  emitter->AND(psr_masked, psr, IRConstant{~mask}, false);

  if (opcode.immediate) {
    emitter->ORR(psr_result, psr_masked, IRConstant{opcode.imm & mask}, false);
  } else {
    auto& reg = emitter->CreateVar(IRDataType::UInt32, "reg");
    auto& reg_masked = emitter->CreateVar(IRDataType::UInt32, "reg_masked");

    emitter->LoadGPR(IRGuestReg{opcode.reg, mode}, reg);
    emitter->AND(reg_masked, reg, IRConstant{mask}, false);
    emitter->ORR(psr_result, psr_masked, reg_masked, false);
  }

  EmitAdvancePC();

  if (opcode.spsr) {
    emitter->StoreSPSR(psr_result, mode);
    return Status::Continue;
  } else {
    // TODO: if only the flags change then we should only break the micro block.
    emitter->StoreCPSR(psr_result);
    return Status::BreakBasicBlock;
  }
}

auto Translator::Handle(ARMMoveRegisterStatus const& opcode) -> Status {
  if (opcode.condition != Condition::AL) {
    return Status::Unimplemented;
  }

  auto& psr = emitter->CreateVar(IRDataType::UInt32, "psr");

  if (opcode.spsr) {
    emitter->LoadSPSR(psr, mode);
  } else {
    emitter->LoadCPSR(psr);
  }

  emitter->StoreGPR(IRGuestReg{opcode.reg, mode}, psr);
  EmitAdvancePC();

  return Status::Continue;
}

} // namespace lunatic::frontend
} // namespace lunatic
