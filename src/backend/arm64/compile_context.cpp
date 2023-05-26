/*
 * Copyright (C) 2023 marysaka. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.hpp"

namespace lunatic::backend {

void ARM64Backend::CompileLoadGPR(CompileContext const& context, IRLoadGPR* op) {
  LoadRegister(context, op->result, state.GetOffsetToGPR(op->reg.mode, op->reg.reg));
}

void ARM64Backend::CompileStoreGPR(CompileContext const& context, IRStoreGPR* op) {
  StoreRegister(context, op->value, state.GetOffsetToGPR(op->reg.mode, op->reg.reg));
}

void ARM64Backend::CompileLoadSPSR(CompileContext const& context, IRLoadSPSR* op) {
  LoadRegister(context, op->result, state.GetOffsetToSPSR(op->mode));
}

void ARM64Backend::CompileStoreSPSR(CompileContext const& context, IRStoreSPSR* op) {
  StoreRegister(context, op->value, state.GetOffsetToSPSR(op->mode));
}

void ARM64Backend::CompileLoadCPSR(CompileContext const& context, IRLoadCPSR* op) {
  LoadRegister(context, op->result, state.GetOffsetToCPSR());
}

void ARM64Backend::CompileStoreCPSR(CompileContext const& context, IRStoreCPSR* op) {
  StoreRegister(context, op->value, state.GetOffsetToCPSR());
}

} // namespace lunatic::backend
