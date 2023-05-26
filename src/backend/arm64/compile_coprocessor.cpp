/*
 * Copyright (C) 2023 marysaka. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.hpp"

namespace lunatic::backend {

void ARM64Backend::CompileMRC(CompileContext const& context, IRReadCoprocessorRegister* op){ NOT_IMPLEMENTED_OP(op); }
void ARM64Backend::CompileMCR(CompileContext const& context, IRWriteCoprocessorRegister* op){ NOT_IMPLEMENTED_OP(op); }

} // namespace lunatic::backend
