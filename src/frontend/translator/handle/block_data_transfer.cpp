/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMBlockDataTransfer const& opcode) -> Status {
  if (opcode.reg_list == 0) {
    return Status::Unimplemented;
  }

  auto transfer_pc = bit::get_bit(opcode.reg_list, 15);
  auto reg_base = IRGuestReg{opcode.reg_base, mode};

  // TODO: clean this up and document "base is not in rlist" caveat
  bool base_is_first = (opcode.reg_list & ((1 << int(opcode.reg_base)) - 1)) == 0;
  bool base_is_last  = (opcode.reg_list >> int(opcode.reg_base)) == 1;
  bool early_writeback = opcode.writeback && !opcode.load && !armv5te && !base_is_first;

  u32 bytes = 0;

  // Calculate the number of bytes to transfer.
  for (int i = 0; i <= 15; i++) {
    if (bit::get_bit(opcode.reg_list, i))
      bytes += sizeof(u32);
  }

  auto& base_lo = emitter->CreateVar(IRDataType::UInt32, "base_lo");
  auto& base_hi = emitter->CreateVar(IRDataType::UInt32, "base_hi");

  // Calculate the low and high addresses.
  if (opcode.add) {
    emitter->LoadGPR(reg_base, base_lo);
    emitter->ADD(base_hi, base_lo, IRConstant{bytes}, false);
  } else {
    emitter->LoadGPR(reg_base, base_hi);
    emitter->SUB(base_lo, base_hi, IRConstant{bytes}, false);
  }

  auto writeback = [&]() {
    // TODO: is there a cleaner solution without a conditional?
    if (opcode.add) {
      emitter->StoreGPR(reg_base, base_hi);
    } else {
      emitter->StoreGPR(reg_base, base_lo);
    }
  };

  if (!opcode.load || !transfer_pc) {
    EmitAdvancePC();
  }

  auto forced_mode = opcode.user_mode ? Mode::User : mode;
  auto address = &base_lo;

  // STM ARMv4: store new base unless it is the first register
  // STM ARMv5: always store old base.
  if (early_writeback) {
    writeback();
  }

  // Load or store a set of registers from/to memory.
  for (int i = 0; i <= 15; i++)  {
    if (!bit::get_bit(opcode.reg_list, i))
      continue;

    auto  reg  = static_cast<GPR>(i);
    auto& data = emitter->CreateVar(IRDataType::UInt32, "data");
    auto& address_next = emitter->CreateVar(IRDataType::UInt32, "address");

    emitter->ADD(address_next, *address, IRConstant{sizeof(u32)}, false);

    if (opcode.pre_increment == opcode.add) {
      address = &address_next;
    }

    if (opcode.load) {
      emitter->LDR(Word, data, *address);
      emitter->StoreGPR(IRGuestReg{reg, forced_mode}, data);
    } else {
      emitter->LoadGPR(IRGuestReg{reg, forced_mode}, data);
      emitter->STR(Word, data, *address);
    }

    if (opcode.pre_increment != opcode.add) {
      address = &address_next;
    }
  }

  if (opcode.user_mode && opcode.load && transfer_pc) {
    // TODO: base writeback happens in which mode? (this is unpredictable)
    // If writeback happens in the new mode, then this might be difficult
    // to emulate because we cannot know the value of SPSR at compile-time.
    EmitLoadSPSRToCPSR();
  }

  if (opcode.writeback) {
    if (opcode.load) {
      if (armv5te) {
        // LDM ARMv5: writeback if base is the only register or not the last register.
        if (!base_is_last || opcode.reg_list == (1 << int(opcode.reg_base)))
          writeback();
      } else {
        // LDM ARMv4: writeback if base in not in the register list.
        if (!bit::get_bit(opcode.reg_list, int(opcode.reg_base)))
          writeback();
      }
    } else if (!early_writeback) {
      writeback();
    }
  }

  // Flush the pipeline if we loaded R15.
  if (opcode.load && transfer_pc) {
    if (opcode.user_mode) {
      EmitFlush();
    } else if (armv5te) {
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
