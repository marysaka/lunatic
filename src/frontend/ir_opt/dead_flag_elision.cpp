/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include "frontend/ir_opt/dead_flag_elision.hpp"

namespace lunatic {
namespace frontend {

void IRDeadFlagElisionPass::Run(IREmitter& emitter) {
  /**
   * TODO:
   * a) when we remove an update.nzcv opcode, turn i.e. the corresponding ADDS into ADD.
   * b) implement the same logic for the Q-flag (update.q)
   * c) fix any bugs left in this?
   */

  bool updated_n = false;
  bool updated_z = false;
  bool updated_c = false;
  bool updated_v = false;

  auto& code = emitter.Code();
  auto it = code.rbegin();
  auto end = code.rend();

  while (it != end) {
    switch (it->get()->GetClass()) {
      case IROpcodeClass::UpdateFlags: {
        auto op = lunatic_cast<IRUpdateFlags>(it->get());
        
        op->flag_n &= !updated_n;
        op->flag_z &= !updated_z;
        op->flag_c &= !updated_c;
        op->flag_v &= !updated_v;

        if (op->flag_n) updated_n = true;
        if (op->flag_z) updated_z = true;
        if (op->flag_c) updated_c = true;
        if (op->flag_v) updated_v = true;

        if (!op->flag_n && !op->flag_z && !op->flag_c && !op->flag_v) {
           // TODO: do not use begin()?
          if (Repoint(op->result.Get(), op->input.Get(), code.begin(), code.end())) {
            it = std::reverse_iterator{code.erase(std::next(it).base())};
            end = code.rend();
          }
        }
        break;
      }
      case IROpcodeClass::LoadCPSR: {
        updated_n = false;
        updated_z = false;
        updated_c = false;
        updated_v = false;
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
