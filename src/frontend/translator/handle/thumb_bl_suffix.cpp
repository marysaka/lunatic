/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ThumbBranchLinkSuffix const& opcode) -> Status {
  auto& lr  = emitter->CreateVar(IRDataType::UInt32, "lr");
  auto& pc1 = emitter->CreateVar(IRDataType::UInt32, "pc");
  auto& pc2 = emitter->CreateVar(IRDataType::UInt32, "pc");
  auto& pc3 = emitter->CreateVar(IRDataType::UInt32, "pc");

  emitter->LoadGPR(IRGuestReg{GPR::LR, mode}, lr);
  emitter->ADD(pc1, lr, IRConstant{opcode.offset}, false);
  emitter->StoreGPR(IRGuestReg{GPR::LR, mode}, IRConstant{u32((code_address + sizeof(u16)) | 1)});

  if (armv5te && opcode.exchange) {
    auto& cpsr_in  = emitter->CreateVar(IRDataType::UInt32, "cpsr_in");
    auto& cpsr_out = emitter->CreateVar(IRDataType::UInt32, "cpsr_out");
  
    emitter->LoadCPSR(cpsr_in);
    emitter->AND(cpsr_out, cpsr_in, IRConstant{~32U}, false);
    emitter->StoreCPSR(cpsr_out);

    emitter->AND(pc2, pc1, IRConstant{~3U}, false);
    emitter->ADD(pc3, pc2, IRConstant{sizeof(u32) * 2}, false);
    emitter->StoreGPR(IRGuestReg{GPR::PC, mode}, pc3);
  } else {
    emitter->AND(pc2, pc1, IRConstant{~1U}, false);
    emitter->ADD(pc3, pc2, IRConstant{sizeof(u16) * 2}, false);
    emitter->StoreGPR(IRGuestReg{GPR::PC, mode}, pc3);
  }

  return Status::BreakBasicBlock;
}

} // namespace lunatic::frontend
} // namespace lunatic
