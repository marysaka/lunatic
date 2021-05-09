/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <xbyak/xbyak.h>
#include <vector>

#include "common/optional.hpp"
#include "frontend/ir/emitter.hpp"

namespace lunatic {
namespace backend {

struct X64RegisterAllocator {
  X64RegisterAllocator(lunatic::frontend::IREmitter const& emitter);

  auto GetReg32() -> Xbyak::Reg32;

  auto GetReg32(
    lunatic::frontend::IRVariable const& var,
    int location
  ) -> Xbyak::Reg32;

private:
  void CreateVariableExpirationPoints();
  void ExpireVariables(int location);

  lunatic::frontend::IREmitter const& emitter;

  std::vector<Xbyak::Reg32> free_list;
  std::vector<Optional<Xbyak::Reg32>> allocation;
  std::vector<int> expiration_points;
};

} // namespace lunatic::backend
} // namespace lunatic
