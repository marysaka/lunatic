/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <lunatic/integer.hpp>

#include "common/bit.hpp"

namespace lunatic {
namespace frontend {

/// Receives decoded opcode data
template<typename T>
struct ARMDecodeClient {
  /// Return type for handle() methods.
  /// Utilized by decode_arm to inter its return type.
  using return_type = T;

  virtual auto handle(ARMDataProcessing const& opcode) -> T = 0;
  virtual auto undefined(u32 opcode) -> T = 0;
};

/// Decodes an ARM opcode into one of multiple structures,
/// passes the resulting structure to a client and returns the client's return value.
template<typename T, typename U = T::return_type>
inline auto decode_arm(u32 opcode, T& client) -> U {

  return client.undefined(opcode);
}

} // namespace lunatic::frontend
} // namespace lunatic
