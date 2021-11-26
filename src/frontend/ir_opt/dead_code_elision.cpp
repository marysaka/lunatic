/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/ir_opt/dead_code_elision.hpp"

namespace lunatic {
namespace frontend {

void IRDeadCodeElisionPass::Run(IREmitter& emitter) {
	auto& code = emitter.Code();
	auto it = code.begin();
  auto end = code.end();

  while (it != end) {
    switch (it->get()->GetClass()) {
    	// ADD #0 is a no-operation
      case IROpcodeClass::ADD: {
        auto op = lunatic_cast<IRAdd>(it->get());

        if (op->result.HasValue() && op->rhs.IsConstant() && op->rhs.GetConst().value == 0 && !op->update_host_flags) {
          if (Repoint(op->result.Unwrap(), op->lhs.Get(), it, end)) {
            it = code.erase(it);
            continue;
          }
        }
        break;
      }
      // LSL(S) #0 is a no-operation
      case IROpcodeClass::LSL: {
        auto op = lunatic_cast<IRLogicalShiftLeft>(it->get());
        
        if (op->amount.IsConstant() && op->amount.GetConst().value == 0) {
          if (Repoint(op->result.Get(), op->operand.Get(), it, end)) {
            it = code.erase(it);
            continue;
          }
        }
        break;
      }
      // MOV var_a, var_b: var_a is a redundant variable.
      case IROpcodeClass::MOV: {
        auto op = lunatic_cast<IRMov>(it->get());

        if (op->source.IsVariable() && !op->update_host_flags) {
          if (Repoint(op->result.Get(), op->source.GetVar(), it, end)) {
            it = code.erase(it);
            continue;
          }
        }
        break;
      }
      default: {
      	break;
      }
    }

    ++it;
  }
}

} // namespace lunatic::frontend
} // namespace lunatic