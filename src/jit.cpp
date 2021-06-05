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
  JIT(Memory& memory) : memory(memory) {
  }

  bool& IRQLine() override { return irq_line; }

  void Run(int cycles) override {
    cycles_to_run += cycles;

    while (cycles_to_run > 0) {
      if (IRQLine()) {
        SignalIRQ();
      }

      auto block_key = BasicBlock::Key{state};
      auto basic_block = block_cache.Get(block_key);

      if (basic_block == nullptr) {
        // TODO: because BasícBlock is not copyable right now
        // we use dynamic allocation, but that's probably not optimal.
        basic_block = new BasicBlock{block_key};

        translator.Translate(*basic_block, memory);

        if (basic_block->length > 0) {
          for (auto &micro_block : basic_block->micro_blocks) {
            micro_block.emitter.Optimize();
          }
          backend.Compile(memory, state, *basic_block, block_cache);
          block_cache.Set(block_key, basic_block);
        } else {
          auto address = block_key.field.address << 1;
          auto thumb = state.GetCPSR().f.thumb;
          throw std::runtime_error(
            fmt::format("lunatic: unknown opcode @ R15={:08X} (thumb={})", address, thumb)
          );
        }
      }

      cycles_to_run = backend.Call(*basic_block, cycles_to_run);
    }
  }

  auto GetGPR(GPR reg) -> u32& override {
    return GetGPR(reg, GetCPSR().f.mode);
  }

  auto GetGPR(GPR reg) const -> u32 override {
    return GetGPR(reg);
  }

  auto GetGPR(GPR reg, Mode mode) -> u32& override {
    return state.GetGPR(mode, reg);
  }

  auto GetGPR(GPR reg, Mode mode) const -> u32 override {
    return GetGPR(reg, mode);
  }

  auto GetCPSR() -> StatusRegister& {
    return state.GetCPSR();
  }

  auto GetCPSR() const -> StatusRegister {
    return GetCPSR();
  }

  auto GetSPSR(Mode mode) -> StatusRegister& {
    return *state.GetPointerToSPSR(mode);
  }

  auto GetSPSR(Mode mode) const -> StatusRegister {
    return GetSPSR(mode);
  }

private:
  void SignalIRQ() {
    auto& cpsr = GetCPSR();

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

      GetGPR(GPR::PC) = 0x18 + sizeof(u32) * 2;
    }
  }

  bool irq_line = false;
  int cycles_to_run = 0;
  Memory& memory;
  State state;
  Translator translator;
  X64Backend backend;
  BasicBlockCache block_cache;
};

auto CreateCPU(CPU::Descriptor const& descriptor) -> std::unique_ptr<CPU> {
  return std::make_unique<JIT>(descriptor.memory);
}

} // namespace lunatic
