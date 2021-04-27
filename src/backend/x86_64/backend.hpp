/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include "backend/backend.hpp"
#include "frontend/ir/emitter.hpp"
#include "frontend/state.hpp"

namespace lunatic {
namespace backend {

struct X64Backend : Backend {
  void Run(lunatic::frontend::State& state, lunatic::frontend::IREmitter const& emitter, bool int3);
};

} // namespace lunatic::backend
} // namespace lunatic
