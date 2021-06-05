/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/memory.hpp>
#include <fmt/format.h>
#include <vector>

#include "backend/backend.hpp"
#include "frontend/basic_block_cache.hpp"
#include "frontend/state.hpp"
#include "register_allocator.hpp"

// FIXME
using namespace lunatic::frontend;

namespace lunatic {
namespace backend {

struct X64Backend : Backend {
  X64Backend(
    Memory& memory,
    State& state,
    BasicBlockCache const& block_cache,
    bool const& irq_line
  );

  void Compile(BasicBlock& basic_block);

  auto Call(BasicBlock const& basic_block, int max_cycles) -> int {
    return CallBlock(basic_block.function, max_cycles);
  }

private:
  struct CompileContext {
    Xbyak::CodeGenerator& code;
    X64RegisterAllocator& reg_alloc;
    State& state;
    int& location;
  };

  void EmitCallBlock();
  void CompileIROp(
    CompileContext const& context,
    std::unique_ptr<IROpcode> const& op
  );

  void BuildConditionTable();

  void Push(
    Xbyak::CodeGenerator& code,
    std::vector<Xbyak::Reg64> const& regs
  );

  void Pop(
    Xbyak::CodeGenerator& code,
    std::vector<Xbyak::Reg64> const& regs
  );

  void CompileLoadGPR(CompileContext const& context, IRLoadGPR* op);
  void CompileStoreGPR(CompileContext const& context, IRStoreGPR* op);
  void CompileLoadSPSR(CompileContext const& context, IRLoadSPSR* op);
  void CompileStoreSPSR(CompileContext const& context, IRStoreSPSR* op);
  void CompileLoadCPSR(CompileContext const& context, IRLoadCPSR* op);
  void CompileStoreCPSR(CompileContext const& context, IRStoreCPSR* op);
  void CompileClearCarry(CompileContext const& context, IRClearCarry* op);
  void CompileSetCarry(CompileContext const& context, IRSetCarry* op);
  void CompileUpdateFlags(CompileContext const& context, IRUpdateFlags* op);
  void CompileLSL(CompileContext const& context, IRLogicalShiftLeft* op);
  void CompileLSR(CompileContext const& context, IRLogicalShiftRight* op);
  void CompileASR(CompileContext const& context, IRArithmeticShiftRight* op);
  void CompileROR(CompileContext const& context, IRRotateRight* op);
  void CompileAND(CompileContext const& context, IRBitwiseAND* op);
  void CompileBIC(CompileContext const& context, IRBitwiseBIC* op);
  void CompileEOR(CompileContext const& context, IRBitwiseEOR* op);
  void CompileSUB(CompileContext const& context, IRSub* op);
  void CompileRSB(CompileContext const& context, IRRsb* op);
  void CompileADD(CompileContext const& context, IRAdd* op);
  void CompileADC(CompileContext const& context, IRAdc* op);
  void CompileSBC(CompileContext const& context, IRSbc* op);
  void CompileRSC(CompileContext const& context, IRRsc* op);
  void CompileORR(CompileContext const& context, IRBitwiseORR* op);
  void CompileMOV(CompileContext const& context, IRMov* op);
  void CompileMVN(CompileContext const& context, IRMvn* op);
  void CompileMUL(CompileContext const& context, IRMultiply* op);
  void CompileADD64(CompileContext const& context, IRAdd64* op);
  void CompileMemoryRead(CompileContext const& context, IRMemoryRead* op);
  void CompileMemoryWrite(CompileContext const& context, IRMemoryWrite* op);
  void CompileFlush(CompileContext const& context, IRFlush* op);
  void CompileFlushExchange(CompileContext const& context, IRFlushExchange* op);

  Memory& memory;
  State& state;
  BasicBlockCache const& block_cache;
  bool const& irq_line;
  bool condition_table[16][16];
  Xbyak::CodeGenerator code;
  int (*CallBlock)(BasicBlock::CompiledFn, int);

  // TODO: get rid of the thunks eventually.

  auto ReadByte(u32 address, Memory::Bus bus) -> u8 {
    return memory.ReadByte(address, bus);
  }

  auto ReadHalf(u32 address, Memory::Bus bus) -> u16 {
    return memory.ReadHalf(address, bus);
  }

  auto ReadWord(u32 address, Memory::Bus bus) -> u32 {
    return memory.ReadWord(address, bus);
  }

  void WriteByte(u32 address, Memory::Bus bus, u8 value) {
    memory.WriteByte(address, value, bus);
  }

  void WriteHalf(u32 address, Memory::Bus bus, u16 value) {
    memory.WriteHalf(address, value, bus);
  }

  void WriteWord(u32 address, Memory::Bus bus, u32 value) {
    memory.WriteWord(address, value, bus);
  }
};

} // namespace lunatic::backend
} // namespace lunatic
