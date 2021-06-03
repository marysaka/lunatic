/*
 * Copyright (C) 2021 fleroviux
 */

#include <lunatic/jit.hpp>

#include "frontend/state.hpp"
#include "frontend/translator/translator.hpp"
#include "backend/x86_64/backend.hpp"

using namespace lunatic::frontend;
using namespace lunatic::backend;

namespace lunatic {

struct JIT_impl {
  JIT_impl(Memory& memory) : memory(memory) {
    // FIXME: expose emulated CPU state to the user.
    state.GetGPR(Mode::Supervisor, GPR::SP) = 0x03007FE0;
    state.GetGPR(Mode::IRQ, GPR::SP) = 0x03007FA0;
    state.GetGPR(Mode::System, GPR::SP) = 0x03007F00;
    state.GetGPR(Mode::System, GPR::PC) = 0x08000008;
    state.GetCPSR().f.mode = Mode::System;
  }

  bool& IRQLine() { return irq_line; }

  void Run(int cycles) {
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

JIT::JIT(Memory& memory) : pimpl(new JIT_impl(memory)) {}

JIT::~JIT() {
  delete pimpl;
}

bool& JIT::IRQLine() {
  return pimpl->IRQLine();
}

void JIT::Run(int cycles) {
  pimpl->Run(cycles);
}

} // namespace lunatic
