/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMSingleDataTransfer const& opcode) -> bool {
  if (opcode.condition != Condition::AL) {
    return false;
  }

  if (!opcode.pre_increment && opcode.writeback) {
    // LDRT and STRT are not supported right now.
    return false;
  }

  auto offset = IRValue{};

  if (opcode.immediate) {
    offset = IRConstant{opcode.offset_imm};
  } else {
    auto& offset_reg = emitter->CreateVar(IRDataType::UInt32, "base_offset_reg");
    auto& offset_var = emitter->CreateVar(IRDataType::UInt32, "base_offset_shifted");

    emitter->LoadGPR(IRGuestReg{opcode.offset_reg.reg, mode}, offset_reg);

    // TODO: write a helper method to make this pattern less verbose.
    // Or alternatively pass the shift type to the emitter directly?
    switch (opcode.offset_reg.shift) {
      case Shift::LSL: {
        emitter->LSL(offset_var, offset_reg, IRConstant{opcode.offset_reg.amount}, false);
        break;
      }
      case Shift::LSR: {
        emitter->LSR(offset_var, offset_reg, IRConstant{opcode.offset_reg.amount}, false);
        break;
      }
      case Shift::ASR: {
        emitter->ASR(offset_var, offset_reg, IRConstant{opcode.offset_reg.amount}, false);
        break;
      }
      case Shift::ROR: {
        emitter->ROR(offset_var, offset_reg, IRConstant{opcode.offset_reg.amount}, false);
        break;
      }
    }

    offset = offset_var;
  }

  auto& base_old = emitter->CreateVar(IRDataType::UInt32, "base_old");
  auto& base_new = emitter->CreateVar(IRDataType::UInt32, "base_new");

  emitter->LoadGPR(IRGuestReg{opcode.reg_base, mode}, base_old);

  if (opcode.add) {
    emitter->ADD(base_new, base_old, offset, false);
  } else {
    emitter->SUB(base_new, base_old, offset, false);
  }

  auto& address = opcode.pre_increment ? base_new : base_old;

  EmitAdvancePC();

  // TODO: maybe it's cleaner to decode opcode into an enum upfront.
  if (opcode.load) {
    auto& data = emitter->CreateVar(IRDataType::UInt32, "data");

    if (opcode.byte) {
      return false;
    } else {
      // TODO: can we get rid of that annoying cast please?
      emitter->LDR(static_cast<IRMemoryFlags>(Word | Rotate), data, address);
    }

    emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, data);
  } else {
    return false;
  }

  // TODO: handle base == dst case (by performing writeback before destination reg write)
  if (!opcode.pre_increment || opcode.writeback) {
    emitter->StoreGPR(IRGuestReg{opcode.reg_base, mode}, base_new);
  }

  return true;
}

} // namespace lunatic::frontend
} // namespace lunatic
