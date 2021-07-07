/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMSignedHalfwordMultiply const& opcode) -> Status {
  auto& result = emitter->CreateVar(IRDataType::SInt32, "result");
  auto& lhs = emitter->CreateVar(IRDataType::SInt32, "lhs");
  auto& rhs = emitter->CreateVar(IRDataType::SInt32, "rhs");
  auto& lhs_reg = emitter->CreateVar(IRDataType::SInt32, "lhs_reg");
  auto& rhs_reg = emitter->CreateVar(IRDataType::SInt32, "rhs_reg");

  emitter->LoadGPR(IRGuestReg{opcode.reg_lhs, mode}, lhs_reg);
  emitter->LoadGPR(IRGuestReg{opcode.reg_rhs, mode}, rhs_reg);

  if (opcode.x) {
    emitter->ASR(lhs, lhs_reg, IRConstant{16}, false);
  } else {
    auto& tmp = emitter->CreateVar(IRDataType::SInt32);
    emitter->LSL(tmp, lhs_reg, IRConstant{16}, false);
    emitter->ASR(lhs, tmp, IRConstant{16}, false);
  }

  if (opcode.y) {
    emitter->ASR(rhs, rhs_reg, IRConstant{16}, false);
  } else {
    auto& tmp = emitter->CreateVar(IRDataType::SInt32);
    emitter->LSL(tmp, rhs_reg, IRConstant{16}, false);
    emitter->ASR(rhs, tmp, IRConstant{16}, false);
  }

  emitter->MUL({}, result, lhs, rhs, false);

  if (opcode.accumulate) {
    auto& op3 = emitter->CreateVar(IRDataType::SInt32, "op3");
    auto& result_acc = emitter->CreateVar(IRDataType::SInt32, "result_acc");
   
    emitter->LoadGPR(IRGuestReg{opcode.reg_op3, mode}, op3);
    emitter->ADD(result_acc, result, op3, true);
    EmitUpdateQ();
    emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result_acc);
    
    EmitAdvancePC();
    return Status::BreakBasicBlock;
  } else {
    emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);
    EmitAdvancePC();
    return Status::Continue;
  }
}

} // namespace lunatic::frontend
} // namespace lunatic
