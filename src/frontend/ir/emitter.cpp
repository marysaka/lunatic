/*
 * Copyright (C) 2021 fleroviux
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

auto IREmitter::CreateVar(IRDataType data_type, char const* label) -> IRVariable const& {
  auto id = u32(variables.size());
  auto var = new IRVariable{id, data_type, label};

  variables.push_back(std::unique_ptr<IRVariable>(var));
  return *var;
}

void IREmitter::LoadGPR(IRGuestReg reg, IRVariable const& result) {
  code.push_back(std::make_unique<IRLoadGPR>(reg, result));
}

void IREmitter::StoreGPR(IRGuestReg reg, IRValue value) {
  if (value.IsNull()) {
    throw std::runtime_error("StoreGPR: value must not be null");
  }
  code.push_back(std::make_unique<IRStoreGPR>(reg, value));
}

void IREmitter::LoadCPSR(IRVariable const& result) {
  code.push_back(std::make_unique<IRLoadCPSR>(result));
}

void IREmitter::StoreCPSR(IRValue value) {
  if (value.IsNull()) {
    throw std::runtime_error("StoreCPSR: value must not be null");
  }
  code.push_back(std::make_unique<IRStoreCPSR>(value));
}

void IREmitter::LSL(IRVariable const& result, IRVariable const& operand, IRValue amount, bool update_host_flags) {
  if (amount.IsNull()) {
    throw std::runtime_error("LSL: amount must not be null");
  }
  code.push_back(std::make_unique<IRLogicalShiftLeft>(result, operand, amount, update_host_flags));
}

void IREmitter::LSR(IRVariable const& result, IRVariable const& operand, IRValue amount, bool update_host_flags) {
  if (amount.IsNull()) {
    throw std::runtime_error("LSR: amount must not be null");
  }
  code.push_back(std::make_unique<IRLogicalShiftRight>(result, operand, amount, update_host_flags));
}

void IREmitter::ASR(IRVariable const& result, IRVariable const& operand, IRValue amount, bool update_host_flags) {
  if (amount.IsNull()) {
    throw std::runtime_error("ASR: amount must not be null");
  }
  code.push_back(std::make_unique<IRArithmeticShiftRight>(result, operand, amount, update_host_flags));
}

void IREmitter::ROR(IRVariable const& result, IRVariable const& operand, IRValue amount, bool update_host_flags) {
  if (amount.IsNull()) {
    throw std::runtime_error("ROR: amount must not be null");
  }
  code.push_back(std::make_unique<IRRotateRight>(result, operand, amount, update_host_flags));
}

void IREmitter::Add(IRValue result, IRVariable const& lhs, IRValue rhs, bool update_host_flags) {
  if (result.IsConstant()) {
    throw std::runtime_error("Add: result must not be a constant");
  }
  if (rhs.IsNull()) {
    throw std::runtime_error("Add: rhs operand must not be null");
  }
  code.push_back(std::make_unique<IRAdd>(result, lhs, rhs, update_host_flags));
}

void IREmitter::Sub(IRValue result, IRVariable const& lhs, IRValue rhs, bool update_host_flags) {
  if (result.IsConstant()) {
    throw std::runtime_error("Add: result must not be a constant");
  }
  if (rhs.IsNull()) {
    throw std::runtime_error("Sub: rhs operand must not be null");
  }
  code.push_back(std::make_unique<IRSub>(result, lhs, rhs, update_host_flags));
}

// void IREmitter::UpdateFlags(
//   IRVariable const& result,
//   IRVariable const& input,
//   bool flag_n,
//   bool flag_z,
//   bool flag_c,
//   bool flag_v
// ) {
//   code.push_back(std::make_unique<IRUpdateFlags>(result, input, flag_n, flag_z, flag_c, flag_v));
// }

void IREmitter::UpdateNZCV(IRVariable const& result, IRVariable const& input) {
  code.push_back(std::make_unique<IRUpdateFlags>(result, input, true, true, true, true));
}

} // namespace lunatic::frontend
} // namespace lunatic
