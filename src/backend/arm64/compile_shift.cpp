/*
 * Copyright (C) 2023 marysaka. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.hpp"

namespace lunatic::backend {

void ARM64Backend::CompileLSL(CompileContext const& context, IRLogicalShiftLeft* op) {
  DESTRUCTURE_CONTEXT;

  auto& amount = op->amount;
  auto& result_var = op->result.Get();
  auto& operand_var = op->operand.Get();
  auto operand_reg = reg_alloc.GetVariableHostReg(operand_var);

  reg_alloc.ReleaseVarAndReuseHostReg(operand_var, result_var);

  auto result_reg = reg_alloc.GetVariableHostReg(result_var);

  if (!op->update_host_flags) {
    if (amount.IsConstant()) {
      auto amount_value = amount.GetConst().value;

      if (amount_value < 32) {
        code.LSL(result_reg, result_reg, amount_value);
      } else {
        code.MOV(result_reg, WZR);
      }
    } else {
      auto amount_reg = reg_alloc.GetVariableHostReg(amount.GetVar());
      auto tmp_reg = reg_alloc.GetTemporaryHostReg();

      // Branch-less LSL
      code.LSL(result_reg.toX(), result_reg.toX(), 32);
      code.MOV(tmp_reg, 33U);
      code.CMP(amount_reg, tmp_reg);
      code.CSEL(tmp_reg, amount_reg, tmp_reg, oaknut::Cond::LS);
      code.LSL(result_reg.toX(), result_reg.toX(), tmp_reg.toX());
      code.LSR(result_reg.toX(), result_reg.toX(), 32);
    }
  } else {
    oaknut::WReg amount_reg(-1);

    if (amount.IsConstant()) {
      amount_reg = reg_alloc.GetTemporaryHostReg();
      code.MOV(amount_reg, amount.GetConst().value);
    } else {
      amount_reg = reg_alloc.GetVariableHostReg(amount.GetVar());
    }

    oaknut::Label zero_returned;
    oaknut::Label end;

    auto tmp_reg0 = reg_alloc.GetTemporaryHostReg();
    auto tmp_reg1 = reg_alloc.GetTemporaryHostReg();

    code.TST(amount_reg, 0xFF);
    code.B(oaknut::Cond::EQ, zero_returned);

    code.NEG(tmp_reg0, amount_reg);
    code.LSR(tmp_reg0, operand_reg, tmp_reg0);
    code.LSL(result_reg, tmp_reg0, tmp_reg0);
    code.UBFIZ(tmp_reg1, tmp_reg1, 29, 1);
    code.CMP(amount_reg, 32);
    code.CSEL(result_reg, result_reg, WZR, LT);
    code.CSEL(tmp_reg1, tmp_reg1, WZR, LE);

    // Update carry flag
    code.AND(HostFlagsReg, HostFlagsReg, ~(1 << 29));
    code.ORR(HostFlagsReg, HostFlagsReg, tmp_reg1.toX());

    // Reflect update on the actual system register.
    UpdateSystemFlags(context, HostFlagsReg, 0xD0000000);

    code.B(end);

    code.l(zero_returned);
    code.MOV(result_reg, operand_reg);
    code.l(end);
  }
}

void ARM64Backend::CompileLSR(CompileContext const& context, IRLogicalShiftRight* op) {
  DESTRUCTURE_CONTEXT;

  auto& amount = op->amount;
  auto& result_var = op->result.Get();
  auto& operand_var = op->operand.Get();
  auto operand_reg = reg_alloc.GetVariableHostReg(operand_var);

  reg_alloc.ReleaseVarAndReuseHostReg(operand_var, result_var);

  auto result_reg = reg_alloc.GetVariableHostReg(result_var);

  if (!op->update_host_flags) {
    if (amount.IsConstant()) {
      auto amount_value = amount.GetConst().value;

      if (amount_value < 32) {
        code.LSR(result_reg, result_reg, amount_value);
      } else {
        code.MOV(result_reg, WZR);
      }
    } else {
      auto amount_reg = reg_alloc.GetVariableHostReg(amount.GetVar());
      auto tmp_reg = reg_alloc.GetTemporaryHostReg();

      code.AND(tmp_reg, amount_reg, 0xff);
      code.LSR(result_reg, operand_reg, tmp_reg);
      code.CMP(tmp_reg, 32);
      code.CSEL(result_reg, result_reg, WZR, LT);
    }
  } else {
    oaknut::WReg amount_reg(-1);

    if (amount.IsConstant()) {
      amount_reg = reg_alloc.GetTemporaryHostReg();
      code.MOV(amount_reg, amount.GetConst().value);
    } else {
      amount_reg = reg_alloc.GetVariableHostReg(amount.GetVar());
    }

    oaknut::Label zero_returned;
    oaknut::Label end;

    auto tmp_reg0 = reg_alloc.GetTemporaryHostReg();
    auto tmp_reg1 = reg_alloc.GetTemporaryHostReg();

    code.TST(amount_reg, 0xFF);
    code.B(oaknut::Cond::EQ, zero_returned);

    code.SUB(tmp_reg0, amount_reg, 1);
    code.LSR(tmp_reg0, operand_reg, tmp_reg0);
    code.LSR(result_reg, tmp_reg0, tmp_reg0);
    code.UBFIZ(tmp_reg1, tmp_reg1, 29, 1);
    code.CMP(amount_reg, 32);
    code.CSEL(result_reg, result_reg, WZR, LT);
    code.CSEL(tmp_reg1, tmp_reg1, WZR, LE);

    // Update carry flag
    code.AND(HostFlagsReg, HostFlagsReg, ~(1 << 29));
    code.ORR(HostFlagsReg, HostFlagsReg, tmp_reg1.toX());

    // Reflect update on the actual system register.
    UpdateSystemFlags(context, HostFlagsReg, 0xD0000000);

    code.B(end);

    code.l(zero_returned);
    code.MOV(result_reg, operand_reg);
    code.l(end);
  }
}

void ARM64Backend::CompileASR(CompileContext const& context, IRArithmeticShiftRight* op) {
  DESTRUCTURE_CONTEXT;

  auto& amount = op->amount;
  auto& result_var = op->result.Get();
  auto& operand_var = op->operand.Get();
  auto operand_reg = reg_alloc.GetVariableHostReg(operand_var);

  reg_alloc.ReleaseVarAndReuseHostReg(operand_var, result_var);

  auto result_reg = reg_alloc.GetVariableHostReg(result_var);

  if (!op->update_host_flags) {
    if (amount.IsConstant()) {
      auto amount_value = amount.GetConst().value;

      code.ASR(result_reg, result_reg, std::max(amount_value, 31U));
    } else {
      auto amount_reg = reg_alloc.GetVariableHostReg(amount.GetVar());
      auto tmp_reg0 = reg_alloc.GetTemporaryHostReg();
      auto tmp_reg1 = reg_alloc.GetTemporaryHostReg();

      code.MOV(tmp_reg1, 31);
      code.AND(tmp_reg0, amount_reg, 0xff);
      code.CMP(tmp_reg0, tmp_reg1);
      code.CSEL(tmp_reg0, tmp_reg0, tmp_reg1, LS);
      code.ASR(result_reg, operand_reg, tmp_reg0);
    }
  } else {
    oaknut::WReg amount_reg(-1);

    if (amount.IsConstant()) {
      amount_reg = reg_alloc.GetTemporaryHostReg();
      code.MOV(amount_reg, amount.GetConst().value);
    } else {
      amount_reg = reg_alloc.GetVariableHostReg(amount.GetVar());
    }

    oaknut::Label zero_returned;
    oaknut::Label end;

    auto tmp_reg0 = reg_alloc.GetTemporaryHostReg();
    auto tmp_reg1 = reg_alloc.GetTemporaryHostReg();

    code.ANDS(tmp_reg0, amount_reg, 0xFF);
    code.B(oaknut::Cond::EQ, zero_returned);

    code.MOV(tmp_reg1, 63);
    code.CMP(tmp_reg0, tmp_reg1);
    code.CSEL(tmp_reg0, tmp_reg0, tmp_reg1, LS);

    code.SXTW(result_reg.toX(), operand_reg);
    code.SUB(tmp_reg1, tmp_reg0, 1);
    code.ASR(tmp_reg1.toX(), result_reg.toX(), tmp_reg1.toX());
    code.ASR(result_reg.toX(), result_reg.toX(), tmp_reg0.toX());
    code.UBFIZ(tmp_reg1, tmp_reg1, 29, 1);

    // Update carry flag
    code.AND(HostFlagsReg, HostFlagsReg, ~(1 << 29));
    code.ORR(HostFlagsReg, HostFlagsReg, tmp_reg1.toX());

    // Reflect update on the actual system register.
    UpdateSystemFlags(context, HostFlagsReg, 0xD0000000);

    code.B(end);

    code.l(zero_returned);
    code.MOV(result_reg, operand_reg);
    code.l(end);
  }
}

void ARM64Backend::CompileROR(CompileContext const& context, IRRotateRight* op) {
  DESTRUCTURE_CONTEXT;

  auto& amount = op->amount;
  auto& result_var = op->result.Get();
  auto& operand_var = op->operand.Get();
  auto operand_reg = reg_alloc.GetVariableHostReg(operand_var);

  reg_alloc.ReleaseVarAndReuseHostReg(operand_var, result_var);

  auto result_reg = reg_alloc.GetVariableHostReg(result_var);

  if (!op->update_host_flags) {
    if (amount.IsConstant()) {
      auto amount_value = amount.GetConst().value % 32;

      code.ROR(result_reg, result_reg, amount_value);
    } else {
      auto amount_reg = reg_alloc.GetVariableHostReg(amount.GetVar());

      code.ROR(result_reg, operand_reg, amount_reg);
    }
  } else {
    oaknut::WReg amount_reg(-1);

    if (amount.IsConstant()) {
      amount_reg = reg_alloc.GetTemporaryHostReg();
      code.MOV(amount_reg, amount.GetConst().value);
    } else {
      amount_reg = reg_alloc.GetVariableHostReg(amount.GetVar());
    }

    auto tmp_reg0 = reg_alloc.GetTemporaryHostReg();

    code.TST(amount_reg, 0xff);
    code.LSR(tmp_reg0, result_reg, 31 - 29);
    code.AND(tmp_reg0, tmp_reg0, 1 << 29);
    code.CSEL(tmp_reg0, tmp_reg0, WZR, EQ);

    // Update carry flag
    code.AND(HostFlagsReg, HostFlagsReg, ~(1 << 29));
    code.ORR(HostFlagsReg, HostFlagsReg, tmp_reg0.toX());

    // Reflect update on the actual system register.
    UpdateSystemFlags(context, HostFlagsReg, 0xD0000000);
  }
}

} // namespace lunatic::backend
