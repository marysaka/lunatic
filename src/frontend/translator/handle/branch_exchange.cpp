/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMBranchExchange const& opcode) -> Status {
  if (armv5te && opcode.link) {
    emitter->StoreGPR(IRGuestReg{GPR::LR, mode}, IRConstant{code_address + opcode_size});
  }

  auto& address = emitter->CreateVar(IRDataType::UInt32, "address");
  emitter->LoadGPR(IRGuestReg{opcode.reg, mode}, address);
  EmitFlushExchange(address);

  return Status::BreakBasicBlock;
}

} // namespace lunatic::frontend
} // namespace lunatic
