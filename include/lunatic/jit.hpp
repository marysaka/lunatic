/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/memory.hpp>

namespace lunatic {

struct JIT {
  JIT(Memory& memory);
 ~JIT();

  bool& IRQLine();
  void Run(int cycles);

private:
  struct JIT_impl* pimpl = nullptr;
};

} // namespace lunatic
