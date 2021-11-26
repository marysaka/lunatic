/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include "frontend/ir_opt/pass.hpp"

namespace lunatic {
namespace frontend {

struct IRDeadCodeElisionPass final : IRPass {
  void Run(IREmitter& emitter) override;
};

} // namespace lunatic::frontend
} // namespace lunatic
