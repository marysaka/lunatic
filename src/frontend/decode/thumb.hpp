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

using ARMDataOp = ARMDataProcessing::Opcode;

enum class ThumbDataOp {
  AND = 0,
  EOR = 1,
  LSL = 2,
  LSR = 3,
  ASR = 4,
  ADC = 5,
  SBC = 6,
  ROR = 7,
  TST = 8,
  NEG = 9,
  CMP = 10,
  CMN = 11,
  ORR = 12,
  MUL = 13,
  BIC = 14,
  MVN = 15
};

template<typename T, typename U = typename T::return_type>
inline auto decode_move_shifted_register(u16 opcode, T& client) -> U {
  return client.Handle(ARMDataProcessing{
    .condition = Condition::AL,
    .opcode = ARMDataOp::MOV,
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
    .opcode = bit::get_bit(opcode, 9) ? ARMDataOp::SUB : ARMDataOp::ADD,
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

template<typename T, typename U = typename T::return_type>
inline auto decode_mov_cmp_add_sub_imm(u16 opcode, T& client) -> U {
  ARMDataOp op;

  switch (bit::get_field(opcode, 11, 2)) {
    case 0b00: op = ARMDataOp::MOV; break;
    case 0b01: op = ARMDataOp::CMP; break;
    case 0b10: op = ARMDataOp::ADD; break;
    case 0b11: op = ARMDataOp::SUB; break;
  }

  return client.Handle(ARMDataProcessing{
    .condition = Condition::AL,
    .opcode = op,
    .immediate = true,
    .set_flags = true,
    .reg_dst = bit::get_field<u16, GPR>(opcode, 8, 3),
    .reg_op1 = bit::get_field<u16, GPR>(opcode, 8, 3),
    .op2_imm = {
      .value = bit::get_field<u16, u8>(opcode, 0, 8),
      .shift = 0
    }
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_alu(u16 opcode, T& client) -> U {
  auto op = bit::get_field<u16, ThumbDataOp>(opcode, 6, 4);
  auto reg_dst = bit::get_field<u16, GPR>(opcode, 0, 3);
  auto reg_src = bit::get_field<u16, GPR>(opcode, 3, 3);

  auto decode_dp_alu = [&](ARMDataOp op) {
    return client.Handle(ARMDataProcessing{
      .condition = Condition::AL,
      .opcode = static_cast<ARMDataOp>(op),
      .immediate = false,
      .set_flags = true,
      .reg_dst = reg_dst,
      .reg_op1 = reg_dst,
      .op2_reg = {
        .reg = reg_src,
        .shift = {
          .type = Shift::LSL,
          .immediate = true,
          .amount_imm = 0
        }
      }
    });
  };

  auto decode_dp_shift = [&](Shift type) {
    return client.Handle(ARMDataProcessing{
      .condition = Condition::AL,
      .opcode = ARMDataOp::MOV,
      .immediate = false,
      .set_flags = true,
      .reg_dst = reg_dst,
      .op2_reg = {
        .reg = reg_dst,
        .shift = {
          .type = type,
          .immediate = false,
          .amount_reg = reg_src
        }
      }
    });
  };

  auto decode_dp_neg = [&]() {
    return client.Handle(ARMDataProcessing{
      .condition = Condition::AL,
      .opcode = ARMDataOp::RSB,
      .immediate = true,
      .set_flags = true,
      .reg_dst = reg_dst,
      .reg_op1 = reg_src,
      .op2_imm = {
        .value = 0,
        .shift = 0
      }
    });
  };

  auto decode_mul = [&]() {
    return client.Handle(ARMMultiply{
      .condition = Condition::AL,
      .accumulate = false,
      .set_flags = true,
      .reg_op1 = reg_dst,
      .reg_op2 = reg_src,
      .reg_dst = reg_dst
    });
  };

  switch (op) {
    case ThumbDataOp::AND: return decode_dp_alu(ARMDataOp::AND);
    case ThumbDataOp::EOR: return decode_dp_alu(ARMDataOp::EOR);
    case ThumbDataOp::LSL: return decode_dp_shift(Shift::LSL);
    case ThumbDataOp::LSR: return decode_dp_shift(Shift::LSR);
    case ThumbDataOp::ASR: return decode_dp_shift(Shift::ASR);
    case ThumbDataOp::ADC: return decode_dp_alu(ARMDataOp::ADC);
    case ThumbDataOp::SBC: return decode_dp_alu(ARMDataOp::SBC);
  //case ThumbDataOp::ROR: return decode_dp_shift(Shift::ROR);
    case ThumbDataOp::ROR: return client.Undefined(opcode);
    case ThumbDataOp::TST: return decode_dp_alu(ARMDataOp::TST);
    case ThumbDataOp::NEG: return decode_dp_neg();
    case ThumbDataOp::CMP: return decode_dp_alu(ARMDataOp::CMP);
    case ThumbDataOp::CMN: return decode_dp_alu(ARMDataOp::CMN);
    case ThumbDataOp::ORR: return decode_dp_alu(ARMDataOp::ORR);
    case ThumbDataOp::MUL: return decode_mul();
    case ThumbDataOp::BIC: return decode_dp_alu(ARMDataOp::BIC);
    case ThumbDataOp::MVN: return decode_dp_alu(ARMDataOp::MVN);
  }
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
  if ((instruction & 0xE000) == 0x2000) return decode_mov_cmp_add_sub_imm(instruction, client);
  if ((instruction & 0xFC00) == 0x4000) return decode_alu(instruction, client);
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
