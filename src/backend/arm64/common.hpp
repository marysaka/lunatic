/*
 * Copyright (C) 2022 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include "backend.hpp"

#define DESTRUCTURE_CONTEXT auto& [code, reg_alloc, state] = context;

#define NOT_IMPLEMENTED_OP(OP) (throw std::runtime_error(fmt::format("lunatic: unhandled IR opcode in {}: {}", __FUNCTION__, OP->ToString())))

using namespace oaknut::util;

namespace lunatic::backend {

inline auto ReadByte(Memory& memory, u32 address, Memory::Bus bus) -> u8 {
  return memory.ReadByte(address, bus);
}

inline auto ReadHalf(Memory& memory, u32 address, Memory::Bus bus) -> u16 {
  return memory.ReadHalf(address, bus);
}

inline auto ReadWord(Memory& memory, u32 address, Memory::Bus bus) -> u32 {
  return memory.ReadWord(address, bus);
}

inline void WriteByte(Memory& memory, u32 address, Memory::Bus bus, u8 value) {
  memory.WriteByte(address, value, bus);
}

inline void WriteHalf(Memory& memory, u32 address, Memory::Bus bus, u16 value) {
  memory.WriteHalf(address, value, bus);
}

inline void WriteWord(Memory& memory, u32 address, Memory::Bus bus, u32 value) {
  memory.WriteWord(address, value, bus);
}

inline auto ReadCoprocessor(Coprocessor* coprocessor, uint opcode1, uint cn, uint cm, uint opcode2) -> u32 {
  return coprocessor->Read(opcode1, cn, cm, opcode2);
}

inline void WriteCoprocessor(Coprocessor* coprocessor, uint opcode1, uint cn, uint cm, uint opcode2, u32 value) {
  coprocessor->Write(opcode1, cn, cm, opcode2, value);
}

} // namespace lunatic::backend
