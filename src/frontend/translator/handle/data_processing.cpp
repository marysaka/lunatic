/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <fmt/format.h>

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

// TODO: handle R15 and SPSR loading special cases

auto Translator::Handle(ARMDataProcessing const& opcode) -> bool {
  using Opcode = ARMDataProcessing::Opcode;

  if (opcode.condition != Condition::AL) {
    return false;
  }

  if (opcode.set_flags && opcode.opcode == Opcode::MOV) {
    return false;
  }

  auto op2 = IRValue{};

  // TODO: clean this unholy mess up.
  auto shifter_update_carry = opcode.set_flags && 
    opcode.opcode != Opcode::ADC &&
    opcode.opcode != Opcode::SBC &&
    opcode.opcode != Opcode::RSC;

  // TODO: do not update the carry flag if it will be overwritten.
  if (opcode.immediate) {
    auto value = opcode.op2_imm.value;
    auto shift = opcode.op2_imm.shift;

    op2 = IRConstant{bit::rotate_right<u32>(value, shift)};

    if (shifter_update_carry && shift != 0) {
      bool carry = bit::get_bit<u32, bool>(value, shift - 1);
      // TODO: update the carry flag in host flags!
    }
  } else {
    auto& shift = opcode.op2_reg.shift;

    auto  amount = IRValue{};
    auto& source = emitter->CreateVar(IRDataType::UInt32, "shift_source");
    auto& result = emitter->CreateVar(IRDataType::UInt32, "shift_result");

    emitter->LoadGPR(IRGuestReg{opcode.op2_reg.reg, mode}, source);

    if (shift.immediate) {
      amount = IRConstant(u32(shift.amount_imm));
    } else {
      amount = emitter->CreateVar(IRDataType::UInt32, "shift_amount");
      emitter->LoadGPR(IRGuestReg{shift.amount_reg, mode}, amount.GetVar());
    }

    switch (shift.type) {
      case Shift::LSL: {
        emitter->LSL(result, source, amount, shifter_update_carry);
        break;
      }
      case Shift::LSR: {
        emitter->LSR(result, source, amount, shifter_update_carry);
        break;
      }
      case Shift::ASR: {
        emitter->ASR(result, source, amount, shifter_update_carry);
        break;
      }
      case Shift::ROR: {
        emitter->ROR(result, source, amount, shifter_update_carry);
        break;
      }
    }

    op2 = result;
  }

  // TODO: fix the naming... this is atrocious...
  switch (opcode.opcode) {
    case Opcode::AND: {
      auto& op1 = emitter->CreateVar(IRDataType::UInt32, "op1");
      auto& result = emitter->CreateVar(IRDataType::UInt32, "result");

      emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, op1);
      emitter->AND(result, op1, op2, opcode.set_flags);
      emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);

      if (opcode.set_flags) {
        EmitUpdateNZC();
      }
      break;
    }
    case Opcode::EOR: {
      auto& op1 = emitter->CreateVar(IRDataType::UInt32, "op1");
      auto& result = emitter->CreateVar(IRDataType::UInt32, "result");

      emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, op1);
      emitter->EOR(result, op1, op2, opcode.set_flags);
      emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);

      if (opcode.set_flags) {
        EmitUpdateNZC();
      }
      break;
    }

    case Opcode::SUB: {
      auto& op1 = emitter->CreateVar(IRDataType::UInt32, "op1");
      auto& result = emitter->CreateVar(IRDataType::UInt32, "result");

      emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, op1);
      emitter->SUB(result, op1, op2, opcode.set_flags);
      emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);

      if (opcode.set_flags) {
        EmitUpdateNZCV();
      }
      break;
    }
    case Opcode::ADD: {
      auto& op1 = emitter->CreateVar(IRDataType::UInt32, "op1");
      auto& result = emitter->CreateVar(IRDataType::UInt32, "result");

      emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, op1);
      emitter->ADD(result, op1, op2, opcode.set_flags);
      emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);

      if (opcode.set_flags) {
        EmitUpdateNZCV();
      }
      break;
    }
    case Opcode::ADC: {
      auto& op1 = emitter->CreateVar(IRDataType::UInt32, "op1");
      auto& result = emitter->CreateVar(IRDataType::UInt32, "result");

      emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, op1);
      emitter->ADC(result, op1, op2, opcode.set_flags);
      emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);

      if (opcode.set_flags) {
        EmitUpdateNZCV();
      }
      break;
    }
    case Opcode::SBC: {
      auto& op1 = emitter->CreateVar(IRDataType::UInt32, "op1");
      auto& result = emitter->CreateVar(IRDataType::UInt32, "result");

      emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, op1);
      emitter->SBC(result, op1, op2, opcode.set_flags);
      emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);

      if (opcode.set_flags) {
        EmitUpdateNZCV();
      }
      break;
    }

    case Opcode::TST: {
      auto& op1 = emitter->CreateVar(IRDataType::UInt32, "op1");
      emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, op1);
      emitter->AND({}, op1, op2, opcode.set_flags);
      EmitUpdateNZC();
      break;
    }
    case Opcode::TEQ: {
      auto& op1 = emitter->CreateVar(IRDataType::UInt32, "op1");
      emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, op1);
      emitter->EOR({}, op1, op2, opcode.set_flags);
      EmitUpdateNZC();
      break;
    }
    case Opcode::CMP: {
      auto& op1 = emitter->CreateVar(IRDataType::UInt32, "op1");
      emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, op1);
      emitter->SUB({}, op1, op2, opcode.set_flags);
      EmitUpdateNZCV();
      break;
    }
    case Opcode::CMN: {
      auto& op1 = emitter->CreateVar(IRDataType::UInt32, "op1");
      emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, op1);
      emitter->ADD({}, op1, op2, opcode.set_flags);
      EmitUpdateNZCV();
      break;
    }

    case Opcode::MOV: {
      // TODO: update NZC flags
      emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, op2);
      break;
    }
    default: {
      return false;
    }
  }

  if (opcode.reg_dst == GPR::PC) {
    // Hmm... this really gets spammed a lot in ARMWrestler right now.
    // fmt::print("DataProcessing: unhandled write to R15\n");
    return false;
  }

  return true;
}

} // namespace lunatic::frontend
} // namespace lunatic
