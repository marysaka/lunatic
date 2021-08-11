/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMCoprocessorRegisterTransfer const& opcode) -> Status {
  // TODO: throw an undefined opcode exception.
  if (coprocessors[opcode.coprocessor_id] == nullptr) {
    return Status::Unimplemented;
  }

  auto& data = emitter->CreateVar(IRDataType::UInt32, "data");

  if (opcode.load) {
    emitter->MRC(
      data,
      opcode.coprocessor_id,
      opcode.opcode1,
      opcode.cn,
      opcode.cm,
      opcode.opcode2
    );
    emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, data);
  } else {
    emitter->LoadGPR(IRGuestReg{opcode.reg_dst, mode}, data);
    emitter->MCR(
      data,
      opcode.coprocessor_id,
      opcode.opcode1,
      opcode.cn,
      opcode.cm,
      opcode.opcode2
    );
  }

  EmitAdvancePC();
  return Status::Continue;
}

} // namespace lunatic::frontend
} // namespace lunatic
