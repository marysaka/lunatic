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

using DPOpcode = ARMDataProcessing::Opcode;

template<typename T, typename U = typename T::return_type>
inline auto decode_move_shifted_register(u16 opcode, T& client) -> U {
  return client.Handle(ARMDataProcessing{
    .condition = Condition::AL,
    .opcode = DPOpcode::MOV,
    .immediate = false,
    .set_flags = true,
    .reg_dst = bit::get_field<u16, GPR>(opcode, 0, 3),
    .op2_reg = {
      .reg = bit::get_field<u16, GPR>(opcode, 3, 3),
      .shift = {
        .type = bit::get_field<u16, Shift>(opcode, 11, 2),
        .immediate = true,
        .amount_imm = bit::get_field(opcode, 6, 5)
      }
    }
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_add_sub(u16 opcode, T& client) -> U {
  return client.Handle(ARMDataProcessing{
    .condition = Condition::AL,
    .opcode = bit::get_bit(opcode, 9) ? DPOpcode::SUB : DPOpcode::ADD,
    .immediate = bit::get_bit<u16, bool>(opcode, 10),
    .set_flags = true,
    .reg_dst = bit::get_field<u16, GPR>(opcode, 0, 3),
    .reg_op1 = bit::get_field<u16, GPR>(opcode, 3, 3),
    .op2_reg = {
      .reg = bit::get_field<u16, GPR>(opcode, 6, 3),
      .shift = {
        .type = Shift::LSL,
        .immediate = true,
        .amount_imm = 0
      }
    },
    .op2_imm = {
      .value = bit::get_field<u16, u8>(opcode, 6, 3),
      .shift = 0
    }
  });
}

} // namespace lunatic::frontend::detail

/// Decodes a Thumb opcode into one of multiple structures,
/// passes the resulting structure to a client and returns the client's return value.
template<typename T, typename U = typename T::return_type>
inline auto decode_thumb(u16 instruction, T& client) -> U {
  using namespace detail;

  // TODO: use string pattern based approach to decoding.

  if ((instruction & 0xF800) <  0x1800) return decode_move_shifted_register(instruction, client);
  if ((instruction & 0xF800) == 0x1800) return decode_add_sub(instruction, client);
//  if ((instruction & 0xE000) == 0x2000) return ThumbInstrType::MoveCompareAddSubImm;
//  if ((instruction & 0xFC00) == 0x4000) return ThumbInstrType::ALU;
//  if ((instruction & 0xFC00) == 0x4400) return ThumbInstrType::HighRegisterOps;
//  if ((instruction & 0xF800) == 0x4800) return ThumbInstrType::LoadStoreRelativePC;
//  if ((instruction & 0xF200) == 0x5000) return ThumbInstrType::LoadStoreOffsetReg;
//  if ((instruction & 0xF200) == 0x5200) return ThumbInstrType::LoadStoreSigned;
//  if ((instruction & 0xE000) == 0x6000) return ThumbInstrType::LoadStoreOffsetImm;
//  if ((instruction & 0xF000) == 0x8000) return ThumbInstrType::LoadStoreHword;
//  if ((instruction & 0xF000) == 0x9000) return ThumbInstrType::LoadStoreRelativeSP;
//  if ((instruction & 0xF000) == 0xA000) return ThumbInstrType::LoadAddress;
//  if ((instruction & 0xFF00) == 0xB000) return ThumbInstrType::AddOffsetToSP;
//  if ((instruction & 0xFF00) == 0xB200) return ThumbInstrType::SignOrZeroExtend;
//  if ((instruction & 0xF600) == 0xB400) return ThumbInstrType::PushPop;
//  if ((instruction & 0xFFE0) == 0xB640) return ThumbInstrType::SetEndianess;
//  if ((instruction & 0xFFE0) == 0xB660) return ThumbInstrType::ChangeProcessorState;
//  if ((instruction & 0xFF00) == 0xBA00) return ThumbInstrType::ReverseBytes;
//  if ((instruction & 0xFF00) == 0xBE00) return ThumbInstrType::SoftwareBreakpoint;
//  if ((instruction & 0xF000) == 0xC000) return ThumbInstrType::LoadStoreMultiple;
//  if ((instruction & 0xFF00) <  0xDF00) return ThumbInstrType::ConditionalBranch;
//  if ((instruction & 0xFF00) == 0xDF00) return ThumbInstrType::SoftwareInterrupt;
//  if ((instruction & 0xF800) == 0xE000) return ThumbInstrType::UnconditionalBranch;
//  if ((instruction & 0xF800) == 0xE800) return ThumbInstrType::LongBranchLinkExchangeSuffix;
//  if ((instruction & 0xF800) == 0xF000) return ThumbInstrType::LongBranchLinkPrefix;
//  if ((instruction & 0xF800) == 0xF800) return ThumbInstrType::LongBranchLinkSuffix;

  // TODO: this is broken. can't distinguish between undefined ARM or Thumb opcode.
  return client.Undefined(instruction);
}

} // namespace lunatic::frontend
} // namespace lunatic
