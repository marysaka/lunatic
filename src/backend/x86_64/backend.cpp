/*
 * Copyright (C) 2021 fleroviux
 */

#include <algorithm>
#include <list>
#include <stdexcept>
#include <xbyak/xbyak.h>

#include "backend.hpp"
#include "register_allocator.hpp"

namespace lunatic {
namespace backend {

using namespace lunatic::frontend;

// TODO: implement spilling to the stack

// TODO: optimize cases where an operand variable expires during the IR opcode we're currently compiling.
// In that case the host register that is assigned to it might be reused for the result variable to do some optimization.

// TODO: right now we only have UInt32 but how to deal with different variable or constant data types?

void X64Backend::Run(State& state, IREmitter const& emitter, bool int3) {
  using namespace Xbyak::util;

  Xbyak::CodeGenerator code;
  X64RegisterAllocator reg_alloc { emitter, code };

  // Load pointer to state into RCX
  code.mov(rcx, u64(&state));

  // Load carry flag from state into AX register.
  // Right now we assume we will only need the old carry, is this true?
  code.mov(edx, dword[rcx + state.GetOffsetToCPSR()]);
  code.bt(edx, 29); // CF = value of bit 29
  code.lahf();

  int location = 0;

  for (auto const& op_ : emitter.Code()) {
    switch (op_->GetClass()) {
      case IROpcodeClass::LoadGPR: {
        auto op = lunatic_cast<IRLoadGPR>(op_.get());
        auto address  = rcx + state.GetOffsetToGPR(op->reg.mode, op->reg.reg);
        auto host_reg = reg_alloc.GetReg32(op->result, location);
        code.mov(host_reg, dword[address]);
        break;
      }
      case IROpcodeClass::StoreGPR: {
        auto op = lunatic_cast<IRStoreGPR>(op_.get());
        auto address = rcx + state.GetOffsetToGPR(op->reg.mode, op->reg.reg);
        if (op->value.IsConstant()) {
          code.mov(dword[address], op->value.GetConst().value);
        } else {
          auto host_reg = reg_alloc.GetReg32(op->value.GetVar(), location);
          code.mov(dword[address], host_reg);
        }
        break;
      }
      case IROpcodeClass::LoadCPSR: {
        auto op = lunatic_cast<IRLoadCPSR>(op_.get());
        auto address = rcx + state.GetOffsetToCPSR();
        auto host_reg = reg_alloc.GetReg32(op->result, location);
        code.mov(host_reg, dword[address]);
        break;
      }
      case IROpcodeClass::StoreCPSR: {
        auto op = lunatic_cast<IRStoreCPSR>(op_.get());
        auto address = rcx + state.GetOffsetToCPSR();
        if (op->value.IsConstant()) {
          code.mov(dword[address], op->value.GetConst().value);
        } else {
          auto host_reg = reg_alloc.GetReg32(op->value.GetVar(), location);
          code.mov(dword[address], host_reg);
        }
        break;
      }
      case IROpcodeClass::LogicalShiftLeft: {
        auto op = lunatic_cast<IRLogicalShiftLeft>(op_.get());
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
          // TODO: properly allocate a temporary register instead of push/pop.
          code.push(rax);
          code.mov(eax, 33);
          code.cmp(amount_reg, u8(33));
          code.cmovl(eax, amount_reg);
          if (op->update_host_flags) {
            code.sahf();
          }
          code.shl(result_reg.cvt64(), al);
          code.pop(rax);
        }

        if (op->update_host_flags) {
          code.lahf();
          if (int3) {
            code.int3();
          }
        }

        code.shr(result_reg.cvt64(), 32);
        break;
      }
      case IROpcodeClass::Add: {
        auto op = lunatic_cast<IRAdd>(op_.get());
        auto result_reg = reg_alloc.GetReg32(op->result, location);
        auto lhs_reg = reg_alloc.GetReg32(op->lhs, location);
        code.mov(result_reg, lhs_reg);
        if (op->rhs.IsConstant()) {
          auto imm = op->rhs.GetConst().value;
          code.add(result_reg, imm);
        } else {
          auto rhs_reg = reg_alloc.GetReg32(op->rhs.GetVar(), location);
          code.add(result_reg, rhs_reg);
        }
        if (op->update_host_flags) {
          code.lahf();
          code.seto(al);
        }
        break;
      }
      case IROpcodeClass::UpdateFlags: {
        auto op  = lunatic_cast<IRUpdateFlags>(op_.get());
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
        if (int3) {
          code.int3();
        }

        // Clear the bits to be updated, then OR the new values.
        code.mov(result_reg, input_reg);
        code.and_(result_reg, ~mask);
        code.or_(result_reg, eax);

        code.pop(rcx);
        break;
      }
      default: {
        throw std::runtime_error(fmt::format("X64Backend: unhandled IR opcode: {}", op_->ToString()));
      }
    }

    location++;
  }

  // Restore any callee-saved registers.
  reg_alloc.Finalize();

  // if (int3) {
  //   code.int3();
  // }
  code.ret();
  code.getCode<void (*)()>()();
}

} // namespace lunatic::backend
} // namespace lunatic
