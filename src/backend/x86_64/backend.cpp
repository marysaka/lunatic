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

  std::list<Xbyak::Reg64> host_regs {
    rax, rcx, rdx, r8, r9
  };

  std::unordered_map<u32, Xbyak::Reg64> reg_map;

  auto alloc_reg = [&]() -> Xbyak::Reg64 {
    if (host_regs.size() == 0) {
      throw std::runtime_error("X64Backend: ran out of registers to allocate");
    }
    auto reg = host_regs.front();
    host_regs.pop_front();
    return reg;
  };

  auto get_var_reg = [&](IRVariable const& var) -> Xbyak::Reg64 {
    auto match = reg_map.find(var.id);

    if (match != reg_map.end()) {
      return match->second;
    }

    reg_map[var.id] = alloc_reg();
    return reg_map[var.id];
  };

  Xbyak::CodeGenerator code;

  for (auto const& op_ : emitter.Code()) {
    switch (op_->GetClass()) {
      case IROpcodeClass::LoadGPR: {
        auto op = lunatic_cast<IRLoadGPR>(op_.get());
        auto address = (void*)(state.GetPointerToGPR(op->reg.mode, op->reg.reg));
        code.mov(get_var_reg(op->result), dword[address]);
        break;
      }
      case IROpcodeClass::StoreGPR: {
        auto op = lunatic_cast<IRStoreGPR>(op_.get());
        auto address = (void*)(state.GetPointerToGPR(op->reg.mode, op->reg.reg));
        if (op->value.IsConstant()) {
          // ugh this is horrible, how to fix?
          auto tmp_reg = alloc_reg();
          code.mov(tmp_reg, op->value.GetConst().value);
          code.mov(dword[address], tmp_reg);
        } else {
          code.mov(dword[address], get_var_reg(op->value.GetVar()));
        }
        break;
      }
    }
  }

  code.ret();
  code.getCode<void (*)()>()();
}

} // namespace lunatic::backend
} // namespace lunatic
