/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <fmt/format.h>
#include <stdexcept>

#include "register.hpp"
#include "value.hpp"

namespace lunatic {
namespace frontend {

enum class IROpcodeClass {
  LoadGPR,
  StoreGPR,
  LoadCPSR,
  StoreCPSR,
  UpdateFlags,
  LogicalShiftLeft,
  LogicalShiftRight,
  ArithmeticShiftRight,
  RotateRight,
  AND,
  EOR,
  Sub,
  Add,
  AddWithCarry,
  SubWithCarry,
  ORR,
  BIC
};

struct IROpcode {
  virtual ~IROpcode() = default;

  virtual auto GetClass() const -> IROpcodeClass = 0;
  virtual auto Reads (IRVariable const& var) const -> bool = 0;
  virtual auto Writes(IRVariable const& var) const -> bool = 0;
  virtual auto ToString() const -> std::string = 0;
};

template<IROpcodeClass _klass>
struct IROpcodeBase : IROpcode {
  static constexpr IROpcodeClass klass = _klass;

  auto GetClass() const -> IROpcodeClass override { return _klass; }
};

struct IRLoadGPR final : IROpcodeBase<IROpcodeClass::LoadGPR> {
  IRLoadGPR(IRGuestReg reg, IRVariable const& result) : reg(reg), result(result) {}

  /// The GPR to load
  IRGuestReg reg;

  /// The variable to load the GPR into
  IRVariable const& result;

  auto Reads(IRVariable const& var) const -> bool override {
    return false;
  }

  auto Writes(IRVariable const& var) const -> bool override {
    return &var == &result;
  }

  auto ToString() const -> std::string override {
    return fmt::format("ldgpr {}, {}", std::to_string(reg), std::to_string(result));
  }
};

struct IRStoreGPR final : IROpcodeBase<IROpcodeClass::StoreGPR> {
  IRStoreGPR(IRGuestReg reg, IRValue value) : reg(reg), value(value) {}

  /// The GPR to write
  IRGuestReg reg;

  /// The variable or constant to write to the GPR (must be non-null)
  IRValue value;

  auto Reads(IRVariable const& var) const -> bool override {
    if (value.IsVariable()) {
      return &var == &value.GetVar();
    }
    return false;
  }

  auto Writes(IRVariable const& var) const -> bool override {
    return false;
  }

  auto ToString() const -> std::string override {
    return fmt::format("stgpr {}, {}", std::to_string(reg), std::to_string(value));
  }
};

struct IRLoadCPSR final : IROpcodeBase<IROpcodeClass::LoadCPSR> {
  IRLoadCPSR(IRVariable const& result) : result(result) {}

  /// The variable to load CPSR into
  IRVariable const& result;

  auto Reads(IRVariable const& var) const -> bool override {
    return false;
  }

  auto Writes(IRVariable const& var) const -> bool override {
    return &var == &result;
  }

  auto ToString() const -> std::string override {
    return fmt::format("ldcpsr {}", std::to_string(result));
  }  
};

struct IRStoreCPSR final : IROpcodeBase<IROpcodeClass::StoreCPSR> {
  IRStoreCPSR(IRValue value) : value(value) {}

  /// The variable or constant to write to CPSR
  IRValue value;

  auto Reads(IRVariable const& var) const -> bool override {
    return value.IsVariable() && &var == &value.GetVar();
  }

  auto Writes(IRVariable const& var) const -> bool override {
    return false;
  }

  auto ToString() const -> std::string override {
    return fmt::format("stcpsr {}", std::to_string(value));
  }  
};

struct IRUpdateFlags final : IROpcodeBase<IROpcodeClass::UpdateFlags> {
  // TODO: take care of the sticky overflow flag
  IRUpdateFlags(
    IRVariable const& result,
    IRVariable const& input,
    bool flag_n,
    bool flag_z,
    bool flag_c,
    bool flag_v
  ) : result(result), input(input), flag_n(flag_n), flag_z(flag_z), flag_c(flag_c), flag_v(flag_v) {}

  IRVariable const& result;
  IRVariable const& input;
  bool flag_n;
  bool flag_z;
  bool flag_c;
  bool flag_v;

  auto Reads(IRVariable const& var) const -> bool override {
    return &input == &var;
  }

  auto Writes(IRVariable const& var) const -> bool override {
    return &result == &var;
  }

  auto ToString() const -> std::string override {
    return fmt::format("update.{}{}{}{} {}, {}",
      flag_n ? 'n' : '-',
      flag_z ? 'z' : '-',
      flag_c ? 'c' : '-',
      flag_v ? 'v' : '-',
      std::to_string(result),
      std::to_string(input));
  }
};

template<IROpcodeClass _klass>
struct IRShifterBase : IROpcodeBase<_klass> {
  IRShifterBase(
    IRVariable const& result,
    IRVariable const& operand,
    IRValue amount,
    bool update_host_flags
  ) : result(result), operand(operand), amount(amount) {}

  /// The variable to store the result in
  IRVariable const& result;

  /// The operand to be shifted
  IRVariable const& operand;

  /// The variable or constant shift amount
  IRValue amount;

  /// Whether to update/cache the host system flags
  bool update_host_flags;

  auto Reads(IRVariable const& var) const -> bool override {
    return &var == &operand || (amount.IsVariable() && &var == &amount.GetVar());
  }

  auto Writes(IRVariable const& var) const -> bool override {
    return &var == &result;
  }
};

struct IRLogicalShiftLeft final : IRShifterBase<IROpcodeClass::LogicalShiftLeft> {
  using IRShifterBase::IRShifterBase;

  auto ToString() const -> std::string override {
    return fmt::format("lsl {}, {}, {}", std::to_string(result), std::to_string(operand), std::to_string(amount));
  }
};

struct IRLogicalShiftRight final : IRShifterBase<IROpcodeClass::LogicalShiftRight> {
  using IRShifterBase::IRShifterBase;

  auto ToString() const -> std::string override {
    return fmt::format("lsr {}, {}, {}", std::to_string(result), std::to_string(operand), std::to_string(amount));
  }
};

struct IRArithmeticShiftRight final : IRShifterBase<IROpcodeClass::ArithmeticShiftRight> {
  using IRShifterBase::IRShifterBase;

  auto ToString() const -> std::string override {
    return fmt::format("asr {}, {}, {}", std::to_string(result), std::to_string(operand), std::to_string(amount));
  }
};

struct IRRotateRight final : IRShifterBase<IROpcodeClass::RotateRight> {
  using IRShifterBase::IRShifterBase;

  auto ToString() const -> std::string override {
    return fmt::format("ror {}, {}, {}", std::to_string(result), std::to_string(operand), std::to_string(amount));
  }
};

template<IROpcodeClass _klass>
struct IRBinaryOpBase : IROpcodeBase<_klass> {
  IRBinaryOpBase(
    IRValue result,
    IRVariable const& lhs,
    IRValue rhs,
    bool update_host_flags
  ) : result(result), lhs(lhs), rhs(rhs), update_host_flags(update_host_flags) {}

  /// A variable to store the result or null if the result is unused
  IRValue result;

  /// The left-hand side operand (variable)
  IRVariable const& lhs;

  /// The right-hand side operand (variable or constant)
  IRValue rhs;

  /// Whether to update/cache the host system flags
  bool update_host_flags;

  auto Reads(IRVariable const& var) const -> bool override {
    return &lhs == &var || (rhs.IsVariable() && &rhs.GetVar() == &var);
  }

  auto Writes(IRVariable const& var) const -> bool override {
    return result.IsVariable() && (&result.GetVar() == &var);
  }  
};

struct IRBitwiseAND final : IRBinaryOpBase<IROpcodeClass::AND> {
  using IRBinaryOpBase::IRBinaryOpBase;

  auto ToString() const -> std::string override {
    return fmt::format("and{} {}, {}, {}",
      update_host_flags ? "s" : "",
      std::to_string(result),
      std::to_string(lhs),
      std::to_string(rhs));
  }
};

struct IRBitwiseEOR final : IRBinaryOpBase<IROpcodeClass::EOR> {
  using IRBinaryOpBase::IRBinaryOpBase;

  auto ToString() const -> std::string override {
    return fmt::format("eor{} {}, {}, {}",
      update_host_flags ? "s" : "",
      std::to_string(result),
      std::to_string(lhs),
      std::to_string(rhs));
  }
};

struct IRSub final : IRBinaryOpBase<IROpcodeClass::Sub> {
  using IRBinaryOpBase::IRBinaryOpBase;

  auto ToString() const -> std::string override {
    return fmt::format("sub{} {}, {}, {}",
      update_host_flags ? "s" : "",
      std::to_string(result),
      std::to_string(lhs),
      std::to_string(rhs));
  }
};

struct IRAdd final : IRBinaryOpBase<IROpcodeClass::Add> {
  using IRBinaryOpBase::IRBinaryOpBase;

  auto ToString() const -> std::string override {
    return fmt::format("add{} {}, {}, {}",
      update_host_flags ? "s" : "",
      std::to_string(result),
      std::to_string(lhs),
      std::to_string(rhs));
  }
};

} // namespace lunatic::frontend
} // namespace lunatic

// TODO: consider that T should be a pointer type.
template<typename T>
auto lunatic_cast(lunatic::frontend::IROpcode* value) -> T* {
  if (value->GetClass() != T::klass) {
    throw std::runtime_error("bad lunatic_cast");
  }

  return reinterpret_cast<T*>(value);
}
