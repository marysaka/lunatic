/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include "basic_block.hpp"

namespace lunatic {
namespace frontend {

struct BasicBlockCache {
  auto Get(BasicBlock::Key key) const -> BasicBlock* {
    auto table = data[key.value >> 19];
    if (table == nullptr) {
      return nullptr;
    }
    return table->data[key.value & 0x7FFFF];
  }

  void Set(BasicBlock::Key key, BasicBlock* block) {
    auto hash = key.value >> 19;
    auto table = data[hash];
    if (table == nullptr) {
      table = new Table{};
      data[hash] = table;
    }
    table->data[key.value & 0x7FFFF] = block;
  }

  struct Table {
    //int use_count = 0;
    BasicBlock* data[0x80000] {nullptr};
  };

  Table* data[0x40000] {nullptr};
};

} // namespace lunatic::frontend
} // namespace lunatic
