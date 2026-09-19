#include <cpu_instructions.h>
#include <cpu.h>

const struct gb_cpu_instruction instructions[256] = {
    {"NOP", 0, cpu_noop},         // 0x00
    {"LD BC, nn", 2, cpu_ld_bc_nn},   // 0x01
    {"LD (BC), A", 0, cpu_ld_bc_a},  // 0x02
    {"INC BC", 0, cpu_inc_bc},      // 0x03
    {"INC B", 0, cpu_inc_b},       // 0x04
    {"DEC B", 0, cpu_dec_b},       // 0x05
    {"LD B, n", 1, cpu_ld_b_n},     // 0x06
    {"RLCA", 0, nullptr},        // 0x07
    {"LD (nn), SP", 2, nullptr}, // 0x08
    {"ADD HL, BC", 0, cpu_add_hl_bc},  // 0x09
    {"LD A, (BC)", 0, cpu_ld_a_bc},  // 0x0a
    {"DEC BC", 0, cpu_dec_bc},      // 0x0b
    {"INC C", 0, cpu_inc_c},       // 0x0c
    {"DEC C", 0, cpu_dec_c},       // 0x0d
    {"LD C, n", 1, cpu_ld_c_n},     // 0x0e
    {"RRCA", 0, nullptr},        // 0x0f
    {"STOP", 1, nullptr},        // 0x10
    {"LD DE, nn", 2, cpu_ld_de_nn},   // 0x11
    {"LD (DE), A", 0, cpu_ld_de_a},  // 0x12
    {"INC DE", 0, cpu_inc_de},      // 0x13
    {"INC D", 0, cpu_inc_d},       // 0x14
    {"DEC D", 0, cpu_dec_d},       // 0x15
    {"LD D, n", 1, cpu_ld_d_n},     // 0x16
    {"RLA", 0, nullptr},         // 0x17
    {"JR e", 1, cpu_jr_e},        // 0x18
    {"ADD HL, DE", 0, cpu_add_hl_de},  // 0x19
    {"LD A, (DE)", 0, cpu_ld_a_de},  // 0x1a
    {"DEC DE", 0, cpu_dec_de},      // 0x1b
    {"INC E", 0, cpu_inc_e},       // 0x1c
    {"DEC E", 0, cpu_dec_e},       // 0x1d
    {"LD E, n", 1, cpu_ld_e_n},     // 0x1e
    {"RRA", 0, nullptr},         // 0x1f
    {"JR NZ, e", 1, cpu_jr_nz_e},    // 0x20
    {"LD HL, nn", 2, cpu_ld_hl_nn},   // 0x21
    {"LD (HL+), A", 0, nullptr}, // 0x22
    {"INC HL", 0, cpu_inc_hl},      // 0x23
    {"INC H", 0, cpu_inc_h},       // 0x24
    {"DEC H", 0, cpu_dec_h},       // 0x25
    {"LD H, n", 1, cpu_ld_h_n},     // 0x26
    {"DAA", 0, nullptr},         // 0x27
    {"JR Z, e", 1, cpu_jr_z_e},     // 0x28
    {"ADD HL, HL", 0, nullptr},  // 0x29
    {"LD A, (HL+)", 0, nullptr}, // 0x2a
    {"DEC HL", 0, cpu_dec_hl},      // 0x2b
    {"INC L", 0, cpu_inc_l},       // 0x2c
    {"DEC L", 0, cpu_dec_l},       // 0x2d
    {"LD L, n", 1, cpu_ld_l_n},     // 0x2e
    {"CPL", 0, nullptr},         // 0x2f
    {"JR NC, e", 1, cpu_jr_nc_e},    // 0x30
    {"LD SP, nn", 2, cpu_ld_sp_nn},   // 0x31
    {"LD (HL-), A", 0, cpu_ldd_hl_a}, // 0x32
    {"INC SP", 0, cpu_inc_sp},      // 0x33
    {"INC (HL)", 0, nullptr},    // 0x34
    {"DEC (HL)", 0, nullptr},    // 0x35
    {"LD (HL), n", 1, nullptr},  // 0x36
    {"SCF", 0, nullptr},         // 0x37
    {"JR C, e", 1, cpu_jr_c_e},     // 0x38
    {"ADD HL, SP", 0, cpu_add_hl_sp},  // 0x39
    {"LD A, (HL-)", 0, nullptr}, // 0x3a
    {"DEC SP", 0, cpu_dec_sp},      // 0x3b
    {"INC A", 0, cpu_inc_a},       // 0x3c
    {"DEC A", 0, cpu_dec_a},       // 0x3d
    {"LD A, n", 1, cpu_ld_a_n},     // 0x3e
    {"CCF", 0, nullptr},         // 0x3f
    {"LD B, B", 0, nullptr},     // 0x40
    {"LD B, C", 0, nullptr},     // 0x41
    {"LD B, D", 0, nullptr},     // 0x42
    {"LD B, E", 0, nullptr},     // 0x43
    {"LD B, H", 0, nullptr},     // 0x44
    {"LD B, L", 0, nullptr},     // 0x45
    {"LD B, (HL)", 0, cpu_ld_b_hl},  // 0x46
    {"LD B, A", 0, nullptr},     // 0x47
    {"LD C, B", 0, nullptr},     // 0x48
    {"LD C, C", 0, nullptr},     // 0x49
    {"LD C, D", 0, nullptr},     // 0x4a
    {"LD C, E", 0, nullptr},     // 0x4b
    {"LD C, H", 0, nullptr},     // 0x4c
    {"LD C, L", 0, nullptr},     // 0x4d
    {"LD C, (HL)", 0, cpu_ld_c_hl},  // 0x4e
    {"LD C, A", 0, nullptr},     // 0x4f
    {"LD D, B", 0, nullptr},     // 0x50
    {"LD D, C", 0, nullptr},     // 0x51
    {"LD D, D", 0, nullptr},     // 0x52
    {"LD D, E", 0, nullptr},     // 0x53
    {"LD D, H", 0, nullptr},     // 0x54
    {"LD D, L", 0, nullptr},     // 0x55
    {"LD D, (HL)", 0, cpu_ld_d_hl},  // 0x56
    {"LD D, A", 0, nullptr},     // 0x57
    {"LD E, B", 0, nullptr},     // 0x58
    {"LD E, C", 0, nullptr},     // 0x59
    {"LD E, D", 0, nullptr},     // 0x5a
    {"LD E, E", 0, nullptr},     // 0x5b
    {"LD E, H", 0, nullptr},     // 0x5c
    {"LD E, L", 0, nullptr},     // 0x5d
    {"LD E, (HL)", 0, cpu_ld_e_hl},  // 0x5e
    {"LD E, A", 0, nullptr},     // 0x5f
    {"LD H, B", 0, nullptr},     // 0x60
    {"LD H, C", 0, nullptr},     // 0x61
    {"LD H, D", 0, nullptr},     // 0x62
    {"LD H, E", 0, nullptr},     // 0x63
    {"LD H, H", 0, nullptr},     // 0x64
    {"LD H, L", 0, nullptr},     // 0x65
    {"LD H, (HL)", 0, cpu_ld_h_hl},  // 0x66
    {"LD H, A", 0, nullptr},     // 0x67
    {"LD L, B", 0, nullptr},     // 0x68
    {"LD L, C", 0, nullptr},     // 0x69
    {"LD L, D", 0, nullptr},     // 0x6a
    {"LD L, E", 0, nullptr},     // 0x6b
    {"LD L, H", 0, nullptr},     // 0x6c
    {"LD L, L", 0, nullptr},     // 0x6d
    {"LD L, (HL)", 0, cpu_ld_l_hl},  // 0x6e
    {"LD L, A", 0, nullptr},     // 0x6f
    {"LD (HL), B", 0, cpu_ld_hl_b},  // 0x70
    {"LD (HL), C", 0, cpu_ld_hl_c},  // 0x71
    {"LD (HL), D", 0, cpu_ld_hl_d},  // 0x72
    {"LD (HL), E", 0, cpu_ld_hl_e},  // 0x73
    {"LD (HL), H", 0, cpu_ld_hl_h},  // 0x74
    {"LD (HL), L", 0, cpu_ld_hl_l},  // 0x75
    {"HALT", 0, nullptr},        // 0x76
    {"LD (HL), A", 0, cpu_ld_hl_a},  // 0x77
    {"LD A, B", 0, nullptr},     // 0x78
    {"LD A, C", 0, nullptr},     // 0x79
    {"LD A, D", 0, nullptr},     // 0x7a
    {"LD A, E", 0, nullptr},     // 0x7b
    {"LD A, H", 0, nullptr},     // 0x7c
    {"LD A, L", 0, nullptr},     // 0x7d
    {"LD A, (HL)", 0, cpu_ld_a_hl},  // 0x7e
    {"LD A, A", 0, nullptr},     // 0x7f
    {"ADD A, B", 0, cpu_add_a_b},    // 0x80
    {"ADD A, C", 0, cpu_add_a_c},    // 0x81
    {"ADD A, D", 0, cpu_add_a_d},    // 0x82
    {"ADD A, E", 0, cpu_add_a_e},    // 0x83
    {"ADD A, H", 0, cpu_add_a_h},    // 0x84
    {"ADD A, L", 0, cpu_add_a_l},    // 0x85
    {"ADD A, (HL)", 0, nullptr}, // 0x86
    {"ADD A, A", 0, nullptr},    // 0x87
    {"ADC A, B", 0, cpu_adc_a_b},    // 0x88
    {"ADC A, C", 0, cpu_adc_a_c},    // 0x89
    {"ADC A, D", 0, cpu_adc_a_d},    // 0x8a
    {"ADC A, E", 0, cpu_adc_a_e},    // 0x8b
    {"ADC A, H", 0, cpu_adc_a_h},    // 0x8c
    {"ADC A, L", 0, cpu_adc_a_l},    // 0x8d
    {"ADC A, (HL)", 0, nullptr}, // 0x8e
    {"ADC A, A", 0, nullptr},    // 0x8f
    {"SUB B", 0, cpu_sub_a_b},       // 0x90
    {"SUB C", 0, cpu_sub_a_c},       // 0x91
    {"SUB D", 0, cpu_sub_a_d},       // 0x92
    {"SUB E", 0, cpu_sub_a_e},       // 0x93
    {"SUB H", 0, cpu_sub_a_h},       // 0x94
    {"SUB L", 0, cpu_sub_a_l},       // 0x95
    {"SUB (HL)", 0, nullptr},    // 0x96
    {"SUB A", 0, nullptr},       // 0x97
    {"SBC A, B", 0, nullptr},    // 0x98
    {"SBC A, C", 0, nullptr},    // 0x99
    {"SBC A, D", 0, nullptr},    // 0x9a
    {"SBC A, E", 0, nullptr},    // 0x9b
    {"SBC A, H", 0, nullptr},    // 0x9c
    {"SBC A, L", 0, nullptr},    // 0x9d
    {"SBC A, (HL)", 0, nullptr}, // 0x9e
    {"SBC A, A", 0, nullptr},    // 0x9f
    {"AND B", 0, nullptr},       // 0xa0
    {"AND C", 0, nullptr},       // 0xa1
    {"AND D", 0, nullptr},       // 0xa2
    {"AND E", 0, nullptr},       // 0xa3
    {"AND H", 0, nullptr},       // 0xa4
    {"AND L", 0, nullptr},       // 0xa5
    {"AND (HL)", 0, nullptr},    // 0xa6
    {"AND A", 0, nullptr},       // 0xa7
    {"XOR B", 0, nullptr},       // 0xa8
    {"XOR C", 0, nullptr},       // 0xa9
    {"XOR D", 0, nullptr},       // 0xaa
    {"XOR E", 0, nullptr},       // 0xab
    {"XOR H", 0, nullptr},       // 0xac
    {"XOR L", 0, nullptr},       // 0xad
    {"XOR (HL)", 0, nullptr},    // 0xae
    {"XOR A", 0, cpu_xor_a},       // 0xaf
    {"OR B", 0, nullptr},        // 0xb0
    {"OR C", 0, nullptr},        // 0xb1
    {"OR D", 0, nullptr},        // 0xb2
    {"OR E", 0, nullptr},        // 0xb3
    {"OR H", 0, nullptr},        // 0xb4
    {"OR L", 0, nullptr},        // 0xb5
    {"OR (HL)", 0, nullptr},     // 0xb6
    {"OR A", 0, nullptr},        // 0xb7
    {"CP B", 0, nullptr},        // 0xb8
    {"CP C", 0, nullptr},        // 0xb9
    {"CP D", 0, nullptr},        // 0xba
    {"CP E", 0, nullptr},        // 0xbb
    {"CP H", 0, nullptr},        // 0xbc
    {"CP L", 0, nullptr},        // 0xbd
    {"CP (HL)", 0, nullptr},     // 0xbe
    {"CP A", 0, nullptr},        // 0xbf
    {"RET NZ", 0, nullptr},      // 0xc0
    {"POP BC", 0, nullptr},      // 0xc1
    {"JP NZ, nn", 2, nullptr},   // 0xc2
    {"JP nn", 2, cpu_jp_nn},       // 0xc3
    {"CALL NZ, nn", 2, nullptr}, // 0xc4
    {"PUSH BC", 0, nullptr},     // 0xc5
    {"ADD A, n", 1, nullptr},    // 0xc6
    {"RST 00H", 0, nullptr},     // 0xc7
    {"RET Z", 0, nullptr},       // 0xc8
    {"RET", 0, nullptr},         // 0xc9
    {"JP Z, nn", 2, nullptr},    // 0xca
    {"PREFIX CB", 1, nullptr},   // 0xcb
    {"CALL Z, nn", 2, nullptr},  // 0xcc
    {"CALL nn", 2, nullptr},     // 0xcd
    {"ADC A, n", 1, nullptr},    // 0xce
    {"RST 08H", 0, nullptr},     // 0xcf
    {"RET NC", 0, nullptr},      // 0xd0
    {"POP DE", 0, nullptr},      // 0xd1
    {"JP NC, nn", 2, nullptr},   // 0xd2
    {"ILLEGAL", 0, nullptr},     // 0xd3
    {"CALL NC, nn", 2, nullptr}, // 0xd4
    {"PUSH DE", 0, nullptr},     // 0xd5
    {"SUB n", 1, nullptr},       // 0xd6
    {"RST 10H", 0, nullptr},     // 0xd7
    {"RET C", 0, nullptr},       // 0xd8
    {"RETI", 0, nullptr},        // 0xd9
    {"JP C, nn", 2, nullptr},    // 0xda
    {"ILLEGAL", 0, nullptr},     // 0xdb
    {"CALL C, nn", 2, nullptr},  // 0xdc
    {"ILLEGAL", 0, nullptr},     // 0xdd
    {"SBC A, n", 1, nullptr},    // 0xde
    {"RST 18H", 0, nullptr},     // 0xdf
    {"LDH (n), A", 1, nullptr},  // 0xe0
    {"POP HL", 0, nullptr},      // 0xe1
    {"LD (C), A", 0, nullptr},   // 0xe2
    {"ILLEGAL", 0, nullptr},     // 0xe3
    {"ILLEGAL", 0, nullptr},     // 0xe4
    {"PUSH HL", 0, nullptr},     // 0xe5
    {"AND n", 1, nullptr},       // 0xe6
    {"RST 20H", 0, nullptr},     // 0xe7
    {"ADD SP, e", 1, nullptr},   // 0xe8
    {"JP HL", 0, nullptr},       // 0xe9
    {"LD (nn), A", 2, nullptr},  // 0xea
    {"ILLEGAL", 0, nullptr},     // 0xeb
    {"ILLEGAL", 0, nullptr},     // 0xec
    {"ILLEGAL", 0, nullptr},     // 0xed
    {"XOR n", 1, nullptr},       // 0xee
    {"RST 28H", 0, nullptr},     // 0xef
    {"LDH A, (n)", 1, nullptr},  // 0xf0
    {"POP AF", 0, nullptr},      // 0xf1
    {"LD A, (C)", 0, nullptr},   // 0xf2
    {"DI", 0, nullptr},          // 0xf3
    {"ILLEGAL", 0, nullptr},     // 0xf4
    {"PUSH AF", 0, nullptr},     // 0xf5
    {"OR n", 1, nullptr},        // 0xf6
    {"RST 30H", 0, nullptr},     // 0xf7
    {"LD HL, SP+e", 1, nullptr}, // 0xf8
    {"LD SP, HL", 0, nullptr},   // 0xf9
    {"LD A, (nn)", 2, nullptr},  // 0xfa
    {"EI", 0, nullptr},          // 0xfb
    {"ILLEGAL", 0, nullptr},     // 0xfc
    {"ILLEGAL", 0, nullptr},     // 0xfd
    {"CP n", 1, nullptr},        // 0xfe
    {"RST 38H", 0, nullptr}      // 0xff
};
