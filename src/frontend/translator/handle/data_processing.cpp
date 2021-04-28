/*
 * Copyright (C) 2021 fleroviux
 */

#include <fmt/format.h>

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::handle(ARMDataProcessing const& opcode) -> bool {
  using Opcode = ARMDataProcessing::Opcode;

  if (opcode.condition != Condition::AL) {
    return false;
  }

  if (opcode.set_flags) {
    return false;
  }

  // Do not load GPR if we're not going to use it (e.g. MOV)
  // auto op1 = emitter->LoadGPR(IRGuestReg{opcode.reg_op1, mode});
  auto op2 = IRValue{};

  if (opcode.immediate) {
    // TODO: update the carry flag in CPSR!
    op2 = IRConstant{
      bit::rotate_right<u32>(opcode.op2_imm.value, opcode.op2_imm.shift)
    };
  } else {
    return false;
  }

  // TODO: fix the naming... this is atrocious...
  switch (opcode.opcode) {
    case Opcode::MOV: {
      // TODO: update flags and all...
      emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, op2);
      break;
    }
    default: {
      fmt::print("DataProcessing: unhandled opcode 0x{:X}\n", opcode.opcode);
      return false;
    }
  }

  if (opcode.reg_dst == GPR::PC) {
    fmt::print("DataProcessing: unhandled write to R15");
    return false;
  }

  fmt::print("Successfully compiled one data processing opcode! ^-^\n");
  return true;
}

} // namespace lunatic::frontend
} // namespace lunatic
