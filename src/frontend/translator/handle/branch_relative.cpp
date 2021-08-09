/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMBranchRelative const& opcode) -> Status {
  auto target_addr = code_address + opcode_size * 2 + opcode.offset;

  if (opcode.link) {
    emitter->StoreGPR(IRGuestReg{GPR::LR, mode}, IRConstant{code_address + opcode_size});
  }

  if (opcode.exchange) {
    // NOTE: the code assumes that we are in ARM mode right now.
    auto& cpsr_in  = emitter->CreateVar(IRDataType::UInt32, "cpsr_in");
    auto& cpsr_out = emitter->CreateVar(IRDataType::UInt32, "cpsr_out");

    // Switch into Thumb mode.
    emitter->LoadCPSR(cpsr_in);
    emitter->ORR(cpsr_out, cpsr_in, IRConstant{32}, false);
    emitter->StoreCPSR(cpsr_out);

    target_addr += sizeof(u16) * 2;
  } else {
    target_addr += opcode_size * 2;
  }

  emitter->StoreGPR(IRGuestReg{GPR::PC, mode}, IRConstant{target_addr});
  return Status::BreakBasicBlock;
}

} // namespace lunatic::frontend
} // namespace lunatic
