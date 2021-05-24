/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMMultiplyLong const& opcode) -> Status {

  //EmitAdvancePC();
  return Status::Unimplemented;
}

} // namespace lunatic::frontend
} // namespace lunatic
