/*
 * Copyright (C) 2023 marysaka. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.hpp"

namespace lunatic::backend {

void ARM64Backend::CompileClearCarry(CompileContext const& context, IRClearCarry* op) {
  DESTRUCTURE_CONTEXT;

  code.AND(HostFlagsReg, HostFlagsReg, ~(1 << 29));
}

void ARM64Backend::CompileSetCarry(CompileContext const& context, IRSetCarry* op) {
  DESTRUCTURE_CONTEXT;

  code.ORR(HostFlagsReg, HostFlagsReg, 1 << 29);
}

void ARM64Backend::CompileUpdateFlags(CompileContext const& context, IRUpdateFlags* op) {
  DESTRUCTURE_CONTEXT;

  u32 mask = 0;
  auto& result_var = op->result.Get();
  auto& input_var = op->input.Get();
  auto input_reg  = reg_alloc.GetVariableHostReg(input_var);

  reg_alloc.ReleaseVarAndReuseHostReg(input_var, result_var);

  auto result_reg = reg_alloc.GetVariableHostReg(result_var);

  if (op->flag_n) mask |= 0x80000000;
  if (op->flag_z) mask |= 0x40000000;
  if (op->flag_c) mask |= 0x20000000;
  if (op->flag_v) mask |= 0x10000000;

  auto flags_reg = reg_alloc.GetTemporaryHostReg();

  code.AND(flags_reg, HostFlagsReg.toW(), mask);

  if (result_reg.index() != input_reg.index()) {
    code.MOV(result_reg, input_reg);
  }

  // Clear the bits to be updated, then OR the new values.
  code.AND(result_reg, result_reg, ~mask);
  code.ORR(result_reg, result_reg, flags_reg);
}

void ARM64Backend::CompileUpdateSticky(CompileContext const& context, IRUpdateSticky* op) {
  DESTRUCTURE_CONTEXT;

  auto result_reg = reg_alloc.GetVariableHostReg(op->result.Get());
  auto input_reg  = reg_alloc.GetVariableHostReg(op->input.Get());

  // Construct Q from overflow flag.
  code.MOV(result_reg, HostFlagsReg.toW());
  code.AND(result_reg, result_reg, 1 << 28);
  code.LSR(result_reg, result_reg, 1);
  code.ORR(result_reg, result_reg, input_reg);
}

} // namespace lunatic::backend
