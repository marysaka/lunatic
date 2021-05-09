/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Translate(BasicBlock& block, Memory& memory) -> bool {
  auto address = block.key.field.address;

  if (address & 1) {
    // Thumb mode is not supported right now.
    return false;
  }

  mode = block.key.field.mode;
  opcode_size = (block.key.field.address & 1) ? sizeof(u16) : sizeof(u32);
  emitter = &block.emitter;

  static constexpr int kMaxBlockSize = 32;

  for (int i = 0; i < kMaxBlockSize; i++) {
    auto instruction = memory.FastRead<u32, Memory::Bus::Code>(address);
    auto status = decode_arm(instruction, *this);

    if (status == Status::Unimplemented) {
      /* TODO: this is not optimal.
       * Let the callee know that we have something to run,
       * but that the interpreter has to take over after that instead...
       */
      return i != 0;
    }

    if (status == Status::BreakBasicBlock) {
      break;
    }

    address += sizeof(u32);
  }

  return true;
}

auto Translator::Undefined(u32 opcode) -> Status {
  return Status::Unimplemented;
}

void Translator::EmitUpdateNZC() {
  auto& cpsr_in  = emitter->CreateVar(IRDataType::UInt32, "cpsr_in");
  auto& cpsr_out = emitter->CreateVar(IRDataType::UInt32, "cpsr_out");

  emitter->LoadCPSR(cpsr_in);
  emitter->UpdateNZC(cpsr_out, cpsr_in);
  emitter->StoreCPSR(cpsr_out);
}

void Translator::EmitUpdateNZCV() {
  auto& cpsr_in  = emitter->CreateVar(IRDataType::UInt32, "cpsr_in");
  auto& cpsr_out = emitter->CreateVar(IRDataType::UInt32, "cpsr_out");

  emitter->LoadCPSR(cpsr_in);
  emitter->UpdateNZCV(cpsr_out, cpsr_in);
  emitter->StoreCPSR(cpsr_out);
}

void Translator::EmitAdvancePC() {
  auto& r15_in  = emitter->CreateVar(IRDataType::UInt32, "r15_in");
  auto& r15_out = emitter->CreateVar(IRDataType::UInt32, "r15_out");

  emitter->LoadGPR(IRGuestReg{GPR::PC, mode}, r15_in);
  emitter->ADD(r15_out, r15_in, IRConstant{opcode_size}, false);
  emitter->StoreGPR(IRGuestReg{GPR::PC, mode}, r15_out);
}

void Translator::EmitFlushPipeline() {
  auto& r15_in  = emitter->CreateVar(IRDataType::UInt32, "r15_in");
  auto& r15_out = emitter->CreateVar(IRDataType::UInt32, "r15_out");

  emitter->LoadGPR(IRGuestReg{GPR::PC, mode}, r15_in);
  emitter->ADD(r15_out, r15_in, IRConstant{opcode_size * 2}, false);
  emitter->StoreGPR(IRGuestReg{GPR::PC, mode}, r15_out);
}

void Translator::EmitLoadSPSRToCPSR() {
  auto& spsr = emitter->CreateVar(IRDataType::UInt32, "spsr");
  emitter->LoadSPSR(spsr, mode);
  emitter->StoreCPSR(spsr);
}

} // namespace lunatic::frontend
} // namespace lunatic
