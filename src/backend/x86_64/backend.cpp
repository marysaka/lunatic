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

void X64Backend::Compile(Memory& memory, State& state, BasicBlock& basic_block) {
  // TODO: do not keep the code in memory forever.
  auto& emitter = basic_block.emitter;
  auto  code = new Xbyak::CodeGenerator{};
  auto  reg_alloc = X64RegisterAllocator{emitter, *code};
  auto  location = 0;
  auto  context = CompileContext{*code, reg_alloc, state, location};

  this->memory = &memory;

  code->push(rbx);
  code->push(rsi);
  code->push(rdi);
  code->push(rbp);
  code->push(r12);
  code->push(r13);
  code->push(r14);
  code->push(r15);
  code->sub(rsp, 8);

  // Load pointer to state into RCX
  code->mov(rcx, u64(&state));

  // Load carry flag from state into AX register.
  // Right now we assume we will only need the old carry, is this true?
  code->mov(edx, dword[rcx + state.GetOffsetToCPSR()]);
  code->bt(edx, 29); // CF = value of bit 29
  code->lahf();

  for (auto const& op : emitter.Code()) {
    switch (op->GetClass()) {
      case IROpcodeClass::LoadGPR:
        CompileLoadGPR(context, lunatic_cast<IRLoadGPR>(op.get()));
        break;
      case IROpcodeClass::StoreGPR:
        CompileStoreGPR(context, lunatic_cast<IRStoreGPR>(op.get()));
        break;
      case IROpcodeClass::LoadCPSR:
        CompileLoadCPSR(context, lunatic_cast<IRLoadCPSR>(op.get()));
        break;
      case IROpcodeClass::StoreCPSR:
        CompileStoreCPSR(context, lunatic_cast<IRStoreCPSR>(op.get()));
        break;
      case IROpcodeClass::ClearCarry:
        code->and_(ah, ~1);
        break;
      case IROpcodeClass::SetCarry:
        code->or_(ah, 1);
        break;
      case IROpcodeClass::UpdateFlags:
        CompileUpdateFlags(context, lunatic_cast<IRUpdateFlags>(op.get()));
        break;
      case IROpcodeClass::LSL:
        CompileLSL(context, lunatic_cast<IRLogicalShiftLeft>(op.get()));
        break;
      case IROpcodeClass::LSR:
        CompileLSR(context, lunatic_cast<IRLogicalShiftRight>(op.get()));
        break;
      case IROpcodeClass::ASR:
        CompileASR(context, lunatic_cast<IRArithmeticShiftRight>(op.get()));
        break;
      case IROpcodeClass::ROR:
        CompileROR(context, lunatic_cast<IRRotateRight>(op.get()));
        break;
      case IROpcodeClass::AND:
        CompileAND(context, lunatic_cast<IRBitwiseAND>(op.get()));
        break;
      case IROpcodeClass::BIC:
        CompileBIC(context, lunatic_cast<IRBitwiseBIC>(op.get()));
        break;
      case IROpcodeClass::EOR:
        CompileEOR(context, lunatic_cast<IRBitwiseEOR>(op.get()));
        break;
      case IROpcodeClass::SUB:
        CompileSUB(context, lunatic_cast<IRSub>(op.get()));
        break;
      case IROpcodeClass::RSB:
        CompileRSB(context, lunatic_cast<IRRsb>(op.get()));
        break;
      case IROpcodeClass::ADD:
        CompileADD(context, lunatic_cast<IRAdd>(op.get()));
        break;
      case IROpcodeClass::ADC:
        CompileADC(context, lunatic_cast<IRAdc>(op.get()));
        break;
      case IROpcodeClass::SBC:
        CompileSBC(context, lunatic_cast<IRSbc>(op.get()));
        break;
      case IROpcodeClass::RSC:
        CompileRSC(context, lunatic_cast<IRRsc>(op.get()));
        break;
      case IROpcodeClass::ORR:
        CompileORR(context, lunatic_cast<IRBitwiseORR>(op.get()));
        break;
      case IROpcodeClass::MOV:
        CompileMOV(context, lunatic_cast<IRMov>(op.get()));
        break;
      case IROpcodeClass::MVN:
        CompileMVN(context, lunatic_cast<IRMvn>(op.get()));
        break;
      case IROpcodeClass::MemoryRead:
        CompileMemoryRead(context, lunatic_cast<IRMemoryRead>(op.get()));
        break;
      case IROpcodeClass::MemoryWrite:
        CompileMemoryWrite(context, lunatic_cast<IRMemoryWrite>(op.get()));
        break;
      default:
        throw std::runtime_error(
          fmt::format("X64Backend: unhandled IR opcode: {}", op->ToString())
        );
    }

    location++;
  }

  code->add(rsp, 8);
  code->pop(r15);
  code->pop(r14);
  code->pop(r13);
  code->pop(r12);
  code->pop(rbp);
  code->pop(rdi);
  code->pop(rsi);
  code->pop(rbx);
  code->ret();

  basic_block.function = code->getCode<BasicBlock::CompiledFn>();
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

  // TODO: allocate temporary registers?
  code.push(rax);
  code.push(rcx);

  // Convert NZCV bits from AX register into the guest format.
  // Clear the bits which are not to be updated.
  code.mov(ecx, 0xC101);
  code.pext(eax, eax, ecx);
  code.shl(eax, 28);
  code.and_(eax, mask);

  // Clear the bits to be updated, then OR the new values.
  code.mov(result_reg, input_reg);
  code.and_(result_reg, ~mask);
  code.or_(result_reg, eax);

  code.pop(rcx);
  code.pop(rax);
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

    // TODO: check for DTCM!!!

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
      if (flags & Signed) {
        code.movsx(result_reg, word[rcx + result_reg.cvt64()]);
      } else {
        code.movzx(result_reg, word[rcx + result_reg.cvt64()]);
      }
    }

    if (flags & Byte) {
      code.and_(result_reg, Memory::kPageMask);
      if (flags & Signed) {
        code.movsx(result_reg, byte[rcx + result_reg.cvt64()]);
      } else {
        code.movzx(result_reg, byte[rcx + result_reg.cvt64()]);
      }
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

  if (flags & Signed) {
    if (flags & Half) {
      code.movsx(result_reg, result_reg.cvt16());
    }

    if (flags & Byte) {
      code.movsx(result_reg, result_reg.cvt8());
    }
  }

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

  static constexpr auto kHalfSignedARMv4T = Half | Signed | ARMv4T; 

  // ARM7TDMI/ARMv4T special case: unaligned LDRSH is effectively LDRSB.
  if ((flags & kHalfSignedARMv4T) == kHalfSignedARMv4T) {
    auto label_aligned = Xbyak::Label{};

    code.bt(address_reg, 1);
    code.jnc(label_aligned);
    code.shr(result_reg, 8);
    code.movsx(result_reg, result_reg.cvt8());
    code.L(label_aligned);
  }

  code.pop(rcx);
}

void X64Backend::CompileMemoryWrite(CompileContext const& context, IRMemoryWrite* op) {
  DESTRUCTURE_CONTEXT;

  auto source_reg = reg_alloc.GetReg32(op->source, location);
  auto address_reg = reg_alloc.GetReg32(op->address, location);
  auto flags = op->flags;

  auto label_slowmem = Xbyak::Label{};
  auto label_final = Xbyak::Label{};
  auto pagetable = memory->pagetable.get();

  // TODO: properly allocate free registers.
  code.push(rcx);
  code.push(rdx);

  if (pagetable != nullptr) {
    code.mov(rcx, u64(pagetable));

    // TODO: check for ITCM and DTCM!!!

    // Get the page table entry
    code.mov(edx, address_reg);
    code.shr(edx, Memory::kPageShift);
    code.mov(rcx, qword[rcx + rdx * sizeof(uintptr)]);

    // Check if the entry is a null pointer.
    code.test(rcx, rcx);
    code.jz(label_slowmem);

    code.mov(edx, address_reg);

    if (flags & Word) {
      code.and_(edx, Memory::kPageMask & ~3);
      code.mov(dword[rcx + rdx], source_reg);
    }

    if (flags & Half) {
      code.and_(edx, Memory::kPageMask & ~1);
      code.mov(word[rcx + rdx], source_reg.cvt16());
    }

    if (flags & Byte) {
      code.and_(edx, Memory::kPageMask);
      code.mov(byte[rcx + rdx], source_reg.cvt8());
    }

    code.jmp(label_final);
  }

  code.L(label_slowmem);

  // TODO: determine which registers need to be saved.
  code.push(rax);
  code.push(r8);
  code.push(r9);
  code.push(r10);
  code.push(r11);

  code.mov(edx, address_reg);

  if (flags & Word) {
    code.and_(edx, ~3);
    code.mov(rax, u64((void*)&X64Backend::WriteWord));
  }

  if (flags & Half) {
    code.and_(edx, ~1);
    code.mov(rax, u64((void*)&X64Backend::WriteHalf));
  }

  if (flags & Byte) {
    code.mov(rax, u64((void*)&X64Backend::WriteByte));
  }

  code.mov(rcx, u64(this));
  code.mov(r8d, u32(Memory::Bus::Data));
  code.mov(r9d, source_reg);
  code.sub(rsp, 0x20);
  code.call(rax);
  code.add(rsp, 0x20);

  code.pop(r11);
  code.pop(r10);
  code.pop(r9);
  code.pop(r8);
  code.pop(rax);

  code.L(label_final);
  code.pop(rdx);
  code.pop(rcx);
}

} // namespace lunatic::backend
} // namespace lunatic
