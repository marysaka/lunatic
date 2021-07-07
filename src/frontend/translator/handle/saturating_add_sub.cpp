/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMSaturatingAddSub const& opcode) -> Status {
  auto& result = emitter->CreateVar(IRDataType::UInt32);
  auto& lhs = emitter->CreateVar(IRDataType::UInt32); 
  auto& rhs = emitter->CreateVar(IRDataType::UInt32);

  emitter->LoadGPR(IRGuestReg{opcode.reg_lhs, mode}, lhs);
  emitter->LoadGPR(IRGuestReg{opcode.reg_rhs, mode}, rhs);

  auto rhs_value = IRValue{};

  if (opcode.double_rhs) {
    // TODO: this likely can be optimized since both operands are equal.
    rhs_value = emitter->CreateVar(IRDataType::UInt32);
    emitter->QADD(rhs_value.GetVar(), rhs, rhs);
    EmitUpdateQ();
  } else {
    rhs_value = rhs;
  }

  if (opcode.subtract) {
    emitter->QSUB(result, lhs, rhs_value.GetVar());
  } else {
    emitter->QADD(result, lhs, rhs_value.GetVar());
  }

  emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);

  EmitUpdateQ();
  EmitAdvancePC();
  return Status::BreakMicroBlock;
}

} // namespace lunatic::frontend
} // namespace lunatic
