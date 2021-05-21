/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMSingleDataSwap const& opcode) -> Status {
  if (opcode.reg_dst == GPR::PC) {
    return Status::Unimplemented;
  }

  auto& tmp = emitter->CreateVar(IRDataType::UInt32, "tmp");
  auto& address = emitter->CreateVar(IRDataType::UInt32, "address");
  auto& source = emitter->CreateVar(IRDataType::UInt32, "source");

  emitter->LoadGPR(IRGuestReg{opcode.reg_base, mode}, address);
  emitter->LoadGPR(IRGuestReg{opcode.reg_src, mode}, source);

  if (opcode.byte) {
    emitter->LDR(Byte, tmp, address);
    emitter->STR(Byte, source, address);
  } else {
    emitter->LDR(Word | Rotate, tmp, address);
    emitter->STR(Word, source, address);
  }

  emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, tmp);
  EmitAdvancePC();
  return Status::Continue;
}

} // namespace lunatic::frontend
} // namespace lunatic
