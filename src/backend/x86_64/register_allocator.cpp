/*
 * Copyright (C) 2021 fleroviux
 */

#include "register_allocator.hpp"

using namespace lunatic::frontend;
using namespace Xbyak::util;

namespace lunatic {
namespace backend {

X64RegisterAllocator::X64RegisterAllocator(IREmitter const& emitter, Xbyak::CodeGenerator& code)
    : emitter(emitter)
    , code(code) {
  Xbyak::Reg32 regs[] = {
    // ecx,
    edx,
    ebx,
    esi,
    edi,
    ebp,
    r8d,
    r9d,
    r10d,
    r11d,
    r12d,
    r13d,
    r14d,
    r15d
  };

  for (auto reg : regs) {
    if (IsCallerSaved(reg)) {
      free_caller_saved.push_back(reg);
    } else {
      free_callee_saved.push_back(reg);
    }
  }

  CreateVariableExpirationPoints();
}

void X64RegisterAllocator::Finalize() {
  auto end = restore_list.rend();

  for (auto it = restore_list.rbegin(); it != end; ++it) {
    auto reg = *it;
    code.pop(reg.cvt64());
  }
}

auto X64RegisterAllocator::GetReg32(IRVariable const& var, int location) -> Xbyak::Reg32 {
  auto match = allocation.find(var.id);

  if (match != allocation.end()) {
    // Variable is already allocated to a register currently.
    return match->second;
  }

  // TODO: this probably can be deferred to the point where we would otherwise have to spill a register.
  ExpireVariables(location);

  // Try to find a register that is caller saved first.
  if (free_caller_saved.size() != 0) {
    auto reg = free_caller_saved.back();
    allocation[var.id] = reg;
    free_caller_saved.pop_back();
    return reg;
  }

  // If that didn't work attempt to use a callee saved register (we have to push and pop it once)
  if (free_callee_saved.size() != 0) {
    auto reg = free_callee_saved.back();
    restore_list.push_back(reg);
    code.push(reg.cvt64());
    allocation[var.id] = reg;
    free_callee_saved.pop_back();
    return reg;
  }

  // Take care of this later...
  throw std::runtime_error("need to handle spilling");
}

void X64RegisterAllocator::CreateVariableExpirationPoints() {
  for (auto const& var : emitter.Vars()) {
    int expiration_point = -1;
    int location = 0;

    for (auto const& op : emitter.Code()) {
      if (op->Writes(*var) || op->Reads(*var)) {
        expiration_point = location;
      }

      location++;
    }

    if (expiration_point != -1) {
      expiration_points[var->id] = expiration_point;
    }
  }
}

void X64RegisterAllocator::ExpireVariables(int location) {
  for (auto const& var : emitter.Vars()) {
    auto expiration_point = expiration_points[var->id];

    if (location > expiration_point) {
      auto match = allocation.find(var->id);
      if (match != allocation.end()) {
        auto reg = match->second;
        // Even if the register would be callee saved it was already saved at this point,
        // so we don't really care.
        free_caller_saved.push_back(reg);
        allocation.erase(match);
      }
    }
  }
}

auto X64RegisterAllocator::IsCallerSaved(Xbyak::Reg32 reg) -> bool {
  return reg == eax || reg == ecx || reg == edx || reg == r8d || reg == r9d;
}

} // namespace lunatic::backend
} // namespace lunatic
