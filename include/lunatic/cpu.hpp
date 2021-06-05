/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/memory.hpp>
#include <memory>

namespace lunatic {

enum class GPR {
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

enum class Mode : uint {
  User = 0x10,
  FIQ = 0x11,
  IRQ = 0x12,
  Supervisor = 0x13,
  Abort = 0x17,
  Undefined = 0x18,
  System = 0x1F
};

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

struct CPU {
  struct Descriptor {
    enum class Model {
      ARM7,
      ARM9
    } model = Model::ARM9;
    int block_size = 32;
    Memory& memory;
  };

  virtual auto IRQLine() -> bool& = 0;
  virtual void Run(int cycles) = 0;

  virtual auto GetGPR(GPR reg) -> u32& = 0;
  virtual auto GetGPR(GPR reg) const -> u32 = 0;
  virtual auto GetGPR(GPR reg, Mode mode) -> u32& = 0;
  virtual auto GetGPR(GPR reg, Mode mode) const -> u32 = 0;
  virtual auto GetCPSR() -> StatusRegister& = 0;
  virtual auto GetCPSR() const -> StatusRegister = 0;
  virtual auto GetSPSR(Mode mode) -> StatusRegister& = 0;
  virtual auto GetSPSR(Mode mode) const -> StatusRegister = 0;
};

auto CreateCPU(CPU::Descriptor const& descriptor) -> std::unique_ptr<CPU>;

} // namespace lunatic
