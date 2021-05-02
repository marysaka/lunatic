/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
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
