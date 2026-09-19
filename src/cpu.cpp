#include <memory_bus.h>
#include <cpu.h>
#include <cpu_routines.h>
#include <cpu_instructions.h>
#include <cstdint>
#include <stdio.h>
#include <emulator_core.h>

gb_cpu_registers cpu_registers;
uint8_t cpu_current_op_code = 0;
uint32_t cpu_instructions_counter = 0;
cpu_execute_op cpu_current_instruction_execute = nullptr;

void cpu_reset() {
  // After executing boot rom registers should have these values
  cpu_registers.af = 0x1B0;
  cpu_registers.bc = 0x0013;
  cpu_registers.de = 0x00D8;
  cpu_registers.hl = 0x014D;
  cpu_registers.pc = 0x0100;
  cpu_registers.sp = 0xFFFE;
}

void cpu_fetch() {
  // TODO: Read from memory bus  instead of directly fromt rom data
  cpu_current_op_code = memory_bus_read(cpu_registers.pc++);
  const gb_cpu_instruction instruction = instructions[cpu_current_op_code];
  cpu_current_instruction_execute = instruction.execute;
}

bool cpu_execute() {
  if (!cpu_current_instruction_execute) {
    const gb_cpu_instruction &instruction = instructions[cpu_current_op_code];
    const uint8_t pchi = ((cpu_registers.pc - 1) & 0xFF00) >> 8;
    const uint8_t pclo = ((cpu_registers.pc - 1) & 0xFF);
    printf("Unknown instruction %.2X at: %.2X%.2X (%s), count %i\n", cpu_current_op_code, pchi, pclo,
           instruction.disassembly, cpu_instructions_counter);
    return false;
  }
  // actually executes the instruction
  ((cpu_execute_op)cpu_current_instruction_execute)();
  return true;
}

void cpu_noop() // 0x00
{
  core_advance_cpu_clocks(4);
}

void cpu_ld_bc_nn() // 0x01
{
  cpu_routine_ld_16(cpu_registers.b, cpu_registers.c);
}

void cpu_ld_bc_a()  // 0x02
{
  cpu_routine_ld_ptr16_from_8(cpu_registers.bc, cpu_registers.a);
}

void cpu_inc_bc() // 0x03
{
  cpu_routine_inc_16(cpu_registers.bc);
}

void cpu_inc_b()
{
  cpu_routine_inc_8(cpu_registers.b);
}

void cpu_dec_b()  // 0x05
{
  cpu_routine_dec_8(cpu_registers.b);
}

void cpu_ld_b_n() // 0x06
{
  cpu_routine_ld_8(cpu_registers.b);
}

void cpu_add_hl_bc()    // 0x09
{
  cpu_routine_add_hl_16(cpu_registers.bc);
}

void cpu_ld_a_bc()  // 0x0A
{
  cpu_routine_ld_8_from_ptr16(cpu_registers.a, cpu_registers.bc);
}

void cpu_dec_bc()   // 0x0B
{
  cpu_routine_dec_16(cpu_registers.bc);
}

void cpu_inc_c()    // 0x0C
{
  cpu_routine_inc_8(cpu_registers.c);
}

void cpu_dec_c()  // 0x0D
{
  cpu_routine_dec_8(cpu_registers.c);
}

void cpu_ld_c_n() // 0x0E
{
  cpu_routine_ld_8(cpu_registers.c);
}

void cpu_ld_de_nn() // 0x11
{
  cpu_routine_ld_16(cpu_registers.d, cpu_registers.e);
}

void cpu_ld_de_a()  // 0x12
{
  cpu_routine_ld_ptr16_from_8(cpu_registers.de, cpu_registers.a);
}

void cpu_inc_de() // 0x13
{
  cpu_routine_inc_16(cpu_registers.de);
}

void cpu_inc_d()  // 0x14
{
  cpu_routine_inc_8(cpu_registers.d);
}

void cpu_dec_d()  // 0x15
{
  cpu_routine_dec_8(cpu_registers.d);
}

void cpu_ld_d_n() // 0x16
{
  cpu_routine_ld_8(cpu_registers.d);
}

void cpu_jr_e()  // 0x18
{
  cpu_routine_jr(true);
}

void cpu_add_hl_de()  // 0x19
{
  cpu_routine_add_hl_16(cpu_registers.de);
}

void cpu_ld_a_de()  // 0x1A
{
  cpu_routine_ld_8_from_ptr16(cpu_registers.a, cpu_registers.de);
}

void cpu_dec_de()   // 0x1B
{
  cpu_routine_dec_16(cpu_registers.de);
}

void cpu_inc_e()    // 0x1C
{
  cpu_routine_inc_8(cpu_registers.e);
}

void cpu_dec_e()  // 0x1D
{
  cpu_routine_dec_8(cpu_registers.e);
}

void cpu_ld_e_n()   // 0x1E
{
  cpu_routine_ld_8(cpu_registers.e);
}

void cpu_jr_nz_e()    // 0x20
{
    cpu_routine_jr(!GET_FLAG_ZERO);
}

void cpu_ld_hl_nn()   // 0x21
{
  cpu_routine_ld_16(cpu_registers.h, cpu_registers.l);
}

void cpu_inc_hl()   // 0x23
{
  cpu_routine_inc_16(cpu_registers.hl);
}

void cpu_inc_h()  // 0x24
{
  cpu_routine_inc_8(cpu_registers.h);
}

void cpu_dec_h()    // 0x25
{
  cpu_routine_dec_8(cpu_registers.h);
}

void cpu_ld_h_n()   // 0x26
{
  cpu_routine_ld_8(cpu_registers.h);
}

void cpu_jr_z_e()   // 0x28
{
  cpu_routine_jr(GET_FLAG_ZERO);
}

void cpu_dec_hl()   // 0x2B
{
  cpu_routine_dec_16(cpu_registers.hl);
}

void cpu_inc_l()  // 0x2C
{
  cpu_routine_inc_8(cpu_registers.l);
}

void cpu_dec_l()    // 0x2D
{
  cpu_routine_dec_8(cpu_registers.l);
}

void cpu_ld_l_n()   // 0x2E
{
  cpu_routine_ld_8(cpu_registers.l);
}

void cpu_jr_nc_e()  // 0x30
{
  cpu_routine_jr(!GET_FLAG_CARRY);
}

void cpu_ld_sp_nn() // 0x31
{
  cpu_routine_ld_16(cpu_registers.s, cpu_registers.p);
}

void cpu_ldd_hl_a()   // 0x32
{
  core_advance_cpu_clocks(4);
  memory_bus_write(cpu_registers.hl, cpu_registers.a);
  cpu_registers.hl = (cpu_registers.hl - 1) & 0xFFFF;
  core_advance_cpu_clocks(4);
}

void cpu_inc_sp()   // 0x33
{
  cpu_routine_inc_16(cpu_registers.sp);
}

void cpu_jr_c_e()   // 0x38
{
  cpu_routine_jr(GET_FLAG_CARRY);
}

void cpu_add_hl_sp()    // 0x39
{
  cpu_routine_add_hl_16(cpu_registers.sp);
}

void cpu_dec_sp()   // 0x3B
{
  cpu_routine_dec_16(cpu_registers.sp);
}

void cpu_inc_a()  // 0x3C
{
  cpu_routine_inc_8(cpu_registers.a);
}

void cpu_dec_a()  // 0x3D
{
  cpu_routine_dec_8(cpu_registers.a);
}

void cpu_ld_a_n()   // 0x3E
{
  cpu_routine_ld_8(cpu_registers.a);
}

void cpu_ld_b_hl()  // 0x46
{
  cpu_routine_ld_8_from_ptr16(cpu_registers.b, cpu_registers.hl);
}

void cpu_ld_c_hl()  // 0x4E
{
  cpu_routine_ld_8_from_ptr16(cpu_registers.c, cpu_registers.hl);
}

void cpu_ld_d_hl()  // 0x56
{
  cpu_routine_ld_8_from_ptr16(cpu_registers.d, cpu_registers.hl);
}

void cpu_ld_e_hl()  // 0x5E
{
  cpu_routine_ld_8_from_ptr16(cpu_registers.e, cpu_registers.hl);
}

void cpu_ld_h_hl()    // 0x66
{
  cpu_routine_ld_8_from_ptr16(cpu_registers.h, cpu_registers.hl);
}

void cpu_ld_l_hl()  // 0x6E
{
  cpu_routine_ld_8_from_ptr16(cpu_registers.l, cpu_registers.hl);
}

void cpu_ld_hl_b()    // 0x70
{
  cpu_routine_ld_ptr16_from_8(cpu_registers.hl, cpu_registers.b);
}

void cpu_ld_hl_c()  // 0x71
{
cpu_routine_ld_ptr16_from_8(cpu_registers.hl, cpu_registers.c);  
}

void cpu_ld_hl_d()  // 0x72
{
  cpu_routine_ld_ptr16_from_8(cpu_registers.hl, cpu_registers.d);  
}

void cpu_ld_hl_e()  // 0x73
{
  cpu_routine_ld_ptr16_from_8(cpu_registers.hl, cpu_registers.e);  
}

void cpu_ld_hl_h()  // 0x74
{
  cpu_routine_ld_ptr16_from_8(cpu_registers.hl, cpu_registers.h);  
}

void cpu_ld_hl_l()  // 0x75
{
  cpu_routine_ld_ptr16_from_8(cpu_registers.hl, cpu_registers.l);  
}

void cpu_ld_hl_a()  // 0x77
{
  cpu_routine_ld_ptr16_from_8(cpu_registers.hl, cpu_registers.a);  
}

void cpu_ld_a_hl()  // 0x7E
{
  cpu_routine_ld_8_from_ptr16(cpu_registers.a, cpu_registers.hl);
}

void cpu_add_a_b()  // 0x80
{
  cpu_routine_add_a_8(cpu_registers.b);
}

void cpu_add_a_c()  // 0x81
{
  cpu_routine_add_a_8(cpu_registers.c);
}

void cpu_add_a_d()  // 0x82
{
  cpu_routine_add_a_8(cpu_registers.d);
}

void cpu_add_a_e()  // 0x83
{
  cpu_routine_add_a_8(cpu_registers.e);
}

void cpu_add_a_h()  // 0x84
{
  cpu_routine_add_a_8(cpu_registers.h);
}

void cpu_add_a_l()  // 0x85
{
  cpu_routine_add_a_8(cpu_registers.l);
}

void cpu_adc_a_b()  // 0x88
{
  cpu_routine_adc_a_8(cpu_registers.b);
}

void cpu_adc_a_c()  // 0x89
{
  cpu_routine_adc_a_8(cpu_registers.c);
}

void cpu_adc_a_d()  // 0x8a
{
  cpu_routine_adc_a_8(cpu_registers.d);
}

void cpu_adc_a_e()  // 0x8b
{
  cpu_routine_adc_a_8(cpu_registers.e);
}

void cpu_adc_a_h()  // 0x8c
{
  cpu_routine_adc_a_8(cpu_registers.h);
}

void cpu_adc_a_l()  // 0x8d
{
  cpu_routine_adc_a_8(cpu_registers.l);
}

void cpu_sub_a_b()  // 0x90
{
  cpu_routine_sub_a_8(cpu_registers.b);
}

void cpu_sub_a_c()  // 0x91
{
  cpu_routine_sub_a_8(cpu_registers.c);
}

void cpu_sub_a_d()  // 0x92
{
  cpu_routine_sub_a_8(cpu_registers.d);
}

void cpu_sub_a_e()  // 0x93
{
  cpu_routine_sub_a_8(cpu_registers.e);
}

void cpu_sub_a_h()  // 0x94
{
  cpu_routine_sub_a_8(cpu_registers.h);
}

void cpu_sub_a_l()  // 0x95
{
  cpu_routine_sub_a_8(cpu_registers.l);
}

void cpu_sbc_a_b()        // 0x98
{
  cpu_routine_sbc_a_8(cpu_registers.b);
}

void cpu_sbc_a_c()        // 0x99
{
  cpu_routine_sbc_a_8(cpu_registers.c);
}

void cpu_sbc_a_d()        // 0x9A
{
  cpu_routine_sbc_a_8(cpu_registers.d);
}

void cpu_sbc_a_e()        // 0x9B
{
  cpu_routine_sbc_a_8(cpu_registers.e);
}

void cpu_sbc_a_h()        // 0x9C
{
  cpu_routine_sbc_a_8(cpu_registers.h);
}

void cpu_sbc_a_l()        // 0x9D
{
  cpu_routine_sbc_a_8(cpu_registers.l);
}


void cpu_and_a_b()         // 0xA0
{
  cpu_routine_and_a_8(cpu_registers.b);
}

void cpu_and_a_c()         // 0xA1
{
  cpu_routine_and_a_8(cpu_registers.c);
}

void cpu_and_a_d()         // 0xA2
{
  cpu_routine_and_a_8(cpu_registers.d);
}

void cpu_and_a_e()         // 0xA3
{
  cpu_routine_and_a_8(cpu_registers.e);
}

void cpu_and_a_h()         // 0xA4
{
  cpu_routine_and_a_8(cpu_registers.h);
}

void cpu_and_a_l()         // 0xA5
{
  cpu_routine_and_a_8(cpu_registers.l);
}


void cpu_xor_a()  // 0xAF
{
  cpu_registers.a = 0;
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_CARRY(0);
  SET_FLAG_HALF_CARRY(0);
  SET_FLAG_ZERO(1);
  core_advance_cpu_clocks(4);

}

void cpu_jp_nn()    // 0xC3
{
  core_advance_cpu_clocks(4);
  uint32_t temp = memory_bus_read(cpu_registers.pc++);
  cpu_registers.pc &= 0xFFFF;
  core_advance_cpu_clocks(4);
  temp |= ((uint32_t)memory_bus_read(cpu_registers.pc)) << 8;
  cpu_registers.pc &= 0xFFFF;
  core_advance_cpu_clocks(4);
  cpu_registers.pc = temp;
  core_advance_cpu_clocks(4);
}
