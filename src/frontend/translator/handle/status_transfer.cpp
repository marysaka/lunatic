
#include "frontend/translator/translator.hpp"

namespace lunatic {
namespace frontend {

auto Translator::Handle(ARMMoveStatusRegister const& opcode) -> Status {
  return Status::Unimplemented;
}

auto Translator::Handle(ARMMoveRegisterStatus const& opcode) -> Status {
  return Status::Unimplemented;
}

} // namespace lunatic::frontend
} // namespace lunatic
