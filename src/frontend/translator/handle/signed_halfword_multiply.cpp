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

auto Translator::Handle(ARMSignedWordHalfwordMultiply const& opcode) -> Status {
  auto& result_mul = emitter->CreateVar(IRDataType::SInt32, "result_mul");
  auto& result_asr = emitter->CreateVar(IRDataType::SInt32, "result_asr");
  auto& lhs = emitter->CreateVar(IRDataType::SInt32, "lhs");
  auto& rhs = emitter->CreateVar(IRDataType::SInt32, "rhs");
  auto& rhs_reg = emitter->CreateVar(IRDataType::SInt32, "rhs_reg");

  emitter->LoadGPR(IRGuestReg{opcode.reg_lhs, mode}, lhs);
  emitter->LoadGPR(IRGuestReg{opcode.reg_rhs, mode}, rhs_reg);

  if (opcode.y) {
    emitter->ASR(rhs, rhs_reg, IRConstant{16}, false);
  } else {
    auto& tmp = emitter->CreateVar(IRDataType::SInt32);
    emitter->LSL(tmp, rhs_reg, IRConstant{16}, false);
    emitter->ASR(rhs, tmp, IRConstant{16}, false);
  }

  emitter->MUL({}, result_mul, lhs, rhs, false);
  emitter->ASR(result_asr, result_mul, IRConstant{16}, false);

  if (opcode.accumulate) {
    auto& op3 = emitter->CreateVar(IRDataType::SInt32, "op3");
    auto& result_acc = emitter->CreateVar(IRDataType::SInt32, "result_acc");
   
    emitter->LoadGPR(IRGuestReg{opcode.reg_op3, mode}, op3);
    emitter->ADD(result_acc, result_asr, op3, true);
    EmitUpdateQ();
    emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result_acc);
    
    EmitAdvancePC();
    return Status::BreakBasicBlock;
  } else {
    emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result_asr);
    EmitAdvancePC();
    return Status::Continue;
  }
}

auto Translator::Handle(ARMSignedHalfwordMultiplyAccumulateLong const& opcode) -> Status {
  auto& result_hi = emitter->CreateVar(IRDataType::SInt32, "result_hi");
  auto& result_lo = emitter->CreateVar(IRDataType::SInt32, "result_lo");
  auto& result_acc_hi = emitter->CreateVar(IRDataType::SInt32, "result_acc_hi");
  auto& result_acc_lo = emitter->CreateVar(IRDataType::SInt32, "result_acc_lo");
  auto& lhs = emitter->CreateVar(IRDataType::SInt32, "lhs");
  auto& rhs = emitter->CreateVar(IRDataType::SInt32, "rhs");
  auto& lhs_reg = emitter->CreateVar(IRDataType::SInt32, "lhs_reg");
  auto& rhs_reg = emitter->CreateVar(IRDataType::SInt32, "rhs_reg");
  auto& dst_hi = emitter->CreateVar(IRDataType::SInt32, "dst_hi");
  auto& dst_lo = emitter->CreateVar(IRDataType::SInt32, "dst_lo");

  emitter->LoadGPR(IRGuestReg{opcode.reg_lhs, mode}, lhs_reg);
  emitter->LoadGPR(IRGuestReg{opcode.reg_rhs, mode}, rhs_reg);
  emitter->LoadGPR(IRGuestReg{opcode.reg_dst_hi, mode}, dst_hi);
  emitter->LoadGPR(IRGuestReg{opcode.reg_dst_lo, mode}, dst_lo);

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

  emitter->MUL(result_hi, result_lo, lhs, rhs, false);

  emitter->ADD64(
    result_acc_hi,
    result_acc_lo,
    dst_hi,
    dst_lo,
    result_hi,
    result_lo,
    false
  );
  
  emitter->StoreGPR(IRGuestReg{opcode.reg_dst_hi, mode}, result_acc_hi);
  emitter->StoreGPR(IRGuestReg{opcode.reg_dst_lo, mode}, result_acc_lo);

  EmitAdvancePC();
  return Status::Continue;
}

} // namespace lunatic::frontend
} // namespace lunatic
