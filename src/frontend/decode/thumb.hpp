/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/integer.hpp>

// FIXME: do not include the whole damn thing.
#include "arm.hpp"

namespace lunatic {
namespace frontend {

namespace detail {

} // namespace lunatic::frontend::detail

/// Decodes a Thumb opcode into one of multiple structures,
/// passes the resulting structure to a client and returns the client's return value.
template<typename T, typename U = typename T::return_type>
inline auto decode_thumb(u16 instruction, T& client) -> U {

  // TODO: this is broken. can't distinguish between undefined ARM or Thumb opcode.
  return client.Undefined(instruction);
}

} // namespace lunatic::frontend
} // namespace lunatic
