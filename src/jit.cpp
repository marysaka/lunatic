/*
 * Copyright (C) 2021 fleroviux
 */

#include <lunatic/jit.hpp>

namespace lunatic {

struct JIT_impl {
};

JIT::JIT() : pimpl(new JIT_impl()) {}

JIT::~JIT() {
  delete pimpl;
}

} // namespace lunatic
