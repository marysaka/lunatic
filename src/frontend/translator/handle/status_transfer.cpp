
#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMMoveStatusRegister const& opcode) -> Status {
  return Status::Unimplemented;
}

auto Translator::Handle(ARMMoveRegisterStatus const& opcode) -> Status {
  auto& psr = emitter->CreateVar(IRDataType::UInt32, "psr");

  if (opcode.spsr) {
    emitter->LoadSPSR(psr, mode);
  } else {
    emitter->LoadCPSR(psr);
  }

  emitter->StoreGPR(IRGuestReg{opcode.reg, mode}, psr);

  return Status::Continue;
}

} // namespace lunatic::frontend
} // namespace lunatic
