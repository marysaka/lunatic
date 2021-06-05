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
      auto match = block_cache.find(block_key.value);

      if (match != block_cache.end()) {
        auto basic_block = match->second;
        basic_block->function();
        cycles_to_run -= basic_block->length;
      } else {
        // TODO: because BasícBlock is not copyable right now
        // we use dynamic allocation, but that's probably not optimal.
        auto basic_block = new BasicBlock{block_key};

        translator.Translate(*basic_block, memory);

        if (basic_block->length > 0) {
          for (auto &micro_block : basic_block->micro_blocks) {
            micro_block.emitter.Optimize();
          }
          backend.Compile(memory, state, *basic_block);
          block_cache[block_key.value] = basic_block;
          basic_block->function();
          cycles_to_run -= basic_block->length;
        } else {
          auto address = block_key.field.address & ~1;
          auto thumb = state.GetCPSR().f.thumb;
          throw std::runtime_error(
            fmt::format("lunatic: unknown opcode @ {:08X} (thumb = {})", address, thumb)
          );
        }
      }
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
    auto& cpsr = state.GetCPSR();

    if (!cpsr.f.mask_irq) {
      *state.GetPointerToSPSR(Mode::IRQ) = cpsr;

      cpsr.f.mode = Mode::IRQ;
      cpsr.f.mask_irq = 1;
      if (cpsr.f.thumb) {
        state.GetGPR(Mode::IRQ, GPR::LR) = state.GetGPR(Mode::IRQ, GPR::PC);
      } else {
        state.GetGPR(Mode::IRQ, GPR::LR) = state.GetGPR(Mode::IRQ, GPR::PC) - 4;
      }
      cpsr.f.thumb = 0;

      state.GetGPR(Mode::IRQ, GPR::PC) = 0x18 + sizeof(u32) * 2;
    }
  }

  bool irq_line = false;
  int cycles_to_run = 0;
  Memory& memory;
  State state;
  Translator translator;
  X64Backend backend;
  std::unordered_map<u64, BasicBlock*> block_cache;
};

auto CreateCPU(CPU::Descriptor const& descriptor) -> std::unique_ptr<CPU> {
  return std::make_unique<JIT>(descriptor.memory);
}

} // namespace lunatic
