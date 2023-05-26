/*
 * Copyright (C) 2023 marysaka. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.hpp"

namespace lunatic::backend {

void ARM64Backend::HandleTCMRead(CompileContext const& context, oaknut::WReg const& address_reg, oaknut::WReg const& result_reg, IRMemoryFlags flags, oaknut::XReg const& tcm_address_reg, oaknut::WReg const& tcm_scratch_reg, Memory::TCM const& tcm, oaknut::Label & label_final) {
  if (tcm.data == nullptr) {
    return;
  }

  DESTRUCTURE_CONTEXT;

  auto label_not_tcm = oaknut::Label{};

  code.MOVP2R(tcm_address_reg, &tcm);

  code.LDRB(tcm_scratch_reg, tcm_address_reg, offsetof(Memory::TCM, config.enable_read));
  code.CMP(tcm_scratch_reg, 0);
  code.B(oaknut::Cond::EQ, label_not_tcm);

  code.LDR(tcm_scratch_reg, tcm_address_reg, offsetof(Memory::TCM, config.base));
  code.CMP(address_reg, tcm_scratch_reg);
  code.B(oaknut::Cond::LO, label_not_tcm);

  code.LDR(tcm_scratch_reg, tcm_address_reg, offsetof(Memory::TCM, config.limit));
  code.CMP(address_reg, tcm_scratch_reg);
  code.B(oaknut::Cond::GT, label_not_tcm);

  code.LDR(tcm_scratch_reg, tcm_address_reg, offsetof(Memory::TCM, config.base));
  code.SUB(tcm_scratch_reg, address_reg, tcm_scratch_reg);

  code.MOVP2R(tcm_address_reg, &tcm.data);

  bool is_signed = (flags & Signed) != 0;

  if (flags & Word) {
    code.AND(tcm_scratch_reg, tcm_scratch_reg, tcm.mask & ~3);
    code.ADD(tcm_address_reg, tcm_address_reg, tcm_scratch_reg);
    code.LDR(result_reg, tcm_address_reg);
  } else if (flags & Half) {
    code.AND(tcm_scratch_reg, tcm_scratch_reg, tcm.mask & ~3);
    code.ADD(tcm_address_reg, tcm_address_reg, tcm_scratch_reg);

    if (is_signed) {
      code.LDRSH(result_reg, tcm_address_reg);
    } else {
      code.LDRH(result_reg, tcm_address_reg);
    }
  } else if (flags & Byte) {
    code.AND(tcm_scratch_reg, tcm_scratch_reg, tcm.mask);
    code.ADD(tcm_address_reg, tcm_address_reg, tcm_scratch_reg);

    if (is_signed) {
      code.LDRSB(result_reg, tcm_address_reg);
    } else {
      code.LDRB(result_reg, tcm_address_reg);
    }
  }

  code.B(label_final);
  code.l(label_not_tcm);
}

void ARM64Backend::CompileMemoryRead(CompileContext const& context, IRMemoryRead* op) {
  DESTRUCTURE_CONTEXT;

  oaknut::WReg address_reg(-1);
  auto& address = op->address;

  if (address.IsVariable()) {
    address_reg = reg_alloc.GetVariableHostReg(address.GetVar());
  } else {
    address_reg = reg_alloc.GetTemporaryHostReg();
    code.MOV(address_reg, address.GetConst().value);
  }

  auto result_reg = reg_alloc.GetVariableHostReg(op->result.Get());
  auto flags = op->flags;

  auto label_slowmem = oaknut::Label{};
  auto label_final = oaknut::Label{};
  auto pagetable = memory.pagetable.get();

  auto& itcm = memory.itcm;
  auto& dtcm = memory.dtcm;

  auto scratch_reg0 = reg_alloc.GetTemporaryHostReg().toX();
  auto scratch_reg1 = reg_alloc.GetTemporaryHostReg();

  HandleTCMRead(context, address_reg, result_reg, flags, scratch_reg0, scratch_reg1, itcm, label_final);
  HandleTCMRead(context, address_reg, result_reg, flags, scratch_reg0, scratch_reg1, dtcm, label_final);

  if (pagetable != nullptr) {
    // Get the page table entry
    code.MOVP2R(scratch_reg0, pagetable);
    code.LSR(scratch_reg1.toW(), address_reg, Memory::kPageShift);
    code.LSL(scratch_reg1.toW(), scratch_reg1.toW(), std::log2(sizeof(uintptr_t)));
    code.ADD(scratch_reg0, scratch_reg0, scratch_reg1.toX());
    code.LDR(scratch_reg0, scratch_reg0);

    // Check if the entry is a null pointer.
    code.TST(scratch_reg0, scratch_reg0);
    code.B(oaknut::Cond::EQ, label_slowmem);

    code.MOV(scratch_reg1.toW(), address_reg);

    bool is_signed = (flags & Signed) != 0;

    if (flags & Word) {
      code.AND(scratch_reg1, scratch_reg1, Memory::kPageMask & ~3);
      code.ADD(scratch_reg0, scratch_reg0, scratch_reg1.toX());
      code.LDR(result_reg, scratch_reg0);
    } else if (flags & Half) {
      code.AND(scratch_reg1, scratch_reg1, Memory::kPageMask & ~1);
      code.ADD(scratch_reg0, scratch_reg0, scratch_reg1.toX());

      if (is_signed) {
        code.LDRSH(result_reg, scratch_reg0);
      } else {
        code.LDRH(result_reg, scratch_reg0);
      }
    } else if (flags & Byte) {
      code.AND(scratch_reg1, scratch_reg1, Memory::kPageMask);
      code.ADD(scratch_reg0, scratch_reg0, scratch_reg1.toX());

      if (is_signed) {
        code.LDRSB(result_reg, scratch_reg0);
      } else {
        code.LDRB(result_reg, scratch_reg0);
      }
    }

    code.B(label_final);
  }

  // TODO: Slow path!
  // TODO: Slow path!
  code.l(label_slowmem);

  uintptr_t slow_memory_callback;

  if (flags & Word) {
    code.AND(scratch_reg0, address_reg.toX(), ~3);
    slow_memory_callback = reinterpret_cast<uintptr_t>(ReadWord);
  } else if (flags & Half) {
    code.AND(scratch_reg0, address_reg.toX(), ~1);
    slow_memory_callback = reinterpret_cast<uintptr_t>(ReadHalf);
  } else if (flags & Byte) {
    code.MOV(scratch_reg0, address_reg.toX());
    slow_memory_callback = reinterpret_cast<uintptr_t>(ReadByte);
  }

  code.MOVP2R(X0, &memory);
  code.MOV(X2, uint32_t(Memory::Bus::Data));
  EmitFunctionCall(context, slow_memory_callback, { X0, scratch_reg0, X2 });
  code.MOV(result_reg, W0);

  code.l(label_final);
}

void ARM64Backend::HandleTCMWrite(CompileContext const& context, oaknut::WReg const& address_reg, oaknut::WReg const& source_reg, IRMemoryFlags flags, oaknut::XReg const& tcm_address_reg, oaknut::WReg const& tcm_scratch_reg, Memory::TCM const& tcm, oaknut::Label & label_final) {
  if (tcm.data == nullptr) {
    return;
  }

  DESTRUCTURE_CONTEXT;

  auto label_not_tcm = oaknut::Label{};

  code.MOVP2R(tcm_address_reg, &tcm);

  code.LDRB(tcm_scratch_reg, tcm_address_reg, offsetof(Memory::TCM, config.enable_read));
  code.CMP(tcm_scratch_reg, 0);
  code.B(oaknut::Cond::EQ, label_not_tcm);

  code.LDR(tcm_scratch_reg, tcm_address_reg, offsetof(Memory::TCM, config.base));
  code.CMP(address_reg, tcm_scratch_reg);
  code.B(oaknut::Cond::LO, label_not_tcm);

  code.LDR(tcm_scratch_reg, tcm_address_reg, offsetof(Memory::TCM, config.limit));
  code.CMP(address_reg, tcm_scratch_reg);
  code.B(oaknut::Cond::GT, label_not_tcm);

  code.LDR(tcm_scratch_reg, tcm_address_reg, offsetof(Memory::TCM, config.base));
  code.SUB(tcm_scratch_reg, address_reg, tcm_scratch_reg);

  code.MOVP2R(tcm_address_reg, &tcm.data);

  if (flags & Word) {
    code.AND(tcm_scratch_reg, tcm_scratch_reg, tcm.mask & ~3);
    code.ADD(tcm_address_reg, tcm_address_reg, tcm_scratch_reg);
    code.STR(source_reg, tcm_address_reg);
  } else if (flags & Half) {
    code.AND(tcm_scratch_reg, tcm_scratch_reg, tcm.mask & ~3);
    code.ADD(tcm_address_reg, tcm_address_reg, tcm_scratch_reg);
    code.STRH(source_reg, tcm_address_reg);
  } else if (flags & Byte) {
    code.AND(tcm_scratch_reg, tcm_scratch_reg, tcm.mask);
    code.ADD(tcm_address_reg, tcm_address_reg, tcm_scratch_reg);
    code.STRB(source_reg, tcm_address_reg);
  }

  code.B(label_final);
  code.l(label_not_tcm);
}

void ARM64Backend::CompileMemoryWrite(CompileContext const& context, IRMemoryWrite* op) {
  DESTRUCTURE_CONTEXT;

  oaknut::WReg source_reg(-1);
  auto& source = op->source;

  if (source.IsVariable()) {
    source_reg = reg_alloc.GetVariableHostReg(source.GetVar());
  } else {
    source_reg = reg_alloc.GetTemporaryHostReg();
    code.MOV(source_reg, source.GetConst().value);
  }

  oaknut::WReg address_reg(-1);
  auto& address = op->address;

  if (address.IsVariable()) {
    address_reg = reg_alloc.GetVariableHostReg(address.GetVar());
  } else {
    address_reg = reg_alloc.GetTemporaryHostReg();
    code.MOV(address_reg, address.GetConst().value);
  }

  auto scratch_reg = reg_alloc.GetTemporaryHostReg();
  auto flags = op->flags;

  auto label_slowmem = oaknut::Label{};
  auto label_final = oaknut::Label{};
  auto pagetable = memory.pagetable.get();

  auto& itcm = memory.itcm;
  auto& dtcm = memory.dtcm;

  auto scratch_reg0 = reg_alloc.GetTemporaryHostReg().toX();
  auto scratch_reg1 = reg_alloc.GetTemporaryHostReg();

  HandleTCMWrite(context, address_reg, source_reg, flags, scratch_reg0, scratch_reg1, itcm, label_final);
  HandleTCMWrite(context, address_reg, source_reg, flags, scratch_reg0, scratch_reg1, dtcm, label_final);

  if (pagetable != nullptr) {
    // Get the page table entry
    code.MOVP2R(scratch_reg0, pagetable);
    code.LSR(scratch_reg1.toW(), address_reg, Memory::kPageShift);
    code.LSL(scratch_reg1.toW(), scratch_reg1.toW(), std::log2(sizeof(uintptr_t)));
    code.ADD(scratch_reg0, scratch_reg0, scratch_reg1.toX());
    code.LDR(scratch_reg0, scratch_reg0);

    // Check if the entry is a null pointer.
    code.TST(scratch_reg0, scratch_reg0);
    code.B(oaknut::Cond::EQ, label_slowmem);

    code.MOV(scratch_reg1.toW(), address_reg);

    if (flags & Word) {
      code.AND(scratch_reg1, scratch_reg1, Memory::kPageMask & ~3);
      code.ADD(scratch_reg0, scratch_reg0, scratch_reg1.toX());
      code.STR(source_reg, scratch_reg0);
    } else if (flags & Half) {
      code.AND(scratch_reg1, scratch_reg1, Memory::kPageMask & ~1);
      code.ADD(scratch_reg0, scratch_reg0, scratch_reg1.toX());
      code.STRH(source_reg, scratch_reg0);
    } else if (flags & Byte) {
      code.AND(scratch_reg1, scratch_reg1, Memory::kPageMask);
      code.ADD(scratch_reg0, scratch_reg0, scratch_reg1.toX());
      code.STRB(source_reg, scratch_reg0);
    }

    code.B(label_final);
  }

  code.l(label_slowmem);

  uintptr_t slow_memory_callback;

  if (flags & Word) {
    code.AND(scratch_reg0, address_reg.toX(), ~3);
    slow_memory_callback = reinterpret_cast<uintptr_t>(&WriteWord);
  } else if (flags & Half) {
    code.AND(scratch_reg0, address_reg.toX(), ~1);
    slow_memory_callback = reinterpret_cast<uintptr_t>(&WriteHalf);
  } else if (flags & Byte) {
    code.MOV(scratch_reg0, address_reg.toX());
    slow_memory_callback = reinterpret_cast<uintptr_t>(&WriteByte);
  }

  code.MOVP2R(X0, &memory);
  code.MOV(X2, uint32_t(Memory::Bus::Data));
  EmitFunctionCall(context, slow_memory_callback, { X0, scratch_reg0, X2, source_reg.toX() });

  code.l(label_final);
}

} // namespace lunatic::backend
