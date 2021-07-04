/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMCountLeadingZeros const& opcode) -> Status {
  auto& result = emitter->CreateVar(IRDataType::UInt32, "result");
  auto& operand = emitter->CreateVar(IRDataType::UInt32, "operand");

  emitter->LoadGPR(IRGuestReg{opcode.reg_src, mode}, operand);
  emitter->CLZ(result, operand);
  emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, result);

  EmitAdvancePC();
  return Status::Continue;
}

} // namespace lunatic::frontend
} // namespace lunatic
