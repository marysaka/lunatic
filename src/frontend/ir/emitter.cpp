/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdexcept>

#include "emitter.hpp"

namespace lunatic {
namespace frontend {

auto IREmitter::ToString() const -> std::string {
  std::string source;
  size_t location = 0;

  for (auto const& var : variables) {
    source += fmt::format("{} {}\r\n", std::to_string(var->data_type), std::to_string(*var));
  }

  source += "\r\n";

  for (auto const& op : code) {
    source += fmt::format("{:03} {}\r\n", location++, op->ToString());
  }

  return source;
}

void IREmitter::Optimize() {
  // TODO: come up with a hashing method that produces smaller hashes.
  auto get_gpr_id = [](IRGuestReg reg) -> int {
    auto id = static_cast<int>(reg.reg);
    auto mode = reg.mode;

    if (id <= 7 || (id <= 12 && mode != Mode::FIQ) || id == 15) {
      return id;
    }

    if (mode == Mode::User) {
      mode = Mode::System;
    }

    return (static_cast<int>(mode) << 4) | id;
  };

  // Forward pass: remove redundant register reads
  {
    auto it = code.begin();
    auto end = code.end();
    IRValue current_gpr_value[512] {};
    IRValue current_cpsr_value;

    while (it != end) {
      auto& op_ = *it;
      auto klass = op_->GetClass();

      switch (klass) {
        case IROpcodeClass::StoreGPR: {
          auto op = lunatic_cast<IRStoreGPR>(op_.get());
          auto gpr_id = get_gpr_id(op->reg);

          current_gpr_value[gpr_id] = op->value;
          break;
        }
        case IROpcodeClass::LoadGPR: {
          auto  op = lunatic_cast<IRLoadGPR>(op_.get());
          auto  gpr_id  = get_gpr_id(op->reg);
          auto  var_src = current_gpr_value[gpr_id];
          auto& var_dst = op->result;

          if (!var_src.IsNull()) {
            it = code.erase(it);

            // TODO: do not copy source, repoint destination to source instead.
            // This would only work reliably if source is a variable.
            code.insert(it, std::make_unique<IRMov>(var_dst, var_src, false));

            if (var_src.IsVariable()) {
              // Destination is a younger variable that now contains the current GPR value. 
              // This helps to reduce the lifetime of the source variable.
              current_gpr_value[gpr_id] = var_dst;
            }
            continue;
          } else {
            current_gpr_value[gpr_id] = var_dst;
          }
          break;
        }
        case IROpcodeClass::StoreCPSR: {
          current_cpsr_value = lunatic_cast<IRStoreCPSR>(op_.get())->value;
          break;
        }
        case IROpcodeClass::LoadCPSR: {
          auto  op = lunatic_cast<IRLoadCPSR>(op_.get());
          auto  var_src = current_cpsr_value;
          auto& var_dst = op->result;

          if (!var_src.IsNull()) {
            it = code.erase(it);

            // TODO: do not copy source, repoint destination to source instead.
            // This would only work reliably if source is a variable.
            code.insert(it, std::make_unique<IRMov>(var_dst, var_src, false));

            if (var_src.IsVariable()) {
              // Destination is a younger variable that now contains the current GPR value. 
              // This helps to reduce the lifetime of the source variable.
              current_cpsr_value = var_dst;
            }
            continue;
          } else {
            current_cpsr_value = var_dst;
          }
          break;
        }
        default: break;
      }

      ++it;
    }
  }

  // Backward pass: remove redundant register stores
  {
    bool gpr_already_stored[512] {false};
    bool cpsr_already_stored = false;
    auto it = code.rbegin();
    auto end = code.rend();

    while (it != end) {
      auto& op_ = *it;
      auto klass = op_->GetClass();

      switch (klass) {
        case IROpcodeClass::StoreGPR: {
          auto op = lunatic_cast<IRStoreGPR>(op_.get());
          auto gpr_id = get_gpr_id(op->reg);

          if (gpr_already_stored[gpr_id]) {
            it = std::reverse_iterator{code.erase(std::next(it).base())};
            end = code.rend();
            continue;
          } else {
            gpr_already_stored[gpr_id] = true;
          }
          break;
        }
        case IROpcodeClass::StoreCPSR: {
          if (cpsr_already_stored) {
            it = std::reverse_iterator{code.erase(std::next(it).base())};
            end = code.rend();
            continue;
          } else {
            cpsr_already_stored = true;
          }
          break;
        }
        default: break;
      }

      ++it;
    }
  }
}

auto IREmitter::CreateVar(
  IRDataType data_type,
  char const* label
) -> IRVariable const& {
  auto id = u32(variables.size());
  auto var = new IRVariable{id, data_type, label};

  variables.push_back(std::unique_ptr<IRVariable>(var));
  return *var;
}

void IREmitter::LoadGPR(IRGuestReg reg, IRVariable const& result) {
  Push<IRLoadGPR>(reg, result);
}

void IREmitter::StoreGPR(IRGuestReg reg, IRValue value) {
  if (value.IsNull()) {
    throw std::runtime_error("StoreGPR: value must not be null");
  }
  Push<IRStoreGPR>(reg, value);
}

void IREmitter::LoadSPSR (IRVariable const& result, Mode mode) {
  // TODO: I'm not sure if here is the right place to handle this.
  if (mode == Mode::User || mode == Mode::System) {
    Push<IRLoadCPSR>(result);
  } else {
    Push<IRLoadSPSR>(result, mode);
  }
}

void IREmitter::StoreSPSR(IRValue value, Mode mode) {
  // TODO: I'm not sure if here is the right place to handle this.
  if (mode == Mode::User || mode == Mode::System) {
    return;
  }
  Push<IRStoreSPSR>(value, mode);
}

void IREmitter::LoadCPSR(IRVariable const& result) {
  Push<IRLoadCPSR>(result);
}

void IREmitter::StoreCPSR(IRValue value) {
  if (value.IsNull()) {
    throw std::runtime_error("StoreCPSR: value must not be null");
  }
  Push<IRStoreCPSR>(value);
}

void IREmitter::ClearCarry() {
  Push<IRClearCarry>();
}

void IREmitter::SetCarry() {
  Push<IRSetCarry>();
}

void IREmitter::UpdateNZ(
  IRVariable const& result,
  IRVariable const& input
) {
  Push<IRUpdateFlags>(result, input, true, true, false, false);
}

void IREmitter::UpdateNZC(
  IRVariable const& result,
  IRVariable const& input
) {
  Push<IRUpdateFlags>(result, input, true, true, true, false);
}

void IREmitter::UpdateNZCV(
  IRVariable const& result,
  IRVariable const& input
) {
  Push<IRUpdateFlags>(result, input, true, true, true, true);
}

void IREmitter::UpdateQ(
  IRVariable const& result,
  IRVariable const& input
) {
  Push<IRUpdateSticky>(result, input);
}

void IREmitter::LSL(
  IRVariable const& result,
  IRVariable const& operand,
  IRValue amount,
  bool update_host_flags
) {
  if (amount.IsNull()) {
    throw std::runtime_error("LSL: amount must not be null");
  }
  Push<IRLogicalShiftLeft>(result, operand, amount, update_host_flags);
}

void IREmitter::LSR(
  IRVariable const& result,
  IRVariable const& operand,
  IRValue amount,
  bool update_host_flags
) {
  if (amount.IsNull()) {
    throw std::runtime_error("LSR: amount must not be null");
  }
  Push<IRLogicalShiftRight>(result, operand, amount, update_host_flags);
}

void IREmitter::ASR(
  IRVariable const& result,
  IRVariable const& operand,
  IRValue amount,
  bool update_host_flags
) {
  if (amount.IsNull()) {
    throw std::runtime_error("ASR: amount must not be null");
  }
  Push<IRArithmeticShiftRight>(result, operand, amount, update_host_flags);
}

void IREmitter::ROR(
  IRVariable const& result,
  IRVariable const& operand,
  IRValue amount,
  bool update_host_flags
) {
  if (amount.IsNull()) {
    throw std::runtime_error("ROR: amount must not be null");
  }
  Push<IRRotateRight>(result, operand, amount, update_host_flags);
}

void IREmitter::AND(
  Optional<IRVariable const&> result,
  IRVariable const& lhs,
  IRValue rhs,
  bool update_host_flags
) {
  if (rhs.IsNull()) {
    throw std::runtime_error("AND: rhs operand must not be null");
  }
  Push<IRBitwiseAND>(result, lhs, rhs, update_host_flags);
}

void IREmitter::BIC(
  IRVariable const& result,
  IRVariable const& lhs,
  IRValue rhs,
  bool update_host_flags
) {
  if (rhs.IsNull()) {
    throw std::runtime_error("BIC: rhs operand must not be null");
  }
  Push<IRBitwiseBIC>(result, lhs, rhs, update_host_flags);
}

void IREmitter::EOR(
  Optional<IRVariable const&> result,
  IRVariable const& lhs,
  IRValue rhs,
  bool update_host_flags
) {
  if (rhs.IsNull()) {
    throw std::runtime_error("EOR: rhs operand must not be null");
  }
  Push<IRBitwiseEOR>(result, lhs, rhs, update_host_flags);
}

void IREmitter::SUB(
  Optional<IRVariable const&> result,
  IRVariable const& lhs,
  IRValue rhs,
  bool update_host_flags
) {
  if (rhs.IsNull()) {
    throw std::runtime_error("SUB: rhs operand must not be null");
  }
  Push<IRSub>(result, lhs, rhs, update_host_flags);
}

void IREmitter::RSB(
  IRVariable const& result,
  IRVariable const& lhs,
  IRValue rhs,
  bool update_host_flags
) {
  if (rhs.IsNull()) {
    throw std::runtime_error("RSB: rhs operand must not be null");
  }
  Push<IRRsb>(result, lhs, rhs, update_host_flags);
}

void IREmitter::ADD(
  Optional<IRVariable const&> result,
  IRVariable const& lhs,
  IRValue rhs,
  bool update_host_flags
) {
  if (rhs.IsNull()) {
    throw std::runtime_error("ADD: rhs operand must not be null");
  }
  Push<IRAdd>(result, lhs, rhs, update_host_flags);
}

void IREmitter::ADC(
  IRVariable const& result,
  IRVariable const& lhs,
  IRValue rhs,
  bool update_host_flags
) {
  if (rhs.IsNull()) {
    throw std::runtime_error("ADC: rhs operand must not be null");
  }
  Push<IRAdc>(result, lhs, rhs, update_host_flags);
}

void IREmitter::SBC(
  IRVariable const& result,
  IRVariable const& lhs,
  IRValue rhs,
  bool update_host_flags
) {
  if (rhs.IsNull()) {
    throw std::runtime_error("SBC: rhs operand must not be null");
  }
  Push<IRSbc>(result, lhs, rhs, update_host_flags);
}

void IREmitter::RSC(
  IRVariable const& result,
  IRVariable const& lhs,
  IRValue rhs,
  bool update_host_flags
) {
  if (rhs.IsNull()) {
    throw std::runtime_error("RSC: rhs operand must not be null");
  }
  Push<IRRsc>(result, lhs, rhs, update_host_flags);
}

void IREmitter::ORR(
  IRVariable const& result,
  IRVariable const& lhs,
  IRValue rhs,
  bool update_host_flags
) {
  if (rhs.IsNull()) {
    throw std::runtime_error("ORR: rhs operand must not be null");
  }
  Push<IRBitwiseORR>(result, lhs, rhs, update_host_flags);
}

void IREmitter::MOV(
  IRVariable const& result,
  IRValue source,
  bool update_host_flags
) {
  Push<IRMov>(result, source, update_host_flags);
}

void IREmitter::MVN(
  IRVariable const& result,
  IRValue source,
  bool update_host_flags
) {
  Push<IRMvn>(result, source, update_host_flags);
}

void IREmitter::MUL(
  Optional<IRVariable const&> result_hi,
  IRVariable const& result_lo,
  IRVariable const& lhs,
  IRVariable const& rhs,
  bool update_host_flags
) {
  if (lhs.data_type != rhs.data_type) {
    throw std::runtime_error("MUL: LHS and RHS operands must have same data type.");
  }
  Push<IRMultiply>(result_hi, result_lo, lhs, rhs, update_host_flags);
}

void IREmitter::ADD64(
  IRVariable const& result_hi,
  IRVariable const& result_lo,
  IRVariable const& lhs_hi,
  IRVariable const& lhs_lo,
  IRVariable const& rhs_hi,
  IRVariable const& rhs_lo,
  bool update_host_flags
) {
  Push<IRAdd64>(result_hi, result_lo, lhs_hi, lhs_lo, rhs_hi, rhs_lo, update_host_flags);
}

void IREmitter::LDR(
  IRMemoryFlags flags,
  IRVariable const& result,
  IRVariable const& address
) {
  Push<IRMemoryRead>(flags, result, address);
}

void IREmitter::STR(
  IRMemoryFlags flags,
  IRVariable const& source,
  IRVariable const& address
) {
  Push<IRMemoryWrite>(flags, source, address);
}

void IREmitter::Flush(
  IRVariable const& address_out,
  IRVariable const& address_in,
  IRVariable const& cpsr_in
) {
  Push<IRFlush>(address_out, address_in, cpsr_in);
}

void IREmitter::FlushExchange(
  IRVariable const& address_out,
  IRVariable const& cpsr_out,
  IRVariable const& address_in,
  IRVariable const& cpsr_in
) {
  Push<IRFlushExchange>(address_out, cpsr_out, address_in, cpsr_in);
}

void IREmitter::CLZ(
  IRVariable const& result,
  IRVariable const& operand
) {
  Push<IRCountLeadingZeros>(result, operand);
}

void IREmitter::QADD(
  IRVariable const& result,
  IRVariable const& lhs,
  IRVariable const& rhs
) {
  Push<IRSaturatingAdd>(result, lhs, rhs);
}

void IREmitter::QSUB(
  IRVariable const& result,
  IRVariable const& lhs,
  IRVariable const& rhs
) {
  Push<IRSaturatingSub>(result, lhs, rhs);
}


void IREmitter::MRC(
  IRVariable const& result,
  int coprocessor_id,
  int opcode1,
  int cn,
  int cm,
  int opcode2
) {
  Push<IRReadCoprocessorRegister>(result, coprocessor_id, opcode1, cn, cm, opcode2);
}

void IREmitter::MCR(
  IRValue value,
  int coprocessor_id,
  int opcode1,
  int cn,
  int cm,
  int opcode2
) {
  Push<IRWriteCoprocessorRegister>(value, coprocessor_id, opcode1, cn, cm, opcode2);
}

} // namespace lunatic::frontend
} // namespace lunatic
