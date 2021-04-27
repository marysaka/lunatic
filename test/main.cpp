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

  auto state = State{};
  auto backend = X64Backend{};

  auto run = [&](IREmitter const& code, bool int3 = false) {
    fmt::print(code.ToString());
    fmt::print("\n");

    backend.Run(state, code, int3);

    for (int i = 0; i < 16; i++) {
      fmt::print("r{} = 0x{:08X}\n", i, state.GetGPR(Mode::User, static_cast<GPR>(i)));
    }

    fmt::print("cpsr = 0x{:08X}\n", state.GetCPSR().v);
  };

  // TODO: test cases that use register-specified shift amount

//   // Test #0 - operand = 0x11223344, amount = 0, carry_in = 1
//   // Expected r0 = 0x11223344, carry_out = 1
//   {
//     auto code = IREmitter{};  
//     state.GetGPR(Mode::User, GPR::R0) = 0x11223344;
//     state.GetCPSR().f.c = 1;

//     auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
//     auto& result   = code.CreateVar(IRDataType::UInt32, "result");
//     auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
//     auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

//     code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
//     code.LSL(result, operand, IRConstant{u32(0)}, true);
//     code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
//     code.LoadCPSR(cpsr_in);
//     code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
//     code.StoreCPSR(cpsr_out);

//     run(code);
//   }

//   // Test #1 - operand = 0x80000000, amount = 1, carry_in = 0
//   // Expected r0 = 0x00000000, carry_out = 1
//   {
//     auto code = IREmitter{};  
//     state.GetGPR(Mode::User, GPR::R0) = 0x80000000;
//     state.GetCPSR().f.c = 0;

//     auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
//     auto& result   = code.CreateVar(IRDataType::UInt32, "result");
//     auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
//     auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

//     code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
//     code.LSL(result, operand, IRConstant{u32(1)}, true);
//     code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
//     code.LoadCPSR(cpsr_in);
//     code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
//     code.StoreCPSR(cpsr_out);

//     run(code);
//   }

//   // Test #2 - operand = 0x00000001, amount = 31, carry_in = 1
//   // Expected r0 = 0x80000000, carry_out = 0
//   {
//     auto code = IREmitter{};  
//     state.GetGPR(Mode::User, GPR::R0) = 0x00000001;
//     state.GetCPSR().f.c = 1;

//     auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
//     auto& result   = code.CreateVar(IRDataType::UInt32, "result");
//     auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
//     auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

//     code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
//     code.LSL(result, operand, IRConstant{u32(31)}, true);
//     code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
//     code.LoadCPSR(cpsr_in);
//     code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
//     code.StoreCPSR(cpsr_out);

//     run(code);
//   }

//   // Test #3 - operand = 0x00000001, amount = 32, carry_in = 0
//   // Expected r0 = 0x00000000, carry_out = 1
//   // !!! random bug, sometimes this works and other times not !!!
//   // note: after a reboot this doesn't seem to reproduce anymore?!?
//   // possibly ASLR related?!?
//   {
//     auto code = IREmitter{};  
//     state.GetGPR(Mode::User, GPR::R0) = 0x00000001;
//     state.GetCPSR().f.c = 0;

//     auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
//     auto& result   = code.CreateVar(IRDataType::UInt32, "result");
//     auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
//     auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

//     code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
//     code.LSL(result, operand, IRConstant{u32(32)}, true);
//     code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
//     code.LoadCPSR(cpsr_in);
//     code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
//     code.StoreCPSR(cpsr_out);

//     // run(code, true);
//     run(code);
//   }

// // --------------------------------------------------------------------

//   // Test #0 - operand = 0x80000000, amount = 0 (immediate), carry_in = 0
//   // Expected r0 = 0x00000000, carry_out = 1
//   {
//     auto code = IREmitter{};  
//     state.GetGPR(Mode::User, GPR::R0) = 0x80000000;
//     state.GetCPSR().f.c = 0;

//     auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
//     auto& result   = code.CreateVar(IRDataType::UInt32, "result");
//     auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
//     auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

//     code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
//     code.LSR(result, operand, IRConstant{u32(0)}, true);
//     code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
//     code.LoadCPSR(cpsr_in);
//     code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
//     code.StoreCPSR(cpsr_out);

//     run(code);
//   }

//   // Test #1 - operand = 0x80000000, amount = 0 (register), carry_in = 1
//   // Expected r0 = 0x80000000, carry_out = 1
//   {
//     auto code = IREmitter{};  
//     state.GetGPR(Mode::User, GPR::R0) = 0x80000000;
//     state.GetGPR(Mode::User, GPR::R1) = 0;
//     state.GetCPSR().f.c = 1;

//     auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
//     auto& amount   = code.CreateVar(IRDataType::UInt32, "amount");
//     auto& result   = code.CreateVar(IRDataType::UInt32, "result");
//     auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
//     auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

//     code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
//     code.LoadGPR(IRGuestReg{GPR::R1, Mode::User}, amount);
//     code.LSR(result, operand, amount, true);
//     code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
//     code.LoadCPSR(cpsr_in);
//     code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
//     code.StoreCPSR(cpsr_out);

//     run(code);
//   }

//   // Test #2 - operand = 0x80000000, amount = 31 (immediate), carry_in = 1
//   // Expected r0 = 0x00000001, carry_out = 0
//   {
//     auto code = IREmitter{};  
//     state.GetGPR(Mode::User, GPR::R0) = 0x80000000;
//     state.GetCPSR().f.c = 1;

//     auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
//     auto& result   = code.CreateVar(IRDataType::UInt32, "result");
//     auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
//     auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

//     code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
//     code.LSR(result, operand, IRConstant{u32(31)}, true);
//     code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
//     code.LoadCPSR(cpsr_in);
//     code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
//     code.StoreCPSR(cpsr_out);

//     run(code);
//   }

//   // Test #3 - operand = 0x80000000, amount = 31 (register), carry_in = 1
//   // Expected r0 = 0x00000001, carry_out = 0
//   {
//     auto code = IREmitter{};  
//     state.GetGPR(Mode::User, GPR::R0) = 0x80000000;
//     state.GetGPR(Mode::User, GPR::R1) = 31;
//     state.GetCPSR().f.c = 1;

//     auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
//     auto& amount   = code.CreateVar(IRDataType::UInt32, "amount");
//     auto& result   = code.CreateVar(IRDataType::UInt32, "result");
//     auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
//     auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

//     code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
//     code.LoadGPR(IRGuestReg{GPR::R1, Mode::User}, amount);
//     code.LSR(result, operand, amount, true);
//     code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
//     code.LoadCPSR(cpsr_in);
//     code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
//     code.StoreCPSR(cpsr_out);

//     run(code);
//   }

//   // Test #4 - operand = 0x80000000, amount = 32 (immediate), carry_in = 0
//   // Expected r0 = 0x00000000, carry_out = 1
//   {
//     auto code = IREmitter{};  
//     state.GetGPR(Mode::User, GPR::R0) = 0x80000000;
//     state.GetCPSR().f.c = 0;

//     auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
//     auto& result   = code.CreateVar(IRDataType::UInt32, "result");
//     auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
//     auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

//     code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
//     code.LSR(result, operand, IRConstant{u32(32)}, true);
//     code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
//     code.LoadCPSR(cpsr_in);
//     code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
//     code.StoreCPSR(cpsr_out);

//     run(code);
//   }

//   // Test #5 - operand = 0x80000000, amount = 32 (register), carry_in = 0
//   // Expected r0 = 0x00000000, carry_out = 1
//   {
//     auto code = IREmitter{};  
//     state.GetGPR(Mode::User, GPR::R0) = 0x80000000;
//     state.GetGPR(Mode::User, GPR::R1) = 32;
//     state.GetCPSR().f.c = 0;

//     auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
//     auto& amount   = code.CreateVar(IRDataType::UInt32, "amount");
//     auto& result   = code.CreateVar(IRDataType::UInt32, "result");
//     auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
//     auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

//     code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
//     code.LoadGPR(IRGuestReg{GPR::R1, Mode::User}, amount);
//     code.LSR(result, operand, amount, true);
//     code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
//     code.LoadCPSR(cpsr_in);
//     code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
//     code.StoreCPSR(cpsr_out);

//     run(code);
//   }

// --------------------------------------------------------------------

  // // Test #0 - operand = 0x80000000, amount = 0 (immediate), carry_in = 0
  // // Expected r0 = 0xFFFFFFFF, carry_out = 1
  // {
  //   auto code = IREmitter{};  
  //   state.GetGPR(Mode::User, GPR::R0) = 0x80000000;
  //   state.GetCPSR().f.c = 0;

  //   auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
  //   auto& result   = code.CreateVar(IRDataType::UInt32, "result");
  //   auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
  //   auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

  //   code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
  //   code.ASR(result, operand, IRConstant{u32(0)}, true);
  //   code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
  //   code.LoadCPSR(cpsr_in);
  //   code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
  //   code.StoreCPSR(cpsr_out);

  //   run(code);
  // }

  // // Test #1 - operand = 0x80000000, amount = 0 (register), carry_in = 1
  // // Expected r0 = 0x80000000, carry_out = 1
  // {
  //   auto code = IREmitter{};  
  //   state.GetGPR(Mode::User, GPR::R0) = 0x80000000;
  //   state.GetGPR(Mode::User, GPR::R1) = 0;
  //   state.GetCPSR().f.c = 1;

  //   auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
  //   auto& amount   = code.CreateVar(IRDataType::UInt32, "amount");
  //   auto& result   = code.CreateVar(IRDataType::UInt32, "result");
  //   auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
  //   auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

  //   code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
  //   code.LoadGPR(IRGuestReg{GPR::R1, Mode::User}, amount);
  //   code.ASR(result, operand, amount, true);
  //   code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
  //   code.LoadCPSR(cpsr_in);
  //   code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
  //   code.StoreCPSR(cpsr_out);

  //   run(code);
  // }

  // // Test #2 - operand = 0x80000000, amount = 31 (immediate), carry_in = 1
  // // Expected r0 = 0xFFFFFFFF, carry_out = 0
  // {
  //   auto code = IREmitter{};  
  //   state.GetGPR(Mode::User, GPR::R0) = 0x80000000;
  //   state.GetCPSR().f.c = 1;

  //   auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
  //   auto& result   = code.CreateVar(IRDataType::UInt32, "result");
  //   auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
  //   auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

  //   code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
  //   code.ASR(result, operand, IRConstant{u32(31)}, true);
  //   code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
  //   code.LoadCPSR(cpsr_in);
  //   code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
  //   code.StoreCPSR(cpsr_out);

  //   run(code);
  // }

  // // Test #3 - operand = 0x80000000, amount = 31 (register), carry_in = 1
  // // Expected r0 = 0xFFFFFFFF, carry_out = 0
  // {
  //   auto code = IREmitter{};  
  //   state.GetGPR(Mode::User, GPR::R0) = 0x80000000;
  //   state.GetGPR(Mode::User, GPR::R1) = 31;
  //   state.GetCPSR().f.c = 1;

  //   auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
  //   auto& amount   = code.CreateVar(IRDataType::UInt32, "amount");
  //   auto& result   = code.CreateVar(IRDataType::UInt32, "result");
  //   auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
  //   auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

  //   code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
  //   code.LoadGPR(IRGuestReg{GPR::R1, Mode::User}, amount);
  //   code.ASR(result, operand, amount, true);
  //   code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
  //   code.LoadCPSR(cpsr_in);
  //   code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
  //   code.StoreCPSR(cpsr_out);

  //   run(code);
  // }

  // // Test #4 - operand = 0x80000000, amount = 32 (immediate), carry_in = 0
  // // Expected r0 = 0xFFFFFFFF, carry_out = 1
  // {
  //   auto code = IREmitter{};  
  //   state.GetGPR(Mode::User, GPR::R0) = 0x80000000;
  //   state.GetCPSR().f.c = 0;

  //   auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
  //   auto& result   = code.CreateVar(IRDataType::UInt32, "result");
  //   auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
  //   auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

  //   code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
  //   code.ASR(result, operand, IRConstant{u32(32)}, true);
  //   code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
  //   code.LoadCPSR(cpsr_in);
  //   code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
  //   code.StoreCPSR(cpsr_out);

  //   run(code);
  // }

  // // Test #5 - operand = 0x80000000, amount = 32 (register), carry_in = 0
  // // Expected r0 = 0xFFFFFFFF, carry_out = 1
  // {
  //   auto code = IREmitter{};  
  //   state.GetGPR(Mode::User, GPR::R0) = 0x80000000;
  //   state.GetGPR(Mode::User, GPR::R1) = 32;
  //   state.GetCPSR().f.c = 0;

  //   auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
  //   auto& amount   = code.CreateVar(IRDataType::UInt32, "amount");
  //   auto& result   = code.CreateVar(IRDataType::UInt32, "result");
  //   auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
  //   auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

  //   code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
  //   code.LoadGPR(IRGuestReg{GPR::R1, Mode::User}, amount);
  //   code.ASR(result, operand, amount, true);
  //   code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
  //   code.LoadCPSR(cpsr_in);
  //   code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
  //   code.StoreCPSR(cpsr_out);

  //   run(code);
  // }

// ---------------------------------------------------------------------------

  {
    auto code = IREmitter{};  
    state.GetGPR(Mode::User, GPR::R0) = 0xAABBCCDD;
    state.GetCPSR().f.c = 1;

    auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
    auto& result   = code.CreateVar(IRDataType::UInt32, "result");
    auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
    auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

    code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
    code.ROR(result, operand, IRConstant{u32(0)}, true);
    code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
    code.LoadCPSR(cpsr_in);
    code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
    code.StoreCPSR(cpsr_out);

    run(code);
  }

  {
    auto code = IREmitter{};  
    state.GetGPR(Mode::User, GPR::R0) = 0xAABBCCDD;
    state.GetCPSR().f.c = 1;

    auto& operand  = code.CreateVar(IRDataType::UInt32, "operand");
    auto& result   = code.CreateVar(IRDataType::UInt32, "result");
    auto& cpsr_in  = code.CreateVar(IRDataType::UInt32, "cpsr_in");
    auto& cpsr_out = code.CreateVar(IRDataType::UInt32, "cpsr_out");

    code.LoadGPR(IRGuestReg{GPR::R0, Mode::User}, operand);
    code.ROR(result, operand, IRConstant{u32(16)}, true);
    code.StoreGPR(IRGuestReg{GPR::R0, Mode::User}, result);
    code.LoadCPSR(cpsr_in);
    code.UpdateFlags(cpsr_out, cpsr_in, false, false, true, false);
    code.StoreCPSR(cpsr_out);

    run(code);
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