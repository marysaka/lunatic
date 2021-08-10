/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMHalfwordSignedTransfer const& opcode) -> Status {
  auto offset = IRValue{};
  bool should_writeback = !opcode.pre_increment || opcode.writeback;
  bool should_flush_pipeline = opcode.load && opcode.reg_dst == GPR::PC;

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
  auto& data = emitter->CreateVar(IRDataType::UInt32, "data");

  auto writeback = [&]() {
    if (should_writeback) {
      emitter->StoreGPR(IRGuestReg{opcode.reg_base, mode}, base_new);
    }
  };

  EmitAdvancePC();

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
      } else if (armv5te) {
        auto reg_dst_a = opcode.reg_dst;
        auto reg_dst_b = static_cast<GPR>(static_cast<int>(reg_dst_a) + 1);
        auto& address_a = address;
        auto& address_b = emitter->CreateVar(IRDataType::UInt32);
        auto& data_a = data;
        auto& data_b = emitter->CreateVar(IRDataType::UInt32, "data");

        // LDRD with odd-numbered destination register is undefined.
        if ((static_cast<int>(reg_dst_a) & 1) == 1) {
          return Status::Unimplemented;
        }

        emitter->ADD(address_b, address_a, IRConstant{sizeof(u32)}, false);
        emitter->LDR(Word, data_a, address_a);
        emitter->LDR(Word, data_b, address_b);
        emitter->StoreGPR(IRGuestReg{reg_dst_a, mode}, data_a);
        writeback();
        emitter->StoreGPR(IRGuestReg{reg_dst_b, mode}, data_b);

        if (reg_dst_b == GPR::PC) {
          should_flush_pipeline = true;
        }
      } else {
        writeback();
      }
      break;
    }
    case 3: {
      if (opcode.load) {
        writeback();
        if (armv5te) {
          emitter->LDR(Half | Signed, data, address);
        } else {
          emitter->LDR(Half | Signed | ARMv4T, data, address);
        }
        emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, data);
      } else {
        if (armv5te) {
          auto reg_dst_a = opcode.reg_dst;
          auto reg_dst_b = static_cast<GPR>(static_cast<int>(reg_dst_a) + 1);
          auto& address_a = address;
          auto& address_b = emitter->CreateVar(IRDataType::UInt32);
          auto& data_a = data;
          auto& data_b = emitter->CreateVar(IRDataType::UInt32, "data");

          // STRD with odd-numbered destination register is undefined.
          if ((static_cast<int>(reg_dst_a) & 1) == 1) {
            return Status::Unimplemented;
          }
          
          emitter->LoadGPR(IRGuestReg{reg_dst_a, mode}, data_a);
          emitter->LoadGPR(IRGuestReg{reg_dst_b, mode}, data_b);
          emitter->ADD(address_b, address_a, IRConstant{sizeof(u32)}, false);
          emitter->STR(Word, data_a, address_a);
          emitter->STR(Word, data_b, address_b);
        }

        writeback();
      }
      break;
    }
    default: {
      // Unreachable?
      return Status::Unimplemented;
    }
  }

  if (should_flush_pipeline) {
    if (armv5te) {
      // Branch with exchange
      auto& address = emitter->CreateVar(IRDataType::UInt32, "address");
      emitter->LoadGPR(IRGuestReg{GPR::PC, mode}, address);
      EmitFlushExchange(address);
    } else {
      EmitFlushNoSwitch();
    }
    return Status::BreakBasicBlock;
  }

  return Status::Continue;
}

} // namespace lunatic::frontend
} // namespace lunatic
