/*
 * Copyright (C) 2021 fleroviux
 */

#include "register_allocator.hpp"

using namespace lunatic::frontend;
using namespace Xbyak::util;

namespace lunatic {
namespace backend {

X64RegisterAllocator::X64RegisterAllocator(Xbyak::CodeGenerator& code) : code(code) {
  Xbyak::Reg32 regs[] = {
    ecx,
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
}

void X64RegisterAllocator::Finalize() {
  auto end = restore_list.rend();

  for (auto it = restore_list.rbegin(); it != end; ++it) {
    auto reg = *it;
    code.pop(reg.cvt64());
  }
}

auto X64RegisterAllocator::GetReg32(IRVariable const& var) -> Xbyak::Reg32 {
  auto match = allocation.find(var.id);

  if (match != allocation.end()) {
    // Variable is already allocated to a register currently.
    return match->second;
  }

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

auto X64RegisterAllocator::IsCallerSaved(Xbyak::Reg32 reg) -> bool {
  return reg == eax || reg == ecx || reg == edx || reg == r8d || reg == r9d;
}

} // namespace lunatic::backend
} // namespace lunatic
