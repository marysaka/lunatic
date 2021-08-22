/*
 * Copyright (C) 2021 fleroviux
 */

#include <lunatic/cpu.hpp>

#include "frontend/state.hpp"
#include "frontend/translator/translator.hpp"
#include "backend/x86_64/backend.hpp"

using namespace lunatic::frontend;
using namespace lunatic::backend;

namespace lunatic {

struct JIT final : CPU {
  JIT(CPU::Descriptor const& descriptor)
      : exception_base(descriptor.exception_base)
      , memory(descriptor.memory)
      , translator(descriptor)
      , backend(descriptor, state, block_cache, irq_line) {
  }

 ~JIT() override {
    // TODO: release any memory allocated for basic blocks.
  }

  bool& IRQLine() override {
    return irq_line;
  }

  void WaitForIRQ() override {
    wait_for_irq = true;
  }

  auto IsWaitingForIRQ() -> bool override {
    return wait_for_irq;
  }

  void Run(int cycles) override {
    if (IsWaitingForIRQ() && !IRQLine()) {
      return;
    }

    cycles_to_run += cycles;

    while (cycles_to_run > 0) {
      if (IRQLine()) {
        SignalIRQ();
      }

      auto block_key = BasicBlock::Key{state};
      auto basic_block = block_cache.Get(block_key);

      if (basic_block == nullptr) {
        basic_block = new BasicBlock{block_key};

        translator.Translate(*basic_block);
        for (auto &micro_block : basic_block->micro_blocks) {
          micro_block.emitter.Optimize();
        }
        backend.Compile(*basic_block);
        block_cache.Set(block_key, basic_block);
      }

      cycles_to_run = backend.Call(*basic_block, cycles_to_run);

      if (IsWaitingForIRQ()) {
        cycles_to_run = 0;
        return;
      }
    }
  }

  auto GetGPR(GPR reg) -> u32& override {
    return GetGPR(reg, GetCPSR().f.mode);
  }

  auto GetGPR(GPR reg) const -> u32 override {
    return const_cast<JIT*>(this)->GetGPR(reg);
  }

  auto GetGPR(GPR reg, Mode mode) -> u32& override {
    return state.GetGPR(mode, reg);
  }

  auto GetGPR(GPR reg, Mode mode) const -> u32 override {
    return const_cast<JIT*>(this)->GetGPR(reg, mode);
  }

  auto GetCPSR() -> StatusRegister& override {
    return state.GetCPSR();
  }

  auto GetCPSR() const -> StatusRegister override {
    return const_cast<JIT*>(this)->GetCPSR();
  }

  auto GetSPSR(Mode mode) -> StatusRegister& override {
    return *state.GetPointerToSPSR(mode);
  }

  auto GetSPSR(Mode mode) const -> StatusRegister override {
    return const_cast<JIT*>(this)->GetSPSR(mode);
  }

private:
  void SignalIRQ() {
    auto& cpsr = GetCPSR();

    wait_for_irq = false;

    if (!cpsr.f.mask_irq) {
      GetSPSR(Mode::IRQ) = cpsr;

      cpsr.f.mode = Mode::IRQ;
      cpsr.f.mask_irq = 1;
      if (cpsr.f.thumb) {
        GetGPR(GPR::LR) = GetGPR(GPR::PC);
      } else {
        GetGPR(GPR::LR) = GetGPR(GPR::PC) - 4;
      }
      cpsr.f.thumb = 0;

      GetGPR(GPR::PC) = exception_base + 0x18 + sizeof(u32) * 2;
    }
  }

  bool irq_line = false;
  bool wait_for_irq = false;
  int cycles_to_run = 0;
  u32 exception_base;
  Memory& memory;
  State state;
  Translator translator;
  BasicBlockCache block_cache;
  X64Backend backend;
};

auto CreateCPU(CPU::Descriptor const& descriptor) -> std::unique_ptr<CPU> {
  return std::make_unique<JIT>(descriptor);
}

} // namespace lunatic
