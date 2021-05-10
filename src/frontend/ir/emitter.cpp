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
  // TODO: come up with a better hashing method.
  // The mode field doesn't use all values and in most cases it doesn't matter.
  auto get_gpr_id = [](IRGuestReg reg) -> int {
    return (static_cast<int>(reg.mode) << 4) | static_cast<int>(reg.reg);
  };

  // Forward pass: remove redundant register reads
  {
    auto it = code.begin();
    auto end = code.end();
    IRValue current_gpr_value[512] {};

    while (it != end) {
      auto& op_ = *it;
      auto klass = op_->GetClass();

      if (klass == IROpcodeClass::StoreGPR) {
        auto op = lunatic_cast<IRStoreGPR>(op_.get());
        auto gpr_id = get_gpr_id(op->reg);

        current_gpr_value[gpr_id] = op->value;
      } else if (klass == IROpcodeClass::LoadGPR) {
        auto  op = lunatic_cast<IRLoadGPR>(op_.get());
        auto  gpr_id  = get_gpr_id(op->reg);
        auto  var_src = current_gpr_value[gpr_id];
        auto& var_dst = op->result;

        if (!var_src.IsNull()) {
          auto it_old = it;
          ++it;
          // TODO: repoint subsequent uses if source is a variable.
          code.insert(it, std::make_unique<IRMov>(var_dst, var_src, false));
          code.erase(it_old);
          continue;
        } else {
          current_gpr_value[gpr_id] = var_dst;
        }
      }

      ++it;
    }
  }

  // Backward pass: remove redundant register stores
  {
    bool gpr_already_stored[512] {false};
    auto it = code.rbegin();
    auto end = code.rend();

    while (it != end) {
      auto& op_ = *it;
      auto klass = op_->GetClass();

      if (klass == IROpcodeClass::StoreGPR) {
        auto op = lunatic_cast<IRStoreGPR>(op_.get());
        auto gpr_id = get_gpr_id(op->reg);

        if (gpr_already_stored[gpr_id]) {
          auto it_old = it;
          ++it;
          code.erase(std::next(it_old).base());
          continue;
        } else {
          gpr_already_stored[gpr_id] = true;
        }
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

void IREmitter::LoadSPSR (IRVariable const& result, State::Mode mode) {
  Push<IRLoadSPSR>(result, mode);
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

void IREmitter::UpdateNZCV(
  IRVariable const& result,
  IRVariable const& input
) {
  Push<IRUpdateFlags>(result, input, true, true, true, true);
}

void IREmitter::UpdateNZC(
  IRVariable const& result,
  IRVariable const& input
) {
  Push<IRUpdateFlags>(result, input, true, true, true, false);
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

} // namespace lunatic::frontend
} // namespace lunatic
