#pragma once
#include <emulator_core.h>
#include <memory_bus.h>
#include <cpu.h>

#define cpu_routine_add_hl_16(reg16)                                     \
{                                                                       \
  SET_FLAG_SUBTRACT(0);                                                 \
  uint32_t temp = cpu_registers.hl + reg16;                             \
  SET_FLAG_CARRY(temp > 0xFFFF)                                         \
  bool hc = ((cpu_registers.hl & 0x0FFF) + (reg16 + 0x0FFF)) > 0x0FFF;  \
  SET_FLAG_HALF_CARRY(hc);                                              \
  core_advance_cpu_clocks(4);                                           \
  cpu_registers.hl = temp & 0xFFFF;                                     \
  core_advance_cpu_clocks(4);                                           \
}

#define cpu_routine_add_a_8(reg8)                                       \
{                                                                      \
  SET_FLAG_SUBTRACT(0);                                                \
  uint16_t temp = (uint16_t)cpu_registers.a + (uint16_t)reg8;          \
  bool hc = (((cpu_registers.a & 0x0F) + (reg8 & 0x0F)) > 0x0F);       \
  SET_FLAG_HALF_CARRY(hc);                                             \
  SET_FLAG_CARRY(temp > 0xFF);                                         \
  cpu_registers.a = (uint8_t)temp;                                     \
  SET_FLAG_ZERO(cpu_registers.a == 0);                                 \
  core_advance_cpu_clocks(4);                                          \
}

#define cpu_routine_adc_a_8(reg8)                                                         \
{                                                                                        \
  SET_FLAG_SUBTRACT(0);                                                                  \
  uint8_t carry = GET_FLAG_CARRY;                                                        \
  uint16_t temp = (uint16_t)cpu_registers.a + (uint16_t)reg8 + (uint16_t)carry;          \
  bool hc = (((cpu_registers.a & 0x0F) + (reg8 & 0x0F) + carry) > 0x0F);                 \
  SET_FLAG_HALF_CARRY(hc);                                                               \
  SET_FLAG_CARRY(temp > 0xFF);                                                           \
  cpu_registers.a = (uint8_t)temp;                                                       \
  SET_FLAG_ZERO(cpu_registers.a == 0);                                                   \
  core_advance_cpu_clocks(4);                                                            \
}

#define cpu_routine_inc_16(reg16)    \
{                                   \
  core_advance_cpu_clocks(4);       \
  reg16 = (reg16 + 1);              \
  core_advance_cpu_clocks(4);       \
}

#define cpu_routine_dec_16(reg16)    \
{                                   \
  core_advance_cpu_clocks(4);       \
  reg16 = (reg16 - 1);              \
  core_advance_cpu_clocks(4);       \
}

#define cpu_routine_inc_8(reg8)                      \
{                                                   \
    SET_FLAG_SUBTRACT(0);                           \
    SET_FLAG_HALF_CARRY((reg8 & 0xF) == 0xF);       \
    reg8++;                                         \
    SET_FLAG_ZERO(reg8 == 0);                       \
    core_advance_cpu_clocks(4);                     \
}

#define cpu_routine_dec_8(reg8)                      \
{                                                   \
    SET_FLAG_SUBTRACT(1);                           \
    SET_FLAG_HALF_CARRY((reg8 & 0xF) == 0x0);       \
    reg8--;                                         \
    SET_FLAG_ZERO(reg8 == 0);                       \
    core_advance_cpu_clocks(4);                     \
}

#define cpu_routine_ld_16(reg_hi, reg_lo)            \
{                                                   \
    core_advance_cpu_clocks(4);                     \
    reg_lo = memory_bus_read(cpu_registers.pc++);   \
    core_advance_cpu_clocks(4);                     \
    reg_hi = memory_bus_read(cpu_registers.pc++);   \
    core_advance_cpu_clocks(4);                     \
}

#define cpu_routine_ld_8(reg8)                         \
{                                                     \
    core_advance_cpu_clocks(4);                       \
    reg8 = memory_bus_read(cpu_registers.pc++);       \
    core_advance_cpu_clocks(4);                       \
}

#define cpu_routine_ld_8_from_ptr16(reg8, reg16)    \
{                                                  \
  core_advance_cpu_clocks(4);                      \
  reg8 = memory_bus_read(reg16);                   \
  core_advance_cpu_clocks(4);                      \
}

#define cpu_routine_ld_ptr16_from_8(reg16, reg8) \
{                                               \
  core_advance_cpu_clocks(4);                   \
  memory_bus_write(reg16, reg8);                \
  core_advance_cpu_clocks(4);                   \
}

#define cpu_routine_jr(condition)                                  \
{                                                                 \
  core_advance_cpu_clocks(4);                                     \
  int8_t offset = (int8_t)(memory_bus_read(cpu_registers.pc++));  \
  core_advance_cpu_clocks(4);                                     \
  if (condition) {                                                \
    cpu_registers.pc = (uint16_t)(cpu_registers.pc + offset);     \
    core_advance_cpu_clocks(4);                                   \
  }                                                               \
}