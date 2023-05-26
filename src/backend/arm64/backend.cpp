/*
 * Copyright (C) 2023 marysaka. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <algorithm>
#include <cstdlib>
#include <list>
#include <stdexcept>

#include "backend.hpp"
#include "common/aligned_memory.hpp"
#include "common/bit.hpp"

using namespace oaknut::util;

namespace lunatic {
namespace backend {

ARM64Backend::ARM64Backend(
  CPU::Descriptor const& descriptor,
  State& state,
  BasicBlockCache& block_cache,
  bool const& irq_line
)   : memory(descriptor.memory)
    , state(state)
    , coprocessors(descriptor.coprocessors)
    , block_cache(block_cache)
    , irq_line(irq_line) {
  CreateCodeGenerator();
  EmitCallBlock();
}

ARM64Backend::~ARM64Backend() {
  delete code;
  delete code_memory_block;
}

void ARM64Backend::CreateCodeGenerator() {
  code_memory_block = new memory::CodeBlockMemory(kCodeBufferSize);
#if defined(__APPLE__)
  is_writeable = false;
#else
  is_writeable = true;
#endif
  code = new oaknut::CodeGenerator{static_cast<std::uint32_t*>(code_memory_block->GetPointer())};
}

void ARM64Backend::Push(oaknut::CodeGenerator &code, std::vector<oaknut::XReg> const& regs) {
  const auto RegisterSize = sizeof(uint64_t);

  if (regs.size() > 0) {
    code.SUB(SP, SP, ((regs.size() + 1) / 2) * 2 * RegisterSize);

    for (size_t i = 0; i < regs.size() - 1; i += 2) {
      code.STP(regs[i], regs[i + 1], SP, i * RegisterSize);
    }

    if ((regs.size() % 2) != 0) {
      size_t i = regs.size() - 1;

      code.STR(regs[i], SP, i * RegisterSize);
    }
  }
}

void ARM64Backend::Pop(oaknut::CodeGenerator &code, std::vector<oaknut::XReg> const& regs) {
  const auto RegisterSize = sizeof(uint64_t);

  if (regs.size() > 0) {
    for (size_t i = 0; i < regs.size() - 1; i += 2) {
      code.LDP(regs[i], regs[i + 1], SP,  i * RegisterSize);
    }

    if ((regs.size() % 2) != 0) {
      size_t i = regs.size() - 1;

      code.LDR(regs[i], SP, i * RegisterSize);
    }

    code.ADD(SP, SP, ((regs.size() + 1) / 2) * 2 * RegisterSize);
  }
}

void ARM64Backend::EmitCallBlock() {
  if (!is_writeable) {
    code_memory_block->ProtectForWrite();
    is_writeable = true;
  }

  CallBlock = code->ptr<int (*)(BasicBlock::CompiledFn, int, lunatic::frontend::State *)>();

  Push(*code, CallBlockSavedRegisters);

  auto stack_displacement = 2 * sizeof(u64) + ARM64RegisterAllocator::kSpillAreaSize * sizeof(u32);
  code->SUB(SP, SP, stack_displacement);
  code->MOV(X29, SP);

  code->MOV(X12, X0); // X0 - function pointer
  code->MOV(CycleCounterReg, X1); // X1 = cycle counter
  code->MOV(StatePointerReg, X2); // X2 = pointer to guest state (lunatic::frontend::State)

  // Call function
  code->BLR(X12);

  code->MOV(X0, CycleCounterReg);
  code->ADD(SP, SP, stack_displacement);

  Pop(*code, CallBlockSavedRegisters);
  code->RET();
}

void ARM64Backend::Compile(BasicBlock& basic_block) {
  if (!TryCompile(basic_block)) {
    code->set_ptr(static_cast<std::uint32_t*>(code_memory_block->GetPointer()));
    EmitCallBlock();
    TryCompile(basic_block);
  }
}

bool ARM64Backend::TryCompile(BasicBlock& basic_block) {
  if (!is_writeable) {
    code_memory_block->ProtectForWrite();
    is_writeable = true;
  }

  auto label_return_to_dispatch = oaknut::Label{};

  auto opcode_size = basic_block.key.Thumb() ? sizeof(u16) : sizeof(u32);
  auto number_of_micro_blocks = basic_block.micro_blocks.size();

  basic_block.function = code->ptr<BasicBlock::CompiledFn>();

  for (size_t i = 0; i < number_of_micro_blocks; i++) {
      auto const& micro_block = basic_block.micro_blocks[i];
      auto& emitter  = micro_block.emitter;
      auto condition = micro_block.condition;
      auto reg_alloc = ARM64RegisterAllocator{emitter, *code};
      auto context   = CompileContext{*code, reg_alloc, state};

      auto label_skip = oaknut::Label{};
      auto label_done = oaknut::Label{};

      // Skip past the micro block if its condition is not met
      EmitConditionalBranch(condition, label_skip);

      // Print each IR opcode
      for (auto const& op : emitter.Code()) {
        fmt::print("{}\n", op->ToString());
      }
      fmt::print("\n");

      // Compile each IR opcode inside the micro block
      for (auto const& op : emitter.Code()) {
        CompileIROp(context, op);
        reg_alloc.AdvanceLocation();
      }

      /* Once we reached the end of the basic block,
       * check if we can emit a jump to an already compiled basic block.
       * Also update the cycle counter in that case and return to the dispatcher
       * in the case that we ran out of cycles.
       */
      if (basic_block.enable_fast_dispatch && i == number_of_micro_blocks - 1) {
        // TODO
        fmt::print("Not implemented\n");
        std::abort();
      }

      /* The program counter is normally updated via IR opcodes.
       * But if we skipped past the code which'd do that, we need to manually
       * update the program counter.
       */
      if (condition != Condition::AL) {
        code->B(label_done);

        code->l(label_skip);
        code->ADD(X1, StatePointerReg, state.GetOffsetToGPR(Mode::User, GPR::PC));
        code->LDR(X0, X1);
        code->ADD(X0, X0, micro_block.length * opcode_size);
        code->STR(X0, X1);

        code->l(label_done);
      }
  }

  if (basic_block.enable_fast_dispatch) {
    // TODO
    fmt::print("Not implemented\n");
    std::abort();
  } else {
    code->SUB(CycleCounterReg, CycleCounterReg, basic_block.length);
    code->RET();
  }

  Link(basic_block);

  basic_block.RegisterReleaseCallback([this](BasicBlock const& basic_block) {
    OnBasicBlockToBeDeleted(basic_block);
  });

  // TODO: change CodeGenerator policy to be able to throw on out of bounds and return false here when needed.
  return true;
}

void ARM64Backend::EmitConditionalBranch(Condition condition, oaknut::Label& label_skip) {
  if (condition == Condition::AL) {
    return;
  }

  // Restore status register
  code->ADD(HostFlagsReg, StatePointerReg, state.GetOffsetToCPSR());
  code->LDR(HostFlagsReg, HostFlagsReg);
  code->AND(X0, HostFlagsReg, 0xF0000000);
  code->MSR(oaknut::SystemReg::NZCV, X0);

  oaknut::Cond native_condition;

  switch (condition) {
    case Condition::EQ:
      native_condition = oaknut::Cond::NE;
      break;
    case Condition::NE:
      native_condition = oaknut::Cond::EQ;
      break;
    case Condition::CS:
      native_condition = oaknut::Cond::CC;
      break;
    case Condition::CC:
      native_condition = oaknut::Cond::CS;
      break;
    case Condition::MI:
      native_condition = oaknut::Cond::PL;
      break;
    case Condition::PL:
      native_condition = oaknut::Cond::MI;
      break;
    case Condition::VS:
      native_condition = oaknut::Cond::VC;
      break;
    case Condition::VC:
      native_condition = oaknut::Cond::VS;
      break;
    case Condition::HI:
      native_condition = oaknut::Cond::LS;
      break;
    case Condition::LS:
      native_condition = oaknut::Cond::HI;
      break;
    case Condition::GE:
      native_condition = oaknut::Cond::LT;
      break;
    case Condition::LT:
      native_condition = oaknut::Cond::GE;
      break;
    case Condition::GT:
      native_condition = oaknut::Cond::LE;
      break;
    case Condition::LE:
      native_condition = oaknut::Cond::GT;
      break;
    case Condition::NV:
      native_condition = oaknut::Cond::AL;
      break;
    default:
      fmt::print("Unsupported Condition {}\n", condition);
      std::abort();
  }

  code->B(native_condition, label_skip);
}

void ARM64Backend::CompileIROp(
  CompileContext const& context,
  std::unique_ptr<IROpcode> const& op
) {
  switch (op->GetClass()) {
    case IROpcodeClass::NOP: break;

    // Context access (compile_context.cpp)
    case IROpcodeClass::LoadGPR: CompileLoadGPR(context, lunatic_cast<IRLoadGPR>(op.get())); break;
    case IROpcodeClass::StoreGPR: CompileStoreGPR(context, lunatic_cast<IRStoreGPR>(op.get())); break;
    case IROpcodeClass::LoadSPSR: CompileLoadSPSR(context, lunatic_cast<IRLoadSPSR>(op.get())); break;
    case IROpcodeClass::StoreSPSR: CompileStoreSPSR(context, lunatic_cast<IRStoreSPSR>(op.get())); break;
    case IROpcodeClass::LoadCPSR: CompileLoadCPSR(context, lunatic_cast<IRLoadCPSR>(op.get())); break;
    case IROpcodeClass::StoreCPSR: CompileStoreCPSR(context, lunatic_cast<IRStoreCPSR>(op.get())); break;
    case IROpcodeClass::ClearCarry: CompileClearCarry(context, lunatic_cast<IRClearCarry>(op.get())); break;
    case IROpcodeClass::SetCarry:   CompileSetCarry(context, lunatic_cast<IRSetCarry>(op.get())); break;
    case IROpcodeClass::UpdateFlags: CompileUpdateFlags(context, lunatic_cast<IRUpdateFlags>(op.get())); break;
    case IROpcodeClass::UpdateSticky: CompileUpdateSticky(context, lunatic_cast<IRUpdateSticky>(op.get())); break;
    
    // Barrel shifter (compile_shift.cpp)
    case IROpcodeClass::LSL: CompileLSL(context, lunatic_cast<IRLogicalShiftLeft>(op.get())); break;
    case IROpcodeClass::LSR: CompileLSR(context, lunatic_cast<IRLogicalShiftRight>(op.get())); break;
    case IROpcodeClass::ASR: CompileASR(context, lunatic_cast<IRArithmeticShiftRight>(op.get())); break;
    case IROpcodeClass::ROR: CompileROR(context, lunatic_cast<IRRotateRight>(op.get())); break;
    
    // ALU (compile_alu.cpp)
    case IROpcodeClass::AND: CompileAND(context, lunatic_cast<IRBitwiseAND>(op.get())); break;
    case IROpcodeClass::BIC: CompileBIC(context, lunatic_cast<IRBitwiseBIC>(op.get())); break;
    case IROpcodeClass::EOR: CompileEOR(context, lunatic_cast<IRBitwiseEOR>(op.get())); break;
    case IROpcodeClass::SUB: CompileSUB(context, lunatic_cast<IRSub>(op.get())); break;
    case IROpcodeClass::RSB: CompileRSB(context, lunatic_cast<IRRsb>(op.get())); break;
    case IROpcodeClass::ADD: CompileADD(context, lunatic_cast<IRAdd>(op.get())); break;
    case IROpcodeClass::ADC: CompileADC(context, lunatic_cast<IRAdc>(op.get())); break;
    case IROpcodeClass::SBC: CompileSBC(context, lunatic_cast<IRSbc>(op.get())); break;
    case IROpcodeClass::RSC: CompileRSC(context, lunatic_cast<IRRsc>(op.get())); break;
    case IROpcodeClass::ORR: CompileORR(context, lunatic_cast<IRBitwiseORR>(op.get())); break;
    case IROpcodeClass::MOV: CompileMOV(context, lunatic_cast<IRMov>(op.get())); break;
    case IROpcodeClass::MVN: CompileMVN(context, lunatic_cast<IRMvn>(op.get())); break;
    case IROpcodeClass::CLZ: CompileCLZ(context, lunatic_cast<IRCountLeadingZeros>(op.get())); break;
    case IROpcodeClass::QADD: CompileQADD(context, lunatic_cast<IRSaturatingAdd>(op.get())); break;
    case IROpcodeClass::QSUB: CompileQSUB(context, lunatic_cast<IRSaturatingSub>(op.get())); break;

    // Multiply (and accumulate) (compile_multiply.cpp)
    case IROpcodeClass::MUL: CompileMUL(context, lunatic_cast<IRMultiply>(op.get())); break;
    case IROpcodeClass::ADD64: CompileADD64(context, lunatic_cast<IRAdd64>(op.get())); break;
   
    // Memory read/write (compile_memory.cpp)
    case IROpcodeClass::MemoryRead: CompileMemoryRead(context, lunatic_cast<IRMemoryRead>(op.get())); break;
    case IROpcodeClass::MemoryWrite: CompileMemoryWrite(context, lunatic_cast<IRMemoryWrite>(op.get())); break;
    
    // Pipeline flush (compile_flush.cpp)
    case IROpcodeClass::Flush: CompileFlush(context, lunatic_cast<IRFlush>(op.get())); break;
    case IROpcodeClass::FlushExchange: CompileFlushExchange(context, lunatic_cast<IRFlushExchange>(op.get())); break;

    // Coprocessor access (compile_coprocessor.cpp)
    case IROpcodeClass::MRC: CompileMRC(context, lunatic_cast<IRReadCoprocessorRegister>(op.get())); break;
    case IROpcodeClass::MCR: CompileMCR(context, lunatic_cast<IRWriteCoprocessorRegister>(op.get())); break;

    default: {
      throw std::runtime_error(
        fmt::format("lunatic: unhandled IR opcode: {}", op->ToString())
      );
    }
  }
}

void ARM64Backend::LoadRegister(ARM64Backend::CompileContext const& context, IRVarRef const& result, uintptr offset) {
  auto& [code, reg_alloc, _] = context;

  auto address_reg = reg_alloc.GetTemporaryHostReg().toX();
  auto host_reg = reg_alloc.GetVariableHostReg(result.Get());

  code.ADD(address_reg, StatePointerReg, offset);
  code.LDR(host_reg, address_reg);
}

void ARM64Backend::StoreRegister(ARM64Backend::CompileContext const& context, IRAnyRef const& value, uintptr offset) {
  auto& [code, reg_alloc, _] = context;

  auto address_reg = reg_alloc.GetTemporaryHostReg().toX();

  code.ADD(address_reg, StatePointerReg, offset);

  if (value.IsConstant()) {
    auto value_reg = reg_alloc.GetTemporaryHostReg();

    code.MOV(value_reg, value.GetConst().value);
    code.STR(value_reg, address_reg);
  } else {
    auto host_reg = reg_alloc.GetVariableHostReg(value.GetVar());

    code.STR(host_reg, address_reg);
  }
}

void ARM64Backend::UpdateSystemFlags(CompileContext const& context, oaknut::XReg value_reg, uint32_t mask) {
  auto& [code, reg_alloc, _] = context;

  auto tmp_reg0 = reg_alloc.GetTemporaryHostReg().toX();
  auto tmp_reg1 = reg_alloc.GetTemporaryHostReg().toX();

  code.MRS(tmp_reg0, oaknut::SystemReg::NZCV);
  code.MOV(tmp_reg1, mask);
  code.AND(tmp_reg0, tmp_reg0, tmp_reg1);
  code.ORR(tmp_reg0, tmp_reg0, value_reg);
  code.AND(tmp_reg0, tmp_reg0, 0xF0000000);
  code.MSR(oaknut::SystemReg::NZCV, tmp_reg0);
}


void ARM64Backend::UpdateHostFlagsFromSystem(CompileContext const& context, uint32_t mask) {
  auto& [code, reg_alloc, _] = context;

  auto tmp_reg0 = reg_alloc.GetTemporaryHostReg().toX();
  auto mask_reg = reg_alloc.GetTemporaryHostReg().toX();

  code.MOV(mask_reg, mask);
  code.MRS(tmp_reg0, oaknut::SystemReg::NZCV);
  code.AND(tmp_reg0, tmp_reg0, mask_reg);
  code.MVN(mask_reg, mask_reg);
  code.AND(HostFlagsReg, HostFlagsReg, mask_reg);
  code.ORR(HostFlagsReg, HostFlagsReg, tmp_reg0);

}


void ARM64Backend::EmitFunctionCall(CompileContext const& context, uintptr_t function_ptr, std::vector<oaknut::XReg> const& arguments) {
  assert(arguments.size() < 8);

  auto& [code, reg_alloc, _] = context;

  auto regs_saved = GetUsedHostRegsFromList(reg_alloc, CallerSavedRegisters);

  Push(code, regs_saved);

  // Setup arguments
  for (int i = 0; i < arguments.size(); i++) {
    if (i == arguments[i].index()) {
      continue;
    }

    code.MOV(oaknut::XReg(i), arguments[i]);
  }

  code.MOV(X9, function_ptr);
  code.BLR(X9);

  Pop(code, regs_saved);
}

std::vector<oaknut::XReg> ARM64Backend::GetUsedHostRegsFromList(ARM64RegisterAllocator const& reg_alloc, std::vector<oaknut::XReg> const& regs) {
  auto regs_used = std::vector<oaknut::XReg>{};

  for (auto reg : regs) {
    if (!reg_alloc.IsHostRegFree(reg)) {
      regs_used.push_back(reg);
    }
  }

  return regs_used;
}


void ARM64Backend::Link(BasicBlock& basic_block) {
  auto iterator = block_linking_table.find(basic_block.key);

  if (iterator == block_linking_table.end()) {
    return;
  }

  for (auto linking_block : iterator->second) {
    u8* patch = linking_block->branch_target.patch_location;
    u32 relative_address = (u32)((s64)basic_block.function - (s64)patch);

    auto current_ptr = code->ptr<uint32_t*>();
    code->set_ptr(reinterpret_cast<std::uint32_t*>(patch));
    code->B(relative_address);
    code->set_ptr(current_ptr);

    basic_block.linking_blocks.push_back(linking_block);
  }

  block_linking_table.erase(iterator);
}

void ARM64Backend::OnBasicBlockToBeDeleted(BasicBlock const& basic_block) {
  // TODO: release the allocated JIT buffer memory.

  auto const& branch_target = basic_block.branch_target;

  // Do not leave a dangling pointer to the block in the block linking table or the target block.
  if (!branch_target.key.IsEmpty()) {
    auto iterator = block_linking_table.find(branch_target.key);

    if (iterator != block_linking_table.end()) {
      auto& linking_blocks = iterator->second;

      linking_blocks.erase(std::find(
        linking_blocks.begin(), linking_blocks.end(), &basic_block));
    }

    auto target_block = block_cache.Get(branch_target.key);

    if (target_block) {
      auto& linking_blocks = target_block->linking_blocks;

      auto iterator = std::find(
        linking_blocks.begin(), linking_blocks.end(), &basic_block);

      if (iterator != linking_blocks.end()) {
        linking_blocks.erase(iterator);
      }
    }
  }
}

int ARM64Backend::Call(BasicBlock const& basic_block, int max_cycles) {
  if (is_writeable) {
    code_memory_block->ProtectForExecute();
    code_memory_block->Invalidate();
    is_writeable = false;
  }

  return CallBlock(basic_block.function, max_cycles, &state);
}


std::unique_ptr<Backend> Backend::CreateBackend(CPU::Descriptor const& descriptor,
                                                State& state,
                                                BasicBlockCache& block_cache,
                                                bool const& irq_line) {
  return std::make_unique<ARM64Backend>(descriptor, state, block_cache, irq_line);
}

} // namespace lunatic::backend
} // namespace lunatic
