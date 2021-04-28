/*
 * Copyright (C) 2020 fleroviux
 */

#include "basic_block.hpp"

namespace lunatic {
namespace frontend {

BasicBlock::Key::Key(State& state) {
  auto const& cpsr = state.GetCPSR();
  field.mode = cpsr.f.mode;
  field.address = state.GetGPR(field.mode, State::GPR::PC);
  if (cpsr.f.thumb) {
    field.address = ((field.address & ~1) - 4) | 1;
  } else {
    field.address = (field.address & ~3) - 8;
  }
}

} // namespace lunatic::frontend
} // namespace lunatic
