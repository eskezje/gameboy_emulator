#include "interrupts.h"
#include <cstdint>
#include <timer.h>
#include <memory_bus.h>
/*
We have DIV at 0xFF04 - the free-running clock/divider
We have TIMA at 0xFF05 - the actual programmable timer counter
We have TMA at 0xFF06 - value TIMA reloads from after overflow
We have TAC at 0xFF07 - enables TIMA and chooses how fast it increments

Lets say we have
TIMA = 00
TMA =  80
TAC =  101
then for TAC
bit 2 == 1 -> timer enabled 
bits 1:0 == 01 TIMA increments every 4 M-cycles
TAC 00 -> every 256  M-cycles
TAC 01 -> every 4    M-cycles
TAC 10 -> every 16   M-cycles
TAC 11 -> every 64   M-cycles

and 1 -M-cycles = 4 T-cycles
*/


extern uint8_t cpu_halt_count;

const uint16_t timer_tac_edge_bits[4] = {9, 3, 5, 7};

gb_timer_register* timer_registers = (gb_timer_register*)(memory + 0xFF04);

uint16_t timer_internal_sysclk = 0;
uint8_t timer_interrupt_delay = 0;

void timer_init()
{
  timer_internal_sysclk = 0xABCC;
  timer_registers->timer_div = 0xAB;
  timer_registers->timer_tac = 0;
  timer_registers->timer_tima = 0;
  timer_registers->timer_tma = 0;
}

void timer_check_clock_edges(const uint16_t prev_sysclk)
{
  const uint8_t div_prev = timer_registers->timer_div;
  const bool div_bit_4_prev = (timer_registers->timer_div >> 4) & 1;

  // expose upper 8 bits
  timer_registers->timer_div = timer_internal_sysclk >> 8;

  const bool div_bit_4_now = (timer_registers->timer_div >> 4) & 1;
  if (div_bit_4_prev & !div_bit_4_now) // falling edge on div bit 4
  {
    // TODO: Tick APU counter here
  }

  const bool tac_timer_enable = CHECK_BIT(timer_registers->timer_tac, 2);
  if (!tac_timer_enable) {
    return;
  }
  const uint8_t tac_clock_select = timer_registers->timer_tac & 0b00000011;
  const uint16_t tac_divider_bit = timer_tac_edge_bits[tac_clock_select];
  const bool prev_edge = CHECK_BIT(prev_sysclk, tac_divider_bit);
  const bool curr_edge = CHECK_BIT(timer_internal_sysclk, tac_divider_bit);
  if (prev_edge & !curr_edge) { // falling edge on tac bit
    timer_tick_tima();
  }
}

void timer_increase_div(const uint8_t cycles)
{
  for (uint8_t c = 0; c < cycles; c++) {
    const uint16_t prev_sysclk = timer_internal_sysclk;
    timer_internal_sysclk++;

    timer_check_clock_edges(prev_sysclk);
  }
}


void timer_advance_clocks(const uint8_t cycles)
{
  if (timer_interrupt_delay> 0) {
    for (uint8_t c = 0; c< cycles; c++) {
      timer_interrupt_delay--;
      if (timer_interrupt_delay == 0) {
        interrupt_raise_flag(INTERRUPT_FLAG_TIMER);
        break;
      }
    }
  }
  if (cpu_halt_count == 2) {
    return; // dont tick div when cpu is stopped
  }
  timer_increase_div(cycles);
}


void timer_on_div_write(const uint8_t value)
{
  const uint16_t prev_sysclk = timer_internal_sysclk;
  timer_internal_sysclk = 0x0000;
  timer_check_clock_edges(prev_sysclk);
}

void timer_tick_tima()
{
  if (timer_registers->timer_tima != 0xFF) {
    timer_registers->timer_tima++;
  }
  else {    // TIMA overflow
    // TODO: TIMA should remain 0x00 during the 4 T-cycles overflow delay 
    // Reload TMA and request the interrupt when the delay is complete
    timer_registers->timer_tima = timer_registers->timer_tma;
    timer_interrupt_delay = 4;
  }
}
