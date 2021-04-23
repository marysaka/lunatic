/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <lunatic/integer.hpp>
#include <string>

namespace lunatic {
namespace frontend {

/// Stores the state of the emulated ARM core.
struct State {
  State();

  /// Enumeration of ARM general-purpose registers (GPRs).
  enum class GPR : u8 {
    R0 = 0,
    R1 = 1,
    R2 = 2,
    R3 = 3,
    R4 = 4,
    R5 = 5,
    R6 = 6,
    R7 = 7,
    R8 = 8,
    R9 = 9,
    R10 = 10,
    R11 = 11,
    R12 = 12,
    SP = 13,
    LR = 14,
    PC = 15,
  };

  /// Enumeration of ARM processor modes.
  enum class Mode : uint {
    User = 0x10,
    FIQ = 0x11,
    IRQ = 0x12,
    Supervisor = 0x13,
    Abort = 0x17,
    Undefined = 0x18,
    System = 0x1F
  };

  /// Program Status Register, keeps track of ARM processor mode, IRQ masking and flags.
  union StatusRegister {
    struct {
      Mode mode : 5;
      uint thumb : 1;
      uint mask_fiq : 1;
      uint mask_irq : 1;
      uint reserved : 19;
      uint q : 1;
      uint v : 1;
      uint c : 1;
      uint z : 1;
      uint n : 1;
    } f;
    u32 v = static_cast<u32>(Mode::System);
  };

  /// Reset the ARM core.
  void Reset();

  /// \returns for a given processor mode a reference to a general-purpose register.
  auto GetGPR(Mode mode, GPR reg) -> u32& { return *GetPointerToGPR(mode, reg); }

  /// \returns reference to the current program status register (cpsr).
  auto GetCPSR() -> StatusRegister& { return common.cpsr; }

  /// \returns for a given processor mode the pointer to a general-purpose register.
  auto GetPointerToGPR(Mode mode, GPR reg) -> u32*;

  /// \returns for a given processor mode the pointer to the saved program status register (spsr).
  auto GetPointerToSPSR(Mode mode) -> StatusRegister*;

  /// \returns pointer to the current program status register (cpsr).
  auto GetPointerToCPSR() -> StatusRegister*;

private:
  void InitializeLookupTable();

  /// Common registers r0 - r7, r15 and cpsr.
  /// These registers are visible in all ARM processor modes.
  struct {
    u32 reg[8] {0};
    u32 r15 = 0x00000008;
    StatusRegister cpsr = {};
  } common;

  /// Banked registers r8 - r14 and spsr for FIQ and User/System processor modes.
  /// User/System mode r8 - r12 are also used by all other modes except FIQ.
  struct {
    u32 reg[7] {0};
    StatusRegister spsr = {};
  } fiq, sys;

  /// Banked registers r13 - r14 and spsr for IRQ, supervisor,
  /// abort and undefined processor modes.
  struct {
    u32 reg[2] {0};
    StatusRegister spsr = {};
  } irq, svc, abt, und;

  /// Per ARM processor mode address/pointer lookup table for GPRs and SPSRs.
  struct LookupTable {
    u32* gpr[16] {nullptr};
    StatusRegister* spsr {nullptr};
  } table[0x20] = {};
};

} // namespace lunatic::frontend
} // namespace lunatic

namespace std {

inline auto to_string(lunatic::frontend::State::Mode value) -> std::string {
  using Mode = lunatic::frontend::State::Mode;
  switch (value) {
    case Mode::User: return "usr";
    case Mode::FIQ: return "fiq";
    case Mode::IRQ: return "irq";
    case Mode::Supervisor: return "svc";
    case Mode::Abort: return "abt";
    case Mode::Undefined: return "und";
    case Mode::System: return "sys";
  }
}

} // namespace std