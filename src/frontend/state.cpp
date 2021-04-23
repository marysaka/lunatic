/*
 * Copyright (C) 2020 fleroviux
 */

#include <stdexcept>

#include "state.hpp"

namespace lunatic {
namespace frontend {

State::State() {
  InitializeLookupTable();
  Reset();
}

void State::Reset() {
  common = {};
  fiq = {};
  sys = {};
  irq = {};
  svc = {};
  abt = {};
  und = {};
}

auto State::GetPointerToGPR(Mode mode, GPR reg) -> u32* {
  auto id = static_cast<int>(reg);
  if (id > 15) {
    throw std::runtime_error("GetPointerToGPR: requirement not met: 'id' must be <= 15.");
  }
  auto pointer = table[static_cast<int>(mode)].gpr[id];
  if (pointer == nullptr) {
    throw std::runtime_error(
      "GetPointerToGPR: requirement not met: 'mode' must be valid ARM processor mode.");
  }
  return pointer;
}

auto State::GetPointerToSPSR(Mode mode) -> StatusRegister* {
  auto pointer = table[static_cast<int>(mode)].spsr;
  if (pointer == nullptr) {
    throw std::runtime_error(
      "GetPointerToSPSR: requirement not met: 'mode' must be valid ARM processor mode "
      "and may not be System or User mode.");
  }
  return pointer;
}

auto State::GetPointerToCPSR() -> StatusRegister* {
  return &common.cpsr;
}

void State::InitializeLookupTable() {
  Mode modes[] = {
    Mode::User,
    Mode::FIQ,
    Mode::IRQ,
    Mode::Supervisor,
    Mode::Abort,
    Mode::Undefined,
    Mode::System
  };

  for (auto mode : modes) {
    for (int i = 0; i <= 7; i++)
      table[static_cast<int>(mode)].gpr[i] = &common.reg[i];
    auto source = mode == Mode::FIQ ? fiq.reg : sys.reg;
    for (int i = 8; i <= 12; i++)
      table[static_cast<int>(mode)].gpr[i] = &source[i - 8];
    table[static_cast<int>(mode)].gpr[15] = &common.r15;
  }

  for (int i = 0; i < 2; i++) {
    table[static_cast<int>(Mode::User)].gpr[13 + i] = &sys.reg[6 + i];
    table[static_cast<int>(Mode::FIQ)].gpr[13 + i] = &fiq.reg[6 + i];
    table[static_cast<int>(Mode::IRQ)].gpr[13 + i] = &irq.reg[i];
    table[static_cast<int>(Mode::Supervisor)].gpr[13 + i] = &svc.reg[i];
    table[static_cast<int>(Mode::Abort)].gpr[13 + i] = &abt.reg[i];
    table[static_cast<int>(Mode::Undefined)].gpr[13 + i] = &und.reg[i];
    table[static_cast<int>(Mode::System)].gpr[13 + i] = &sys.reg[6 + i];
  }

  table[static_cast<int>(Mode::User)].spsr = nullptr;
  table[static_cast<int>(Mode::FIQ)].spsr = &fiq.spsr;
  table[static_cast<int>(Mode::IRQ)].spsr = &irq.spsr;
  table[static_cast<int>(Mode::Supervisor)].spsr = &svc.spsr;
  table[static_cast<int>(Mode::Abort)].spsr = &abt.spsr;
  table[static_cast<int>(Mode::Undefined)].spsr = &und.spsr;
  table[static_cast<int>(Mode::System)].spsr = nullptr;
}

} // namespace lunatic::frontend
} // namespace lunatic