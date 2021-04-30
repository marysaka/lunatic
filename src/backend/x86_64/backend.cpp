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
        break;
      }
      case IROpcodeClass::LogicalShiftRight: {
        auto op = lunatic_cast<IRLogicalShiftRight>(op_.get());
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
        break;
      }
      case IROpcodeClass::ArithmeticShiftRight: {
        auto op = lunatic_cast<IRArithmeticShiftRight>(op_.get());
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
        break;
      }
      case IROpcodeClass::RotateRight: {
        auto op = lunatic_cast<IRRotateRight>(op_.get());
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
        break;
      }
      case IROpcodeClass::Add: {
        auto op = lunatic_cast<IRAdd>(op_.get());
        auto lhs_reg = reg_alloc.GetReg32(op->lhs, location);

        if (op->rhs.IsConstant()) {
          auto imm = op->rhs.GetConst().value;

          if (op->result.IsNull()) {
            code.cmp(lhs_reg, -imm);
            code.cmc();
          } else {
            auto result_reg = reg_alloc.GetReg32(op->result.GetVar(), location);

            code.mov(result_reg, lhs_reg);
            code.add(result_reg, imm); 
          }
        } else {
          auto rhs_reg = reg_alloc.GetReg32(op->rhs.GetVar(), location);

          if (op->result.IsNull()) {
            // TODO: optimize this.
            code.push(rcx);
            code.mov(ecx, lhs_reg);
            code.add(ecx, rhs_reg);
            code.pop(rcx);
          } else {
            auto result_reg = reg_alloc.GetReg32(op->result.GetVar(), location);

            code.mov(result_reg, lhs_reg);
            code.add(result_reg, rhs_reg);
          }
        }

        if (op->update_host_flags) {
          code.lahf();
          code.seto(al);
        }
        break;
      }
      case IROpcodeClass::Sub: {
        auto op = lunatic_cast<IRSub>(op_.get());
        auto lhs_reg = reg_alloc.GetReg32(op->lhs, location);

        if (op->rhs.IsConstant()) {
          auto imm = op->rhs.GetConst().value;

          if (op->result.IsNull()) {
            code.cmp(lhs_reg, imm);
            code.cmc();
          } else {
            auto result_reg = reg_alloc.GetReg32(op->result.GetVar(), location);

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
            auto result_reg = reg_alloc.GetReg32(op->result.GetVar(), location);

            code.mov(result_reg, lhs_reg);
            code.sub(result_reg, rhs_reg);
            code.cmc();
          }
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
