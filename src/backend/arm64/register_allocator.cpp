/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <iterator>

#include "register_allocator.hpp"

using namespace lunatic::frontend;
using namespace oaknut::util;

namespace lunatic {
namespace backend {

ARM64RegisterAllocator::ARM64RegisterAllocator(
  IREmitter const& emitter,
  oaknut::CodeGenerator& code
) : emitter(emitter), code(code) {
  // Static allocation:
  //   - X13: host flags (TODO determine that)
  //   - X14: number of cycles left
  //   - X15: pointer to guest state (lunatic::frontend::State)
  //   - SP: pointer to stack frame / spill area.
  free_host_regs = {
    W0,
    W1,
    W2,
    W2,
    W3,
    W4,
    W5,
    W6,
    W7,
    W8,
    W9,
    W10,
    W11,
    W12,
    W16,
    W17,
  };

  auto number_of_vars = emitter.Vars().size();
  var_id_to_host_reg.resize(number_of_vars);
  var_id_to_point_of_last_use.resize(number_of_vars);
  var_id_to_spill_slot.resize(number_of_vars);

  EvaluateVariableLifetimes();

  current_op_iter = emitter.Code().begin();
}

void ARM64RegisterAllocator::AdvanceLocation() {
  location++;
  current_op_iter++;

  // Release host regs that hold variables which now are dead.
  ReleaseDeadVariables();

  // Release host regs the previous opcode allocated temporarily.
  ReleaseTemporaryHostRegs();
}

auto ARM64RegisterAllocator::GetVariableHostReg(IRVariable const& var) -> oaknut::WReg {
  // Check if the variable is already allocated to a register at the moment.
  auto maybe_reg = var_id_to_host_reg[var.id];
  if (maybe_reg.HasValue()) {
    return oaknut::WReg(maybe_reg.Unwrap());
  }

  auto reg = FindFreeHostReg();

  // If the variable was spilled previously then restore its previous value.
  auto maybe_spill = var_id_to_spill_slot[var.id];
  if (maybe_spill.HasValue()) {
    auto slot = maybe_spill.Unwrap();
    code.LDR(reg, SP, slot * sizeof(u32));
    free_spill_bitmap[slot] = false;
    var_id_to_spill_slot[var.id] = {};
  }

  var_id_to_host_reg[var.id] = reg.index();
  return reg;
}

auto ARM64RegisterAllocator::GetTemporaryHostReg() -> oaknut::WReg {
  auto reg = FindFreeHostReg();
  temp_host_regs.push_back(reg);
  return reg;
}

void ARM64RegisterAllocator::ReleaseVarAndReuseHostReg(
  IRVariable const& var_old,
  IRVariable const& var_new
) {
  if (var_id_to_host_reg[var_new.id].HasValue()) {
    return;
  }

  auto point_of_last_use = var_id_to_point_of_last_use[var_old.id];

  if (point_of_last_use == location) {
    auto maybe_reg = var_id_to_host_reg[var_old.id];

    if (maybe_reg.HasValue()) {
      var_id_to_host_reg[var_new.id] = maybe_reg;
      var_id_to_host_reg[var_old.id] = {};
    }
  }
}

bool ARM64RegisterAllocator::IsHostRegFree(oaknut::XReg reg) const {
  auto begin = free_host_regs.begin();
  auto end = free_host_regs.end();

  return std::find_if(begin, end, [reg](oaknut::WReg x) { return x.index() == reg.index(); }) != end;
}

void ARM64RegisterAllocator::EvaluateVariableLifetimes() {
  for (auto const& var : emitter.Vars()) {
    int point_of_last_use = -1;
    int location = 0;

    for (auto const& op : emitter.Code()) {
      if (op->Writes(*var) || op->Reads(*var)) {
        point_of_last_use = location;
      }

      location++;
    }

    if (point_of_last_use != -1) {
      var_id_to_point_of_last_use[var->id] = point_of_last_use;
    }
  }
}

void ARM64RegisterAllocator::ReleaseDeadVariables() {
  for (auto const& var : emitter.Vars()) {
    auto point_of_last_use = var_id_to_point_of_last_use[var->id];

    if (location > point_of_last_use) {
      auto maybe_reg = var_id_to_host_reg[var->id];
      if (maybe_reg.HasValue()) {
        free_host_regs.push_back(oaknut::WReg(maybe_reg.Unwrap()));
        var_id_to_host_reg[var->id] = {};
      }
    }
  }
}

void ARM64RegisterAllocator::ReleaseTemporaryHostRegs() {
  for (auto reg : temp_host_regs) {
    free_host_regs.push_back(reg);
  }
  temp_host_regs.clear();
}

auto ARM64RegisterAllocator::FindFreeHostReg() -> oaknut::WReg {
  if (free_host_regs.size() != 0) {
    auto reg = free_host_regs.back();
    free_host_regs.pop_back();
    return reg;
  }

  auto const& current_op = *current_op_iter;

  // Find a variable to be spilled and deallocate it.
  // TODO: think of a smart way to pick which variable/register to spill.
  for (auto const& var : emitter.Vars()) {
    if (var_id_to_host_reg[var->id].HasValue()) {
      // Make sure the variable that we spill is not currently used.
      if (current_op->Reads(*var) || current_op->Writes(*var)) {
        continue;
      }

      auto reg = oaknut::WReg(var_id_to_host_reg[var->id].Unwrap());

      var_id_to_host_reg[var->id] = {};

      // Spill the variable into one of the free slots.
      for (int slot = 0; slot < kSpillAreaSize; slot++) {
        if (!free_spill_bitmap[slot]) {
          code.STR(reg, SP, slot * sizeof(u32));
          free_spill_bitmap[slot] = true;
          var_id_to_spill_slot[var->id] = slot;
          return reg;
        }
      }
    }
  }

  throw std::runtime_error("ARM64RegisterAllocator: out of registers and spill space.");
}

} // namespace lunatic::backend
} // namespace lunatic
