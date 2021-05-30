/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMMultiplyLong const& opcode) -> Status {

  auto data_type = opcode.sign_extend ? IRDataType::SInt32 : IRDataType::UInt32;

  auto& lhs = emitter->CreateVar(data_type, "lhs");
  auto& rhs = emitter->CreateVar(data_type, "rhs");
  auto& result_hi = emitter->CreateVar(IRDataType::UInt32, "result_hi");
  auto& result_lo = emitter->CreateVar(IRDataType::UInt32, "result_lo");

  emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, lhs);
  emitter->LoadGPR(IRGuestReg{opcode.reg_op2, mode}, rhs);

  emitter->MUL(result_hi, result_lo, lhs, rhs, opcode.set_flags && !opcode.accumulate);

  if (opcode.accumulate) {
    auto& dst_hi = emitter->CreateVar(IRDataType::UInt32, "dst_hi");
    auto& dst_lo = emitter->CreateVar(IRDataType::UInt32, "dst_lo");
    auto& result_acc_hi = emitter->CreateVar(IRDataType::UInt32, "result_acc_hi");
    auto& result_acc_lo = emitter->CreateVar(IRDataType::UInt32, "result_acc_lo");

    emitter->LoadGPR(IRGuestReg{opcode.reg_dst_hi, mode}, dst_hi);
    emitter->LoadGPR(IRGuestReg{opcode.reg_dst_lo, mode}, dst_lo);

    emitter->ADD64(
      result_acc_hi,
      result_acc_lo,
      result_hi,
      result_lo,
      dst_hi,
      dst_lo,
      opcode.set_flags);

    emitter->StoreGPR(IRGuestReg{opcode.reg_dst_hi, mode}, result_acc_hi);
    emitter->StoreGPR(IRGuestReg{opcode.reg_dst_lo, mode}, result_acc_lo);
  } else {
    emitter->StoreGPR(IRGuestReg{opcode.reg_dst_hi, mode}, result_hi);
    emitter->StoreGPR(IRGuestReg{opcode.reg_dst_lo, mode}, result_lo);
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
