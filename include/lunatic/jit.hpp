/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

namespace lunatic {

struct JIT {
  JIT();
 ~JIT();

private:
  struct JIT_impl* pimpl = nullptr;
};

} // namespace lunatic
