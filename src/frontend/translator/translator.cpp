/*
 * Copyright (C) 2021 fleroviux
 */

#include "translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::translate(BasicBlock& block, Memory& memory) -> bool {
  auto address = block.key.field.address;

  if (address & 1) {
    // Thumb mode is not supported right now.
    return false;
  }

  mode = block.key.field.mode;
  emitter = &block.emitter;

  auto instruction = memory.FastRead<u32, Memory::Bus::Code>(address);

  return decode_arm(instruction, *this);
}

auto Translator::undefined(u32 opcode) -> bool {
  return false;
}

} // namespace lunatic::frontend
} // namespace lunatic
