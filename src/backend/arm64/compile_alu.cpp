/*
 * Copyright (C) 2023 marysaka. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.hpp"


namespace lunatic::backend {

template<typename Fn1, typename Fn2>
static void EmitALUOp(ARM64Backend::CompileContext const& context, Optional<IRVariable const&> &result, IRVarRef const& lhs, IRAnyRef const& rhs, bool update_host_flags, Fn1 op, Fn2 op_with_flags, uint32_t mask) {
  DESTRUCTURE_CONTEXT;

  auto& lhs_var = lhs.Get();
  auto  lhs_reg = reg_alloc.GetVariableHostReg(lhs_var);

  if (rhs.IsConstant()) {
    auto imm = rhs.GetConst().value;

    auto rhs_reg = reg_alloc.GetTemporaryHostReg();
    code.MOV(rhs_reg, imm);

    if (result.IsNull()) {
      if (update_host_flags) {
        op_with_flags(context, WZR, lhs_reg, rhs_reg);
      } else {
        op(context, WZR, lhs_reg, rhs_reg);
      }
    } else {
      auto& result_var = result.Unwrap();

      reg_alloc.ReleaseVarAndReuseHostReg(lhs_var, result_var);

      auto result_reg = reg_alloc.GetVariableHostReg(result_var);

      if (update_host_flags) {
        op_with_flags(context, result_reg, lhs_reg, rhs_reg);
      } else {
        op(context, result_reg, lhs_reg, rhs_reg);
      }
    }
  } else {
    auto& rhs_var = rhs.GetVar();
    auto  rhs_reg = reg_alloc.GetVariableHostReg(rhs_var);

    if (result.IsNull()) {
      if (update_host_flags) {
        op_with_flags(context, WZR, lhs_reg, rhs_reg);
      } else {
        op(context, WZR, lhs_reg, rhs_reg);
      }
    } else {
      auto& result_var = result.Unwrap();

      reg_alloc.ReleaseVarAndReuseHostReg(lhs_var, result_var);
      reg_alloc.ReleaseVarAndReuseHostReg(rhs_var, result_var);

      auto result_reg = reg_alloc.GetVariableHostReg(result_var);

      if (update_host_flags) {
        op_with_flags(context, result_reg, lhs_reg, rhs_reg);
      } else {
        op(context, result_reg, lhs_reg, rhs_reg);
      }
    }
  }

  if (update_host_flags) {
    ARM64Backend::UpdateHostFlagsFromSystem(context, mask);
  }
}


void ARM64Backend::CompileAND(CompileContext const& context, IRBitwiseAND* op) {
  EmitALUOp(context, op->result, op->lhs, op->rhs, op->update_host_flags,
            [&](auto const& context, auto& result, auto& a, auto& b) { code->AND(result, a, b); },
            [&](auto const& context, auto& result, auto& a, auto& b) { code->ANDS(result, a, b); },
            0xC0000000);
}

void ARM64Backend::CompileBIC(CompileContext const& context, IRBitwiseBIC* op) { 
  EmitALUOp(context, op->result, op->lhs, op->rhs, op->update_host_flags,
            [&](auto const& context, auto& result, auto& a, auto& b) { code->BIC(result, a, b); },
            [&](auto const& context, auto& result, auto& a, auto& b) { code->BICS(result, a, b); },
            0xC0000000);
}

void ARM64Backend::CompileEOR(CompileContext const& context, IRBitwiseEOR* op) {
  EmitALUOp(context, op->result, op->lhs, op->rhs, op->update_host_flags,
            [&](auto const& context, auto& result, auto& a, auto& b) { code->EOR(result, a, b); },
            [&](auto const& context, auto& result, auto& a, auto& b) {
              code->EOR(result, a, b);
              code->TST(result, result);
            },
            0xC0000000);
}

void ARM64Backend::CompileSUB(CompileContext const& context, IRSub* op) { 
  EmitALUOp(context, op->result, op->lhs, op->rhs, op->update_host_flags,
            [&](auto const& context, auto& result, auto& a, auto& b) { code->SUB(result, a, b); },
            [&](auto const& context, auto& result, auto& a, auto& b) { code->SUBS(result, a, b); },
            0xF0000000);
}

void ARM64Backend::CompileRSB(CompileContext const& context, IRRsb* op) { NOT_IMPLEMENTED_OP(op); }

void ARM64Backend::CompileADD(CompileContext const& context, IRAdd* op) {
  EmitALUOp(context, op->result, op->lhs, op->rhs, op->update_host_flags,
            [&](auto const& context, auto& result, auto& a, auto& b) { code->ADD(result, a, b); },
            [&](auto const& context, auto& result, auto& a, auto& b) { code->ADDS(result, a, b); },
            0xF0000000);
}

void ARM64Backend::CompileADC(CompileContext const& context, IRAdc* op) {
  EmitALUOp(context, op->result, op->lhs, op->rhs, op->update_host_flags,
            [&](auto const& context, auto& result, auto& a, auto& b) { code->ADC(result, a, b); },
            [&](auto const& context, auto& result, auto& a, auto& b) { code->ADCS(result, a, b); },
            0xF0000000);
}

void ARM64Backend::CompileSBC(CompileContext const& context, IRSbc* op) {
  EmitALUOp(context, op->result, op->lhs, op->rhs, op->update_host_flags,
            [&](auto const& context, auto& result, auto& a, auto& b) { code->SBC(result, a, b); },
            [&](auto const& context, auto& result, auto& a, auto& b) { code->SBCS(result, a, b); },
            0xF0000000);
}

void ARM64Backend::CompileRSC(CompileContext const& context, IRRsc* op) { NOT_IMPLEMENTED_OP(op); }

void ARM64Backend::CompileORR(CompileContext const& context, IRBitwiseORR* op) {
  EmitALUOp(context, op->result, op->lhs, op->rhs, op->update_host_flags,
            [&](auto const& context, auto& result, auto& a, auto& b) { code->ORR(result, a, b); },
            [&](auto const& context, auto& result, auto& a, auto& b) {
              code->ORR(result, a, b);
              code->TST(result, result);
            },
            0xC0000000);
}

void ARM64Backend::CompileMOV(CompileContext const& context, IRMov* op) {
  DESTRUCTURE_CONTEXT;

  auto& result_var = op->result.Get();
  auto  result_reg = oaknut::WReg(-1);

  if (op->source.IsConstant()) {
    result_reg = reg_alloc.GetVariableHostReg(result_var);

    code.MOV(result_reg, op->source.GetConst().value);
  } else {
    auto& source_var = op->source.GetVar();
    auto  source_reg = reg_alloc.GetVariableHostReg(source_var);

    reg_alloc.ReleaseVarAndReuseHostReg(source_var, result_var);
    result_reg = reg_alloc.GetVariableHostReg(result_var);

    if (result_reg.index() != source_reg.index()) {
      code.MOV(result_reg, source_reg);
    }
  }


  if (op->update_host_flags) {
    code.TST(result_reg, result_reg);
    UpdateHostFlagsFromSystem(context, 0xC0000000);
  }
}

void ARM64Backend::CompileMVN(CompileContext const& context, IRMvn* op) {
  DESTRUCTURE_CONTEXT;

  auto& result_var = op->result.Get();
  auto  result_reg = oaknut::WReg(-1);

  if (op->source.IsConstant()) {
    result_reg = reg_alloc.GetVariableHostReg(result_var);

    code.MOV(result_reg, op->source.GetConst().value);
    code.MVN(result_reg, result_reg);
  } else {
    auto& source_var = op->source.GetVar();
    auto  source_reg = reg_alloc.GetVariableHostReg(source_var);

    reg_alloc.ReleaseVarAndReuseHostReg(source_var, result_var);
    result_reg = reg_alloc.GetVariableHostReg(result_var);

    code.MVN(result_reg, source_reg);
  }


  if (op->update_host_flags) {
    code.TST(result_reg, result_reg);
    UpdateHostFlagsFromSystem(context, 0xC0000000);
  }
}

void ARM64Backend::CompileCLZ(CompileContext const& context, IRCountLeadingZeros* op) {
  DESTRUCTURE_CONTEXT;

  auto& result_var = op->result.Get();
  auto& operand_var = op->operand.Get();
  auto  operand_reg = reg_alloc.GetVariableHostReg(operand_var);

  reg_alloc.ReleaseVarAndReuseHostReg(operand_var, result_var);

  code.CLZ(reg_alloc.GetVariableHostReg(op->result.Get()), operand_reg);
}

void ARM64Backend::CompileQADD(CompileContext const& context, IRSaturatingAdd* op) { NOT_IMPLEMENTED_OP(op); }
void ARM64Backend::CompileQSUB(CompileContext const& context, IRSaturatingSub* op) { NOT_IMPLEMENTED_OP(op); }

} // namespace lunatic::backend
