/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <lunatic/integer.hpp>

#include "frontend/state.hpp"

namespace lunatic {
namespace frontend {

enum class Condition {
  EQ = 0,
  NE = 1,
  CS = 2,
  CC = 3,
  MI = 4,
  PL = 5,
  VS = 6,
  VC = 7,
  HI = 8,
  LS = 9,
  GE = 10,
  LT = 11,
  GT = 12,
  LE = 13,
  AL = 14,
  NV = 15
};

enum class Shift {
  LSL = 0,
  LSR = 1,
  ASR = 2,
  ROR = 3
};

using GPR = State::GPR;
using Mode = State::Mode;

} // namespace lunatic::frontend
} // namespace lunatic