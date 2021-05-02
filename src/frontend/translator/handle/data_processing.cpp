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

auto Translator::Handle(ARMDataProcessing const& opcode) -> bool {
  using Opcode = ARMDataProcessing::Opcode;

  if (opcode.condition != Condition::AL) {
    return false;
  }

  auto op2 = IRValue{};
  bool advance_pc_early = false;

  // TODO: clean this unholy mess up.
  // TODO: do not update the carry flag if it will be overwritten.
  auto shifter_update_carry = opcode.set_flags && 
    opcode.opcode != Opcode::ADC &&
    opcode.opcode != Opcode::SBC &&
    opcode.opcode != Opcode::RSC;

  if (opcode.immediate) {
    auto value = opcode.op2_imm.value;
    auto shift = opcode.op2_imm.shift;

    op2 = IRConstant{bit::rotate_right<u32>(value, shift)};

    if (shifter_update_carry && shift != 0) {
      bool carry = bit::get_bit<u32, bool>(value, shift - 1);
      if (carry) {
        emitter->SetCarry();
      } else {
        emitter->ClearCarry();
      }
    }
  } else {
    auto& shift = opcode.op2_reg.shift;

    auto  amount = IRValue{};
    auto& source = emitter->CreateVar(IRDataType::UInt32, "shift_source");
    auto& result = emitter->CreateVar(IRDataType::UInt32, "shift_result");

    if (shift.immediate) {
      // TODO: optimize case when amount == 0
      amount = IRConstant(u32(shift.amount_imm));
    } else {
      amount = emitter->CreateVar(IRDataType::UInt32, "shift_amount");
      emitter->LoadGPR(IRGuestReg{shift.amount_reg, mode}, amount.GetVar());

      EmitAdvancePC();
      advance_pc_early = true;
    }

    emitter->LoadGPR(IRGuestReg{opcode.op2_reg.reg, mode}, source);

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
    case Opcode::RSB: {
      auto& op1 = emitter->CreateVar(IRDataType::UInt32, "op1");
      auto& result = emitter->CreateVar(IRDataType::UInt32, "result");

      emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, op1);
      emitter->RSB(result, op1, op2, opcode.set_flags);
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
    case Opcode::RSC: {
      auto& op1 = emitter->CreateVar(IRDataType::UInt32, "op1");
      auto& result = emitter->CreateVar(IRDataType::UInt32, "result");

      emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, op1);
      emitter->RSC(result, op1, op2, opcode.set_flags);
      emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);

      if (opcode.set_flags) {
        EmitUpdateNZCV();
      }
      break;
    }
    case Opcode::ORR: {
      auto& op1 = emitter->CreateVar(IRDataType::UInt32, "op1");
      auto& result = emitter->CreateVar(IRDataType::UInt32, "result");

      emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, op1);
      emitter->ORR(result, op1, op2, opcode.set_flags);
      emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);

      if (opcode.set_flags) {
        EmitUpdateNZC();
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
      if (opcode.set_flags) {
        auto& result = emitter->CreateVar(IRDataType::UInt32, "result");
        emitter->MOV(result, op2, true);
        emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);
        EmitUpdateNZC();
      } else {
        emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, op2);
      }
      break;
    }
    case Opcode::BIC: {
      auto& op1 = emitter->CreateVar(IRDataType::UInt32, "op1");
      auto& result = emitter->CreateVar(IRDataType::UInt32, "result");

      emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, op1);
      emitter->BIC(result, op1, op2, opcode.set_flags);
      emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);

      if (opcode.set_flags) {
        EmitUpdateNZCV();
      }
      break;
    }
    case Opcode::MVN: {
      auto& result = emitter->CreateVar(IRDataType::UInt32, "result");
      emitter->MVN(result, op2, opcode.set_flags);
      emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);
      if (opcode.set_flags) {
        EmitUpdateNZC();
      }
      break;
    }
  }

  if (opcode.reg_dst == GPR::PC) {
    // TODO: handle CMP, CMN, TST and TEQ opcodes.
    if (opcode.set_flags) {
      EmitLoadSPSRToCPSR();
    }
    EmitFlushPipeline();
  } else if (!advance_pc_early) {
    EmitAdvancePC();
  }

  return true;
}

} // namespace lunatic::frontend
} // namespace lunatic
