/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <xbyak/xbyak.h>
#include <unordered_map>
#include <vector>

#include "frontend/ir/value.hpp"

namespace lunatic {
namespace backend {

struct X64RegisterAllocator {
  X64RegisterAllocator(Xbyak::CodeGenerator& code);

  void Finalize();
  auto GetReg32(lunatic::frontend::IRVariable const& var) -> Xbyak::Reg32;

private:
  static auto IsCallerSaved(Xbyak::Reg32 reg) -> bool;

  Xbyak::CodeGenerator& code;

  // std::vector<Xbyak::Reg32> free_registers {
  //   ecx,
  //   edx,
  //   ebx,
  //   esi,
  //   edi,
  // //ebp,
  //   r8d,
  //   r9d,
  //   r10d,
  //   r11d,
  //   r12d,
  //   r13d,
  //   r14d,
  //   r15d
  // };

  std::vector<Xbyak::Reg32> free_caller_saved;
  std::vector<Xbyak::Reg32> free_callee_saved;
  std::vector<Xbyak::Reg32> restore_list;

  std::unordered_map<u32, Xbyak::Reg32> allocation;
};

} // namespace lunatic::backend
} // namespace lunatic
