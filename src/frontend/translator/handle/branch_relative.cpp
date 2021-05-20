/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMBranchRelative const& opcode) -> Status {
  auto target_addr = IRConstant{code_address + opcode.offset + opcode_size * 4};

  if (opcode.link) {
    emitter->StoreGPR(IRGuestReg{GPR::LR, mode}, IRConstant{code_address + opcode_size});
  }

  emitter->StoreGPR(IRGuestReg{GPR::PC, mode}, target_addr);

  return Status::BreakBasicBlock;
}

} // namespace lunatic::frontend
} // namespace lunatic
