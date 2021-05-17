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

// TODO: optimize cases where an operand variable expires during the IR opcode we're currently compiling.
// In that case the host register that is assigned to it might be reused for the result variable to do some optimization.

// TODO: right now we only have UInt32 but how to deal with different variable or constant data types?

#define DESTRUCTURE_CONTEXT auto& [code, reg_alloc, state, location] = context;

namespace lunatic {
namespace backend {

// using namespace lunatic::frontend;
using namespace Xbyak::util;

#ifdef _WIN64

#define ABI_MSVC

static constexpr Xbyak::Reg64 kRegArg0 = rcx;
static constexpr Xbyak::Reg64 kRegArg1 = rdx;
static constexpr Xbyak::Reg64 kRegArg2 = r8;
static constexpr Xbyak::Reg64 kRegArg3 = r9;

#else

#define ABI_SYSV

static constexpr Xbyak::Reg64 kRegArg0 = rdi;
static constexpr Xbyak::Reg64 kRegArg1 = rsi;
static constexpr Xbyak::Reg64 kRegArg2 = rdx;
static constexpr Xbyak::Reg64 kRegArg3 = rcx;

#endif

void X64Backend::Compile(Memory& memory, State& state, BasicBlock& basic_block) {
  // TODO: do not keep the code in memory forever.
  auto& emitter = basic_block.emitter;
  auto  code = new Xbyak::CodeGenerator{};
  auto  reg_alloc = X64RegisterAllocator{emitter, *code};
  auto  location = 0;
  auto  context = CompileContext{*code, reg_alloc, state, location};

  this->memory = &memory;

  auto stack_displacement = sizeof(u64) + X64RegisterAllocator::kSpillAreaSize * sizeof(u32);

  Push(*code, {rbx, rbp, r12, r13, r14, r15});
#ifdef ABI_MSVC
  Push(*code, {rsi, rdi});
#else

#endif
  code->sub(rsp, stack_displacement);
  code->mov(rbp, rsp);

  // Load pointer to state into RCX
  code->mov(rcx, u64(&state));

  // Load carry flag from state into AX register.
  // Right now we assume we will only need the old carry, is this true?
  code->mov(edx, dword[rcx + state.GetOffsetToCPSR()]);
  code->bt(edx, 29); // CF = value of bit 29
  code->lahf();

  for (auto const& op : emitter.Code()) {
    reg_alloc.SetCurrentLocation(location);

    switch (op->GetClass()) {
      case IROpcodeClass::LoadGPR:
        CompileLoadGPR(context, lunatic_cast<IRLoadGPR>(op.get()));
        break;
      case IROpcodeClass::StoreGPR:
        CompileStoreGPR(context, lunatic_cast<IRStoreGPR>(op.get()));
        break;
      case IROpcodeClass::LoadSPSR:
        CompileLoadSPSR(context, lunatic_cast<IRLoadSPSR>(op.get()));
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
      case IROpcodeClass::Flush:
        CompileFlush(context, lunatic_cast<IRFlush>(op.get()));
        break;
      case IROpcodeClass::FlushExchange:
        CompileExchange(context, lunatic_cast<IRFlushExchange>(op.get()));
        break;
      default:
        throw std::runtime_error(
          fmt::format("X64Backend: unhandled IR opcode: {}", op->ToString())
        );
    }

    location++;
  }

  code->add(rsp, stack_displacement);
#ifdef ABI_MSVC
  Pop(*code, {rsi, rdi});
#endif
  Pop(*code, {rbx, rbp, r12, r13, r14, r15});
  code->ret();

  basic_block.function = code->getCode<BasicBlock::CompiledFn>();
}

void X64Backend::Push(
  Xbyak::CodeGenerator& code,
  std::vector<Xbyak::Reg64> const& regs
) {
  for (auto reg : regs) {
    code.push(reg);
  }
}

void X64Backend::Pop(
  Xbyak::CodeGenerator& code,
  std::vector<Xbyak::Reg64> const& regs
) {
  auto end = regs.rend();

  for (auto it = regs.rbegin(); it != end; ++it) {
    code.pop(*it);
  }
}

void X64Backend::CompileLoadGPR(CompileContext const& context, IRLoadGPR* op) {
  DESTRUCTURE_CONTEXT;

  auto address  = rcx + state.GetOffsetToGPR(op->reg.mode, op->reg.reg);
  auto host_reg = reg_alloc.GetVariableHostReg(op->result);

  code.mov(host_reg, dword[address]);
}

void X64Backend::CompileStoreGPR(CompileContext const& context, IRStoreGPR* op) {
  DESTRUCTURE_CONTEXT;

  auto address = rcx + state.GetOffsetToGPR(op->reg.mode, op->reg.reg);

  if (op->value.IsConstant()) {
    code.mov(dword[address], op->value.GetConst().value);
  } else {
    auto host_reg = reg_alloc.GetVariableHostReg(op->value.GetVar());

    code.mov(dword[address], host_reg);
  }
}

void X64Backend::CompileLoadSPSR(CompileContext const& context, IRLoadSPSR* op) {
  DESTRUCTURE_CONTEXT;

  auto address = rcx + state.GetOffsetToSPSR(op->mode);
  auto host_reg = reg_alloc.GetVariableHostReg(op->result);

  code.mov(host_reg, dword[address]);
}

void X64Backend::CompileLoadCPSR(CompileContext const& context, IRLoadCPSR* op) {
  DESTRUCTURE_CONTEXT;

  auto address = rcx + state.GetOffsetToCPSR();
  auto host_reg = reg_alloc.GetVariableHostReg(op->result);

  code.mov(host_reg, dword[address]);
}

void X64Backend::CompileStoreCPSR(CompileContext const& context, IRStoreCPSR* op) {
  DESTRUCTURE_CONTEXT;

  auto address = rcx + state.GetOffsetToCPSR();

  if (op->value.IsConstant()) {
    code.mov(dword[address], op->value.GetConst().value);
  } else {
    auto host_reg = reg_alloc.GetVariableHostReg(op->value.GetVar());

    code.mov(dword[address], host_reg);
  }
}

void X64Backend::CompileUpdateFlags(CompileContext const& context, IRUpdateFlags* op) {
  DESTRUCTURE_CONTEXT;

  u32 mask = 0;
  auto result_reg = reg_alloc.GetVariableHostReg(op->result);
  auto input_reg  = reg_alloc.GetVariableHostReg(op->input);

  if (op->flag_n) mask |= 0x80000000;
  if (op->flag_z) mask |= 0x40000000;
  if (op->flag_c) mask |= 0x20000000;
  if (op->flag_v) mask |= 0x10000000;

  auto pext_mask_reg = reg_alloc.GetTemporaryHostReg();
  auto flags_reg = reg_alloc.GetTemporaryHostReg();

  // Convert NZCV bits from AX register into the guest format.
  // Clear the bits which are not to be updated.
  code.mov(pext_mask_reg, 0xC101);
  code.pext(flags_reg, eax, pext_mask_reg);
  code.shl(flags_reg, 28);
  code.and_(flags_reg, mask);

  // Clear the bits to be updated, then OR the new values.
  code.mov(result_reg, input_reg);
  code.and_(result_reg, ~mask);
  code.or_(result_reg, flags_reg);
}

void X64Backend::CompileLSL(CompileContext const& context, IRLogicalShiftLeft* op) {
  DESTRUCTURE_CONTEXT;

  auto amount = op->amount;
  auto result_reg = reg_alloc.GetVariableHostReg(op->result);
  auto operand_reg = reg_alloc.GetVariableHostReg(op->operand);

  code.mov(result_reg, operand_reg);
  code.shl(result_reg.cvt64(), 32);

  if (amount.IsConstant()) {
    if (op->update_host_flags) {
      code.sahf();
    }
    code.shl(result_reg.cvt64(), u8(std::min(amount.GetConst().value, 33U)));
  } else {
    auto amount_reg = reg_alloc.GetVariableHostReg(op->amount.GetVar());

    code.push(rcx);
    code.mov(cl, 33);
    code.cmp(amount_reg.cvt8(), u8(33));
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
  auto result_reg = reg_alloc.GetVariableHostReg(op->result);
  auto operand_reg = reg_alloc.GetVariableHostReg(op->operand);

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
    auto amount_reg = reg_alloc.GetVariableHostReg(op->amount.GetVar());
    code.push(rcx);
    code.mov(cl, 33);
    code.cmp(amount_reg.cvt8(), u8(33));
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
  auto result_reg = reg_alloc.GetVariableHostReg(op->result);
  auto operand_reg = reg_alloc.GetVariableHostReg(op->operand);

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
    auto amount_reg = reg_alloc.GetVariableHostReg(op->amount.GetVar());
    code.push(rcx);
    code.mov(cl, 33);
    code.cmp(amount_reg.cvt8(), u8(33));
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
  auto result_reg = reg_alloc.GetVariableHostReg(op->result);
  auto operand_reg = reg_alloc.GetVariableHostReg(op->operand);

  code.mov(result_reg, operand_reg);

  if (amount.IsConstant()) {
    auto amount_value = amount.GetConst().value;

    // ROR #0 equals to RRX #1
    if (amount_value == 0) {
      code.sahf();
      code.rcr(result_reg, 1);
    } else {
      if (op->update_host_flags) {
        code.sahf();
      }
      code.ror(result_reg, u8(amount_value));
    }
  } else {
    auto amount_reg = reg_alloc.GetVariableHostReg(op->amount.GetVar());

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

  auto lhs_reg = reg_alloc.GetVariableHostReg(op->lhs);

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;

    if (op->result.IsNull()) {
      code.test(lhs_reg, imm);
    } else {
      auto result_reg = reg_alloc.GetVariableHostReg(op->result.Unwrap());

      code.mov(result_reg, lhs_reg);
      code.and_(result_reg, imm);
    }
  } else {
    auto rhs_reg = reg_alloc.GetVariableHostReg(op->rhs.GetVar());

    if (op->result.IsNull()) {
      code.test(lhs_reg, rhs_reg);
    } else {
      auto result_reg = reg_alloc.GetVariableHostReg(op->result.Unwrap());

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

  auto result_reg = reg_alloc.GetVariableHostReg(op->result.Unwrap());
  auto lhs_reg = reg_alloc.GetVariableHostReg(op->lhs);

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;

    code.mov(result_reg, lhs_reg);
    code.and_(result_reg, ~imm);
  } else {
    auto rhs_reg = reg_alloc.GetVariableHostReg(op->rhs.GetVar());

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

  auto lhs_reg = reg_alloc.GetVariableHostReg(op->lhs);

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
      auto result_reg = reg_alloc.GetVariableHostReg(op->result.Unwrap());

      code.mov(result_reg, lhs_reg);
      code.xor_(result_reg, imm);
    }
  } else {
    auto rhs_reg = reg_alloc.GetVariableHostReg(op->rhs.GetVar());

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
      auto result_reg = reg_alloc.GetVariableHostReg(op->result.Unwrap());

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

  auto lhs_reg = reg_alloc.GetVariableHostReg(op->lhs);

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;

    if (op->result.IsNull()) {
      code.cmp(lhs_reg, imm);
      code.cmc();
    } else {
      auto result_reg = reg_alloc.GetVariableHostReg(op->result.Unwrap());

      code.mov(result_reg, lhs_reg);
      code.sub(result_reg, imm);
      code.cmc();
    }
  } else {
    auto rhs_reg = reg_alloc.GetVariableHostReg(op->rhs.GetVar());

    if (op->result.IsNull()) {
      code.cmp(lhs_reg, rhs_reg);
      code.cmc();
    } else {
      auto result_reg = reg_alloc.GetVariableHostReg(op->result.Unwrap());

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

  auto result_reg = reg_alloc.GetVariableHostReg(op->result.Unwrap());
  auto lhs_reg = reg_alloc.GetVariableHostReg(op->lhs);

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;

    code.mov(result_reg, imm);
    code.sub(result_reg, lhs_reg);
    code.cmc();
  } else {
    auto rhs_reg = reg_alloc.GetVariableHostReg(op->rhs.GetVar());

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

  auto lhs_reg = reg_alloc.GetVariableHostReg(op->lhs);

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
      auto result_reg = reg_alloc.GetVariableHostReg(op->result.Unwrap());

      code.mov(result_reg, lhs_reg);
      code.add(result_reg, imm);
    }
  } else {
    auto rhs_reg = reg_alloc.GetVariableHostReg(op->rhs.GetVar());

    if (op->result.IsNull()) {
      // eax will be trashed by lahf anyways
      code.mov(eax, lhs_reg);
      code.add(eax, rhs_reg);
    } else {
      auto result_reg = reg_alloc.GetVariableHostReg(op->result.Unwrap());

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

  auto result_reg = reg_alloc.GetVariableHostReg(op->result.Unwrap());
  auto lhs_reg = reg_alloc.GetVariableHostReg(op->lhs);

  code.sahf();

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;

    code.mov(result_reg, lhs_reg);
    code.adc(result_reg, imm);
  } else {
    auto rhs_reg = reg_alloc.GetVariableHostReg(op->rhs.GetVar());

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

  auto result_reg = reg_alloc.GetVariableHostReg(op->result.Unwrap());
  auto lhs_reg = reg_alloc.GetVariableHostReg(op->lhs);

  code.sahf();
  code.cmc();

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;

    code.mov(result_reg, lhs_reg);
    code.sbb(result_reg, imm);
    code.cmc();
  } else {
    auto rhs_reg = reg_alloc.GetVariableHostReg(op->rhs.GetVar());

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

  auto result_reg = reg_alloc.GetVariableHostReg(op->result.Unwrap());
  auto lhs_reg = reg_alloc.GetVariableHostReg(op->lhs);

  code.sahf();
  code.cmc();

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;

    code.mov(result_reg, imm);
    code.sbb(result_reg, lhs_reg);
    code.cmc();
  } else {
    auto rhs_reg = reg_alloc.GetVariableHostReg(op->rhs.GetVar());

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

  auto result_reg = reg_alloc.GetVariableHostReg(op->result.Unwrap());
  auto lhs_reg = reg_alloc.GetVariableHostReg(op->lhs);

  if (op->rhs.IsConstant()) {
    auto imm = op->rhs.GetConst().value;

    code.mov(result_reg, lhs_reg);
    code.or_(result_reg, imm);
  } else {
    auto rhs_reg = reg_alloc.GetVariableHostReg(op->rhs.GetVar());

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

  auto result_reg = reg_alloc.GetVariableHostReg(op->result);

  if (op->source.IsConstant()) {
    code.mov(result_reg, op->source.GetConst().value);
  } else {
    code.mov(result_reg, reg_alloc.GetVariableHostReg(op->source.GetVar()));
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

  auto result_reg = reg_alloc.GetVariableHostReg(op->result);

  if (op->source.IsConstant()) {
    code.mov(result_reg, op->source.GetConst().value);
  } else {
    code.mov(result_reg, reg_alloc.GetVariableHostReg(op->source.GetVar()));
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

  auto result_reg = reg_alloc.GetVariableHostReg(op->result);
  auto address_reg = reg_alloc.GetVariableHostReg(op->address);
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
  Push(code, {rax, rdx, r8, r9, r10, r11});
#ifdef ABI_SYSV
  Push(code, {rsi, rdi});
#endif

  code.mov(kRegArg1.cvt32(), address_reg);

  if (flags & Word) {
    code.and_(kRegArg1.cvt32(), ~3);
    code.mov(rax, u64((void*)&X64Backend::ReadWord));
  }

  if (flags & Half) {
    code.and_(kRegArg1.cvt32(), ~1);
    code.mov(rax, u64((void*)&X64Backend::ReadHalf));
  }

  if (flags & Byte) {
    code.mov(rax, u64((void*)&X64Backend::ReadByte));
  }

  code.mov(kRegArg0, u64(this));
  code.mov(kRegArg2.cvt32(), u32(Memory::Bus::Data));
  code.sub(rsp, 0x28);
  code.call(rax);
  code.add(rsp, 0x28);

#ifdef ABI_SYSV
  Pop(code, {rsi, rdi});
#endif
  Pop(code, {rdx, r8, r9, r10, r11});
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
      code.ror(result_reg, cl);
    }
  }

  static constexpr auto kHalfSignedARMv4T = Half | Signed | ARMv4T;

  // ARM7TDMI/ARMv4T special case: unaligned LDRSH is effectively LDRSB.
  // TODO: this can probably be optimized by checking for misalignment early.
  if ((flags & kHalfSignedARMv4T) == kHalfSignedARMv4T) {
    auto label_aligned = Xbyak::Label{};

    code.bt(address_reg, 0);
    code.jnc(label_aligned);
    code.shr(result_reg, 8);
    code.movsx(result_reg, result_reg.cvt8());
    code.L(label_aligned);
  }

  code.pop(rcx);
}

void X64Backend::CompileMemoryWrite(CompileContext const& context, IRMemoryWrite* op) {
  DESTRUCTURE_CONTEXT;

  auto source_reg  = reg_alloc.GetVariableHostReg(op->source);
  auto address_reg = reg_alloc.GetVariableHostReg(op->address);
  auto scratch_reg = reg_alloc.GetTemporaryHostReg();
  auto flags = op->flags;

  auto label_slowmem = Xbyak::Label{};
  auto label_final = Xbyak::Label{};
  auto pagetable = memory->pagetable.get();

  code.push(rcx);

  if (pagetable != nullptr) {
    code.mov(rcx, u64(pagetable));

    // TODO: check for ITCM and DTCM!!!

    // Get the page table entry
    code.mov(scratch_reg, address_reg);
    code.shr(scratch_reg, Memory::kPageShift);
    code.mov(rcx, qword[rcx + scratch_reg.cvt64() * sizeof(uintptr)]);

    // Check if the entry is a null pointer.
    code.test(rcx, rcx);
    code.jz(label_slowmem);

    code.mov(scratch_reg, address_reg);

    if (flags & Word) {
      code.and_(scratch_reg, Memory::kPageMask & ~3);
      code.mov(dword[rcx + scratch_reg.cvt64()], source_reg);
    }

    if (flags & Half) {
      code.and_(scratch_reg, Memory::kPageMask & ~1);
      code.mov(word[rcx + scratch_reg.cvt64()], source_reg.cvt16());
    }

    if (flags & Byte) {
      code.and_(scratch_reg, Memory::kPageMask);
      code.mov(byte[rcx + scratch_reg.cvt64()], source_reg.cvt8());
    }

    code.jmp(label_final);
  }

  code.L(label_slowmem);

  // TODO: determine which registers need to be saved.
  Push(code, {rax, rdx, r8, r9, r10, r11});
#ifdef ABI_SYSV
  Push(code, {rsi, rdi});
#endif

  code.mov(kRegArg1.cvt32(), address_reg);

  if (flags & Word) {
    code.and_(kRegArg1.cvt32(), ~3);
    code.mov(rax, u64((void*)&X64Backend::WriteWord));
  }

  if (flags & Half) {
    code.and_(kRegArg1.cvt32(), ~1);
    code.mov(rax, u64((void*)&X64Backend::WriteHalf));
  }

  if (flags & Byte) {
    code.mov(rax, u64((void*)&X64Backend::WriteByte));
  }

  code.mov(kRegArg0, u64(this));
  code.mov(kRegArg2.cvt32(), u32(Memory::Bus::Data));
  code.mov(kRegArg3.cvt32(), source_reg);
  code.sub(rsp, 0x28);
  code.call(rax);
  code.add(rsp, 0x28);

#ifdef ABI_SYSV
  Pop(code, {rsi, rdi});
#endif
  Pop(code, {rax, rdx, r8, r9, r10, r11});

  code.L(label_final);
  code.pop(rcx);
}

void X64Backend::CompileFlush(CompileContext const& context, IRFlush* op) {
  DESTRUCTURE_CONTEXT;

  auto cpsr_reg = reg_alloc.GetVariableHostReg(op->cpsr_in);
  auto r15_in_reg = reg_alloc.GetVariableHostReg(op->address_in);
  auto r15_out_reg = reg_alloc.GetVariableHostReg(op->address_out);

  // Thanks to @wheremyfoodat (github.com/wheremyfoodat) for coming up with this.
  code.test(cpsr_reg, 1 << 5);
  code.sete(r15_out_reg.cvt8());
  code.movzx(r15_out_reg, r15_out_reg.cvt8());
  code.lea(r15_out_reg, dword[r15_in_reg + r15_out_reg * 4 + 4]);
}

void X64Backend::CompileExchange(const CompileContext &context, IRFlushExchange *op) {
  DESTRUCTURE_CONTEXT;

  auto address_out_reg = reg_alloc.GetVariableHostReg(op->address_out);
  auto address_in_reg = reg_alloc.GetVariableHostReg(op->address_in);
  auto cpsr_out_reg = reg_alloc.GetVariableHostReg(op->cpsr_out);
  auto cpsr_in_reg = reg_alloc.GetVariableHostReg(op->cpsr_in);

  auto label_arm = Xbyak::Label{};
  auto label_done = Xbyak::Label{};

  // TODO: fix this horrible codegen.
  // a) try to make this branch-less
  // b) do we always need to mask with ~1 / ~3?

  code.mov(address_out_reg, address_in_reg);
  code.mov(cpsr_out_reg, cpsr_in_reg);

  code.test(address_in_reg, 1);
  code.je(label_arm);

  // Thumb
  code.or_(cpsr_out_reg, 1 << 5);
  code.and_(address_out_reg, ~1);
  code.add(address_out_reg, sizeof(u16) * 2);
  code.jmp(label_done);

  // ARM
  code.L(label_arm);
  code.and_(cpsr_out_reg, ~(1 << 5));
  code.and_(address_out_reg, ~3);
  code.add(address_out_reg, sizeof(u32) * 2);

  code.L(label_done);
}

} // namespace lunatic::backend
} // namespace lunatic
