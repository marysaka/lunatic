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
  auto load = opcode.load;
  auto coprocessor_id = opcode.coprocessor_id;
  auto opcode1 = opcode.opcode1;
  auto cn = opcode.cn;
  auto cm = opcode.cm;
  auto opcode2 = opcode.opcode2;
  auto coprocessor = coprocessors[coprocessor_id];

  // TODO: throw an undefined opcode exception.
  if (coprocessor == nullptr) {
    return Status::Unimplemented;
  }

  auto& data = emitter->CreateVar(IRDataType::UInt32, "data");

  if (load) {
    emitter->MRC(data, coprocessor_id, opcode1, cn, cm, opcode2);
    emitter->StoreGPR(IRGuestReg{opcode.reg_dst, mode}, data);
  } else {
    emitter->LoadGPR(IRGuestReg{opcode.reg_dst, mode}, data);
    emitter->MCR(data, coprocessor_id, opcode1, cn, cm, opcode2);
  }

  EmitAdvancePC();

  if (!load && coprocessor->ShouldWriteBreakBasicBlock(opcode1, cn, cm, opcode2)) {
    basic_block->enable_fast_dispatch = false;
    return Status::BreakBasicBlock;
  }

  return Status::Continue;
}

} // namespace lunatic::frontend
} // namespace lunatic
