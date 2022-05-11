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
  auto& address = emitter->CreateVar(IRDataType::UInt32, "address");

  emitter->LoadGPR(IRGuestReg{opcode.reg, mode}, address);

  if (armv5te && opcode.link) {
    auto link_address = code_address + opcode_size;
    if (thumb_mode) {
      link_address |= 1;
    }
    emitter->StoreGPR(IRGuestReg{GPR::LR, mode}, IRConstant{link_address});
  }

  EmitFlushExchange(address);

  return Status::BreakBasicBlock;
}

} // namespace lunatic::frontend
} // namespace lunatic
