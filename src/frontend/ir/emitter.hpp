/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <list>
#include <memory>
#include <vector>

#include "common/optional.hpp"
#include "opcode.hpp"

namespace lunatic {
namespace frontend {

struct IREmitter {
  using InstructionList = std::list<std::unique_ptr<IROpcode>>;
  using VariableList = std::vector<std::unique_ptr<IRVariable>>;

  auto Code() const -> InstructionList const& { return code; }
  auto Vars() const -> VariableList const& { return variables; }
  auto ToString() const -> std::string;
  void Optimize();

  auto CreateVar(
    IRDataType data_type,
    char const* label = nullptr
  ) -> IRVariable const&;

  void LoadGPR (IRGuestReg reg, IRVariable const& result);
  void StoreGPR(IRGuestReg reg, IRValue value);

  void LoadSPSR (IRVariable const& result, State::Mode mode);
  void StoreSPSR(IRValue value, State::Mode mode);
  void LoadCPSR (IRVariable const& result);
  void StoreCPSR(IRValue value);

  void ClearCarry();
  void SetCarry();

  void UpdateNZCV(
    IRVariable const& result,
    IRVariable const& input
  );

  void UpdateNZC(
    IRVariable const& result,
    IRVariable const& input
  );

  void LSL(
    IRVariable const& result,
    IRVariable const& operand,
    IRValue amount,
    bool update_host_flags
  );

  void LSR(
    IRVariable const& result,
    IRVariable const& operand,
    IRValue amount,
    bool update_host_flags
  );

  void ASR(
    IRVariable const& result,
    IRVariable const& operand,
    IRValue amount,
    bool update_host_flags
  );

  void ROR(
    IRVariable const& result,
    IRVariable const& operand,
    IRValue amount,
    bool update_host_flags
  );

  void AND(
    Optional<IRVariable const&> result,
    IRVariable const& lhs,
    IRValue rhs,
    bool update_host_flags
  );

  void BIC(
    IRVariable const& result,
    IRVariable const& lhs,
    IRValue rhs,
    bool update_host_flags
  );

  void EOR(
    Optional<IRVariable const&> result,
    IRVariable const& lhs,
    IRValue rhs,
    bool update_host_flags
  );

  void SUB(
    Optional<IRVariable const&> result,
    IRVariable const& lhs,
    IRValue rhs,
    bool update_host_flags
  );

  void RSB(
    IRVariable const& result,
    IRVariable const& lhs,
    IRValue rhs,
    bool update_host_flags
  );

  void ADD(
    Optional<IRVariable const&> result,
    IRVariable const& lhs,
    IRValue rhs,
    bool update_host_flags
  );

  void ADC(
    IRVariable const& result,
    IRVariable const& lhs,
    IRValue rhs,
    bool update_host_flags
  );

  void SBC(
    IRVariable const& result,
    IRVariable const& lhs,
    IRValue rhs,
    bool update_host_flags
  );

  void RSC(
    IRVariable const& result,
    IRVariable const& lhs,
    IRValue rhs,
    bool update_host_flags
  );

  void ORR(
    IRVariable const& result,
    IRVariable const& lhs,
    IRValue rhs,
    bool update_host_flags
  );

  void MOV(
    IRVariable const& result,
    IRValue source,
    bool update_host_flags
  );

  void MVN(
    IRVariable const& result,
    IRValue source,
    bool update_host_flags
  );

  void LDR(
    IRMemoryFlags flags,
    IRVariable const& result,
    IRVariable const& address
  );

  void STR(
    IRMemoryFlags flags,
    IRVariable const& source,
    IRVariable const& address
  );

  void Flush(
    IRVariable const& address_out,
    IRVariable const& address_in,
    IRVariable const& cpsr_in
  );

  void FlushExchange(
    IRVariable const& address_out,
    IRVariable const& cpsr_out,
    IRVariable const& address_in,
    IRVariable const& cpsr_in
  );

private:
  template<typename T, typename... Args>
  void Push(Args&&... args) {
    code.push_back(std::make_unique<T>(args...));
  }

  InstructionList code;
  VariableList variables;
};

} // namespace lunatic::frontend
} // namespace lunatic
