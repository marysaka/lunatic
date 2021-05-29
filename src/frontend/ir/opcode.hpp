/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
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
  LoadSPSR,
  StoreSPSR,
  LoadCPSR,
  StoreCPSR,
  ClearCarry,
  SetCarry,
  UpdateFlags,
  LSL,
  LSR,
  ASR,
  ROR,
  AND,
  BIC,
  EOR,
  SUB,
  RSB,
  ADD,
  ADC,
  SBC,
  RSC,
  ORR,
  MOV,
  MVN,
  MUL,
  MemoryRead,
  MemoryWrite,
  Flush,
  FlushExchange
};

// TODO: Reads(), Writes() and ToString() should be const,
// but due to the nature of this Optional<T> implementation this is not possible at the moment.

struct IROpcode {
  virtual ~IROpcode() = default;

  virtual auto GetClass() const -> IROpcodeClass = 0;
  virtual auto Reads (IRVariable const& var) -> bool = 0;
  virtual auto Writes(IRVariable const& var) -> bool = 0;
  virtual auto ToString() -> std::string = 0;
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

  auto Reads(IRVariable const& var) -> bool override {
    return false;
  }

  auto Writes(IRVariable const& var) -> bool override {
    return &var == &result;
  }

  auto ToString() -> std::string override {
    return fmt::format("ldgpr {}, {}", std::to_string(reg), std::to_string(result));
  }
};

struct IRStoreGPR final : IROpcodeBase<IROpcodeClass::StoreGPR> {
  IRStoreGPR(IRGuestReg reg, IRValue value) : reg(reg), value(value) {}

  /// The GPR to write
  IRGuestReg reg;

  /// The variable or constant to write to the GPR (must be non-null)
  IRValue value;

  auto Reads(IRVariable const& var) -> bool override {
    if (value.IsVariable()) {
      return &var == &value.GetVar();
    }
    return false;
  }

  auto Writes(IRVariable const& var) -> bool override {
    return false;
  }

  auto ToString() -> std::string override {
    return fmt::format("stgpr {}, {}", std::to_string(reg), std::to_string(value));
  }
};

struct IRLoadSPSR final : IROpcodeBase<IROpcodeClass::LoadSPSR> {
  IRLoadSPSR(IRVariable const& result, State::Mode mode) : result(result), mode(mode) {}

  IRVariable const& result;
  State::Mode mode;

  auto Reads(IRVariable const& var) -> bool override {
    return false;
  }

  auto Writes(IRVariable const& var) -> bool override {
    return &var == &result;
  }

  auto ToString() -> std::string override {
    return fmt::format("ldspsr.{} {}", std::to_string(mode), std::to_string(result));
  }
};

struct IRStoreSPSR final : IROpcodeBase<IROpcodeClass::StoreSPSR> {
  IRStoreSPSR(IRValue value, State::Mode mode) : value(value), mode(mode) {}

  IRValue value;
  State::Mode mode;

  auto Reads(IRVariable const& var) -> bool override {
    return value.IsVariable() && (&var == &value.GetVar());
  }

  auto Writes(IRVariable const& var) -> bool override {
    return false;
  }

  auto ToString() -> std::string override {
    return fmt::format("stspsr.{} {}", std::to_string(mode), std::to_string(value));
  }
};

struct IRLoadCPSR final : IROpcodeBase<IROpcodeClass::LoadCPSR> {
  IRLoadCPSR(IRVariable const& result) : result(result) {}

  /// The variable to load CPSR into
  IRVariable const& result;

  auto Reads(IRVariable const& var) -> bool override {
    return false;
  }

  auto Writes(IRVariable const& var) -> bool override {
    return &var == &result;
  }

  auto ToString() -> std::string override {
    return fmt::format("ldcpsr {}", std::to_string(result));
  }
};

struct IRStoreCPSR final : IROpcodeBase<IROpcodeClass::StoreCPSR> {
  IRStoreCPSR(IRValue value) : value(value) {}

  /// The variable or constant to write to CPSR
  IRValue value;

  auto Reads(IRVariable const& var) -> bool override {
    return value.IsVariable() && &var == &value.GetVar();
  }

  auto Writes(IRVariable const& var) -> bool override {
    return false;
  }

  auto ToString() -> std::string override {
    return fmt::format("stcpsr {}", std::to_string(value));
  }
};

struct IRClearCarry final : IROpcodeBase<IROpcodeClass::ClearCarry> {
  auto Reads(IRVariable const& var) -> bool override {
    return false;
  }

  auto Writes(IRVariable const& var) -> bool override {
    return false;
  }

  auto ToString() -> std::string override {
    return "clearcarry";
  }
};

struct IRSetCarry final : IROpcodeBase<IROpcodeClass::SetCarry> {
  auto Reads(IRVariable const& var) -> bool override {
    return false;
  }

  auto Writes(IRVariable const& var) -> bool override {
    return false;
  }

  auto ToString() -> std::string override {
    return "setcarry";
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

  auto Reads(IRVariable const& var) -> bool override {
    return &input == &var;
  }

  auto Writes(IRVariable const& var) -> bool override {
    return &result == &var;
  }

  auto ToString() -> std::string override {
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
  ) : result(result), operand(operand), amount(amount), update_host_flags(update_host_flags) {}

  /// The variable to store the result in
  IRVariable const& result;

  /// The operand to be shifted
  IRVariable const& operand;

  /// The variable or constant shift amount
  IRValue amount;

  /// Whether to update/cache the host system flags
  bool update_host_flags;

  auto Reads(IRVariable const& var) -> bool override {
    return &var == &operand || (amount.IsVariable() && &var == &amount.GetVar());
  }

  auto Writes(IRVariable const& var) -> bool override {
    return &var == &result;
  }
};

struct IRLogicalShiftLeft final : IRShifterBase<IROpcodeClass::LSL> {
  using IRShifterBase::IRShifterBase;

  auto ToString() -> std::string override {
    return fmt::format("lsl {}, {}, {}", std::to_string(result), std::to_string(operand), std::to_string(amount));
  }
};

struct IRLogicalShiftRight final : IRShifterBase<IROpcodeClass::LSR> {
  using IRShifterBase::IRShifterBase;

  auto ToString() -> std::string override {
    return fmt::format("lsr {}, {}, {}", std::to_string(result), std::to_string(operand), std::to_string(amount));
  }
};

struct IRArithmeticShiftRight final : IRShifterBase<IROpcodeClass::ASR> {
  using IRShifterBase::IRShifterBase;

  auto ToString() -> std::string override {
    return fmt::format("asr {}, {}, {}", std::to_string(result), std::to_string(operand), std::to_string(amount));
  }
};

struct IRRotateRight final : IRShifterBase<IROpcodeClass::ROR> {
  using IRShifterBase::IRShifterBase;

  auto ToString() -> std::string override {
    return fmt::format("ror {}, {}, {}", std::to_string(result), std::to_string(operand), std::to_string(amount));
  }
};

template<IROpcodeClass _klass>
struct IRBinaryOpBase : IROpcodeBase<_klass> {
  IRBinaryOpBase(
    Optional<IRVariable const&> result,
    IRVariable const& lhs,
    IRValue rhs,
    bool update_host_flags
  ) : result(result), lhs(lhs), rhs(rhs), update_host_flags(update_host_flags) {}

  /// An optional result variable
  Optional<IRVariable const&> result;

  /// The left-hand side operand (variable)
  IRVariable const& lhs;

  /// The right-hand side operand (variable or constant)
  IRValue rhs;

  /// Whether to update/cache the host system flags
  bool update_host_flags;

  auto Reads(IRVariable const& var) -> bool override {
    return &lhs == &var || (rhs.IsVariable() && (&rhs.GetVar() == &var));
  }

  auto Writes(IRVariable const& var) -> bool override {
    return result.HasValue() && (&result.Unwrap() == &var);
  }
};

struct IRBitwiseAND final : IRBinaryOpBase<IROpcodeClass::AND> {
  using IRBinaryOpBase::IRBinaryOpBase;

  auto ToString() -> std::string override {
    return fmt::format("and{} {}, {}, {}",
      update_host_flags ? "s" : "",
      std::to_string(result),
      std::to_string(lhs),
      std::to_string(rhs));
  }
};

struct IRBitwiseBIC final : IRBinaryOpBase<IROpcodeClass::BIC> {
  using IRBinaryOpBase::IRBinaryOpBase;

  auto ToString() -> std::string override {
    return fmt::format("bic{} {}, {}, {}",
      update_host_flags ? "s" : "",
      std::to_string(result),
      std::to_string(lhs),
      std::to_string(rhs));
  }
};

struct IRBitwiseEOR final : IRBinaryOpBase<IROpcodeClass::EOR> {
  using IRBinaryOpBase::IRBinaryOpBase;

  auto ToString() -> std::string override {
    return fmt::format("eor{} {}, {}, {}",
      update_host_flags ? "s" : "",
      std::to_string(result),
      std::to_string(lhs),
      std::to_string(rhs));
  }
};

struct IRSub final : IRBinaryOpBase<IROpcodeClass::SUB> {
  using IRBinaryOpBase::IRBinaryOpBase;

  auto ToString() -> std::string override {
    return fmt::format("sub{} {}, {}, {}",
      update_host_flags ? "s" : "",
      std::to_string(result),
      std::to_string(lhs),
      std::to_string(rhs));
  }
};

struct IRRsb final : IRBinaryOpBase<IROpcodeClass::RSB> {
  using IRBinaryOpBase::IRBinaryOpBase;

  auto ToString() -> std::string override {
    return fmt::format("rsb{} {}, {}, {}",
      update_host_flags ? "s" : "",
      std::to_string(result),
      std::to_string(lhs),
      std::to_string(rhs));
  }
};

struct IRAdd final : IRBinaryOpBase<IROpcodeClass::ADD> {
  using IRBinaryOpBase::IRBinaryOpBase;

  auto ToString() -> std::string override {
    return fmt::format("add{} {}, {}, {}",
      update_host_flags ? "s" : "",
      std::to_string(result),
      std::to_string(lhs),
      std::to_string(rhs));
  }
};

struct IRAdc final : IRBinaryOpBase<IROpcodeClass::ADC> {
  using IRBinaryOpBase::IRBinaryOpBase;

  auto ToString() -> std::string override {
    return fmt::format("adc{} {}, {}, {}",
      update_host_flags ? "s" : "",
      std::to_string(result),
      std::to_string(lhs),
      std::to_string(rhs));
  }
};

struct IRSbc final : IRBinaryOpBase<IROpcodeClass::SBC> {
  using IRBinaryOpBase::IRBinaryOpBase;

  auto ToString() -> std::string override {
    return fmt::format("sbc{} {}, {}, {}",
      update_host_flags ? "s" : "",
      std::to_string(result),
      std::to_string(lhs),
      std::to_string(rhs));
  }
};

struct IRRsc final : IRBinaryOpBase<IROpcodeClass::RSC> {
  using IRBinaryOpBase::IRBinaryOpBase;

  auto ToString() -> std::string override {
    return fmt::format("rsc{} {}, {}, {}",
      update_host_flags ? "s" : "",
      std::to_string(result),
      std::to_string(lhs),
      std::to_string(rhs));
  }
};

struct IRBitwiseORR final : IRBinaryOpBase<IROpcodeClass::ORR> {
  using IRBinaryOpBase::IRBinaryOpBase;

  auto ToString() -> std::string override {
    return fmt::format("orr{} {}, {}, {}",
      update_host_flags ? "s" : "",
      std::to_string(result),
      std::to_string(lhs),
      std::to_string(rhs));
  }
};

struct IRMov final : IROpcodeBase<IROpcodeClass::MOV> {
  IRMov(
    IRVariable const& result,
    IRValue source,
    bool update_host_flags
  ) : result(result), source(source), update_host_flags(update_host_flags) {}

  IRVariable const& result;
  IRValue source;
  bool update_host_flags;

  auto Reads(IRVariable const& var) -> bool override {
    return source.IsVariable() && (&source.GetVar() == &var);
  }

  auto Writes(IRVariable const& var) -> bool override {
    return &result == &var;
  }

  auto ToString() -> std::string override {
    return fmt::format("mov{} {}, {}",
      update_host_flags ? "s" : "",
      std::to_string(result),
      std::to_string(source));
  }
};

struct IRMvn final : IROpcodeBase<IROpcodeClass::MVN> {
  IRMvn(
    IRVariable const& result,
    IRValue source,
    bool update_host_flags
  ) : result(result), source(source), update_host_flags(update_host_flags) {}

  IRVariable const& result;
  IRValue source;
  bool update_host_flags;

  auto Reads(IRVariable const& var) -> bool override {
    return source.IsVariable() && (&source.GetVar() == &var);
  }

  auto Writes(IRVariable const& var) -> bool override {
    return &result == &var;
  }

  auto ToString() -> std::string override {
    return fmt::format("mvn{} {}, {}",
      update_host_flags ? "s" : "",
      std::to_string(result),
      std::to_string(source));
  }
};

struct IRMultiply final : IROpcodeBase<IROpcodeClass::MUL> {
  IRMultiply(
    IRVariable const& result,
    IRVariable const& lhs,
    IRVariable const& rhs
  ) : result(result), lhs(lhs), rhs(rhs) {}

  IRVariable const& result;
  IRVariable const& lhs;
  IRVariable const& rhs;

  auto Reads(IRVariable const& var) -> bool override {
    return &var == &lhs || &var == &rhs;
  }

  auto Writes(IRVariable const& var) -> bool override {
    return &var == &result;
  }

  auto ToString() -> std::string override {
    return fmt::format("mul {}, {}, {}",
      std::to_string(result),
      std::to_string(lhs),
      std::to_string(rhs));
  }
};

// TODO: maybe the IR prefixes everywhere are a bit silly...
// TODO: is there a nicer solution that is scoped but allows combining flags easily?
enum IRMemoryFlags {
  Byte = 1,
  Half = 2,
  Word = 4,
  Rotate = 8,
  Signed = 16,
  ARMv4T = 32
};

constexpr auto operator|(IRMemoryFlags lhs, IRMemoryFlags rhs) -> IRMemoryFlags {
  return static_cast<IRMemoryFlags>(int(lhs) | rhs);
}

struct IRMemoryRead final : IROpcodeBase<IROpcodeClass::MemoryRead> {
  IRMemoryRead(
    IRMemoryFlags flags,
    IRVariable const& result,
    IRVariable const& address
  ) : flags(flags), result(result), address(address) {}

  IRMemoryFlags flags;
  IRVariable const& result;
  IRVariable const& address;

  auto Reads(IRVariable const& var) -> bool override {
    return &address == &var;
  }

  auto Writes(IRVariable const& var) -> bool override {
    return &result == &var;
  }

  auto ToString() -> std::string override {
    auto size = "b";

    if (flags & IRMemoryFlags::Half) size = "h";
    if (flags & IRMemoryFlags::Word) size = "w";

    return fmt::format("ldr.{}{} {}, [{}]",
      size,
      (flags & IRMemoryFlags::Rotate) ? "r" : "",
      std::to_string(result),
      std::to_string(address));
  }
};

// TODO: is there a valid case where we'd like source to be a constant?
struct IRMemoryWrite final : IROpcodeBase<IROpcodeClass::MemoryWrite> {
  IRMemoryWrite(
    IRMemoryFlags flags,
    IRVariable const& source,
    IRVariable const& address
  ) : flags(flags), source(source), address(address) {}

  IRMemoryFlags flags;
  IRVariable const& source;
  IRVariable const& address;

  auto Reads(IRVariable const& var) -> bool override {
    return &address == &var || &source == &var;
  }

  auto Writes(IRVariable const& var) -> bool override {
    return false;
  }

  auto ToString() -> std::string override {
    auto size = "b";

    if (flags & IRMemoryFlags::Half) size = "h";
    if (flags & IRMemoryFlags::Word) size = "w";

    return fmt::format("str.{} {}, [{}]",
      size,
      std::to_string(source),
      std::to_string(address));
  }
};

struct IRFlush final : IROpcodeBase<IROpcodeClass::Flush> {
  IRFlush(
    IRVariable const& address_out,
    IRVariable const& address_in,
    IRVariable const& cpsr_in
  ) : address_out(address_out), address_in(address_in), cpsr_in(cpsr_in) {}

  IRVariable const& address_out;
  IRVariable const& address_in;
  IRVariable const& cpsr_in;

  auto Reads(IRVariable const& var) -> bool override {
    return &var == &address_in || &var == &cpsr_in;
  }

  auto Writes(IRVariable const& var) -> bool override {
    return &var == &address_out;
  }

  auto ToString() -> std::string override {
    return fmt::format("flush {}, {}, {}",
      std::to_string(address_out),
      std::to_string(address_in),
      std::to_string(cpsr_in));
  }
};

struct IRFlushExchange final : IROpcodeBase<IROpcodeClass::FlushExchange> {
  IRFlushExchange(
    IRVariable const& address_out,
    IRVariable const& cpsr_out,
    IRVariable const& address_in,
    IRVariable const& cpsr_in
  ) : address_out(address_out), cpsr_out(cpsr_out), address_in(address_in), cpsr_in(cpsr_in) {}

  IRVariable const& address_out;
  IRVariable const& cpsr_out;
  IRVariable const& address_in;
  IRVariable const& cpsr_in;

  auto Reads(IRVariable const& var) -> bool override {
    return &var == &address_in || &var == &cpsr_in;
  }

  auto Writes(IRVariable const& var) -> bool override {
    return &var == &address_out || &var == &cpsr_out;
  }

  auto ToString() -> std::string override {
    return fmt::format("flushxchg {}, {}, {}, {}",
      std::to_string(address_out),
      std::to_string(cpsr_out),
      std::to_string(address_in),
      std::to_string(cpsr_in));
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
