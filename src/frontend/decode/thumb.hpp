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

enum class ThumbHighRegOp {
  ADD = 0,
  CMP = 1,
  MOV = 2,
  BLX = 3
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

template<typename T, typename U = typename T::return_type>
inline auto decode_high_register_ops(u16 opcode, T& client) -> U {
  auto op = bit::get_field<u16, ThumbHighRegOp>(opcode, 8, 2);
  auto high1 = bit::get_bit(opcode, 7) * 8;
  auto high2 = bit::get_bit(opcode, 6) * 8;
  auto reg_dst = static_cast<GPR>(bit::get_field(opcode, 0, 3) | high1);
  auto reg_src = static_cast<GPR>(bit::get_field(opcode, 3, 3) | high2);

  switch (op) {
    case ThumbHighRegOp::ADD: {
      return client.Handle(ARMDataProcessing{
        .condition = Condition::AL,
        .opcode = ARMDataOp::ADD,
        .immediate = false,
        .set_flags = false,
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
    }
    case ThumbHighRegOp::CMP: {
      return client.Handle(ARMDataProcessing{
        .condition = Condition::AL,
        .opcode = ARMDataOp::CMP,
        .immediate = false,
        .set_flags = true,
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
    }
    case ThumbHighRegOp::MOV: {
      return client.Handle(ARMDataProcessing{
        .condition = Condition::AL,
        .opcode = ARMDataOp::MOV,
        .immediate = false,
        .set_flags = false,
        .reg_dst = reg_dst,
        .op2_reg = {
          .reg = reg_src,
          .shift = {
            .type = Shift::LSL,
            .immediate = true,
            .amount_imm = 0
          }
        }
      });
    }
    case ThumbHighRegOp::BLX: {
      return client.Handle(ARMBranchExchange{
        .condition = Condition::AL,
        .reg = reg_src,
        .link = high1 != 0
      });
    }
  }
}

template<typename T, typename U = typename T::return_type>
inline auto decode_load_relative_pc(u16 opcode, T& client) -> U {
  return client.Handle(ARMSingleDataTransfer{
    .condition = Condition::AL,
    .immediate = true,
    .pre_increment = true,
    .add = true,
    .byte = false,
    .writeback = false,
    .load = true,
    .reg_dst = bit::get_field<u16, GPR>(opcode, 8, 3),
    .reg_base = GPR::PC,
    .offset_imm = u32(bit::get_field(opcode, 0, 8) * sizeof(u32))
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_load_store_offset_reg(u16 opcode, T& client) -> U {
  return client.Handle(ARMSingleDataTransfer{
    .condition = Condition::AL,
    .immediate = false,
    .pre_increment = true,
    .add = true,
    .byte = bit::get_bit<u16, bool>(opcode, 10),
    .writeback = false,
    .load = bit::get_bit<u16, bool>(opcode, 11),
    .reg_dst = bit::get_field<u16, GPR>(opcode, 0, 3),
    .reg_base = bit::get_field<u16, GPR>(opcode, 3, 3),
    .offset_reg = {
      .reg = bit::get_field<u16, GPR>(opcode, 6, 3),
      .shift = Shift::LSL,
      .amount = 0
    }
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_load_store_signed(u16 opcode, T& client) -> U {
  auto op = 0;
  auto load = false;

  // TODO: rework this mess, ugh.
  switch (bit::get_field(opcode, 10, 2)) {
    case 0b00: {
      // STRH
      op = 1;
      load = false;
      break;
    }
    case 0b01: {
      // LDRSB
      op = 2;
      load = true;
      break;
    }
    case 0b10: {
      // LDRH
      op = 1;
      load = true;
      break;
    }
    case 0b11: {
      // LDRSH
      op = 3;
      load = true;
      break;
    }
  }

  client.Handle(ARMHalfwordSignedTransfer{
    .condition = Condition::AL,
    .pre_increment = true,
    .add = true,
    .immediate = false,
    .writeback = false,
    .load = load,
    .opcode = op,
    .reg_dst = bit::get_field<u16, GPR>(opcode, 0, 3),
    .reg_base = bit::get_field<u16, GPR>(opcode, 3, 3),
    .offset_reg = bit::get_field<u16, GPR>(opcode, 6, 3)
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_load_store_offset_imm(u16 opcode, T& client) -> U {
  auto byte = bit::get_bit<u16, bool>(opcode, 12);
  auto offset = bit::get_field(opcode, 6, 5);

  if (!byte) offset <<= 2;

  return client.Handle(ARMSingleDataTransfer{
    .condition = Condition::AL,
    .immediate = true,
    .pre_increment = true,
    .add = true,
    .byte = byte,
    .writeback = false,
    .load = bit::get_bit<u16, bool>(opcode, 11),
    .reg_dst = bit::get_field<u16, GPR>(opcode, 0, 3),
    .reg_base = bit::get_field<u16, GPR>(opcode, 3, 3),
    .offset_imm = offset
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_load_store_half(u16 opcode, T& client) -> U {
  return client.Handle(ARMHalfwordSignedTransfer{
    .condition = Condition::AL,
    .pre_increment = true,
    .add = true,
    .immediate = true,
    .writeback = false,
    .load = bit::get_bit<u16, bool>(opcode, 11),
    .opcode = 1,
    .reg_dst = bit::get_field<u16, GPR>(opcode, 0, 3),
    .reg_base = bit::get_field<u16, GPR>(opcode, 3, 3),
    .offset_imm = u32(bit::get_field(opcode, 6, 5) << 1)
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_load_store_relative_sp(u16 opcode, T& client) -> U {
  return client.Handle(ARMSingleDataTransfer{
    .condition = Condition::AL,
    .immediate = true,
    .pre_increment = true,
    .add = true,
    .byte = false,
    .writeback = false,
    .load = bit::get_bit<u16, bool>(opcode, 11),
    .reg_dst = bit::get_field<u16, GPR>(opcode, 8, 3),
    .reg_base = GPR::SP,
    .offset_imm = u32(bit::get_field(opcode, 0, 8) << 2)
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_load_address(u16 opcode, T& client) -> U {
  return client.Handle(ARMDataProcessing{
    .condition = Condition::AL,
    .opcode = ARMDataOp::ADD,
    .immediate = true,
    .set_flags = false,
    .reg_dst = bit::get_field<u16, GPR>(opcode, 8, 3),
    .reg_op1 = bit::get_bit(opcode, 11) ? GPR::SP : GPR::PC,
    .op2_imm = {
      .value = u32(bit::get_field(opcode, 0, 8) << 2),
      .shift = 0
    },
    .thumb_load_address = true
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_add_sp_offset(u16 opcode, T& client) -> U {
  return client.Handle(ARMDataProcessing{
    .condition = Condition::AL,
    .opcode = bit::get_bit(opcode, 7) ? ARMDataOp::SUB : ARMDataOp::ADD,
    .immediate = true,
    .set_flags = false,
    .reg_dst = GPR::SP,
    .reg_op1 = GPR::SP,
    .op2_imm = {
      .value = u32(bit::get_field(opcode, 0, 7) << 2),
      .shift = 0
    }
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_push_pop(u16 opcode, T& client) -> U {
  bool load  = bit::get_bit(opcode, 11);
  auto rlist = bit::get_field(opcode, 0, 8);

  if (bit::get_bit(opcode,  8)) {
    rlist |= (1 << (load ? 15 : 14));
  }

  return client.Handle(ARMBlockDataTransfer{
    .condition = Condition::AL,
    .pre_increment = !load,
    .add = load,
    .user_mode = false,
    .writeback = true,
    .load = load,
    .reg_base = GPR::SP,
    .reg_list = rlist
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_ldm_stm(u16 opcode, T& client) -> U {
  return client.Handle(ARMBlockDataTransfer{
    .condition = Condition::AL,
    .pre_increment = false,
    .add = true,
    .user_mode = false,
    .writeback = true,
    .load = bit::get_bit<u16, bool>(opcode, 11),
    .reg_base = bit::get_field<u16, GPR>(opcode, 8, 3),
    .reg_list = bit::get_field(opcode, 0, 8)
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_conditional_branch(u16 opcode, T& client) -> U {
  auto offset = (bit::get_field<u16, s32>(opcode, 0, 8) << 24) >> 23;
  auto condition = bit::get_field<u16, Condition>(opcode, 8, 4);

  return client.Handle(ARMBranchRelative{
    .condition = condition,
    .offset = offset,
    .link = false
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_svc(u16 opcode, T& client) -> U {
  return client.Handle(ARMException{
    .condition = Condition::AL,
    .exception = Exception::Supervisor,
    .svc_comment = u32(bit::get_field(opcode, 0, 8) << 16)
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_unconditional_branch(u16 opcode, T& client) -> U {
  auto offset = (bit::get_field<u16, s32>(opcode, 0, 11) << 21) >> 20;

  return client.Handle(ARMBranchRelative{
    .condition = Condition::AL,
    .offset = offset,
    .link = false
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_branch_link_prefix(u16 opcode, T& client) -> U {
  auto offset = (bit::get_field<u16, s32>(opcode, 0, 11) << 21) >> 9;

  return client.Handle(ARMDataProcessing{
    .condition = Condition::AL,
    .opcode = ARMDataOp::ADD,
    .immediate = true,
    .set_flags = false,
    .reg_dst = GPR::LR,
    .reg_op1 = GPR::PC,
    .op2_imm = {
      .value = u32(offset),
      .shift = 0
    }
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_branch_link_suffix(u16 opcode, T& client, bool exchange) -> U {
  return client.Handle(ThumbBranchLinkSuffix{
    .offset = bit::get_field<u16, u32>(opcode, 0, 11) << 1,
    .exchange = exchange
  });
}

} // namespace lunatic::frontend::detail

/// Decodes a Thumb opcode into one of multiple structures,
/// passes the resulting structure to a client and returns the client's return value.
template<typename T, typename U = typename T::return_type>
inline auto decode_thumb(u16 opcode, T& client) -> U {
  using namespace detail;

  // TODO: use string pattern based approach to decoding.

  if ((opcode & 0xF800) <  0x1800) return decode_move_shifted_register(opcode, client);
  if ((opcode & 0xF800) == 0x1800) return decode_add_sub(opcode, client);
  if ((opcode & 0xE000) == 0x2000) return decode_mov_cmp_add_sub_imm(opcode, client);
  if ((opcode & 0xFC00) == 0x4000) return decode_alu(opcode, client);
  if ((opcode & 0xFC00) == 0x4400) return decode_high_register_ops(opcode, client);
  if ((opcode & 0xF800) == 0x4800) return decode_load_relative_pc(opcode, client);
  if ((opcode & 0xF200) == 0x5000) return decode_load_store_offset_reg(opcode, client);
  if ((opcode & 0xF200) == 0x5200) return decode_load_store_signed(opcode, client);
  if ((opcode & 0xE000) == 0x6000) return decode_load_store_offset_imm(opcode, client);
  if ((opcode & 0xF000) == 0x8000) return decode_load_store_half(opcode, client);
  if ((opcode & 0xF000) == 0x9000) return decode_load_store_relative_sp(opcode, client);
  if ((opcode & 0xF000) == 0xA000) return decode_load_address(opcode, client);
  if ((opcode & 0xFF00) == 0xB000) return decode_add_sp_offset(opcode, client);
  if ((opcode & 0xF600) == 0xB400) return decode_push_pop(opcode, client);
//  if ((opcode & 0xFF00) == 0xBE00) return ThumbInstrType::SoftwareBreakpoint;
  if ((opcode & 0xF000) == 0xC000) return decode_ldm_stm(opcode, client);
  if ((opcode & 0xFF00) <  0xDF00) return decode_conditional_branch(opcode, client);
  if ((opcode & 0xFF00) == 0xDF00) return decode_svc(opcode, client);
  if ((opcode & 0xF800) == 0xE000) return decode_unconditional_branch(opcode, client);
  if ((opcode & 0xF800) == 0xE800) return decode_branch_link_suffix(opcode, client, true);
  if ((opcode & 0xF800) == 0xF000) return decode_branch_link_prefix(opcode, client);
  if ((opcode & 0xF800) == 0xF800) return decode_branch_link_suffix(opcode, client, false);

  // TODO: this is broken. can't distinguish between undefined ARM or Thumb opcode.
  return client.Undefined(opcode);
}

} // namespace lunatic::frontend
} // namespace lunatic
