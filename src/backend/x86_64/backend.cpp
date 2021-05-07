/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <algorithm>
#include <list>
#include <stdexcept>

#include "backend.hpp"

#define DESTRUCTURE_CONTEXT auto& [code, reg_alloc, state, location] = context;

namespace lunatic {
namespace backend {

// using namespace lunatic::frontend;
using namespace Xbyak::util;

// TODO: implement spilling to the stack

// TODO: optimize cases where an operand variable expires during the IR opcode we're currently compiling.
// In that case the host register that is assigned to it might be reused for the result variable to do some optimization.

// TODO: right now we only have UInt32 but how to deal with different variable or constant data types?

void X64Backend::Run(Memory& memory, State& state, IREmitter const& emitter) {
  Xbyak::CodeGenerator code;
  X64RegisterAllocator reg_alloc { emitter, code };

  this->memory = &memory;

  // Load pointer to state into RCX
  code.mov(rcx, u64(&state));

  // Load carry flag from state into AX register.
  // Right now we assume we will only need the old carry, is this true?
  code.mov(edx, dword[rcx + state.GetOffsetToCPSR()]);
  code.bt(edx, 29); // CF = value of bit 29
  code.lahf();

  int location = 0;

  auto context = CompileContext{code, reg_alloc, state, location};

  for (auto const& op_ : emitter.Code()) {
    switch (op_->GetClass()) {
      case IROpcodeClass::LoadGPR:
        CompileLoadGPR(context, lunatic_cast<IRLoadGPR>(op_.get()));
        break;
      case IROpcodeClass::StoreGPR:
        CompileStoreGPR(context, lunatic_cast<IRStoreGPR>(op_.get()));
        break;
      case IROpcodeClass::LoadCPSR:
        CompileLoadCPSR(context, lunatic_cast<IRLoadCPSR>(op_.get()));
        break;
      case IROpcodeClass::StoreCPSR:
        CompileStoreCPSR(context, lunatic_cast<IRStoreCPSR>(op_.get()));
        break;
      case IROpcodeClass::ClearCarry:
        code.and_(ah, ~1);
        break;
      case IROpcodeClass::SetCarry:
        code.or_(ah, 1);
        break;
      case IROpcodeClass::UpdateFlags:
        CompileUpdateFlags(context, lunatic_cast<IRUpdateFlags>(op_.get()));
        break;
      case IROpcodeClass::LSL:
        CompileLSL(context, lunatic_cast<IRLogicalShiftLeft>(op_.get()));
        break;
      case IROpcodeClass::LSR:
        CompileLSR(context, lunatic_cast<IRLogicalShiftRight>(op_.get()));
        break;
      case IROpcodeClass::ASR:
        CompileASR(context, lunatic_cast<IRArithmeticShiftRight>(op_.get()));
        break;
      case IROpcodeClass::ROR:
        CompileROR(context, lunatic_cast<IRRotateRight>(op_.get()));
        break;
      case IROpcodeClass::AND:
        CompileAND(context, lunatic_cast<IRBitwiseAND>(op_.get()));
        break;
      case IROpcodeClass::BIC:
        CompileBIC(context, lunatic_cast<IRBitwiseBIC>(op_.get()));
        break;
      case IROpcodeClass::EOR:
        CompileEOR(context, lunatic_cast<IRBitwiseEOR>(op_.get()));
        break;
      case IROpcodeClass::SUB:
        CompileSUB(context, lunatic_cast<IRSub>(op_.get()));
        break;
      case IROpcodeClass::RSB:
        CompileRSB(context, lunatic_cast<IRRsb>(op_.get()));
        break;
      case IROpcodeClass::ADD:
        CompileADD(context, lunatic_cast<IRAdd>(op_.get()));
        break;
      case IROpcodeClass::ADC:
        CompileADC(context, lunatic_cast<IRAdc>(op_.get()));
        break;
      case IROpcodeClass::SBC:
        CompileSBC(context, lunatic_cast<IRSbc>(op_.get()));
        break;
      case IROpcodeClass::RSC:
        CompileRSC(context, lunatic_cast<IRRsc>(op_.get()));
        break;
      case IROpcodeClass::ORR:
        CompileORR(context, lunatic_cast<IRBitwiseORR>(op_.get()));
        break;
      case IROpcodeClass::MOV:
        CompileMOV(context, lunatic_cast<IRMov>(op_.get()));
        break;
      case IROpcodeClass::MVN:
        CompileMVN(context, lunatic_cast<IRMvn>(op_.get()));
        break;
      case IROpcodeClass::MemoryRead:
        CompileMemoryRead(context, lunatic_cast<IRMemoryRead>(op_.get()));
        break;
      default:
        throw std::runtime_error(
          fmt::format("X64Backend: unhandled IR opcode: {}", op_->ToString())
        );
    }

    location++;
  }

  // Restore any callee-saved registers.
  reg_alloc.Finalize();

  code.ret();
  code.getCode<void (*)()>()();
}

void X64Backend::CompileLoadGPR(CompileContext const& context, IRLoadGPR* op) {
  DESTRUCTURE_CONTEXT;

  auto address  = rcx + state.GetOffsetToGPR(op->reg.mode, op->reg.reg);
  auto host_reg = reg_alloc.GetReg32(op->result, location);

  code.mov(host_reg, dword[address]);  
}

void X64Backend::CompileStoreGPR(CompileContext const& context, IRStoreGPR* op) {
  DESTRUCTURE_CONTEXT;

  auto address = rcx + state.GetOffsetToGPR(op->reg.mode, op->reg.reg);

  if (op->value.IsConstant()) {
    code.mov(dword[address], op->value.GetConst().value);
  } else {
    auto host_reg = reg_alloc.GetReg32(op->value.GetVar(), location);

    code.mov(dword[address], host_reg);
  }
}

void X64Backend::CompileLoadCPSR(CompileContext const& context, IRLoadCPSR* op) {
  DESTRUCTURE_CONTEXT;

  auto address = rcx + state.GetOffsetToCPSR();
  auto host_reg = reg_alloc.GetReg32(op->result, location);

  code.mov(host_reg, dword[address]);
}

void X64Backend::CompileStoreCPSR(CompileContext const& context, IRStoreCPSR* op) {
  DESTRUCTURE_CONTEXT;

  auto address = rcx + state.GetOffsetToCPSR();
  
  if (op->value.IsConstant()) {
    code.mov(dword[address], op->value.GetConst().value);
  } else {
    auto host_reg = reg_alloc.GetReg32(op->value.GetVar(), location);

    code.mov(dword[address], host_reg);
  }
}

void X64Backend::CompileUpdateFlags(CompileContext const& context, IRUpdateFlags* op) {
  DESTRUCTURE_CONTEXT;

  u32 mask = 0;
  auto result_reg = reg_alloc.GetReg32(op->result, location);
  auto input_reg  = reg_alloc.GetReg32(op->input, location);

  if (op->flag_n) mask |= 0x80000000;
  if (op->flag_z) mask |= 0x40000000;
  if (op->flag_c) mask |= 0x20000000;
  if (op->flag_v) mask |= 0x10000000;

  // TODO: properly allocate a temporary register...
  code.push(rcx);

  // Convert NZCV bits from AX register into the guest format.
  // Clear the bits which are not to be updated.
  // Note: this trashes AX and means that UpdateFlags() cannot be called again,
  // until another IR opcode updates the flags in AX again.
  code.mov(ecx, 0xC101);
  code.pext(eax, eax, ecx);
  code.shl(eax, 28);
  code.and_(eax, mask);

  // Clear the bits to be updated, then OR the new values.
  code.mov(result_reg, input_reg);
  code.and_(result_reg, ~mask);
  code.or_(result_reg, eax);

  code.pop(rcx);
}

void X64Backend::CompileLSL(CompileContext const& context, IRLogicalShiftLeft* op) {
  DESTRUCTURE_CONTEXT;

  auto amount = op->amount;
  auto result_reg = reg_alloc.GetReg32(op->result, location);
  auto operand_reg = reg_alloc.GetReg32(op->operand, location);

  code.mov(result_reg, operand_reg);
  code.shl(result_reg.cvt64(), 32);

  if (amount.IsConstant()) {
    if (op->update_host_flags) {
      code.sahf();
    }
    code.shl(result_reg.cvt64(), u8(std::min(amount.GetConst().value, 33U)));
  } else {
    auto amount_reg = reg_alloc.GetReg32(op->amount.GetVar(), location);
  
    // TODO: is there a better way that avoids push/pop rcx?
    code.push(rcx);

    code.mov(ecx, 33);
    code.cmp(amount_reg, u8(33));
    code.cmovl(ecx, amount_reg);

    if (op->update_host_flags) {
      code.sahf();
    }
    code.shl(result_reg.cvt64(), cl);

    code.pop(rcx);
  }

  if (op->update_host_flags) {
    code.lahf();
  }

  code.shr(result_reg.cvt64(), 32);
}

void X64Backend::CompileLSR(CompileContext const& context, IRLogicalShiftRight* op) {
  DESTRUCTURE_CONTEXT;

  auto amount = op->amount;
  auto result_reg = reg_alloc.GetReg32(op->result, location);
  auto operand_reg = reg_alloc.GetReg32(op->operand, location);

  code.mov(result_reg, operand_reg);

  if (amount.IsConstant()) {
    auto amount_value = amount.GetConst().value;

    // LSR #0 equals to LSR #32
    if (amount_value == 0) {
      amount_value = 32;
    }

    if (op->update_host_flags) {
      code.sahf();
    }

    code.shr(result_reg.cvt64(), u8(std::min(amount_value, 33U)));
  } else {
    auto amount_reg = reg_alloc.GetReg32(op->amount.GetVar(), location);
    // TODO: is there a better way that avoids push/pop rcx?
    code.push(rcx);

    code.mov(ecx, 33);
    code.cmp(amount_reg, u8(33));
    code.cmovl(ecx, amount_reg);

    if (op->update_host_flags) {
      code.sahf();
    }

    code.shr(result_reg.cvt64(), cl);

    code.pop(rcx);
  }

  if (op->update_host_flags) {
    code.lahf();
  }
}

void X64Backend::CompileASR(CompileContext const& context, IRArithmeticShiftRight* op) {
  DESTRUCTURE_CONTEXT;

  auto amount = op->amount;
  auto result_reg = reg_alloc.GetReg32(op->result, location);
  auto operand_reg = reg_alloc.GetReg32(op->operand, location);

  // Mirror sign-bit in the upper 32-bit of the full 64-bit register.
  code.movsxd(result_reg.cvt64(), operand_reg);

  // TODO: change shift amount saturation from 33 to 32 for ASR? 32 would also work I guess?

  if (amount.IsConstant()) {
    auto amount_value = amount.GetConst().value;

    // ASR #0 equals to ASR #32
    if (amount_value == 0) {
      amount_value = 32;
    }

    if (op->update_host_flags) {
      code.sahf();
    }

    code.sar(result_reg.cvt64(), u8(std::min(amount_value, 33U)));
  } else {
    auto amount_reg = reg_alloc.GetReg32(op->amount.GetVar(), location);
    // TODO: is there a better way that avoids push/pop rcx?
    code.push(rcx);

    code.mov(ecx, 33);
    code.cmp(amount_reg, u8(33));
    code.cmovl(ecx, amount_reg);

    if (op->update_host_flags) {
      code.sahf();
    }

    code.sar(result_reg.cvt64(), cl);

    code.pop(rcx);
  }

  if (op->update_host_flags) {
    code.lahf();
  }

  // Clear upper 32-bit of the result
  code.mov(result_reg, result_reg);
}

void X64Backend::CompileROR(CompileContext const& context, IRRotateRight* op) {
  DESTRUCTURE_CONTEXT;

  auto amount = op->amount;
  auto result_reg = reg_alloc.GetReg32(op->result, location);
  auto operand_reg = reg_alloc.GetReg32(op->operand, location);

  code.mov(result_reg, operand_reg);

  if (amount.IsConstant()) {
    auto amount_value = amount.GetConst().value;

    if (op->update_host_flags) {
      code.sahf();
    }

    // ROR #0 equals to RRX #1
    if (amount_value == 0) {
      code.rcr(result_reg, 1);
    } else {
      code.ror(result_reg, u8(amount_value));
    }
  } else {
    auto amount_reg = reg_alloc.GetReg32(op->amount.GetVar(), location);
    // TODO: is there a better way that avoids push/pop rcx?
    code.push(rcx);

    code.mov(cl, amount_reg.cvt8());

    if (op->update_host_flags) {
      code.sahf();
    }

    code.ror(result_reg, cl);

    code.pop(rcx);
  }

  if (op->update_host_flags) {
    code.lahf();
  }
}

void X64Backend::CompileAND(CompileContext const& context, IRBitwiseAND* op) {
  DESTRUCTURE_CONTEXT;

  auto lhs_reg = reg_alloc.GetReg32(op->lhs, location);

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;

    if (op->result.IsNull()) {
      code.test(lhs_reg, imm);
    } else {
      auto result_reg = reg_alloc.GetReg32(op->result.Unwrap(), location);

      code.mov(result_reg, lhs_reg);
      code.and_(result_reg, imm);
    }
  } else {
    auto rhs_reg = reg_alloc.GetReg32(op->rhs.GetVar(), location);

    if (op->result.IsNull()) {
      code.test(lhs_reg, rhs_reg);
    } else {
      auto result_reg = reg_alloc.GetReg32(op->result.Unwrap(), location);

      code.mov(result_reg, lhs_reg);
      code.and_(result_reg, rhs_reg);
    }
  }

  if (op->update_host_flags) {
    // load flags but preserve carry
    code.bt(ax, 8); // CF = value of bit8
    code.lahf();
  }
}

void X64Backend::CompileBIC(CompileContext const& context, IRBitwiseBIC* op) {
  DESTRUCTURE_CONTEXT;

  auto result_reg = reg_alloc.GetReg32(op->result.Unwrap(), location);
  auto lhs_reg = reg_alloc.GetReg32(op->lhs, location);

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;

    code.mov(result_reg, lhs_reg);
    code.and_(result_reg, ~imm);
  } else {
    auto rhs_reg = reg_alloc.GetReg32(op->rhs.GetVar(), location);

    code.mov(result_reg, rhs_reg);
    code.not_(result_reg);
    code.and_(result_reg, lhs_reg);
  }

  if (op->update_host_flags) {
    // load flags but preserve carry
    code.bt(ax, 8); // CF = value of bit8
    code.lahf();
  }
}

void X64Backend::CompileEOR(CompileContext const& context, IRBitwiseEOR* op) {
  DESTRUCTURE_CONTEXT;

  auto lhs_reg = reg_alloc.GetReg32(op->lhs, location);

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;

    if (op->result.IsNull()) {
      /* TODO: possible optimization when lhs_reg != rhs_reg
       *   xor lhs_reg, rhs_reg
       *   bt ax, 8
       *   lahf
       *   xor lhs_reg, rhs_reg
       */
      code.push(lhs_reg.cvt64());
      code.xor_(lhs_reg, imm);
      code.pop(lhs_reg.cvt64());
    } else {
      auto result_reg = reg_alloc.GetReg32(op->result.Unwrap(), location);

      code.mov(result_reg, lhs_reg);
      code.xor_(result_reg, imm);
    }
  } else {
    auto rhs_reg = reg_alloc.GetReg32(op->rhs.GetVar(), location);

    if (op->result.IsNull()) {
      /* TODO: possible optimization when lhs_reg != rhs_reg
       *   xor lhs_reg, rhs_reg
       *   bt ax, 8
       *   lahf
       *   xor lhs_reg, rhs_reg
       */
      code.push(lhs_reg.cvt64());
      code.xor_(lhs_reg, rhs_reg);
      code.pop(lhs_reg.cvt64());
    } else {
      auto result_reg = reg_alloc.GetReg32(op->result.Unwrap(), location);

      code.mov(result_reg, lhs_reg);
      code.xor_(result_reg, rhs_reg);
    }
  }

  if (op->update_host_flags) {
    // load flags but preserve carry
    code.bt(ax, 8); // CF = value of bit8
    code.lahf();
  }
}

void X64Backend::CompileSUB(CompileContext const& context, IRSub* op) {
  DESTRUCTURE_CONTEXT;

  auto lhs_reg = reg_alloc.GetReg32(op->lhs, location);

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;

    if (op->result.IsNull()) {
      code.cmp(lhs_reg, imm);
      code.cmc();
    } else {
      auto result_reg = reg_alloc.GetReg32(op->result.Unwrap(), location);

      code.mov(result_reg, lhs_reg);
      code.sub(result_reg, imm);
      code.cmc();
    }
  } else {
    auto rhs_reg = reg_alloc.GetReg32(op->rhs.GetVar(), location);

    if (op->result.IsNull()) {
      code.cmp(lhs_reg, rhs_reg);
      code.cmc();
    } else {
      auto result_reg = reg_alloc.GetReg32(op->result.Unwrap(), location);

      code.mov(result_reg, lhs_reg);
      code.sub(result_reg, rhs_reg);
      code.cmc();
    }
  }
    
  if (op->update_host_flags) {
    code.lahf();
    code.seto(al);
  }
}

void X64Backend::CompileRSB(CompileContext const& context, IRRsb* op) {
  DESTRUCTURE_CONTEXT;

  auto result_reg = reg_alloc.GetReg32(op->result.Unwrap(), location);
  auto lhs_reg = reg_alloc.GetReg32(op->lhs, location);

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;

    code.mov(result_reg, imm);
    code.sub(result_reg, lhs_reg);
    code.cmc();
  } else {
    auto rhs_reg = reg_alloc.GetReg32(op->rhs.GetVar(), location);

    code.mov(result_reg, rhs_reg);
    code.sub(result_reg, lhs_reg);
    code.cmc();
  }

  if (op->update_host_flags) {
    code.lahf();
    code.seto(al);
  }
}

void X64Backend::CompileADD(CompileContext const& context, IRAdd* op) {
  DESTRUCTURE_CONTEXT;

  auto lhs_reg = reg_alloc.GetReg32(op->lhs, location);

  if (op->result.IsNull() && !op->update_host_flags) {
    return;
  }

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;

    if (op->result.IsNull()) {
      // eax will be trashed by lahf anyways
      code.mov(eax, lhs_reg);
      code.add(eax, imm);
    } else {
      auto result_reg = reg_alloc.GetReg32(op->result.Unwrap(), location);

      code.mov(result_reg, lhs_reg);
      code.add(result_reg, imm); 
    }
  } else {
    auto rhs_reg = reg_alloc.GetReg32(op->rhs.GetVar(), location);

    if (op->result.IsNull()) {
      // eax will be trashed by lahf anyways
      code.mov(eax, lhs_reg);
      code.add(eax, rhs_reg);
    } else {
      auto result_reg = reg_alloc.GetReg32(op->result.Unwrap(), location);

      code.mov(result_reg, lhs_reg);
      code.add(result_reg, rhs_reg);
    }
  }

  if (op->update_host_flags) {
    code.lahf();
    code.seto(al);
  }
}

void X64Backend::CompileADC(CompileContext const& context, IRAdc* op) {
  DESTRUCTURE_CONTEXT;

  auto result_reg = reg_alloc.GetReg32(op->result.Unwrap(), location);
  auto lhs_reg = reg_alloc.GetReg32(op->lhs, location);

  code.sahf();

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;

    code.mov(result_reg, lhs_reg);
    code.adc(result_reg, imm); 
  } else {
    auto rhs_reg = reg_alloc.GetReg32(op->rhs.GetVar(), location);

    code.mov(result_reg, lhs_reg);
    code.adc(result_reg, rhs_reg);
  }

  if (op->update_host_flags) {
    code.lahf();
    code.seto(al);
  }
}

void X64Backend::CompileSBC(CompileContext const& context, IRSbc* op) {
  DESTRUCTURE_CONTEXT;

  auto result_reg = reg_alloc.GetReg32(op->result.Unwrap(), location);
  auto lhs_reg = reg_alloc.GetReg32(op->lhs, location);

  code.sahf();
  code.cmc();

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;

    code.mov(result_reg, lhs_reg);
    code.sbb(result_reg, imm);
    code.cmc();
  } else {
    auto rhs_reg = reg_alloc.GetReg32(op->rhs.GetVar(), location);

    code.mov(result_reg, lhs_reg);
    code.sbb(result_reg, rhs_reg);
    code.cmc();
  }
    
  if (op->update_host_flags) {
    code.lahf();
    code.seto(al);
  }
}

void X64Backend::CompileRSC(CompileContext const& context, IRRsc* op) {
  DESTRUCTURE_CONTEXT;

  auto result_reg = reg_alloc.GetReg32(op->result.Unwrap(), location);
  auto lhs_reg = reg_alloc.GetReg32(op->lhs, location);

  code.sahf();
  code.cmc();

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;

    code.mov(result_reg, imm);
    code.sbb(result_reg, lhs_reg);
    code.cmc();
  } else {
    auto rhs_reg = reg_alloc.GetReg32(op->rhs.GetVar(), location);

    code.mov(result_reg, rhs_reg);
    code.sbb(result_reg, lhs_reg);
    code.cmc();
  }

  if (op->update_host_flags) {
    code.lahf();
    code.seto(al);
  }
}

void X64Backend::CompileORR(CompileContext const& context, IRBitwiseORR* op) {
  DESTRUCTURE_CONTEXT;

  auto result_reg = reg_alloc.GetReg32(op->result.Unwrap(), location);
  auto lhs_reg = reg_alloc.GetReg32(op->lhs, location);

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;
    
    code.mov(result_reg, lhs_reg);
    code.or_(result_reg, imm);
  } else {
    auto rhs_reg = reg_alloc.GetReg32(op->rhs.GetVar(), location);

    code.mov(result_reg, lhs_reg);
    code.or_(result_reg, rhs_reg);
  }

  if (op->update_host_flags) {
    // load flags but preserve carry
    code.bt(ax, 8); // CF = value of bit8
    code.lahf();
  }
}

void X64Backend::CompileMOV(CompileContext const& context, IRMov* op) {
  DESTRUCTURE_CONTEXT;

  auto result_reg = reg_alloc.GetReg32(op->result, location);

  if (op->source.IsConstant()) {
    code.mov(result_reg, op->source.GetConst().value);
  } else {
    code.mov(result_reg, reg_alloc.GetReg32(op->source.GetVar(), location));
  }

  if (op->update_host_flags) {
    code.test(result_reg, result_reg);
    // load flags but preserve carry
    code.bt(ax, 8); // CF = value of bit8
    code.lahf();
  }
}

void X64Backend::CompileMVN(CompileContext const& context, IRMvn* op) {
  DESTRUCTURE_CONTEXT;

  auto result_reg = reg_alloc.GetReg32(op->result, location);

  if (op->source.IsConstant()) {
    code.mov(result_reg, op->source.GetConst().value);
  } else {
    code.mov(result_reg, reg_alloc.GetReg32(op->source.GetVar(), location));
  }

  code.not_(result_reg);

  if (op->update_host_flags) {
    code.test(result_reg, result_reg);
    // load flags but preserve carry
    code.bt(ax, 8); // CF = value of bit8
    code.lahf();
  }
}

void X64Backend::CompileMemoryRead(CompileContext const& context, IRMemoryRead* op) {
  DESTRUCTURE_CONTEXT;

  auto result_reg = reg_alloc.GetReg32(op->result, location);
  auto address_reg = reg_alloc.GetReg32(op->address, location);
  auto flags = op->flags;

  auto label_slowmem = Xbyak::Label{};
  auto label_final = Xbyak::Label{};
  auto pagetable = memory->pagetable.get();

  // TODO: properly allocate a free register.
  // Or statically allocate a register for the page table pointer?
  code.push(rcx);

  if (pagetable != nullptr) {
    code.mov(rcx, u64(pagetable));

    // TODO: check for TCM!!!

    // Get the page table entry
    code.mov(result_reg, address_reg);
    code.shr(result_reg, Memory::kPageShift);
    code.mov(rcx, qword[rcx + result_reg.cvt64() * sizeof(uintptr)]);

    // Check if the entry is a null pointer.
    code.test(rcx, rcx);
    code.jz(label_slowmem);

    code.mov(result_reg, address_reg);

    if (flags & Word) {
      code.and_(result_reg, Memory::kPageMask & ~3);
      code.mov(result_reg, dword[rcx + result_reg.cvt64()]);
    }

    if (flags & Half) {
      code.and_(result_reg, Memory::kPageMask & ~1);
      code.movzx(result_reg, word[rcx + result_reg.cvt64()]);
    }

    if (flags & Byte) {
      code.and_(result_reg, Memory::kPageMask);
      code.movzx(result_reg, byte[rcx + result_reg.cvt64()]);
    }

    code.jmp(label_final);
  }

  code.L(label_slowmem);

  // TODO: determine which registers need to be saved.
  code.push(rax);
  code.push(rdx);
  code.push(r8);
  code.push(r9);
  code.push(r10);
  code.push(r11);

  code.mov(edx, address_reg);

  if (flags & Word) {
    code.and_(edx, ~3);
    code.mov(rax, u64((void*)&X64Backend::ReadWord));
  }

  if (flags & Half) {
    code.and_(edx, ~1);
    code.mov(rax, u64((void*)&X64Backend::ReadHalf));
  }

  if (flags & Byte) {
    code.mov(rax, u64((void*)&X64Backend::ReadByte));
  }

  code.mov(rcx, u64(this));
  code.mov(r8d, u32(Memory::Bus::Data));

  // TODO: make sure the stack is actually properly aligned.
  code.sub(rsp, 0x28);
  code.call(rax);
  code.add(rsp, 0x28);

  code.pop(r11);
  code.pop(r10);
  code.pop(r9);
  code.pop(r8);
  code.pop(rdx);
  code.mov(result_reg, eax);
  code.pop(rax);

  code.L(label_final);

  if (flags & Rotate) {
    if (flags & Word) {
      code.mov(ecx, address_reg);
      code.and_(cl, 3);
      code.shl(cl, 3);
      code.ror(result_reg, cl);
    }

    if (flags & Half) {
      code.mov(ecx, address_reg);
      code.and_(cl, 1);
      code.shl(cl, 3);
      code.ror(result_reg.cvt16(), cl); 
    }
  }

  code.pop(rcx);
}

} // namespace lunatic::backend
} // namespace lunatic
