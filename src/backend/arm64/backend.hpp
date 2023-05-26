/*
 * Copyright (C) 2023 marysaka. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/cpu.hpp>
#include <fmt/format.h>
#include <unordered_map>
#include <vector>

#include <oaknut/oaknut.hpp>

#include "backend/backend.hpp"
#include "common/aligned_memory.hpp"
#include "frontend/basic_block_cache.hpp"
#include "frontend/state.hpp"
#include "register_allocator.hpp"

using namespace lunatic::frontend;
using namespace oaknut::util;

namespace lunatic {
namespace backend {

struct ARM64Backend : Backend {
  ARM64Backend(
    CPU::Descriptor const& descriptor,
    State& state,
    BasicBlockCache& block_cache,
    bool const& irq_line
  );

 ~ARM64Backend();

  struct CompileContext {
    oaknut::CodeGenerator& code;
    ARM64RegisterAllocator& reg_alloc;
    State& state;
  };

  void Compile(BasicBlock& basic_block) override;
  int Call(frontend::BasicBlock const& basic_block, int max_cycles) override;
  static void UpdateSystemFlags(CompileContext const& context, oaknut::XReg value_reg, uint32_t mask);
  static void UpdateHostFlagsFromSystem(CompileContext const& context, uint32_t mask);

private:
  static constexpr size_t kCodeBufferSize = 32 * 1024 * 1024;
  static constexpr oaknut::XReg HostFlagsReg = X13;
  static constexpr oaknut::XReg CycleCounterReg = X14;
  static constexpr oaknut::XReg StatePointerReg = X15;
  static constexpr auto CallBlockSavedRegisters = { X18, X19, X20, X21, X22, X23, X24, X25, X26, X27, X28, X29, X30 };
  static constexpr auto CallerSavedRegisters = { HostFlagsReg, CycleCounterReg, StatePointerReg, X18, X19, X20, X21, X22, X23, X24, X25, X26, X27, X28, X29, X30 };

  void CreateCodeGenerator();
  void EmitCallBlock();
  bool TryCompile(BasicBlock& basic_block);
  void EmitConditionalBranch(Condition condition, oaknut::Label& label_skip);
  void EmitBasicBlockDispatch(oaknut::Label& label_cache_miss);

  void Link(BasicBlock& basic_block);

  void OnBasicBlockToBeDeleted(BasicBlock const& basic_block);

  void CompileIROp(
    CompileContext const& context,
    std::unique_ptr<IROpcode> const& op
  );

  void Push(oaknut::CodeGenerator &code, std::vector<oaknut::XReg> const& regs);
  void Pop(oaknut::CodeGenerator &code, std::vector<oaknut::XReg> const& regs);

  void CompileLoadGPR(CompileContext const& context, IRLoadGPR* op);
  void CompileStoreGPR(CompileContext const& context, IRStoreGPR* op);
  void CompileLoadSPSR(CompileContext const& context, IRLoadSPSR* op);
  void CompileStoreSPSR(CompileContext const& context, IRStoreSPSR* op);
  void CompileLoadCPSR(CompileContext const& context, IRLoadCPSR* op);
  void CompileStoreCPSR(CompileContext const& context, IRStoreCPSR* op);
  void CompileClearCarry(CompileContext const& context, IRClearCarry* op);
  void CompileSetCarry(CompileContext const& context, IRSetCarry* op);
  void CompileUpdateFlags(CompileContext const& context, IRUpdateFlags* op);
  void CompileUpdateSticky(CompileContext const& context, IRUpdateSticky* op);
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
  void CompileCLZ(CompileContext const& context, IRCountLeadingZeros* op);
  void CompileQADD(CompileContext const& context, IRSaturatingAdd* op);
  void CompileQSUB(CompileContext const& context, IRSaturatingSub* op);
  void CompileMUL(CompileContext const& context, IRMultiply* op);
  void CompileADD64(CompileContext const& context, IRAdd64* op);
  void CompileMemoryRead(CompileContext const& context, IRMemoryRead* op);
  void CompileMemoryWrite(CompileContext const& context, IRMemoryWrite* op);
  void CompileFlush(CompileContext const& context, IRFlush* op);
  void CompileFlushExchange(CompileContext const& context, IRFlushExchange* op);
  void CompileMRC(CompileContext const& context, IRReadCoprocessorRegister* op);
  void CompileMCR(CompileContext const& context, IRWriteCoprocessorRegister* op);

  void LoadRegister(CompileContext const& context, IRVarRef const& result, uintptr offset);
  void StoreRegister(CompileContext const& context, IRAnyRef const& value, uintptr offset);
  void EmitFunctionCall(CompileContext const& context, uintptr_t function_ptr, std::vector<oaknut::XReg> const& arguments);

  void HandleTCMRead(CompileContext const& context, oaknut::WReg const& address_reg, oaknut::WReg const& result_reg, IRMemoryFlags flags, oaknut::XReg const& tcm_address_reg, oaknut::WReg const& tcm_scratch_reg, Memory::TCM const& tcm, oaknut::Label & label_final);
  void HandleTCMWrite(CompileContext const& context, oaknut::WReg const& address_reg, oaknut::WReg const& source_reg, IRMemoryFlags flags, oaknut::XReg const& tcm_address_reg, oaknut::WReg const& tcm_scratch_reg, Memory::TCM const& tcm, oaknut::Label & label_final);


  std::vector<oaknut::XReg> GetUsedHostRegsFromList(
    ARM64RegisterAllocator const& reg_alloc,
    std::vector<oaknut::XReg> const& regs
  );

  Memory& memory;
  State& state;
  std::array<Coprocessor*, 16> coprocessors;
  BasicBlockCache& block_cache;
  bool const& irq_line;
  int (*CallBlock)(BasicBlock::CompiledFn, int, lunatic::frontend::State *);

  memory::CodeBlockMemory *code_memory_block;
  bool is_writeable;
  oaknut::CodeGenerator* code;

  std::unordered_map<BasicBlock::Key, std::vector<BasicBlock*>> block_linking_table;
};

} // namespace lunatic::backend
} // namespace lunatic
