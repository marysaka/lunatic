/*
 * Copyright (C) 2021 fleroviux
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

  // TODO: do not update the carry flag if it will be overwritten.
  if (opcode.immediate) {
    auto value = opcode.op2_imm.value;
    auto shift = opcode.op2_imm.shift;

    op2 = IRConstant{bit::rotate_right<u32>(value, shift)};

    if (opcode.set_flags && shift != 0) {
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
        emitter->LSL(result, source, amount, opcode.set_flags);
        break;
      }
      case Shift::LSR: {
        emitter->LSR(result, source, amount, opcode.set_flags);
        break;
      }
      case Shift::ASR: {
        emitter->ASR(result, source, amount, opcode.set_flags);
        break;
      }
      case Shift::ROR: {
        emitter->ROR(result, source, amount, opcode.set_flags);
        break;
      }
    }

    op2 = result;
  }

  // TODO: fix the naming... this is atrocious...
  switch (opcode.opcode) {
    case Opcode::SUB:
    case Opcode::CMP: {
      auto& op1 = emitter->CreateVar(IRDataType::UInt32, "op1");
      auto& result = emitter->CreateVar(IRDataType::UInt32, "result");

      // TODO: do not require a result variable for CMP
      emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, op1);
      emitter->Sub(result, op1, op2, opcode.set_flags);
      if (opcode.opcode == Opcode::SUB) {
        emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);
      }

      if (opcode.set_flags) {
        EmitUpdateNZCV();
      }
      break;
    }
    case Opcode::ADD:
    case Opcode::CMN: {
      auto& op1 = emitter->CreateVar(IRDataType::UInt32, "op1");
      auto& result = emitter->CreateVar(IRDataType::UInt32, "result");

      // TODO: do not require a result variable for CMN
      emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode}, op1);
      emitter->Add(result, op1, op2, opcode.set_flags);
      if (opcode.opcode == Opcode::ADD) {
        emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);
      }

      if (opcode.set_flags) {
        EmitUpdateNZCV();
      }
      break;
    }
    case Opcode::MOV: {
      // TODO: update flags and all...
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
