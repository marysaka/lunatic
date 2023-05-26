/*
 * Copyright (C) 2023 marysaka. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <bitset>
#include <vector>

#include <oaknut/oaknut.hpp>

#include "common/optional.hpp"
#include "frontend/ir/emitter.hpp"

namespace lunatic {
namespace backend {

struct ARM64RegisterAllocator {
  using IREmitter = lunatic::frontend::IREmitter;

  static constexpr int kSpillAreaSize = 32;

  ARM64RegisterAllocator(
    IREmitter const& emitter,
    oaknut::CodeGenerator& code
  );

  /**
   * Advance to the next IR opcode in the IR program.
   */
  void AdvanceLocation();

  /**
   * Get the host register currently allocated to a variable.
   * 
   * @param  var  The variable
   * @returns the host register
   */
  auto GetVariableHostReg(
    lunatic::frontend::IRVariable const& var
  ) -> oaknut::WReg;

  /**
   * Get a temporary host register for use during the current opcode.
   * 
   * @returns the host register
   */
  auto GetTemporaryHostReg() -> oaknut::WReg;

  /**
   * If var_old will be released after the current opcode,
   * then it will be released early and the host register
   * allocated to it will be moved to var_new.
   * The caller is responsible to not read var_old after writing var_new.
   * 
   * @param  var_old  the variable to release
   * @param  var_new  the variable to receive the released host register
   */
  void ReleaseVarAndReuseHostReg(
    lunatic::frontend::IRVariable const& var_old,
    lunatic::frontend::IRVariable const& var_new
  );

  bool IsHostRegFree(oaknut::XReg reg) const;

private:
  /// Determine when each variable will be dead.
  void EvaluateVariableLifetimes();

  /// Release host registers allocated to variables that are dead.
  void ReleaseDeadVariables();

  /// Release host registers allocated for temporary storage.
  void ReleaseTemporaryHostRegs();

  /**
   * Find and allocate a host register that is currently unused.
   * If no register is free attempt to spill a variable to the stack to
   * free its register up.
   *
   * @returns the host register
   */
  auto FindFreeHostReg() -> oaknut::WReg;

  IREmitter const& emitter;
  oaknut::CodeGenerator& code;

  /// Host register that are free and can be allocated.
  std::vector<oaknut::WReg> free_host_regs;

  /// Map variable to its allocated host register (if any).
  std::vector<Optional<int>> var_id_to_host_reg;
  
  /// Map variable to the last location where it's accessed.
  std::vector<int> var_id_to_point_of_last_use;

  /// The set of free/unused spill slots.
  std::bitset<kSpillAreaSize> free_spill_bitmap;

  /// Map variable to the slot it was spilled to (if it is spilled).  
  std::vector<Optional<int>> var_id_to_spill_slot;

  /// Array of currently allocated scratch registers.
  std::vector<oaknut::WReg> temp_host_regs;

  /// The current IR program location.
  int location = 0;

  /// Iterator pointing to the current IR program location.
  IREmitter::InstructionList::const_iterator current_op_iter;
};

} // namespace lunatic::backend
} // namespace lunatic
