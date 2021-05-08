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

auto Translator::Handle(ARMHalfwordSignedTransfer const& opcode) -> bool {
  if (opcode.condition != Condition::AL) {
    return false;
  }

  auto offset = IRValue{};

  if (opcode.immediate) {
    offset = IRConstant{opcode.offset_imm};
  } else {
    auto& offset_reg = emitter->CreateVar(IRDataType::UInt32, "base_offset");
    emitter->LoadGPR(IRGuestReg{opcode.offset_reg, mode}, offset_reg);
    offset = offset_reg;
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

  auto& data = emitter->CreateVar(IRDataType::UInt32, "data");

  auto writeback = [&]() {
    if (!opcode.pre_increment || opcode.writeback) {
      emitter->StoreGPR(IRGuestReg{opcode.reg_base, mode}, base_new);
    }
  };

  switch (opcode.opcode) {
    case 1: {
      if (opcode.load) {
        writeback();
        if (armv5te) {
          emitter->LDR(Half, data, address);
        } else {
          emitter->LDR(Half | Rotate, data, address);
        }
        emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, data);
      } else {
        emitter->LoadGPR(IRGuestReg{opcode.reg_dst, mode}, data);
        emitter->STR(Half, data, address);
        writeback();
      }
      break;
    }
    case 2: {
      if (opcode.load) {
        writeback();
        emitter->LDR(Byte | Signed, data, address);
        emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, data);
      } else {
        // LDRD (unimplemented)
        if (armv5te) {
          return false;
        }
        writeback();
      }
      break;
    }
    case 3: {
      if (opcode.load) {
        // TODO: how to handle the ARM7 and ARM9 difference?
        writeback();
        if (armv5te) {
          emitter->LDR(Half | Signed, data, address);
        } else {
          fmt::print("OwO\n");
          emitter->LDR(Half | Signed | ARMv4T, data, address);
        }
        emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, data);
      } else {
        // STRD (unimplemented)
        if (armv5te) {
          return false;
        }
        writeback();
      }
      break;
    }
    default: {
      // Unreachable
      return false;
    }
  }

  if (opcode.load && opcode.reg_dst == GPR::PC) {
    if (armv5te) {
      // TODO: switch to thumb mode if necessary.
      return false;
    }

    EmitFlushPipeline();
  }

  return true;
}

} // namespace lunatic::frontend
} // namespace lunatic
