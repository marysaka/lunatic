/*
 * Copyright (C) 2023 marysaka. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.hpp"

namespace lunatic::backend {

void ARM64Backend::CompileFlush(CompileContext const& context, IRFlush* op){ NOT_IMPLEMENTED_OP(op); }
void ARM64Backend::CompileFlushExchange(CompileContext const& context, IRFlushExchange* op){
  DESTRUCTURE_CONTEXT;

  auto& address_in_var = op->address_in.Get();
  auto  address_in_reg = reg_alloc.GetVariableHostReg(address_in_var);
  auto& cpsr_in_var = op->cpsr_in.Get();
  auto  cpsr_in_reg = reg_alloc.GetVariableHostReg(cpsr_in_var);

  auto& address_out_var = op->address_out.Get();
  auto& cpsr_out_var = op->cpsr_out.Get();

  reg_alloc.ReleaseVarAndReuseHostReg(address_in_var, address_out_var);
  reg_alloc.ReleaseVarAndReuseHostReg(cpsr_in_var, cpsr_out_var);

  auto address_out_reg = reg_alloc.GetVariableHostReg(address_out_var);
  auto cpsr_out_reg = reg_alloc.GetVariableHostReg(cpsr_out_var);

  auto label_arm = oaknut::Label{};
  auto label_done = oaknut::Label{};

  code.TBZ(address_in_reg, 0, label_arm);

  // Thumb
  code.ORR(cpsr_out_reg, cpsr_in_reg, 1 << 5);
  code.AND(address_out_reg, address_in_reg, ~1);
  code.ADD(address_out_reg, address_out_reg, sizeof(uint16_t) * 2);
  code.B(label_done);

  // ARM
  code.l(label_arm);
  code.AND(cpsr_out_reg, cpsr_in_reg, ~(1 << 5));
  code.AND(address_out_reg, address_in_reg, ~3);
  code.ADD(address_out_reg, address_out_reg, sizeof(uint32_t) * 2);
  code.l(label_done);
}

} // namespace lunatic::backend
