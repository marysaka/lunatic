/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <list>
#include <memory>
#include <vector>

#include "opcode.hpp"

namespace lunatic {
namespace frontend {

struct IREmitter {
  auto Code() const -> std::list<std::unique_ptr<IROpcode>> const& { return code; }
  auto Vars() const -> std::vector<std::unique_ptr<IRVariable>> const& { return variables; }
  auto ToString() const -> std::string;

  auto CreateVar(IRDataType data_type, char const* label = nullptr) -> IRVariable const&;

  void LoadGPR(IRGuestReg reg, IRVariable const& result);
  void StoreGPR(IRGuestReg reg, IRValue value);
  void LoadCPSR(IRVariable const& result);
  void StoreCPSR(IRValue value);
  void LSL(IRVariable const& result, IRVariable const& operand, IRValue amount, bool update_host_flags);
  void LSR(IRVariable const& result, IRVariable const& operand, IRValue amount, bool update_host_flags);
  void ASR(IRVariable const& result, IRVariable const& operand, IRValue amount, bool update_host_flags);
  void ROR(IRVariable const& result, IRVariable const& operand, IRValue amount, bool update_host_flags);
  void Add(IRVariable const& result, IRVariable const& lhs, IRValue rhs, bool update_host_flags);
  void UpdateFlags(
    IRVariable const& result,
    IRVariable const& input,
    bool flag_n,
    bool flag_z,
    bool flag_c,
    bool flag_v
  );

private:
  /// List of emitted IR opcodes
  std::list<std::unique_ptr<IROpcode>> code;

  /// List of allocated variables
  std::vector<std::unique_ptr<IRVariable>> variables;
};

} // namespace lunatic::frontend
} // namespace lunatic
