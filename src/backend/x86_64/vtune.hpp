/*
 * Copyright (C) 2022 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <fmt/format.h>
#include <frontend/basic_block.hpp>

#if LUNATIC_USE_VTUNE

#include <jitprofiling.h>

namespace vtune {

static void ReportCallBlock(u8* codeBegin, const u8* codeEnd) {
  if (iJIT_IsProfilingActive() == iJIT_SAMPLING_ON) {
    char methodName[] = "lunatic_x64_callblock";
    char moduleName[] = "lunatic-JIT";

    iJIT_Method_Load_V2 jmethod = { 0 };
    jmethod.method_id = iJIT_GetNewMethodID();
    jmethod.method_name = methodName;
    jmethod.method_load_address = static_cast<void*>(codeBegin);
    jmethod.method_size = static_cast<unsigned int>(codeEnd - codeBegin);
    jmethod.module_name = moduleName;
    iJIT_NotifyEvent(iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED_V2, static_cast<void*>(&jmethod));
  }
}

static void ReportBasicBlock(lunatic::frontend::BasicBlock& basic_block, const u8* codeEnd) {
  if (iJIT_IsProfilingActive() == iJIT_SAMPLING_ON) {
    auto& key = basic_block.key;

    auto modeStr = [&]() -> std::string {
      using lunatic::Mode;

      switch (key.Mode()) {
        case Mode::User: return "USR";
        case Mode::FIQ: return "FIQ";
        case Mode::IRQ: return "IRQ";
        case Mode::Supervisor: return "SVC";
        case Mode::Abort: return "ABT";
        case Mode::Undefined: return "UND";
        case Mode::System: return "SYS";
        default: return fmt::format("{:02X}", static_cast<uint>(key.Mode()));
      }
    }();

    auto thumbStr = key.Thumb() ? "Thumb" : "ARM";
    auto methodName = fmt::format("lunatic_func_{:X}_{}_{}", key.Address(), modeStr, thumbStr);
    char moduleName[] = "lunatic-JIT";

    iJIT_Method_Load_V2 jmethod = { 0 };
    jmethod.method_id = iJIT_GetNewMethodID();
    jmethod.method_name = methodName.data();
    jmethod.method_load_address = reinterpret_cast<void*>(basic_block.function);
    jmethod.method_size = static_cast<unsigned int>(codeEnd - reinterpret_cast<u8*>(basic_block.function));
    jmethod.module_name = moduleName;
    iJIT_NotifyEvent(iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED_V2, static_cast<void*>(&jmethod));
  }
}

} // namespace vtune

#endif
