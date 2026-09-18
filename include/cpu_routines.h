#pragma once
#include <emulator_core.h>
#include <memory_bus.h>
#include <cpu.h>

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
    SET_FLAG_HALF_CARRY((reg8 & 0xF) == 0x0);       \
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
}

#define cpu_routine_ld_8(reg8)                         \
{                                                     \
    core_advance_cpu_clocks(4);                       \
    reg8 = memory_bus_read(cpu_registers.pc++);       \
    core_advance_cpu_clocks(4);                       \
}

#define cpu_routine_ld_ptr16(reg8, reg16)  \
{                                         \
  core_advance_cpu_clocks(4);             \
  reg8 = memory_bus_read(reg16);          \
  core_advance_cpu_clocks(4);             \
}

#define cpu_routine_ld_ptr8(reg16, reg8) \
{                                       \
  core_advance_cpu_clocks(4);           \
  memory_bus_write(reg16, reg8);        \
  core_advance_cpu_clocks(4);           \
}

#define cpu_routine_jr(condition)                                  \
{                                                                 \
  core_advance_cpu_clocks(4);                                     \
  int8_t offset = (int8_t)(memory_bus_read(cpu_registers.pc++));  \
  core_advance_cpu_clocks(4);                                     \
  if (condition) {                                                \
    cpu_registers.pc = (int16_t)(cpu_registers.pc + offset);      \
  }                                                               \
  core_advance_cpu_clocks(4);                                     \
}
