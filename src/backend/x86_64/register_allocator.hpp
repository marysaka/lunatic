/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <xbyak/xbyak.h>
#include <vector>

#include "common/optional.hpp"
#include "frontend/ir/emitter.hpp"

namespace lunatic {
namespace backend {

struct X64RegisterAllocator {
  X64RegisterAllocator(
    lunatic::frontend::IREmitter const& emitter,
    Xbyak::CodeGenerator& code,
    int spill_area_size
  );

  auto GetReg32(
    lunatic::frontend::IRVariable const& var,
    int location
  ) -> Xbyak::Reg32;

private:
  /// Determine when each variable will be dead.
  void EvaluateVariableLifetimes();

  /**
   * Release host registers allocated to variables that are dead.
   *
   * @param  location  The current location in the IR program.
   * @returns nothing
   */
  void ReleaseDeadVariables(int location);

  /**
   * Find and allocate a host register that is currently unused.
   * If no register is free attempt to spill a variable to the stack to
   * free its register up.
   *
   * @param  location  The current location in the IR program.
   * @returns nothing
   */
  auto FindFreeHostReg(int location) -> Xbyak::Reg32;

  lunatic::frontend::IREmitter const& emitter;
  Xbyak::CodeGenerator& code;

  /// Host register that are free and can be allocated.
  std::vector<Xbyak::Reg32> free_host_regs;

  /// Map variable to its allocated host register (if any).
  std::vector<Optional<Xbyak::Reg32>> var_id_to_host_reg;
  
  /// Map variable to the last location where it's accessed.
  std::vector<int> var_id_to_point_of_last_use;
  
  std::vector<bool> spill_used;
  
  std::vector<Optional<u32>> spill_location;
};

} // namespace lunatic::backend
} // namespace lunatic
