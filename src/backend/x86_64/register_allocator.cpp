/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "register_allocator.hpp"

using namespace lunatic::frontend;
using namespace Xbyak::util;

namespace lunatic {
namespace backend {

X64RegisterAllocator::X64RegisterAllocator(IREmitter const& emitter) : emitter(emitter) {
  // rax and rcx are statically allocated
  free_list = {
    edx,
    ebx,
    esi,
    edi,
    ebp,
    r8d,
    r9d,
    r10d,
    r11d,
    r12d,
    r13d,
    r14d,
    r15d
  };

  allocation.resize(emitter.Vars().size());
  expiration_points.resize(emitter.Vars().size());

  CreateVariableExpirationPoints();
}

auto X64RegisterAllocator::GetReg32() -> Xbyak::Reg32 {
  if (free_list.size() != 0) {
    auto reg = free_list.back();
    free_list.pop_back();
    return reg;
  }

  throw std::runtime_error("X64RegisterAllocator: failed to allocate register");
}

auto X64RegisterAllocator::GetReg32(IRVariable const& var, int location) -> Xbyak::Reg32 {
  // Check if the variable is already allocated to a register at the moment.
  auto maybe_reg = allocation[var.id];
  if (maybe_reg.HasValue()) {
    return maybe_reg.Unwrap();
  }

  // Release any registers that are allocated to expired variables first.
  ExpireVariables(location);

  auto reg = GetReg32();
  allocation[var.id] = reg;
  return reg;
}

void X64RegisterAllocator::CreateVariableExpirationPoints() {
  for (auto const& var : emitter.Vars()) {
    int expiration_point = -1;
    int location = 0;

    for (auto const& op : emitter.Code()) {
      if (op->Writes(*var) || op->Reads(*var)) {
        expiration_point = location;
      }

      location++;
    }

    if (expiration_point != -1) {
      expiration_points[var->id] = expiration_point;
    }
  }
}

void X64RegisterAllocator::ExpireVariables(int location) {
  for (auto const& var : emitter.Vars()) {
    auto expiration_point = expiration_points[var->id];

    if (location > expiration_point) {
      auto maybe_reg = allocation[var->id];
      if (maybe_reg.HasValue()) {
        free_list.push_back(maybe_reg.Unwrap());
        allocation[var->id] = {};
      }
    }
  }
}

} // namespace lunatic::backend
} // namespace lunatic
