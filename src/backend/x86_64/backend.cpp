/*
 * Copyright (C) 2021 fleroviux
 */

#include <list>
#include <stdexcept>
// #include <unordered_map>
#include <xbyak/xbyak.h>

#include "backend.hpp"
#include "register_allocator.hpp"

namespace lunatic {
namespace backend {

using namespace lunatic::frontend;

void X64Backend::Run(State& state, IREmitter const& emitter) {
  using namespace Xbyak::util;

  Xbyak::CodeGenerator code;
  X64RegisterAllocator reg_alloc { code };

  code.mov(rax, u64(&state));

  for (auto const& op_ : emitter.Code()) {
    switch (op_->GetClass()) {
      case IROpcodeClass::LoadGPR: {
        auto op = lunatic_cast<IRLoadGPR>(op_.get());
        auto address  = rax + state.GetOffsetToGPR(op->reg.mode, op->reg.reg);
        auto host_reg = reg_alloc.GetReg32(op->result);
        code.mov(host_reg, dword[address]);
        break;
      }
      case IROpcodeClass::StoreGPR: {
        auto op = lunatic_cast<IRStoreGPR>(op_.get());
        auto address = rax + state.GetOffsetToGPR(op->reg.mode, op->reg.reg);
        if (op->value.IsConstant()) {
          code.mov(dword[address], op->value.GetConst().value);
        } else {
          auto host_reg = reg_alloc.GetReg32(op->value.GetVar());
          code.mov(dword[address], host_reg);
        }
        break;
      }
    }
  }

  // Restore any callee-saved registers.
  reg_alloc.Finalize();

  // code.int3();
  code.ret();
  code.getCode<void (*)()>()();
}

} // namespace lunatic::backend
} // namespace lunatic
