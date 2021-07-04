/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/integer.hpp>

namespace lunatic {

struct Coprocessor {
  virtual ~Coprocessor() = default;

  virtual void Reset() {}

  virtual bool ShouldWriteBreakBasicBlock(
    int opcode1,
    int cn,
    int cm,
    int opcode2
  ) {
    return false;
  }

  virtual auto Read(
    int opcode1,
    int cn,
    int cm,
    int opcode2
  ) -> u32 = 0;

  virtual void Write(
    int opcode1,
    int cn,
    int cm,
    int opcode2,
    u32 value
  ) = 0;
};

} // namespace lunatic
