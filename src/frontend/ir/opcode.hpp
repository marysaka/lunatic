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
  StoreGPR
};

struct IROpcode {
  virtual ~IROpcode() = default;

  virtual auto GetClass() const -> IROpcodeClass = 0;
  virtual bool Reads (IRVariable const& var) const = 0;
  virtual bool Writes(IRVariable const& var) const = 0;
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

  bool Reads(IRVariable const& var) const override {
    return false;
  }

  bool Writes(IRVariable const& var) const override {
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

  bool Reads(IRVariable const& var) const override {
    if (value.IsVariable()) {
      return &var == &value.GetVar();
    }
    return false;
  }

  bool Writes(IRVariable const& var) const override {
    return false;
  }

  auto ToString() const -> std::string override {
    return fmt::format("stgpr {}, {}", std::to_string(reg), std::to_string(value));
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
