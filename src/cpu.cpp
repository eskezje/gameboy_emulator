#include "interrupts.h"
#include <timer.h>
#include <cstdio>
#include <memory_bus.h>
#include <cpu.h>
#include <cpu_routines.h>
#include <cpu_instructions.h>
#include <stdint.h>
#include <stdio.h>
#include <emulator_core.h>

gb_cpu_registers cpu_registers;
uint8_t cpu_current_op_code = 0;
uint32_t cpu_instructions_counter = 0;
cpu_execute_op cpu_current_instruction_execute = nullptr;
uint8_t cpu_halt_count = 0; // 0 == not halted, 1 == halt instruction, 2 == stop instruction
bool cpu_halt_bug =false;

extern bool core_quit_requested;

void cpu_reset() {
  // After executing boot rom registers should have these values
  cpu_registers.af = 0x1B0;
  cpu_registers.bc = 0x0013;
  cpu_registers.de = 0x00D8;
  cpu_registers.hl = 0x014D;
  cpu_registers.pc = 0x0100;
  cpu_registers.sp = 0xFFFE;
}

void cpu_tick()
{
  if (cpu_halt_count == 0) {
    cpu_fetch();
    if (!cpu_execute()) {
      core_quit_requested = true;
      return;
    }
    cpu_instructions_counter++;
  }
  else {
    core_advance_cpu_clocks(4); // halted, waiting for an interrupt to trigger

  }

  interrupt_service_routine();
}

void cpu_fetch() {
  // TODO: Read from memory bus  instead of directly fromt rom data
  
  cpu_current_op_code = memory_bus_read(cpu_registers.pc++);
  const bool is_extended_cb_instruction = cpu_current_op_code == 0xCB;

  if (cpu_halt_bug) {
    cpu_registers.pc--; // repeat one byte during halt bug
    cpu_halt_bug = false;
  }

  if (is_extended_cb_instruction) {
    core_advance_cpu_clocks(4);
    const uint8_t cpu_current_op_code_cb = memory_bus_read(cpu_registers.pc++);
    const gb_cpu_pre_cb_instruction& cb_instruction = cb_instructions[cpu_current_op_code_cb];
    cpu_current_instruction_execute = cb_instruction.execute;
  }
  else {
    const gb_cpu_instruction& instruction = instructions[cpu_current_op_code];
    cpu_current_instruction_execute = instruction.execute;
  }

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

void cpu_rlca()     // 0x07
{
  core_advance_cpu_clocks(4);
  SET_FLAG_CARRY((0b10000000 & cpu_registers.a)>>7);
  SET_FLAG_ZERO(0);
  SET_FLAG_HALF_CARRY(0);
  SET_FLAG_SUBTRACT(0);
  cpu_registers.a = (cpu_registers.a << 1) | GET_FLAG_CARRY;
}

void cpu_ld_nn_sp()     // 0x08
{
  core_advance_cpu_clocks(4);
  uint8_t nn_lsb = memory_bus_read(cpu_registers.pc++);
  core_advance_cpu_clocks(4);
  uint8_t nn_msb = memory_bus_read(cpu_registers.pc++);
  core_advance_cpu_clocks(4);
  uint16_t nn = (uint16_t)(nn_msb << 8) | (uint16_t)nn_lsb;
  core_advance_cpu_clocks(4);
  uint8_t lsb_sp = (cpu_registers.sp & 0x00FF);
  uint8_t msb_sp = ((cpu_registers.sp & 0xFF00) >> 8);
  memory_bus_write(nn++, lsb_sp);
  core_advance_cpu_clocks(4);
  memory_bus_write(nn, msb_sp);
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

void cpu_rrca()     // 0x0F
{
  core_advance_cpu_clocks(4);
  SET_FLAG_CARRY(0b00000001 & cpu_registers.a);
  SET_FLAG_ZERO(0);
  SET_FLAG_HALF_CARRY(0);
  SET_FLAG_SUBTRACT(0);
  cpu_registers.a = (cpu_registers.a >> 1) | (GET_FLAG_CARRY << 7);
}

void cpu_stop()     // 0x10
{
  core_advance_cpu_clocks(4);
  if (memory_bus_read(cpu_registers.pc++) != 0) {
    printf("CPU - Corrupted STOP at PC: %04X, should have operand 0x00\n", cpu_registers.pc);
  }
  core_advance_cpu_clocks(4);
  timer_on_div_write(0);
  cpu_halt_count = 2;
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

void cpu_rla()  // 0x17
{
  core_advance_cpu_clocks(4);
  uint8_t bit7 = (cpu_registers.a & 0b10000000) >> 7;
  cpu_registers.a = (cpu_registers.a << 1) | GET_FLAG_CARRY;
  SET_FLAG_CARRY(bit7);
  SET_FLAG_ZERO(0);
  SET_FLAG_HALF_CARRY(0);
  SET_FLAG_SUBTRACT(0);
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

void cpu_rra()  // 0x1F
{
  core_advance_cpu_clocks(4);
  uint8_t bit1 = (cpu_registers.a & 0b00000001);
  cpu_registers.a = (cpu_registers.a >> 1) | (GET_FLAG_CARRY << 7);
  SET_FLAG_CARRY(bit1);
  SET_FLAG_ZERO(0);
  SET_FLAG_HALF_CARRY(0);
  SET_FLAG_SUBTRACT(0);
}

void cpu_jr_nz_e()    // 0x20
{
    cpu_routine_jr(!GET_FLAG_ZERO);
}

void cpu_ld_hl_nn()   // 0x21
{
  cpu_routine_ld_16(cpu_registers.h, cpu_registers.l);
}


void cpu_ldi_hl_a()   // 0x22
{
  core_advance_cpu_clocks(4);
  memory_bus_write(cpu_registers.hl, cpu_registers.a);
  cpu_registers.hl = (cpu_registers.hl + 1) & 0xFFFF;
  core_advance_cpu_clocks(4);
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
void cpu_daa()             // 0x27
{
  core_advance_cpu_clocks(4);
  if (!GET_FLAG_SUBTRACT) {
    // after an addition, adjust if (half-)carry occured if result is out of bounds
    if (GET_FLAG_CARRY || cpu_registers.a > 0x99) {
      cpu_registers.a += 0x60;
      SET_FLAG_CARRY(1);
    }
    if (GET_FLAG_HALF_CARRY || (cpu_registers.a & 0x0F) > 0x09) {
      cpu_registers.a += 0x6;
    }
  }
  else {
    if (GET_FLAG_CARRY) {
      cpu_registers.a -= 0x60;
    }
    if (GET_FLAG_HALF_CARRY) {
      cpu_registers.a -= 0x6;
    }
  }
  SET_FLAG_ZERO(cpu_registers.a == 0);
  SET_FLAG_HALF_CARRY(0);
}

void cpu_jr_z_e()   // 0x28
{
  cpu_routine_jr(GET_FLAG_ZERO);
}

void cpu_add_hl_hl()    // 0x29
{
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_CARRY((cpu_registers.hl & 0x8000) != 0);
  SET_FLAG_HALF_CARRY((cpu_registers.hl & 0x0800) != 0);
  core_advance_cpu_clocks(4);
  cpu_registers.hl = (cpu_registers.hl << 1) & 0xFFFF;
}

void cpu_ldi_a_hl()     // 0x2A
{
  core_advance_cpu_clocks(4);
  cpu_registers.a = memory_bus_read(cpu_registers.hl);
  core_advance_cpu_clocks(4);
  cpu_registers.hl = (cpu_registers.hl + 1) & 0xFFFF;
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

void cpu_cpl()  // 0x2F
{
  core_advance_cpu_clocks(4);
  cpu_registers.a = ~(cpu_registers.a);
  SET_FLAG_SUBTRACT(1);
  SET_FLAG_HALF_CARRY(1);
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

void cpu_inc__hl()   // 0x34
{
  core_advance_cpu_clocks(4);
  uint32_t data = memory_bus_read(cpu_registers.hl);
  uint32_t result = data + 1;
  core_advance_cpu_clocks(4);
  memory_bus_write(cpu_registers.hl, result & 0xFF);
  core_advance_cpu_clocks(4);
  SET_FLAG_ZERO((result & 0xFF) == 0);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY((data & 0b1111) == 0b1111);
}

void cpu_dec__hl()   // 0x35
{
  core_advance_cpu_clocks(4);
  uint32_t data = memory_bus_read(cpu_registers.hl);
  uint32_t result = data - 1;
  core_advance_cpu_clocks(4);
  memory_bus_write(cpu_registers.hl, result & 0xFF);
  core_advance_cpu_clocks(4);
  SET_FLAG_ZERO((result & 0xFF) == 0);
  SET_FLAG_SUBTRACT(1);
  SET_FLAG_HALF_CARRY((data & 0x0F) == 0);
}

void cpu_ld_hl_n()  // 0x36
{
  core_advance_cpu_clocks(4);
  uint8_t n = memory_bus_read(cpu_registers.pc++);
  core_advance_cpu_clocks(4);
  memory_bus_write(cpu_registers.hl, n);
  core_advance_cpu_clocks(4);
}

void cpu_scf()  // 0x37
{
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(0);
  SET_FLAG_CARRY(1);
}

void cpu_jr_c_e()   // 0x38
{
  cpu_routine_jr(GET_FLAG_CARRY);
}

void cpu_add_hl_sp()    // 0x39
{
  cpu_routine_add_hl_16(cpu_registers.sp);
}

void cpu_ldd_a_hl()        // 0x3A
{
  core_advance_cpu_clocks(4);
  cpu_registers.a = memory_bus_read(cpu_registers.hl);
  core_advance_cpu_clocks(4);
  cpu_registers.hl = (cpu_registers.hl - 1) & 0xFFFF;
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

void cpu_ccf()             // 0x3F
{
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(0);
  SET_FLAG_CARRY(!GET_FLAG_CARRY);
}


void cpu_ld_b_b()          // 0x40
{
  core_advance_cpu_clocks(4);
}

void cpu_ld_b_c()          // 0x41
{
  core_advance_cpu_clocks(4);
  cpu_registers.b = cpu_registers.c;
}

void cpu_ld_b_d()          // 0x42
{
  core_advance_cpu_clocks(4);
  cpu_registers.b = cpu_registers.d;
}

void cpu_ld_b_e()          // 0x43
{
  core_advance_cpu_clocks(4);
  cpu_registers.b = cpu_registers.e;
}

void cpu_ld_b_h()          // 0x44
{
  core_advance_cpu_clocks(4);
  cpu_registers.b = cpu_registers.h;
}

void cpu_ld_b_l()          // 0x45
{
  core_advance_cpu_clocks(4);
  cpu_registers.b = cpu_registers.l;
}

void cpu_ld_b_hl()  // 0x46
{
  cpu_routine_ld_8_from_ptr16(cpu_registers.b, cpu_registers.hl);
}

void cpu_ld_b_a()          // 0x47
{
  core_advance_cpu_clocks(4);
  cpu_registers.b = cpu_registers.a;
}


void cpu_ld_c_b()          // 0x48
{
  core_advance_cpu_clocks(4);
  cpu_registers.c = cpu_registers.b;
}

void cpu_ld_c_c()          // 0x49
{
  core_advance_cpu_clocks(4);
}

void cpu_ld_c_d()          // 0x4A
{
  core_advance_cpu_clocks(4);
  cpu_registers.c = cpu_registers.d;
}

void cpu_ld_c_e()          // 0x4B
{
  core_advance_cpu_clocks(4);
  cpu_registers.c = cpu_registers.e;
}

void cpu_ld_c_h()          // 0x4C
{
  core_advance_cpu_clocks(4);
  cpu_registers.c = cpu_registers.h;
}

void cpu_ld_c_l()          // 0x4D
{
  core_advance_cpu_clocks(4);
  cpu_registers.c = cpu_registers.l;
}



void cpu_ld_c_hl()  // 0x4E
{
  cpu_routine_ld_8_from_ptr16(cpu_registers.c, cpu_registers.hl);
}

void cpu_ld_c_a()          // 0x4F
{
  core_advance_cpu_clocks(4);
  cpu_registers.c = cpu_registers.a;
}

void cpu_ld_d_b()          // 0x50
{
  core_advance_cpu_clocks(4);
  cpu_registers.d = cpu_registers.b;
}

void cpu_ld_d_c()          // 0x51
{
  core_advance_cpu_clocks(4);
  cpu_registers.d = cpu_registers.c;
}

void cpu_ld_d_d()          // 0x52
{
  core_advance_cpu_clocks(4);
}

void cpu_ld_d_e()          // 0x53
{
  core_advance_cpu_clocks(4);
  cpu_registers.d = cpu_registers.e;
}

void cpu_ld_d_h()          // 0x54
{
  core_advance_cpu_clocks(4);
  cpu_registers.d = cpu_registers.h;
}

void cpu_ld_d_l()          // 0x55
{
  core_advance_cpu_clocks(4);
  cpu_registers.d = cpu_registers.l;
}


void cpu_ld_d_hl()  // 0x56
{
  cpu_routine_ld_8_from_ptr16(cpu_registers.d, cpu_registers.hl);
}

void cpu_ld_d_a()          // 0x57
{
  core_advance_cpu_clocks(4);
  cpu_registers.d = cpu_registers.a;
}


void cpu_ld_e_b()          // 0x58
{
  core_advance_cpu_clocks(4);
  cpu_registers.e = cpu_registers.b;
}

void cpu_ld_e_c()          // 0x59
{
  core_advance_cpu_clocks(4);
  cpu_registers.e = cpu_registers.c;
}

void cpu_ld_e_d()          // 0x5A
{
  core_advance_cpu_clocks(4);
  cpu_registers.e = cpu_registers.d;
}

void cpu_ld_e_e()          // 0x5B
{
  core_advance_cpu_clocks(4);
}

void cpu_ld_e_h()          // 0x5C
{
  core_advance_cpu_clocks(4);
  cpu_registers.e = cpu_registers.h;
}

void cpu_ld_e_l()          // 0x5D
{
  core_advance_cpu_clocks(4);
  cpu_registers.e = cpu_registers.l;
}



void cpu_ld_e_hl()  // 0x5E
{
  cpu_routine_ld_8_from_ptr16(cpu_registers.e, cpu_registers.hl);
}

void cpu_ld_e_a()          // 0x5F
{
  core_advance_cpu_clocks(4);
  cpu_registers.e = cpu_registers.a;
}

void cpu_ld_h_b()          // 0x60
{
  core_advance_cpu_clocks(4);
  cpu_registers.h = cpu_registers.b;
}

void cpu_ld_h_c()          // 0x61
{
  core_advance_cpu_clocks(4);
  cpu_registers.h = cpu_registers.c;
}

void cpu_ld_h_d()          // 0x62
{
  core_advance_cpu_clocks(4);
  cpu_registers.h = cpu_registers.d;
}

void cpu_ld_h_e()          // 0x63
{
  core_advance_cpu_clocks(4);
  cpu_registers.h = cpu_registers.e;
}

void cpu_ld_h_h()          // 0x64
{
  core_advance_cpu_clocks(4);
}

void cpu_ld_h_l()          // 0x65
{
  core_advance_cpu_clocks(4);
  cpu_registers.h = cpu_registers.l;
}


void cpu_ld_h_hl()    // 0x66
{
  cpu_routine_ld_8_from_ptr16(cpu_registers.h, cpu_registers.hl);
}

void cpu_ld_h_a()          // 0x67
{
  core_advance_cpu_clocks(4);
  cpu_registers.h = cpu_registers.a;
}

void cpu_ld_l_b()          // 0x68
{
  core_advance_cpu_clocks(4);
  cpu_registers.l = cpu_registers.b;
}

void cpu_ld_l_c()          // 0x69
{
  core_advance_cpu_clocks(4);
  cpu_registers.l = cpu_registers.c;
}

void cpu_ld_l_d()          // 0x6A
{
  core_advance_cpu_clocks(4);
  cpu_registers.l = cpu_registers.d;
}

void cpu_ld_l_e()          // 0x6B
{
  core_advance_cpu_clocks(4);
  cpu_registers.l = cpu_registers.e;
}

void cpu_ld_l_h()          // 0x6C
{
  core_advance_cpu_clocks(4);
  cpu_registers.l = cpu_registers.h;
}

void cpu_ld_l_l()          // 0x6D
{
  core_advance_cpu_clocks(4);
}


void cpu_ld_l_hl()  // 0x6E
{
  cpu_routine_ld_8_from_ptr16(cpu_registers.l, cpu_registers.hl);
}

void cpu_ld_l_a()          // 0x6F
{
  core_advance_cpu_clocks(4);
  cpu_registers.l = cpu_registers.a;
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

void cpu_halt() // 0x76
{
  core_advance_cpu_clocks(4);
  const uint8_t interrupt_enable = memory_bus_read(ADDR_IO_IE);
  const uint8_t interrupt_flag = memory_bus_read(ADDR_IO_IF);
  const bool interrupt_pending = ((interrupt_enable & interrupt_flag) & 0x1F) != 0;
  if (!interrupt_master_enable && interrupt_pending != 0) {
    cpu_halt_bug = true;
  }
  else {
    cpu_halt_count = 1;
  }
}

void cpu_ld_hl_a()  // 0x77
{
  cpu_routine_ld_ptr16_from_8(cpu_registers.hl, cpu_registers.a);  
}

void cpu_ld_a_b()          // 0x78
{
  core_advance_cpu_clocks(4);
  cpu_registers.a = cpu_registers.b;
}

void cpu_ld_a_c()          // 0x79
{
  core_advance_cpu_clocks(4);
  cpu_registers.a = cpu_registers.c;
}

void cpu_ld_a_d()          // 0x7A
{
  core_advance_cpu_clocks(4);
  cpu_registers.a = cpu_registers.d;
}

void cpu_ld_a_e()          // 0x7B
{
  core_advance_cpu_clocks(4);
  cpu_registers.a = cpu_registers.e;
}

void cpu_ld_a_h()          // 0x7C
{
  core_advance_cpu_clocks(4);
  cpu_registers.a = cpu_registers.h;
}

void cpu_ld_a_l()          // 0x7D
{
  core_advance_cpu_clocks(4);
  cpu_registers.a = cpu_registers.l;
}



void cpu_ld_a_hl()  // 0x7E
{
  cpu_routine_ld_8_from_ptr16(cpu_registers.a, cpu_registers.hl);
}

void cpu_ld_a_a()          // 0x7F
{
  core_advance_cpu_clocks(4);
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

void cpu_add_a_hl() // 0x86
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  bool hc = ((data & 0xF) + (cpu_registers.a & 0xF)) > 0xF;
  SET_FLAG_HALF_CARRY(hc);
  SET_FLAG_SUBTRACT(0);
  core_advance_cpu_clocks(4);
  bool carry = ((data & 0xFF) + (cpu_registers.a & 0xFF)) > 0xFF;
  SET_FLAG_CARRY(carry);
  uint8_t result = data + cpu_registers.a;
  cpu_registers.a = result;
  SET_FLAG_ZERO(!cpu_registers.a);
}

void cpu_add_a_a()      // 0x87
{
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY((cpu_registers.a & 0xF)>= 0b00001000);
  SET_FLAG_CARRY(cpu_registers.a >= 0b10000000);
  cpu_registers.a = cpu_registers.a << 1;
  SET_FLAG_ZERO(!cpu_registers.a);
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

void cpu_adc_a_hl() // 0x8E
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  bool hc = ((data & 0xF) + (cpu_registers.a & 0xF) + GET_FLAG_CARRY) > 0xF;
  SET_FLAG_HALF_CARRY(hc);
  SET_FLAG_SUBTRACT(0);
  core_advance_cpu_clocks(4);
  bool carry = ((data & 0xFF) + (cpu_registers.a & 0xFF) + GET_FLAG_CARRY) > 0xFF;
  uint8_t result = data + cpu_registers.a + GET_FLAG_CARRY;
  SET_FLAG_CARRY(carry);
  cpu_registers.a = result;
  SET_FLAG_ZERO(!cpu_registers.a);
}

void cpu_adc_a_a() // 0x8F
{
  core_advance_cpu_clocks(4);
  uint8_t carry = GET_FLAG_CARRY;
  uint16_t result = (uint16_t)cpu_registers.a + (uint16_t)cpu_registers.a + carry;
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(((cpu_registers.a & 0xF) + (cpu_registers.a & 0xF) + carry) > 0xF);
  SET_FLAG_CARRY(result > 0xFF);
  cpu_registers.a = (uint8_t)result;
  SET_FLAG_ZERO(!cpu_registers.a);
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

void cpu_sub_a_hl() // 0x96
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  cpu_routine_sub_a_8(data);
}

void cpu_sub_a_a()  // 0x97 
{
  core_advance_cpu_clocks(4);
  SET_FLAG_ZERO(1);
  SET_FLAG_SUBTRACT(1);
  SET_FLAG_HALF_CARRY(0);
  SET_FLAG_CARRY(0);
  cpu_registers.a = 0;
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

void cpu_sbc_a_hl() // 0x9E
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  cpu_routine_sbc_a_8(data);
}

void cpu_sbc_a_a()        // 0x9F
{
  cpu_routine_sbc_a_8(cpu_registers.a);
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

void cpu_and_a_hl()     // 0xA6
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  cpu_routine_and_a_8(data);
}

void cpu_and_a_a()         // 0xA7
{
  cpu_routine_and_a_8(cpu_registers.a);
}


void cpu_xor_a_b()         // 0xA8
{
  cpu_routine_xor_a_8(cpu_registers.b);
}

void cpu_xor_a_c()         // 0xA9
{
  cpu_routine_xor_a_8(cpu_registers.c);
}

void cpu_xor_a_d()         // 0xAA
{
  cpu_routine_xor_a_8(cpu_registers.d);
}

void cpu_xor_a_e()         // 0xAB
{
  cpu_routine_xor_a_8(cpu_registers.e);
}

void cpu_xor_a_h()         // 0xAC
{
  cpu_routine_xor_a_8(cpu_registers.h);
}

void cpu_xor_a_l()         // 0xAD
{
  cpu_routine_xor_a_8(cpu_registers.l);
}

void cpu_xor_a_hl()         // 0xAE
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  cpu_routine_xor_a_8(data);
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

void cpu_or_a_b()          // 0xB0
{
  cpu_routine_or_a_8(cpu_registers.b);
}

void cpu_or_a_c()          // 0xB1
{
  cpu_routine_or_a_8(cpu_registers.c);
}

void cpu_or_a_d()          // 0xB2
{
  cpu_routine_or_a_8(cpu_registers.d);
}

void cpu_or_a_e()          // 0xB3
{
  cpu_routine_or_a_8(cpu_registers.e);
}

void cpu_or_a_h()          // 0xB4
{
  cpu_routine_or_a_8(cpu_registers.h);
}

void cpu_or_a_l()          // 0xB5
{
  cpu_routine_or_a_8(cpu_registers.l);
}

void cpu_or_a_hl()          // 0xB6
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  cpu_routine_or_a_8(data);
}

void cpu_or_a_a()          // 0xB7
{
  cpu_routine_or_a_8(cpu_registers.a);
}

void cpu_cp_a_b()          // 0xB8
{
  cpu_routine_cp_a_8(cpu_registers.b);
}

void cpu_cp_a_c()          // 0xB9
{
  cpu_routine_cp_a_8(cpu_registers.c);
}

void cpu_cp_a_d()          // 0xBA
{
  cpu_routine_cp_a_8(cpu_registers.d);
}

void cpu_cp_a_e()          // 0xBB
{
  cpu_routine_cp_a_8(cpu_registers.e);
}

void cpu_cp_a_h()          // 0xBC
{
  cpu_routine_cp_a_8(cpu_registers.h);
}

void cpu_cp_a_l()          // 0xBD
{
  cpu_routine_cp_a_8(cpu_registers.l);
}

void cpu_cp_a_hl()      // 0xBE
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  cpu_routine_cp_a_8(data);
}

void cpu_cp_a_a()          // 0xBF
{
  cpu_routine_cp_a_8(cpu_registers.a);
}

void cpu_ret_nz()    // 0xC0
{
  cpu_routine_return_conditional(!GET_FLAG_ZERO);
}

void cpu_pop_bc()   // 0xC1
{
  cpu_routine_pop_16(cpu_registers.b, cpu_registers.c);
}

void cpu_jp_nz()    // 0xC2
{
  cpu_routine_jump_conditional(!GET_FLAG_ZERO);
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

void cpu_call_nz()      // 0xC4
{
  cpu_routine_call_conditional_nn(!GET_FLAG_ZERO);
}

void cpu_push_bc()  // 0xC5
{
  cpu_routine_push_16(cpu_registers.b, cpu_registers.c);
}

void cpu_add_a_n()  // 0xC6
{
  core_advance_cpu_clocks(4);
  uint8_t n = memory_bus_read(cpu_registers.pc);
  uint8_t temp = cpu_registers.a;
  cpu_registers.pc++;
  uint16_t result = cpu_registers.a + n;
  core_advance_cpu_clocks(4);
  cpu_registers.a = (uint8_t)result;
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_ZERO(!cpu_registers.a);
  SET_FLAG_HALF_CARRY(((temp & 0b00001111) + (n & 0b00001111)) >= 0b00010000); 
  SET_FLAG_CARRY(result > 0xFF);
}

void cpu_rst_00()          // 0xC7
{
  cpu_routine_rst(0x00);
}

void cpu_ret_z()    // 0xC8
{
  cpu_routine_return_conditional(GET_FLAG_ZERO);
}

void cpu_ret()      // 0xC9
{
  core_advance_cpu_clocks(4);
  uint8_t lsb = memory_bus_read(cpu_registers.sp++);
  core_advance_cpu_clocks(4);
  uint8_t msb = memory_bus_read(cpu_registers.sp++);
  core_advance_cpu_clocks(4);
  cpu_registers.pc = (uint16_t)(lsb | (msb << 8));
  core_advance_cpu_clocks(4);
}

void cpu_jp_z()    // 0xCA
{
  cpu_routine_jump_conditional(GET_FLAG_ZERO);
}

void cpu_call_z()      // 0xCC
{
  cpu_routine_call_conditional_nn(GET_FLAG_ZERO);
}

void cpu_call_nn()  // 0xCD
{
  core_advance_cpu_clocks(4);
  uint8_t nn_lsb = memory_bus_read(cpu_registers.pc++);
  core_advance_cpu_clocks(4);
  uint8_t nn_msb = memory_bus_read(cpu_registers.pc++);
  core_advance_cpu_clocks(4);
  uint16_t nn = (uint16_t)(nn_lsb | (nn_msb << 8));
  core_advance_cpu_clocks(4);

  cpu_registers.sp--;
  memory_bus_write(cpu_registers.sp, (uint8_t)(cpu_registers.pc >> 8));
  core_advance_cpu_clocks(4);

  cpu_registers.sp--;
  memory_bus_write(cpu_registers.sp, (uint8_t)(cpu_registers.pc));
  core_advance_cpu_clocks(4);
  cpu_registers.pc = nn;
}

void cpu_adc_n()    // 0xCE
{
  core_advance_cpu_clocks(4);
  uint8_t n = memory_bus_read(cpu_registers.pc++);
  uint8_t temp = cpu_registers.a;
  uint16_t result = cpu_registers.a + n + GET_FLAG_CARRY;
  cpu_registers.a = (uint8_t)result;
  core_advance_cpu_clocks(4);
  SET_FLAG_ZERO(!cpu_registers.a);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(((temp & 0b00001111) + (n & 0b00001111) + GET_FLAG_CARRY) >= 0b00010000);
  SET_FLAG_CARRY(result > 0xFF);
}

void cpu_rst_08()          // 0xCF
{
  cpu_routine_rst(0x08);
}

void cpu_ret_nc()    // 0xD0
{
  cpu_routine_return_conditional(!GET_FLAG_CARRY);
}

void cpu_pop_de()   // 0xD1
{
  cpu_routine_pop_16(cpu_registers.d, cpu_registers.e);
}

void cpu_jp_nc()    // 0xD2
{
  cpu_routine_jump_conditional(!GET_FLAG_CARRY);
}

void cpu_call_nc()      // 0xD4
{
  cpu_routine_call_conditional_nn(!GET_FLAG_CARRY);
}

void cpu_push_de()  // 0xD5
{
  cpu_routine_push_16(cpu_registers.d, cpu_registers.e);
}

void cpu_sub_n()    // 0xD6
{
  core_advance_cpu_clocks(4);
  uint8_t n = memory_bus_read(cpu_registers.pc++);
  cpu_routine_sub_a_8(n);
}

void cpu_rst_10()          // 0xD7
{
  cpu_routine_rst(0x10);
}

void cpu_ret_c()    // 0xD8
{
  cpu_routine_return_conditional(GET_FLAG_CARRY);
}

void cpu_reti()            // 0xD9
{
  core_advance_cpu_clocks(4);
  uint8_t lsb = memory_bus_read(cpu_registers.sp++);
  core_advance_cpu_clocks(4);
  uint8_t msb = memory_bus_read(cpu_registers.sp++);
  cpu_registers.pc = (uint16_t)(lsb | (msb << 8));
  core_advance_cpu_clocks(4);
  interrupt_master_enable = true;
  core_advance_cpu_clocks(4);
}


void cpu_jp_c()    // 0xDA
{
  cpu_routine_jump_conditional(GET_FLAG_CARRY);
}

void cpu_call_c()      // 0xDC
{
  cpu_routine_call_conditional_nn(GET_FLAG_CARRY);
}

void cpu_sbc_a_n()  // 0xDE
{
  core_advance_cpu_clocks(4);
  uint8_t n = memory_bus_read(cpu_registers.pc++);
  cpu_routine_sbc_a_8(n);
}

void cpu_rst_18()          // 0xDF
{
  cpu_routine_rst(0x18);
}

void cpu_ldh_n_a()  // 0xE0
{
  core_advance_cpu_clocks(4);
  uint8_t n = memory_bus_read(cpu_registers.pc++);
  core_advance_cpu_clocks(4);
  memory_bus_write((uint16_t)(n | (0xFF00)), cpu_registers.a);
  core_advance_cpu_clocks(4);
}

void cpu_pop_hl()   // 0xE1
{
  cpu_routine_pop_16(cpu_registers.h, cpu_registers.l);
}

void cpu_ldh_c_a()  // 0xE2
{
  core_advance_cpu_clocks(4);
  memory_bus_write((uint16_t)(cpu_registers.c | 0xFF00), cpu_registers.a);
  core_advance_cpu_clocks(4);
}

void cpu_push_hl()  // 0xE5
{
  cpu_routine_push_16(cpu_registers.h, cpu_registers.l);
}

void cpu_and_n()           // 0xE6
{
  core_advance_cpu_clocks(4);
  uint8_t n = memory_bus_read(cpu_registers.pc++);
  cpu_routine_and_a_8(n);
}

void cpu_rst_20()          // 0xE7
{
  cpu_routine_rst(0x20);
}

void cpu_add_sp_e() // 0xE8
{
  core_advance_cpu_clocks(4);
  uint8_t raw = memory_bus_read(cpu_registers.pc++);
  core_advance_cpu_clocks(4);
  int8_t e = (int8_t)raw;
  SET_FLAG_ZERO(0);
  SET_FLAG_SUBTRACT(0);
  core_advance_cpu_clocks(4);
  SET_FLAG_HALF_CARRY(((cpu_registers.sp & 0b00001111) + (raw & 0b00001111)) >= 0b00010000);
  SET_FLAG_CARRY(((cpu_registers.sp & 0xFF) + raw) > 0xFF);
  core_advance_cpu_clocks(4);
  cpu_registers.sp = cpu_registers.sp + e;
}

void cpu_jp_hl()           // 0xE9
{
  core_advance_cpu_clocks(4);
  cpu_registers.pc = cpu_registers.hl;
}

void cpu_ld_nn_a()         // 0xEA
{
  core_advance_cpu_clocks(4);
  uint8_t nn_lsb = memory_bus_read(cpu_registers.pc++);
  core_advance_cpu_clocks(4);
  uint8_t nn_msb = memory_bus_read(cpu_registers.pc++);
  core_advance_cpu_clocks(4);
  uint16_t nn = (uint16_t)(nn_lsb | (nn_msb << 8));
  core_advance_cpu_clocks(4);
  memory_bus_write(nn, cpu_registers.a);
}

void cpu_xor_n()           // 0xEE
{
  core_advance_cpu_clocks(4);
  uint8_t n = memory_bus_read(cpu_registers.pc++);
  cpu_routine_xor_a_8(n);
}

void cpu_rst_28()          // 0xEF
{
  cpu_routine_rst(0x28);
}

void cpu_ldh_a_n()         // 0xF0
{
  core_advance_cpu_clocks(4);
  uint8_t n = memory_bus_read(cpu_registers.pc++);
  core_advance_cpu_clocks(4);
  cpu_registers.a = memory_bus_read((uint16_t)(n | 0xFF00));
  core_advance_cpu_clocks(4);
}

void cpu_pop_af()       // 0xF1
{
  core_advance_cpu_clocks(4);
  uint8_t lsb = memory_bus_read(cpu_registers.sp);
  cpu_registers.sp++;
  core_advance_cpu_clocks(4);
  uint8_t msb = memory_bus_read(cpu_registers.sp);
  cpu_registers.sp++;
  cpu_registers.a = msb;
  cpu_registers.f = (lsb & 0xF0);
  core_advance_cpu_clocks(4);
}

void cpu_ldh_a_c()         // 0xF2
{
  core_advance_cpu_clocks(4);
  cpu_registers.a = memory_bus_read((uint16_t)(cpu_registers.c | 0xFF00));
  core_advance_cpu_clocks(4);
}

void cpu_di()              // 0xF3
{
  core_advance_cpu_clocks(4);
  interrupt_master_enable = false;
  interrupt_enable_ime_delay = 0;
}

void cpu_push_af()  // 0xF5
{
  cpu_routine_push_16(cpu_registers.a, cpu_registers.f);
}

void cpu_or_n()            // 0xF6
{
  core_advance_cpu_clocks(4);
  uint8_t n = memory_bus_read(cpu_registers.pc++);
  cpu_routine_or_a_8(n);
}

void cpu_rst_30()          // 0xF7
{
  cpu_routine_rst(0x30);
}

void cpu_ld_hl_sp_e()      // 0xF8
{
  core_advance_cpu_clocks(4);
  uint8_t raw = memory_bus_read(cpu_registers.pc++);
  int8_t e = (int8_t)raw;
  SET_FLAG_ZERO(0);
  SET_FLAG_SUBTRACT(0);
  core_advance_cpu_clocks(4);
  SET_FLAG_HALF_CARRY(((cpu_registers.sp & 0b00001111) + (raw & 0b00001111)) >= 0b00010000);
  SET_FLAG_CARRY(((cpu_registers.sp & 0xFF) + raw) > 0xFF);
  core_advance_cpu_clocks(4);
  cpu_registers.hl = cpu_registers.sp + e;
}

void cpu_ld_sp_hl()        // 0xF9
{
  core_advance_cpu_clocks(4);
  cpu_registers.sp = cpu_registers.hl;
  core_advance_cpu_clocks(4);
}

void cpu_ld_a_nn()         // 0xFA
{
  core_advance_cpu_clocks(4);
  uint8_t nn_lsb = memory_bus_read(cpu_registers.pc++);
  core_advance_cpu_clocks(4);
  uint8_t nn_msb = memory_bus_read(cpu_registers.pc++);
  core_advance_cpu_clocks(4);
  uint16_t nn = (uint16_t)(nn_lsb | (nn_msb << 8));
  core_advance_cpu_clocks(4);
  cpu_registers.a = memory_bus_read(nn);
}

void cpu_ei()              // 0xFB
{
  core_advance_cpu_clocks(4);
  interrupt_enable_ime_delay = 1;
}

void cpu_cp_n()            // 0xFE
{
  core_advance_cpu_clocks(4);
  uint8_t n = memory_bus_read(cpu_registers.pc++);
  cpu_routine_cp_a_8(n);
}

void cpu_rst_38()          // 0xFF
{
  cpu_routine_rst(0x38);
}



void cpu_cb_rlc_b()    // 0x00
{
  cpu_routine_rlc_8(cpu_registers.b);
}

void cpu_cb_rlc_c()    // 0x01
{
  cpu_routine_rlc_8(cpu_registers.c);
}

void cpu_cb_rlc_d()    // 0x02
{
  cpu_routine_rlc_8(cpu_registers.d);
}

void cpu_cb_rlc_e()    // 0x03
{
  cpu_routine_rlc_8(cpu_registers.e);
}

void cpu_cb_rlc_h()    // 0x04
{
  cpu_routine_rlc_8(cpu_registers.h);
}

void cpu_cb_rlc_l()    // 0x05
{
  cpu_routine_rlc_8(cpu_registers.l);
}

void cpu_cb_rlc_hl()    // 0x06
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  cpu_routine_rlc_8(data);
  core_advance_cpu_clocks(4);
  memory_bus_write(cpu_registers.hl, data);
}

void cpu_cb_rlc_a()    // 0x07
{
  cpu_routine_rlc_8(cpu_registers.a);
}


void cpu_cb_rrc_b()    // 0x08
{
  cpu_routine_rrc_8(cpu_registers.b);
}

void cpu_cb_rrc_c()    // 0x09
{
  cpu_routine_rrc_8(cpu_registers.c);
}

void cpu_cb_rrc_d()    // 0x0a
{
  cpu_routine_rrc_8(cpu_registers.d);
}

void cpu_cb_rrc_e()    // 0x0b
{
  cpu_routine_rrc_8(cpu_registers.e);
}

void cpu_cb_rrc_h()    // 0x0c
{
  cpu_routine_rrc_8(cpu_registers.h);
}

void cpu_cb_rrc_l()    // 0x0d
{
  cpu_routine_rrc_8(cpu_registers.l);
}

void cpu_cb_rrc_hl()    // 0x0e
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  cpu_routine_rrc_8(data);
  core_advance_cpu_clocks(4);
  memory_bus_write(cpu_registers.hl, data);
}

void cpu_cb_rrc_a()    // 0x0f
{
  cpu_routine_rrc_8(cpu_registers.a);
}


void cpu_cb_rl_b()     // 0x10
{
  cpu_routine_rl_8(cpu_registers.b);
}

void cpu_cb_rl_c()     // 0x11
{
  cpu_routine_rl_8(cpu_registers.c);
}

void cpu_cb_rl_d()     // 0x12
{
  cpu_routine_rl_8(cpu_registers.d);
}

void cpu_cb_rl_e()     // 0x13
{
  cpu_routine_rl_8(cpu_registers.e);
}

void cpu_cb_rl_h()     // 0x14
{
  cpu_routine_rl_8(cpu_registers.h);
}

void cpu_cb_rl_l()     // 0x15
{
  cpu_routine_rl_8(cpu_registers.l);
}

void cpu_cb_rl_hl()    // 0x16
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  cpu_routine_rl_8(data);
  core_advance_cpu_clocks(4);
  memory_bus_write(cpu_registers.hl, data);
}

void cpu_cb_rl_a()     // 0x17
{
  cpu_routine_rl_8(cpu_registers.a);
}

void cpu_cb_rr_b()     // 0x18
{
  cpu_routine_rr_8(cpu_registers.b);
}

void cpu_cb_rr_c()     // 0x19
{
  cpu_routine_rr_8(cpu_registers.c);
}

void cpu_cb_rr_d()     // 0x1A
{
  cpu_routine_rr_8(cpu_registers.d);
}

void cpu_cb_rr_e()     // 0x1B
{
  cpu_routine_rr_8(cpu_registers.e);
}

void cpu_cb_rr_h()     // 0x1C
{
  cpu_routine_rr_8(cpu_registers.h);
}

void cpu_cb_rr_l()     // 0x1D
{
  cpu_routine_rr_8(cpu_registers.l);
}

void cpu_cb_rr_hl()    // 0x1E
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  cpu_routine_rr_8(data);
  core_advance_cpu_clocks(4);
  memory_bus_write(cpu_registers.hl, data);
}

void cpu_cb_rr_a()     // 0x1F
{
  cpu_routine_rr_8(cpu_registers.a);
}



void cpu_cb_sla_b()     // 0x20
{
  cpu_routine_sla_8(cpu_registers.b);
}

void cpu_cb_sla_c()     // 0x21
{
  cpu_routine_sla_8(cpu_registers.c);
}

void cpu_cb_sla_d()     // 0x22
{
  cpu_routine_sla_8(cpu_registers.d);
}

void cpu_cb_sla_e()     // 0x23
{
  cpu_routine_sla_8(cpu_registers.e);
}

void cpu_cb_sla_h()     // 0x24
{
  cpu_routine_sla_8(cpu_registers.h);
}

void cpu_cb_sla_l()     // 0x25
{
  cpu_routine_sla_8(cpu_registers.l);
}

void cpu_cb_sla_hl()    // 0x26
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  cpu_routine_sla_8(data);
  core_advance_cpu_clocks(4);
  memory_bus_write(cpu_registers.hl, data);
}


void cpu_cb_sla_a()     // 0x27
{
  cpu_routine_sla_8(cpu_registers.a);
}

void cpu_cb_sra_b()     // 0x28
{
  cpu_routine_sra_8(cpu_registers.b);
}

void cpu_cb_sra_c()     // 0x29
{
  cpu_routine_sra_8(cpu_registers.c);
}

void cpu_cb_sra_d()     // 0x2A
{
  cpu_routine_sra_8(cpu_registers.d);
}

void cpu_cb_sra_e()     // 0x2B
{
  cpu_routine_sra_8(cpu_registers.e);
}

void cpu_cb_sra_h()     // 0x2C
{
  cpu_routine_sra_8(cpu_registers.h);
}

void cpu_cb_sra_l()     // 0x2D
{
  cpu_routine_sra_8(cpu_registers.l);
}

void cpu_cb_sra_hl()    // 0x2E
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  cpu_routine_sra_8(data);
  core_advance_cpu_clocks(4);
  memory_bus_write(cpu_registers.hl, data);
}

void cpu_cb_sra_a()     // 0x2F
{
  cpu_routine_sra_8(cpu_registers.a);
}

void cpu_cb_swap_b()    // 0x30
{
  cpu_routine_swap_8(cpu_registers.b);
}

void cpu_cb_swap_c()    // 0x31
{
  cpu_routine_swap_8(cpu_registers.c);
}

void cpu_cb_swap_d()    // 0x32
{
  cpu_routine_swap_8(cpu_registers.d);
}

void cpu_cb_swap_e()    // 0x33
{
  cpu_routine_swap_8(cpu_registers.e);
}

void cpu_cb_swap_h()    // 0x34
{
  cpu_routine_swap_8(cpu_registers.h);
}

void cpu_cb_swap_l()    // 0x35
{
  cpu_routine_swap_8(cpu_registers.l);
}

void cpu_cb_swap_hl()   // 0x36
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  cpu_routine_swap_8(data);
  core_advance_cpu_clocks(4);
  memory_bus_write(cpu_registers.hl, data);
}

void cpu_cb_swap_a()    // 0x37
{
  cpu_routine_swap_8(cpu_registers.a);
}

void cpu_cb_srl_b()     // 0x38
{
  cpu_routine_srl_8(cpu_registers.b);
}

void cpu_cb_srl_c()     // 0x39
{
  cpu_routine_srl_8(cpu_registers.c);
}

void cpu_cb_srl_d()     // 0x3A
{
  cpu_routine_srl_8(cpu_registers.d);
}

void cpu_cb_srl_e()     // 0x3B
{
  cpu_routine_srl_8(cpu_registers.e);
}

void cpu_cb_srl_h()     // 0x3C
{
  cpu_routine_srl_8(cpu_registers.h);
}

void cpu_cb_srl_l()     // 0x3D
{
  cpu_routine_srl_8(cpu_registers.l);
}

void cpu_cb_srl_hl()    // 0x3E
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  cpu_routine_srl_8(data);
  core_advance_cpu_clocks(4);
  memory_bus_write(cpu_registers.hl, data);
}

void cpu_cb_srl_a()     // 0x3F
{
  cpu_routine_srl_8(cpu_registers.a);
}



void cpu_cb_bit0_b()                    // 0x40
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.b, 0));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit0_c()                    // 0x41
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.c, 0));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit0_d()                    // 0x42
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.d, 0));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit0_e()                    // 0x43
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.e, 0));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit0_h()                    // 0x44
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.h, 0));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit0_l()                    // 0x45
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.l, 0));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit0_hl()                   // 0x46
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  SET_FLAG_ZERO(!CHECK_BIT(data, 0));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit0_a()                    // 0x47
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.a, 0));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}



void cpu_cb_bit1_b()                    // 0x48
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.b, 1));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit1_c()                    // 0x49
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.c, 1));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit1_d()                    // 0x4A
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.d, 1));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit1_e()                    // 0x4B
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.e, 1));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit1_h()                    // 0x4C
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.h, 1));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit1_l()                    // 0x4D
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.l, 1));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit1_hl()                   // 0x4E
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  SET_FLAG_ZERO(!CHECK_BIT(data, 1));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit1_a()                    // 0x4F
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.a, 1));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}



void cpu_cb_bit2_b()                    // 0x50
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.b, 2));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit2_c()                    // 0x51
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.c, 2));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit2_d()                    // 0x52
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.d, 2));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit2_e()                    // 0x53
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.e, 2));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit2_h()                    // 0x54
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.h, 2));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit2_l()                    // 0x55
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.l, 2));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit2_hl()                   // 0x56
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  SET_FLAG_ZERO(!CHECK_BIT(data, 2));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit2_a()                    // 0x57
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.a, 2));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}



void cpu_cb_bit3_b()                   // 0x58
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.b, 3));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit3_c()                   // 0x59
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.c, 3));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit3_d()                   // 0x5A
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.d, 3));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit3_e()                   // 0x5B
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.e, 3));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit3_h()                   // 0x5C
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.h, 3));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit3_l()                   // 0x5D
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.l, 3));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit3_hl()                  // 0x5E
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  SET_FLAG_ZERO(!CHECK_BIT(data, 3));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit3_a()                   // 0x5F
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.a, 3));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}



void cpu_cb_bit4_b()                   // 0x60
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.b, 4));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit4_c()                   // 0x61
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.c, 4));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit4_d()                   // 0x62
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.d, 4));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit4_e()                   // 0x63
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.e, 4));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit4_h()                   // 0x64
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.h, 4));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit4_l()                   // 0x65
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.l, 4));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit4_hl()                  // 0x66
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  SET_FLAG_ZERO(!CHECK_BIT(data, 4));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit4_a()                   // 0x67
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.a, 4));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}



void cpu_cb_bit5_b()                   // 0x68
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.b, 5));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit5_c()                   // 0x69
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.c, 5));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit5_d()                   // 0x6A
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.d, 5));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit5_e()                   // 0x6B
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.e, 5));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit5_h()                   // 0x6C
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.h, 5));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit5_l()                   // 0x6D
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.l, 5));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit5_hl()                  // 0x6E
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  SET_FLAG_ZERO(!CHECK_BIT(data, 5));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit5_a()                   // 0x6F
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.a, 5));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}



void cpu_cb_bit6_b()                   // 0x70
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.b, 6));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit6_c()                   // 0x71
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.c, 6));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit6_d()                   // 0x72
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.d, 6));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit6_e()                   // 0x73
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.e, 6));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit6_h()                   // 0x74
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.h, 6));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit6_l()                   // 0x75
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.l, 6));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit6_hl()                  // 0x76
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  SET_FLAG_ZERO(!CHECK_BIT(data, 6));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit6_a()                   // 0x77
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.a, 6));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}



void cpu_cb_bit7_b()                   // 0x78
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.b, 7));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit7_c()                   // 0x79
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.c, 7));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit7_d()                   // 0x7A
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.d, 7));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit7_e()                   // 0x7B
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.e, 7));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit7_h()                   // 0x7C
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.h, 7));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit7_l()                   // 0x7D
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.l, 7));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit7_hl()                  // 0x7E
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  SET_FLAG_ZERO(!CHECK_BIT(data, 7));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}

void cpu_cb_bit7_a()                   // 0x7F
{
  SET_FLAG_ZERO(!CHECK_BIT(cpu_registers.a, 7));
  core_advance_cpu_clocks(4);
  SET_FLAG_SUBTRACT(0);
  SET_FLAG_HALF_CARRY(1);
}





void cpu_cb_res0_b()     // 0x80
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.b, 0);
}

void cpu_cb_res0_c()     // 0x81
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.c,0);
}

void cpu_cb_res0_d()     // 0x82
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.d,0);
}

void cpu_cb_res0_e()     // 0x83
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.e,0);
}

void cpu_cb_res0_h()     // 0x84
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.h,0);
}

void cpu_cb_res0_l()     // 0x85
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.l,0);
}

void cpu_cb_res0_hl()    // 0x86
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  CLEAR_BIT(data, 0);
  core_advance_cpu_clocks(4);
}

void cpu_cb_res0_a()     // 0x87
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.a,0);
}


void cpu_cb_res1_b()     // 0x88
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.b,1);
}

void cpu_cb_res1_c()     // 0x89
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.c,1);
}

void cpu_cb_res1_d()     // 0x8A
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.d,1);
}

void cpu_cb_res1_e()     // 0x8B
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.e,1);
}

void cpu_cb_res1_h()     // 0x8C
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.h,1);
}

void cpu_cb_res1_l()     // 0x8D
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.l,1);
}

void cpu_cb_res1_hl()    // 0x8E
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  CLEAR_BIT(data, 1);
  core_advance_cpu_clocks(4);
}

void cpu_cb_res1_a()     // 0x8F
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.a,1);
}


void cpu_cb_res2_b()     // 0x90
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.b,2);
}

void cpu_cb_res2_c()     // 0x91
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.c,2);
}

void cpu_cb_res2_d()     // 0x92
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.d,2);
}

void cpu_cb_res2_e()     // 0x93
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.e,2);
}

void cpu_cb_res2_h()     // 0x94
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.h,2);
}

void cpu_cb_res2_l()     // 0x95
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.l,2);
}

void cpu_cb_res2_hl()    // 0x96
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  CLEAR_BIT(data, 2);
  core_advance_cpu_clocks(4);
}

void cpu_cb_res2_a()     // 0x97
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.a,2);
}


void cpu_cb_res3_b()     // 0x98
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.b,3);
}

void cpu_cb_res3_c()     // 0x99
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.c,3);
}

void cpu_cb_res3_d()     // 0x9A
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.d,3);
}

void cpu_cb_res3_e()     // 0x9B
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.e,3);
}

void cpu_cb_res3_h()     // 0x9C
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.h,3);
}

void cpu_cb_res3_l()     // 0x9D
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.l,3);
}

void cpu_cb_res3_hl()    // 0x9E
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  CLEAR_BIT(data, 3);
  core_advance_cpu_clocks(4);
}

void cpu_cb_res3_a()     // 0x9F
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.a,3);
}


void cpu_cb_res4_b()     // 0xA0
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.b,4);
}

void cpu_cb_res4_c()     // 0xA1
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.c,4);
}

void cpu_cb_res4_d()     // 0xA2
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.d,4);
}

void cpu_cb_res4_e()     // 0xA3
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.e,4);
}

void cpu_cb_res4_h()     // 0xA4
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.h,4);
}

void cpu_cb_res4_l()     // 0xA5
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.l,4);
}

void cpu_cb_res4_hl()    // 0xA6
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  CLEAR_BIT(data, 4);
  core_advance_cpu_clocks(4);
}

void cpu_cb_res4_a()     // 0xA7
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.a,4);
}


void cpu_cb_res5_b()     // 0xA8
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.b,5);
}

void cpu_cb_res5_c()     // 0xA9
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.c,5);
}

void cpu_cb_res5_d()     // 0xAA
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.d,5);
}

void cpu_cb_res5_e()     // 0xAB
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.e,5);
}

void cpu_cb_res5_h()     // 0xAC
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.h,5);
}

void cpu_cb_res5_l()     // 0xAD
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.l,5);
}

void cpu_cb_res5_hl()    // 0xAE
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  CLEAR_BIT(data, 5);
  core_advance_cpu_clocks(4);
}

void cpu_cb_res5_a()     // 0xAF
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.a,5);
}


void cpu_cb_res6_b()     // 0xB0
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.b,6);
}

void cpu_cb_res6_c()     // 0xB1
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.c,6);
}

void cpu_cb_res6_d()     // 0xB2
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.d,6);
}

void cpu_cb_res6_e()     // 0xB3
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.e,6);
}

void cpu_cb_res6_h()     // 0xB4
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.h,6);
}

void cpu_cb_res6_l()     // 0xB5
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.l,6);
}

void cpu_cb_res6_hl()    // 0xB6
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  CLEAR_BIT(data, 6);
  core_advance_cpu_clocks(4);
}

void cpu_cb_res6_a()     // 0xB7
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.a,6);
}


void cpu_cb_res7_b()     // 0xB8
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.b,7);
}

void cpu_cb_res7_c()     // 0xB9
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.c,7);
}

void cpu_cb_res7_d()     // 0xBA
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.d,7);
}

void cpu_cb_res7_e()     // 0xBB
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.e,7);
}

void cpu_cb_res7_h()     // 0xBC
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.h,7);
}

void cpu_cb_res7_l()     // 0xBD
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.l,7);
}

void cpu_cb_res7_hl()    // 0xBE
{
  core_advance_cpu_clocks(4);
  uint8_t data = memory_bus_read(cpu_registers.hl);
  CLEAR_BIT(data, 7);
  core_advance_cpu_clocks(4);
}

void cpu_cb_res7_a()     // 0xBF
{
  core_advance_cpu_clocks(4);
  CLEAR_BIT(cpu_registers.a,7);
}

