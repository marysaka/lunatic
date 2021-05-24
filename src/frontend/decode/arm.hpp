/*
 * Copyright (C) 2021 fleroviux. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/integer.hpp>

#include "common/bit.hpp"
#include "definition/block_data_transfer.hpp"
#include "definition/branch_exchange.hpp"
#include "definition/branch_relative.hpp"
#include "definition/data_processing.hpp"
#include "definition/halfword_signed_transfer.hpp"
#include "definition/multiply.hpp"
#include "definition/multiply_long.hpp"
#include "definition/single_data_swap.hpp"
#include "definition/single_data_transfer.hpp"
#include "definition/status_transfer.hpp"
#include "definition/thumb_bl_suffix.hpp"

namespace lunatic {
namespace frontend {

/// Receives decoded opcode data
template<typename T>
struct ARMDecodeClient {
  /// Return type for Handle() methods.
  /// Utilized by decode_arm to inter its return type.
  using return_type = T;

  virtual auto Handle(ARMDataProcessing const& opcode) -> T = 0;
  virtual auto Handle(ARMMoveStatusRegister const& opcode) -> T = 0;
  virtual auto Handle(ARMMoveRegisterStatus const& opcode) -> T = 0;
  virtual auto Handle(ARMMultiply const& opcode) -> T = 0;
  virtual auto Handle(ARMMultiplyLong const& opcode) -> T = 0;
  virtual auto Handle(ARMSingleDataSwap const& opcode) -> T = 0;
  virtual auto Handle(ARMBranchExchange const& opcode) -> T = 0;
  virtual auto Handle(ARMHalfwordSignedTransfer const& opcode) -> T = 0;
  virtual auto Handle(ARMSingleDataTransfer const& opcode) -> T = 0;
  virtual auto Handle(ARMBlockDataTransfer const& opcode) -> T = 0;
  virtual auto Handle(ARMBranchRelative const& opcode) -> T = 0;
  virtual auto Handle(ThumbBranchLinkSuffix const& opcode) -> T = 0;
  virtual auto Undefined(u32 opcode) -> T = 0;
};

namespace detail {

template<typename T, typename U = typename T::return_type>
inline auto decode_data_processing(Condition condition, u32 opcode, T& client) -> U {
  return client.Handle(ARMDataProcessing{
    .condition = condition,
    .opcode = bit::get_field<u32, ARMDataProcessing::Opcode>(opcode, 21, 4),
    .immediate = bit::get_bit<u32, bool>(opcode, 25),
    .set_flags = bit::get_bit<u32, bool>(opcode, 20),
    .reg_dst = bit::get_field<u32, GPR>(opcode, 12, 4),
    .reg_op1 = bit::get_field<u32, GPR>(opcode, 16, 4),
    .op2_reg = {
      .reg = bit::get_field<u32, GPR>(opcode, 0, 4),
      .shift = {
        .type = bit::get_field<u32, Shift>(opcode, 5, 2),
        .immediate = !bit::get_bit<u32, bool>(opcode, 4),
        .amount_reg = bit::get_field<u32, GPR>(opcode, 8, 4),
        .amount_imm = bit::get_field(opcode, 7, 5)
      }
    },
    .op2_imm = {
      .value = bit::get_field(opcode, 0, 8),
      .shift = bit::get_field(opcode, 8, 4) * 2
    }
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_move_status_register(Condition condition, u32 opcode, T& client) -> U {
  return client.Handle(ARMMoveStatusRegister{
    .condition = condition,
    .immediate = bit::get_bit<u32, bool>(opcode, 25),
    .spsr = bit::get_bit<u32, bool>(opcode, 22),
    .fsxc = bit::get_field<u32, int>(opcode, 16, 4),
    .reg = bit::get_field<u32, GPR>(opcode, 0, 4),
    .imm = bit::rotate_right(u32(u8(opcode)), bit::get_field(opcode, 8, 4) * 2)
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_move_register_status(Condition condition, u32 opcode, T& client) -> U {
  return client.Handle(ARMMoveRegisterStatus{
    .condition = condition,
    .spsr = bit::get_bit<u32, bool>(opcode, 25),
    .reg = bit::get_field<u32, GPR>(opcode, 12, 4)
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_multiply(Condition condition, u32 opcode, T& client) -> U {
  return client.Handle(ARMMultiply{
    .condition = condition,
    .accumulate = bit::get_bit<u32, bool>(opcode, 21),
    .set_flags = bit::get_bit<u32, bool>(opcode, 20),
    .reg_op1 = bit::get_field<u32, GPR>(opcode,  0, 4),
    .reg_op2 = bit::get_field<u32, GPR>(opcode,  8, 4),
    .reg_op3 = bit::get_field<u32, GPR>(opcode, 12, 4),
    .reg_dst = bit::get_field<u32, GPR>(opcode, 16, 4)
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_multiply_long(Condition condition, u32 opcode, T& client) -> U {
  return client.Handle(ARMMultiplyLong{
    .condition = condition,
    .sign_extend = bit::get_bit<u32, bool>(opcode, 22),
    .accumulate = bit::get_bit<u32, bool>(opcode, 21),
    .set_flags = bit::get_bit<u32, bool>(opcode, 20),
    .reg_op1 = bit::get_field<u32, GPR>(opcode,  0, 4),
    .reg_op2 = bit::get_field<u32, GPR>(opcode,  8, 4),
    .reg_dst_lo = bit::get_field<u32, GPR>(opcode, 12, 4),
    .reg_dst_hi = bit::get_field<u32, GPR>(opcode, 16, 4)
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_single_data_swap(Condition condition, u32 opcode, T& client) -> U {
  return client.Handle(ARMSingleDataSwap{
    .condition = condition,
    .byte = bit::get_bit<u32, bool>(opcode, 22),
    .reg_src  = bit::get_field<u32, GPR>(opcode,  0, 4),
    .reg_dst  = bit::get_field<u32, GPR>(opcode, 12, 4),
    .reg_base = bit::get_field<u32, GPR>(opcode, 16, 4)
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_branch_exchange(Condition condition, u32 opcode, T& client) -> U {
  return client.Handle(ARMBranchExchange{
    .condition = condition,
    .reg = bit::get_field<u32, GPR>(opcode, 0, 4),
    .link = bit::get_bit<u32, bool>(opcode, 5)
  });
}

// TODO: the name of this instruction group is a misnormer...
template<typename T, typename U = typename T::return_type>
inline auto decode_halfword_signed_transfer(Condition condition, u32 opcode, T& client) -> U {
  return client.Handle(ARMHalfwordSignedTransfer{
    .condition = condition,
    .pre_increment = bit::get_bit<u32, bool>(opcode, 24),
    .add = bit::get_bit<u32, bool>(opcode, 23),
    .immediate = bit::get_bit<u32, bool>(opcode, 22),
    .writeback = bit::get_bit<u32, bool>(opcode, 21),
    .load = bit::get_bit<u32, bool>(opcode, 20),
    .opcode = bit::get_field<u32, int>(opcode, 5, 2),
    .reg_dst = bit::get_field<u32, GPR>(opcode, 12, 4),
    .reg_base = bit::get_field<u32, GPR>(opcode, 16, 4),
    .offset_imm = (opcode & 0xF) | ((opcode >> 4) & 0xF0),
    .offset_reg = bit::get_field<u32, GPR>(opcode, 0, 4)
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_single_data_transfer(Condition condition, u32 opcode, T& client) -> U {
  return client.Handle(ARMSingleDataTransfer{
    .condition = condition,
    .immediate = !bit::get_bit<u32, bool>(opcode, 25),
    .pre_increment = bit::get_bit<u32, bool>(opcode, 24),
    .add = bit::get_bit<u32, bool>(opcode, 23),
    .byte = bit::get_bit<u32, bool>(opcode, 22),
    .writeback = bit::get_bit<u32, bool>(opcode, 21),
    .load = bit::get_bit<u32, bool>(opcode, 20),
    .reg_dst = bit::get_field<u32, GPR>(opcode, 12, 4),
    .reg_base = bit::get_field<u32, GPR>(opcode, 16, 4),
    .offset_imm = bit::get_field(opcode, 0, 12),
    .offset_reg = {
      .reg = bit::get_field<u32, GPR>(opcode, 0, 4),
      .shift = bit::get_field<u32, Shift>(opcode, 5, 2),
      .amount = bit::get_field(opcode, 7, 5)
    }
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_block_data_transfer(Condition condition, u32 opcode, T& client) -> U {
  return client.Handle(ARMBlockDataTransfer{
    .condition = condition,
    .pre_increment = bit::get_bit<u32, bool>(opcode, 24),
    .add = bit::get_bit<u32, bool>(opcode, 23),
    .user_mode = bit::get_bit<u32, bool>(opcode, 22),
    .writeback = bit::get_bit<u32, bool>(opcode, 21),
    .load = bit::get_bit<u32, bool>(opcode, 20),
    .reg_base = bit::get_field<u32, GPR>(opcode, 16, 4),
    .reg_list = bit::get_field<u32, u16>(opcode, 0, 16)
  });
}

template<typename T, typename U = typename T::return_type>
inline auto decode_branch_relative(Condition condition, u32 opcode, T& client) -> U {
  auto offset = opcode & 0xFFFFFF;

  if (offset & 0x800000) {
    offset |= 0xFF000000;
  }

  offset *= sizeof(u32);

  return client.Handle(ARMBranchRelative{
    .condition = condition,
    .offset = s32(offset),
    .link = bit::get_bit<u32, bool>(opcode, 24)
  });
}

} // namespace lunatic::frontend::detail

/// Decodes an ARM opcode into one of multiple structures,
/// passes the resulting structure to a client and returns the client's return value.
template<typename T, typename U = typename T::return_type>
inline auto decode_arm(u32 instruction, T& client) -> U {
  auto opcode = instruction & 0x0FFFFFFF;
  auto condition = bit::get_field<u32, Condition>(instruction, 28, 4);

  using namespace detail;

  // TODO: use string pattern based approach to decoding.
  switch (opcode >> 25) {
    case 0b000: {
      // Data processing immediate shift
      // Miscellaneous instructions (A3-4)
      // Data processing register shift
      // Miscellaneous instructions (A3-4)
      // Multiplies (A3-3)
      // Extra load/stores (A3-5)
      bool set_flags = opcode & (1 << 20);
      auto opcode2 = (opcode >> 21) & 0xF;

      if ((opcode & 0x90) == 0x90) {
        // Multiplies (A3-3)
        // Extra load/stores (A3-5)
        if ((opcode & 0x60) != 0) {
          return decode_halfword_signed_transfer(condition, opcode, client);
        } else {
          switch ((opcode >> 23) & 3) {
            case 0b00:
            case 0b01: {
              auto op = bit::get_field(opcode, 21, 4);
              switch (op) {
                case 0b000:
                case 0b001:
                  // MUL, MLA
                  return decode_multiply(condition, opcode, client);
                case 0b100:
                case 0b101:
                case 0b110:
                case 0b111:
                  // UMULL, UMLAL, SMULL, SMLAL
                  return decode_multiply_long(condition, opcode, client);
                default:
                  return client.Undefined(instruction);
              }
            }
            case 0b10:
              return decode_single_data_swap(condition, opcode, client);
            case 0b11:
              // return ARMInstrType::LoadStoreExclusive;
              return client.Undefined(instruction);
          }
        }
      } else if (!set_flags && opcode2 >= 0b1000 && opcode2 <= 0b1011) {
        // Miscellaneous instructions (A3-4)
        if ((opcode & 0xF0) == 0) {
          // return ARMInstrType::StatusTransfer;
          if (bit::get_bit(opcode, 21)) {
            return decode_move_status_register(condition, opcode, client);
          }
          return decode_move_register_status(condition, opcode, client);
        }

        if ((opcode & 0x6000F0) == 0x200010) {
          // Branch and exchange (no link)
          return decode_branch_exchange(condition, opcode, client);
        }

        if ((opcode & 0x6000F0) == 0x200020) {
          // return ARMInstrType::BranchAndExchangeJazelle;
          return client.Undefined(instruction);
        }

        if ((opcode & 0x6000F0) == 0x600010) {
          // return ARMInstrType::CountLeadingZeros;
          return client.Undefined(instruction);
        }

        if ((opcode & 0x6000F0) == 0x200030) {
          // Branch and exchange with link
          return decode_branch_exchange(condition, opcode, client);
        }

        if ((opcode & 0xF0) == 0x50) {
          // return ARMInstrType::SaturatingAddSubtract;
          return client.Undefined(instruction);
        }

        if ((opcode & 0x6000F0) == 0x200070) {
          // return ARMInstrType::Breakpoint;
          return client.Undefined(instruction);
        }

        if ((opcode & 0x90) == 0x80) {
          // Signed halfword multiply (ARMv5 upwards):
          // SMLAxy, SMLAWy, SMULWy, SMLALxy, SMULxy
          // return ARMInstrType::SignedHalfwordMultiply;
          return client.Undefined(instruction);
        }
      }

      // Data processing immediate shift
      // Data processing register shift
      return decode_data_processing(condition, opcode, client);
    }
    case 0b001: {
      // Data processing immediate
      // Undefined instruction
      // Move immediate to status register
      bool set_flags = opcode & (1 << 20);

      if (!set_flags) {
        auto opcode2 = (opcode >> 21) & 0xF;

        switch (opcode2) {
          case 0b1000:
          case 0b1010:
            // return ARMInstrType::Undefined;
            return client.Undefined(instruction);
          case 0b1001:
          case 0b1011:
            // return ARMInstrType::StatusTransfer;
            return decode_move_status_register(condition, opcode, client);
        }
      }

      return decode_data_processing(condition, opcode, client);
    }
    case 0b010: {
      // Load/store immediate offset
      return decode_single_data_transfer(condition, opcode, client);
    }
    case 0b011: {
      // Load/store register offset
      // Media instructions
      // Architecturally Undefined
      if (opcode & 0x10) {
        // return ARMInstrType::Media;
        return client.Undefined(instruction);
      }

      return decode_single_data_transfer(condition, opcode, client);
    }
    case 0b100: {
      // Load/store multiple
      return decode_block_data_transfer(condition, opcode, client);
    }
    case 0b101: {
      // Branch and branch with link
      return decode_branch_relative(condition, opcode, client);
    }
    case 0b110: {
      // Coprocessor load/store and double register transfers
      // TODO: differentiate between load/store and double reg transfer instructions.
      // return ARMInstrType::CoprocessorLoadStoreAndDoubleRegXfer;
      return client.Undefined(instruction);
    }
    case 0b111: {
      // Coprocessor data processing
      // Coprocessor register transfers
      // Software interrupt
      if ((opcode & 0x1000010) == 0) {
        // return ARMInstrType::CoprocessorDataProcessing;
        return client.Undefined(instruction);
      }

      if ((opcode & 0x1000010) == 0x10) {
        // return ARMInstrType::CoprocessorRegisterXfer;
        return client.Undefined(instruction);
      }

      // return ARMInstrType::SoftwareInterrupt;
      return client.Undefined(instruction);
    }
  }

  return client.Undefined(instruction);
}

} // namespace lunatic::frontend
} // namespace lunatic
