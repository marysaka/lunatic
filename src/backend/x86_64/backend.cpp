/*
 * Copyright (C) 2021 fleroviux
 */

#include <list>
#include <stdexcept>
// #include <unordered_map>
#include <xbyak/xbyak.h>

#include "backend.hpp"
#include "register_allocator.hpp"

namespace lunatic {
namespace backend {

using namespace lunatic::frontend;

void X64Backend::Run(State& state, IREmitter const& emitter) {
  using namespace Xbyak::util;

  Xbyak::CodeGenerator code;
  X64RegisterAllocator reg_alloc { emitter, code };

  code.mov(rcx, u64(&state));

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
      case IROpcodeClass::Add: {
        auto op = lunatic_cast<IRAdd>(op_.get());
        auto result_reg = reg_alloc.GetReg32(op->result, location);
        auto lhs_reg = reg_alloc.GetReg32(op->lhs, location);
        /* TODO: if the LHS variable expires in this very same instruction,
         * then it would be safe to accumulate into the LHS register directly.
         */
        code.mov(result_reg, lhs_reg);
        if (op->rhs.IsConstant()) {
          // TODO: handle different data types, once we have them...?
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
        u32 pext_mask = 0;
        auto result_reg = reg_alloc.GetReg32(op->result, location);
        auto input_reg = reg_alloc.GetReg32(op->input, location);

        if (op->flag_n) { mask |= 0x80000000; pext_mask |= 0x8000; }
        if (op->flag_z) { mask |= 0x40000000; pext_mask |= 0x4000; }
        if (op->flag_c) { mask |= 0x20000000; pext_mask |= 0x0100; }
        if (op->flag_v) { mask |= 0x10000000; pext_mask |= 0x0001; }

        // TODO: likewise to Add, optimize case when input variable is last used in this instruction.

        // TODO: assumes UpdateFlags will not be called again before updating the host flags.
        code.push(rcx);
        code.mov(result_reg, input_reg);
        code.and_(result_reg, ~mask);
        code.mov(ecx, pext_mask);
        code.pext(eax, eax, ecx);
        code.shl(eax, 28);
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

  // code.int3();
  code.ret();
  code.getCode<void (*)()>()();
}

} // namespace lunatic::backend
} // namespace lunatic
