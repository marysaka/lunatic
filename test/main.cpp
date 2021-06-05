/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <lunatic/cpu.hpp>
#include <fmt/format.h>
#include <fstream>
#include <cstring>
#include <SDL.h>
#include <unordered_map>

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

  void WriteWord(u32 address, u32 value, Bus bus) override {
    switch (address) {
      case 0x0400'00D4: {
        dma3_src = value;
        break;
      }
      case 0x0400'00D8: {
        dma3_dst = value;
        break;
      }
      case 0x0400'00DC: {
        if (value & 0x8000'0000) {
          auto count = value & 0xFFFF;
          for (int i = 0; i < count; i++) {
            FastWrite<u32, Bus::System>(dma3_dst, FastRead<u32, Bus::System>(dma3_src));
            dma3_dst += sizeof(u32);
            dma3_src += sizeof(u32);
          }
        }
        break;
      }
    }
  }

  u8 pram[0x400];
  u8 vram[0x20000];
  u8 rom[0x1000000];
  u8 ewram[0x40000];
  u8 iwram[0x8000];

  int vblank_flag;
  u16 keyinput = 0xFFFF;
  u32 dma3_src = 0;
  u32 dma3_dst = 0;
};

static Memory g_memory;
static u16 frame[240 * 160];

void render_frame() {
  for (int i = 0; i < 240 * 160; i++) {
    frame[i] = lunatic::read<u16>(&g_memory.pram, g_memory.vram[i] << 1);
  }
}

int main(int argc, char** argv) {
  using namespace lunatic;

  size_t size;
  std::ifstream file { "armwrestler.gba", std::ios::binary };

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

  auto event = SDL_Event{};

  auto jit = CreateCPU(CPU::Descriptor{.memory = g_memory});

  jit->GetGPR(GPR::SP, Mode::Supervisor) = 0x03007FE0;
  jit->GetGPR(GPR::SP, Mode::IRQ) = 0x03007FA0;
  jit->GetCPSR().f.mode = Mode::System;
  jit->GetGPR(GPR::SP) = 0x03007F00;
  jit->GetGPR(GPR::PC) = 0x08000008;

  for (;;) {
    jit->Run(279620);

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
