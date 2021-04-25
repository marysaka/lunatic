/*
 * Copyright (C) 2021 fleroviux
 */

#include <lunatic/integer.hpp>
#include <arm/arm.hpp>
#include <fmt/format.h>
#include <fstream>
#include <cstring>
#include <SDL.h>

// This is just a test
#include "backend/x86_64/backend.hpp"
#include "frontend/ir/emitter.hpp"

void ir_test() {
  using namespace lunatic::backend;
  using namespace lunatic::frontend;

  using GPR = State::GPR;
  using Mode = State::Mode;

  IREmitter code;

  auto& var0 = code.CreateVar(IRDataType::UInt32);
  auto& var1 = code.CreateVar(IRDataType::UInt32);
  auto& var2 = code.CreateVar(IRDataType::UInt32);

  code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, IRConstant{u32(0xAABBCCDD)});
  code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, var0);
  code.StoreGPR(IRGuestReg{GPR::R2, Mode::User}, IRConstant{u32(0x11223344)});
  code.LoadGPR(IRGuestReg{GPR::R2, Mode::User}, var1);
  code.StoreGPR(IRGuestReg{GPR::R1, Mode::User}, var0);
  code.StoreGPR(IRGuestReg{GPR::R4, Mode::User}, IRConstant{u32(0xC0DEC0DE)});
  code.StoreGPR(IRGuestReg{GPR::R3, Mode::User}, var1);
  code.LoadGPR(IRGuestReg{GPR::R4, Mode::User}, var2);
  code.StoreGPR(IRGuestReg{GPR::R5, Mode::User}, var2);

  // Make the register allocator use a callee saved register
  // auto& var3 = code.CreateVar(IRDataType::UInt32);
  // auto& var4 = code.CreateVar(IRDataType::UInt32);
  // auto& var5 = code.CreateVar(IRDataType::UInt32);
  // code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, var3);
  // code.LoadGPR(IRGuestReg{GPR::R2, Mode::User}, var4);
  // code.LoadGPR(IRGuestReg{GPR::R4, Mode::User}, var5);
  // code.StoreGPR(IRGuestReg{GPR::R6, Mode::User}, var3);
  // code.StoreGPR(IRGuestReg{GPR::R7, Mode::User}, var4);
  // code.StoreGPR(IRGuestReg{GPR::R8, Mode::User}, var5);

  // Add instruction test
  auto& lhs = code.CreateVar(IRDataType::UInt32, "add_lhs");
  auto& rhs = code.CreateVar(IRDataType::UInt32, "add_rhs");
  auto& result = code.CreateVar(IRDataType::UInt32, "add_result");
  code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, lhs);
  code.LoadGPR(IRGuestReg{GPR::R2, Mode::User}, rhs);
  code.Add(result, lhs, rhs);
  code.StoreGPR(IRGuestReg{GPR::R12, Mode::User}, result);

  fmt::print(code.ToString());
  fmt::print("\n");

  auto state = State{};
  auto backend = X64Backend{};
  backend.Run(state, code);

  for (int i = 0; i < 16; i++) {
    fmt::print("r{} = 0x{:08X}\n", i, state.GetGPR(Mode::User, static_cast<GPR>(i)));
  }
}

using namespace lunatic::test;

struct Memory final : arm::MemoryBase {
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
      return 0xFFFF; // stubbed key input
    }

    fmt::print("Memory: invalid read16 @ 0x{:08X}\n", address);
    return 0;
  }

  auto ReadWord(u32 address, Bus bus) -> u32 override {
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
};

static Memory g_memory;
static arm::ARM g_cpu_interp { arm::ARM::Architecture::ARMv5TE, &g_memory };
static u16 frame[240 * 160];

void render_frame() {
  for (int i = 0; i < 240 * 160; i++) {
    frame[i] = read<u16>(&g_memory.pram, g_memory.vram[i] << 1);
  }
}

int main(int argc, char** argv) {
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

  ir_test();

  SDL_Init(SDL_INIT_VIDEO);

  auto window = SDL_CreateWindow(
    "Project lunatic",
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
  state.r15 = 0x08000000;

  auto event = SDL_Event{};

  for (;;) {
    g_cpu_interp.Run(279620);

    render_frame();

    SDL_UpdateTexture(texture, nullptr, frame, sizeof(u16) * 240);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
        goto done;
    }
  }

done:
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
  return 0;
}