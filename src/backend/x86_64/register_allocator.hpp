/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <xbyak/xbyak.h>
#include <unordered_map>
#include <vector>

#include "frontend/ir/emitter.hpp"

namespace lunatic {
namespace backend {

struct X64RegisterAllocator {
  X64RegisterAllocator(
    lunatic::frontend::IREmitter const& emitter,
    Xbyak::CodeGenerator& code
  );

  void Finalize();
  auto GetReg32(lunatic::frontend::IRVariable const& var, int location) -> Xbyak::Reg32;

private:
  static auto IsCallerSaved(Xbyak::Reg32 reg) -> bool;

  void CreateVariableExpirationPoints();
  void ExpireVariables(int location);

  lunatic::frontend::IREmitter const& emitter;
  Xbyak::CodeGenerator& code;

  std::vector<Xbyak::Reg32> free_caller_saved;
  std::vector<Xbyak::Reg32> free_callee_saved;
  std::vector<Xbyak::Reg32> restore_list;

  std::unordered_map<u32, Xbyak::Reg32> allocation;
  std::unordered_map<u32, int> expiration_points;
};

} // namespace lunatic::backend
} // namespace lunatic
