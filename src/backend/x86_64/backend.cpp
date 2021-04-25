/*
 * Copyright (C) 2021 fleroviux
 */

#include <list>
#include <stdexcept>
#include <unordered_map>
#include <xbyak/xbyak.h>

#include "backend.hpp"

namespace lunatic {
namespace backend {

using namespace lunatic::frontend;

void X64Backend::Run(State& state, IREmitter const& emitter) {
  using namespace Xbyak::util;

  std::list<Xbyak::Reg32> host_regs {
    ecx, edx, r8d, r9d
  };

  std::unordered_map<u32, Xbyak::Reg32> reg_map;

  auto alloc_reg = [&]() -> Xbyak::Reg32 {
    if (host_regs.size() == 0) {
      throw std::runtime_error("X64Backend: ran out of registers to allocate");
    }
    auto reg = host_regs.front();
    host_regs.pop_front();
    return reg;
  };

  auto get_var_reg = [&](IRVariable const& var) -> Xbyak::Reg32 {
    auto match = reg_map.find(var.id);

    if (match != reg_map.end()) {
      return match->second;
    }

    reg_map[var.id] = alloc_reg();
    return reg_map[var.id];
  };

  Xbyak::CodeGenerator code;

  code.mov(rax, u64(&state));

  for (auto const& op_ : emitter.Code()) {
    switch (op_->GetClass()) {
      case IROpcodeClass::LoadGPR: {
        auto op = lunatic_cast<IRLoadGPR>(op_.get());
        auto address = rax + state.GetOffsetToGPR(op->reg.mode, op->reg.reg);
        code.mov(get_var_reg(op->result), dword[address]);
        break;
      }
      case IROpcodeClass::StoreGPR: {
        auto op = lunatic_cast<IRStoreGPR>(op_.get());
        auto address = rax + state.GetOffsetToGPR(op->reg.mode, op->reg.reg);
        if (op->value.IsConstant()) {
          code.mov(dword[address], op->value.GetConst().value);
        } else {
          code.mov(dword[address], get_var_reg(op->value.GetVar()));
        }
        break;
      }
    }
  }

  code.ret();
  code.getCode<void (*)()>()();

  // Analyze variable lifetimes
  for (auto const& var : emitter.Vars()) {
    int start = -1;
    int end = -1;

    int location = 0;

    for (auto const& op : emitter.Code()) {
      if (op->Writes(*var)) {
        // if (start != -1) {
        //   throw std::runtime_error("SSA form violation: variable written more than once");
        // }
        start = location;
      } else if (op->Reads(*var)) {
        // if (start == -1) {
        //   throw std::runtime_error("SSA form violation: variable read before it was written");
        // }
        end = location;
      }

      location++;
    }

    fmt::print("{}: {:03} - {:03}\n", std::to_string(*var), start, end);
  }
}

} // namespace lunatic::backend
} // namespace lunatic
