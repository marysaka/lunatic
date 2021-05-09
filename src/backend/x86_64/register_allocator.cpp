/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <iterator>

#include "register_allocator.hpp"

using namespace lunatic::frontend;
using namespace Xbyak::util;

namespace lunatic {
namespace backend {

X64RegisterAllocator::X64RegisterAllocator(
  IREmitter const& emitter,
  Xbyak::CodeGenerator& code,
  int spill_area_size
) : emitter(emitter), code(code) {
  // rax, rcx and rbp are statically allocated
  free_list = {
    edx,
    ebx,
    esi,
    edi,
    r8d,
    r9d,
    r10d,
    r11d,
    r12d,
    r13d,
    r14d,
    r15d
  };

  auto number_of_vars = emitter.Vars().size();

  allocation.resize(number_of_vars);
  expiration_points.resize(number_of_vars);
  spill_used.resize(spill_area_size);
  spill_location.resize(number_of_vars);

  for (int i = 0; i < spill_area_size; i++) {
    spill_used[i] = false;
  }

  CreateVariableExpirationPoints();
}

auto X64RegisterAllocator::GetReg32(int location) -> Xbyak::Reg32 {
  if (free_list.size() != 0) {
    auto reg = free_list.back();
    free_list.pop_back();
    return reg;
  }

  auto number_of_vars = allocation.size();
  auto reg = Xbyak::Reg32{};
  auto var_id = 0;

  // Find a variable to be spilled and deallocate it.
  // TODO: think of a smart way to pick which variable/register to spill.
  for (int i = 0; i < number_of_vars; i++) {
    // TODO: this is sloooooooooow!!!
    auto it = emitter.Code().begin();
    std::advance(it, location);
    auto const& op = *it;    
    auto const& var = *emitter.Vars()[i];

    // Do not spill a variable that we will need during the current operation.
    if (op->Reads(var)) {
      continue;
    }

    if (allocation[i].HasValue()) {
      reg = allocation[i].Unwrap();
      allocation[i] = {};
      var_id = i;
      break;
    }
  }

  auto number_of_slots = spill_used.size();

  // Spill the variable into one of the free slots.
  for (int i = 0; i < number_of_slots; i++) {
    if (!spill_used[i]) {
      spill_used[i] = true;
      spill_location[var_id] = i;
      code.mov(dword[rbp + i * sizeof(u32)], reg);
      break;
    }
  }

  throw std::runtime_error("X64RegisterAllocator: out of registers and spill space.");
}

auto X64RegisterAllocator::GetReg32(IRVariable const& var, int location) -> Xbyak::Reg32 {
  // Check if the variable is already allocated to a register at the moment.
  auto maybe_reg = allocation[var.id];
  if (maybe_reg.HasValue()) {
    return maybe_reg.Unwrap();
  }

  // Release any registers that are allocated to expired variables first.
  ExpireVariables(location);

  auto reg = GetReg32(location);
  
  // If the variable was spilled previously then restore its previous value.
  auto maybe_spill = spill_location[var.id];
  if (maybe_spill.HasValue()) {
    auto slot = maybe_spill.Unwrap();
    code.mov(reg, dword[rbp + slot * sizeof(u32)]);
    spill_used[slot] = false;
    spill_location[var.id] = {};
    fmt::print("Reload from spill :3\n");
  }

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
