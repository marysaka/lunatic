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

  emitter->MUL(result, lhs, rhs);

  if (opcode.accumulate) {
    auto& op3 = emitter->CreateVar(IRDataType::UInt32, "op3");
    auto& result2 = emitter->CreateVar(IRDataType::UInt32, "result2");

    emitter->LoadGPR(IRGuestReg{opcode.reg_op3, mode}, op3);
    emitter->ADD(result2, result, op3, opcode.set_flags);
    if (opcode.set_flags) {
      EmitUpdateNZ();
    }

    emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result2);
  } else {
    if (opcode.set_flags) {
      emitter->AND({}, result, result, true);
      EmitUpdateNZ();
    }

    emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);
  }

  EmitAdvancePC();

  if (opcode.set_flags) {
    return Status::BreakMicroBlock;
  }

  return Status::Continue;
}

} // namespace lunatic::frontend
} // namespace lunatic
