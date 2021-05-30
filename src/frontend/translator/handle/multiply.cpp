/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMMultiply const& opcode) -> Status {
  auto& lhs = emitter->CreateVar(IRDataType::UInt32, "lhs");
  auto& rhs = emitter->CreateVar(IRDataType::UInt32, "rhs");
  auto& result = emitter->CreateVar(IRDataType::UInt32, "result");

  emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, lhs);
  emitter->LoadGPR(IRGuestReg{opcode.reg_op2, mode}, rhs);

  if (opcode.accumulate) {
    auto& op3 = emitter->CreateVar(IRDataType::UInt32, "op3");
    auto& result_acc = emitter->CreateVar(IRDataType::UInt32, "result_acc");

    emitter->LoadGPR(IRGuestReg{opcode.reg_op3, mode}, op3);
    emitter->MUL({}, result, lhs, rhs, false);
    emitter->ADD(result_acc, result, op3, opcode.set_flags);
    emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result_acc);
  } else {
    emitter->MUL({}, result, lhs, rhs, opcode.set_flags);
    emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);
  }

  EmitAdvancePC();

  if (opcode.set_flags) {
    EmitUpdateNZ();
    return Status::BreakMicroBlock;
  }

  return Status::Continue;
}

} // namespace lunatic::frontend
} // namespace lunatic
