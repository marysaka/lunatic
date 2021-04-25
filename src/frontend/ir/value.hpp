/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <fmt/format.h>
#include <lunatic/integer.hpp>
#include <stdexcept>

namespace lunatic {
namespace frontend {

enum class IRDataType {
  UInt32
};

/// Represents an immutable variable
struct IRVariable {
  IRVariable(IRVariable const& other) = delete;

  /// ID that is unique inside the IREmitter instance.
  const u32 id;

  /// The underlying data type
  const IRDataType data_type;

  /// An optional label to hint at the variable usage
  char const* const label;

private:
  friend struct IREmitter;

  IRVariable(
    u32 id,
    IRDataType data_type,
    char const* label
  ) : id(id), data_type(data_type), label(label) {}
};

/// Represents an immediate (constant) value
struct IRConstant {
  IRConstant(u32 value) : data_type(IRDataType::UInt32), value(value) {}

  /// The underlying data type
  const IRDataType data_type;

  /// The underlying constant value
  const u32 value;
};

/// Represents a constant or variable IR opcode argument
struct IRValue {
  IRValue() {}
  IRValue(IRVariable const& variable) : type(Type::Variable), variable(&variable) {}
  IRValue(IRConstant const& constant) : type(Type::Constant), constant( constant) {}

  bool IsNull() const { return type == Type::Null; }
  bool IsVariable() const { return type == Type::Variable; }
  bool IsConstant() const { return type == Type::Constant; }

  auto GetVar() const -> IRVariable const& {
    if (!IsVariable()) {
      throw std::runtime_error("called GetVar() but value is a constant or null");
    }
    return *variable;
  }

  auto GetConst() const -> IRConstant const& {
    if (!IsConstant()) {
      throw std::runtime_error("called GetConst() but value is a variable or null");
    }
    return constant;
  }

private:
  enum class Type {
    Null,
    Variable,
    Constant
  };

  Type type = Type::Null;

  union {
    IRVariable const* variable;
    IRConstant const  constant;
  };
};

} // namespace lunatic::frontend 
} // namespace lunatic

namespace std {

inline auto to_string(lunatic::frontend::IRDataType data_type) -> std::string {
  switch (data_type) {
    case lunatic::frontend::IRDataType::UInt32:
      return "u32";
    default:
      return "???";
  }
}

inline auto to_string(lunatic::frontend::IRVariable const& variable) -> std::string {
  if (variable.label) {
    return fmt::format("var{}_{}", variable.id, variable.label);
  }
  return fmt::format("var{}", variable.id);
}

inline auto to_string(lunatic::frontend::IRConstant const& constant) -> std::string {
  return fmt::format("0x{:08X}", constant.value);
}

inline auto to_string(lunatic::frontend::IRValue const& value) -> std::string {
  if (value.IsNull()) {
    return "(null)";
  }
  if (value.IsConstant()) {
    return std::to_string(value.GetConst());
  }
  return std::to_string(value.GetVar());
}

} // namespace std
