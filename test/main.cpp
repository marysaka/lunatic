/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <lunatic/integer.hpp>
#include <lunatic/memory.hpp>
#include <arm/arm.hpp>
#include <fmt/format.h>
#include <fstream>
#include <cstring>
#include <SDL.h>
#include <unordered_map>

#include "backend/x86_64/backend.hpp"
#include "frontend/translator/translator.hpp"
#include "frontend/state.hpp"

using namespace lunatic::test;

struct JIT {
  using GPR = lunatic::frontend::State::GPR;
  using Mode = lunatic::frontend::State::Mode;

  JIT(lunatic::Memory& memory) : memory(memory) {
  }

  auto Run() -> int {
    using namespace lunatic::backend;
    using namespace lunatic::frontend;

    auto block_key = BasicBlock::Key{state};
    auto match = block_cache.find(block_key.value);

    auto address = block_key.field.address;
    auto mode = block_key.field.mode;
    //fmt::print("address=0x{:08X} mode=0x{:02X}\n", address, mode);

    if (match != block_cache.end()) {
      auto basic_block = match->second;

      basic_block->function();
      return basic_block->cycle_count;
    } else {
      // TODO: because BasícBlock is not copyable right now
      // we use dynamic allocation, but that's probably not optimal.
      auto basic_block = new BasicBlock{block_key};
      auto success = translator.Translate(*basic_block, memory);

      basic_block->emitter.Optimize();

      if (success) {
        backend.Compile(memory, state, *basic_block);
        block_cache[block_key.value] = basic_block;
        basic_block->function();
        return basic_block->cycle_count;
      } else {
        delete basic_block;
      }

      return -1;
    }
  }

  void SaveState(lunatic::test::arm::State& other_state) {
    // TODO: take care of other CPU modes!
    other_state.cpsr.v = state.GetCPSR().v;

    for (uint i = 0; i < 16; i++) {
      other_state.reg[i] = state.GetGPR(state.GetCPSR().f.mode, static_cast<GPR>(i));
    }
  }

  void LoadState(lunatic::test::arm::State const& other_state) {
    // TODO: take care of other CPU modes!
    state.GetCPSR().v = other_state.cpsr.v;

    for (uint i = 0; i < 16; i++) {
      state.GetGPR(state.GetCPSR().f.mode, static_cast<GPR>(i)) = other_state.reg[i];
    }
  }

  lunatic::backend::X64Backend backend;
  lunatic::frontend::State state;
  lunatic::Memory& memory;
  lunatic::frontend::Translator translator;

  // TODO: use a better data structure for block lookup.
  std::unordered_map<u64, BasicBlock*> block_cache;
};

struct Memory final : lunatic::Memory {
  Memory() {
    std::memset(pram, 0, sizeof(pram));
    std::memset(vram, 0, sizeof(vram));
    std::memset(rom, 0, sizeof(rom));
    std::memset(ewram, 0, sizeof(ewram));
    std::memset(iwram, 0, sizeof(iwram));

    pagetable = std::make_unique<std::array<u8*, 1048576>>();

    for (u64 address = 0; address < 0x100000000; address += 4096) {
      auto page = address >> 12;
      auto& entry = (*pagetable)[page];

      switch (address >> 24) {
        case 0x02:
          entry = &ewram[address & 0x3FFFF];
          break;
        case 0x03:
          entry = &iwram[address & 0x7FFF];
          break;
        case 0x05:
          entry = &pram[address & 0x3FF];
          break;
        case 0x06:
          entry = &vram[address & 0x1FFFF];
          break;
        case 0x08:
          entry = &rom[address & 0xFFFFFF];
          break;
      }
    }

    vblank_flag = 0;
  }

  auto ReadByte(u32 address, Bus bus) -> u8 override {
    fmt::print("Memory: invalid read8 @ 0x{:08X}\n", address);
    return 0;
  }

  auto ReadHalf(u32 address, Bus bus) -> u16 override {
    if (address == 0x04000004) {
      vblank_flag ^= 1;
      return vblank_flag;
    }

    if (address == 0x04000130) {
      return keyinput;
    }

    fmt::print("Memory: invalid read16 @ 0x{:08X}\n", address);
    return 0;
  }

  auto ReadWord(u32 address, Bus bus) -> u32 override {
    if (address == 0x04000004) {
      vblank_flag ^= 1;
      return vblank_flag;
    }

    fmt::print("Memory: invalid read32 @ 0x{:08X}\n", address);
    return 0;
  }

  void WriteByte(u32 address, u8  value, Bus bus) override {}
  void WriteHalf(u32 address, u16 value, Bus bus) override {}
  void WriteWord(u32 address, u32 value, Bus bus) override {}

  u8 pram[0x400];
  u8 vram[0x20000];
  u8 rom[0x1000000];
  u8 ewram[0x40000];
  u8 iwram[0x8000];

  int vblank_flag;
  u16 keyinput = 0xFFFF;
};

static Memory g_memory;
static arm::ARM g_cpu_interp { arm::ARM::Architecture::ARMv5TE, &g_memory };
static JIT g_cpu_jit { g_memory };
static u16 frame[240 * 160];

void render_frame() {
  for (int i = 0; i < 240 * 160; i++) {
    frame[i] = lunatic::read<u16>(&g_memory.pram, g_memory.vram[i] << 1);
  }
}

int main(int argc, char** argv) {
  size_t size;
  std::ifstream file { "arm.gba", std::ios::binary };

  if (!file.good()) {
    fmt::print("Failed to open armwrestler.gba.\n");
    return -1;
  }

  file.seekg(0, std::ios::end);
  size = file.tellg();
  file.seekg(0);

  if (size > 0x1000000) {
    fmt::print("armwrestler.gba is too big. may not be larger than 16 MiB.\n");
    return -2;
  }

  file.read((char*)&g_memory.rom, size);
  if (!file.good()) {
    fmt::print("Failed to read the ROM into memory.\n");
    return -3;
  }

  SDL_Init(SDL_INIT_VIDEO);

  auto window = SDL_CreateWindow(
    "Project: lunatic",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    480,
    320,
    SDL_WINDOW_ALLOW_HIGHDPI);

  auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
  auto texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR1555, SDL_TEXTUREACCESS_STREAMING, 240, 160);

  auto& state = g_cpu_interp.GetState();
  g_cpu_interp.Reset();
  g_cpu_interp.SwitchMode(arm::MODE_SYS);
  state.bank[arm::BANK_SVC][arm::BANK_R13] = 0x03007FE0;
  state.bank[arm::BANK_IRQ][arm::BANK_R13] = 0x03007FA0;
  state.r13 = 0x03007F00;
  g_cpu_interp.SetPC(0x08000000);

  // Derp, just initialize the JIT state instead...
  g_cpu_jit.LoadState(state);

  auto event = SDL_Event{};

  for (;;) {
    for (uint i = 0; i < 279620; i++) {
      auto jit_cycles = g_cpu_jit.Run();
      if (jit_cycles == -1) {
        g_cpu_jit.SaveState(state);

        // TODO: just remove pipeline emulation from the interpreter instead.
        g_cpu_interp.SetPC(state.r15 - (state.cpsr.f.thumb ? 4 : 8));
        g_cpu_interp.Run(1);

        g_cpu_jit.LoadState(state);
      } else {
        i += jit_cycles - 1;
      }
    }
    // g_cpu_interp.Run(279620);

    render_frame();

    SDL_UpdateTexture(texture, nullptr, frame, sizeof(u16) * 240);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
        goto done;

      if (event.type == SDL_KEYUP || event.type == SDL_KEYDOWN) {
        auto bit  = -1;
        bool down = event.type == SDL_KEYDOWN;

        switch (reinterpret_cast<SDL_KeyboardEvent*>(&event)->keysym.sym) {
          case SDLK_a: bit = 0; break;
          case SDLK_s: bit = 1; break;
          case SDLK_BACKSPACE: bit = 2; break;
          case SDLK_RETURN: bit = 3; break;
          case SDLK_RIGHT: bit = 4; break;
          case SDLK_LEFT: bit = 5; break;
          case SDLK_UP: bit = 6; break;
          case SDLK_DOWN: bit = 7; break;
          case SDLK_f: bit = 8; break;
          case SDLK_d: bit = 9; break;
        }

        if (bit != -1) {
          if (down) {
            g_memory.keyinput &= ~(1 << bit);
          } else {
            g_memory.keyinput |=  (1 << bit);
          }
        }
      }
    }
  }

done:
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
  return 0;
}
