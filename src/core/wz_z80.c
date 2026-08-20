/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_z80.h"

#include "core/wz_bus.h"
#include "core/wz_machine.h"

#define WZ_Z80_FLAG_C 0x01u
#define WZ_Z80_FLAG_N 0x02u
#define WZ_Z80_FLAG_PV 0x04u
#define WZ_Z80_FLAG_X 0x08u
#define WZ_Z80_FLAG_H 0x10u
#define WZ_Z80_FLAG_Y 0x20u
#define WZ_Z80_FLAG_Z 0x40u
#define WZ_Z80_FLAG_S 0x80u
#define WZ_Z80_TARGET_HL_INDIRECT 6u

static wz_word_t wz_z80_add16(wz_word_t value, wz_word_t amount)
{
    return (wz_word_t)(value + amount);
}

static void wz_z80_increment_r(wz_z80_state_t* state)
{
    state->r = (wz_byte_t)((state->r & 0x80u) | ((state->r + 1u) & 0x7fu));
}

#define WZ_Z80_UN(opcode_value) \
    { (wz_byte_t)(opcode_value), WZ_Z80_PRIMARY_OP_UNSUPPORTED, WZ_Z80_OPCODE_DOCUMENTED_UNIMPLEMENTED }
#define WZ_Z80_IMPL(opcode_value, operation_value) \
    { (wz_byte_t)(opcode_value), (operation_value), WZ_Z80_OPCODE_IMPLEMENTED }
#define WZ_Z80_PREFIX(opcode_value, operation_value) \
    { (wz_byte_t)(opcode_value), (operation_value), WZ_Z80_OPCODE_PREFIX }

static const wz_z80_opcode_decode_t wz_z80_primary_opcode_table[256] = {
    WZ_Z80_IMPL(0x00u, WZ_Z80_PRIMARY_OP_NOP),
    WZ_Z80_UN(0x01u), WZ_Z80_UN(0x02u), WZ_Z80_UN(0x03u),
    WZ_Z80_UN(0x04u), WZ_Z80_UN(0x05u), WZ_Z80_UN(0x06u), WZ_Z80_UN(0x07u),
    WZ_Z80_UN(0x08u), WZ_Z80_UN(0x09u), WZ_Z80_UN(0x0au), WZ_Z80_UN(0x0bu),
    WZ_Z80_UN(0x0cu), WZ_Z80_UN(0x0du), WZ_Z80_UN(0x0eu), WZ_Z80_UN(0x0fu),
    WZ_Z80_UN(0x10u), WZ_Z80_UN(0x11u), WZ_Z80_UN(0x12u), WZ_Z80_UN(0x13u),
    WZ_Z80_UN(0x14u), WZ_Z80_UN(0x15u), WZ_Z80_UN(0x16u), WZ_Z80_UN(0x17u),
    WZ_Z80_UN(0x18u), WZ_Z80_UN(0x19u), WZ_Z80_UN(0x1au), WZ_Z80_UN(0x1bu),
    WZ_Z80_UN(0x1cu), WZ_Z80_UN(0x1du), WZ_Z80_UN(0x1eu), WZ_Z80_UN(0x1fu),
    WZ_Z80_UN(0x20u), WZ_Z80_UN(0x21u), WZ_Z80_UN(0x22u), WZ_Z80_UN(0x23u),
    WZ_Z80_UN(0x24u), WZ_Z80_UN(0x25u), WZ_Z80_UN(0x26u), WZ_Z80_UN(0x27u),
    WZ_Z80_UN(0x28u), WZ_Z80_UN(0x29u), WZ_Z80_UN(0x2au), WZ_Z80_UN(0x2bu),
    WZ_Z80_UN(0x2cu), WZ_Z80_UN(0x2du), WZ_Z80_UN(0x2eu), WZ_Z80_UN(0x2fu),
    WZ_Z80_UN(0x30u), WZ_Z80_UN(0x31u),
    WZ_Z80_IMPL(0x32u, WZ_Z80_PRIMARY_OP_LD_NN_A),
    WZ_Z80_UN(0x33u), WZ_Z80_UN(0x34u), WZ_Z80_UN(0x35u), WZ_Z80_UN(0x36u),
    WZ_Z80_UN(0x37u), WZ_Z80_UN(0x38u), WZ_Z80_UN(0x39u), WZ_Z80_UN(0x3au),
    WZ_Z80_UN(0x3bu), WZ_Z80_UN(0x3cu), WZ_Z80_UN(0x3du),
    WZ_Z80_IMPL(0x3eu, WZ_Z80_PRIMARY_OP_LD_A_N),
    WZ_Z80_UN(0x3fu), WZ_Z80_UN(0x40u), WZ_Z80_UN(0x41u), WZ_Z80_UN(0x42u),
    WZ_Z80_UN(0x43u), WZ_Z80_UN(0x44u), WZ_Z80_UN(0x45u), WZ_Z80_UN(0x46u),
    WZ_Z80_UN(0x47u), WZ_Z80_UN(0x48u), WZ_Z80_UN(0x49u), WZ_Z80_UN(0x4au),
    WZ_Z80_UN(0x4bu), WZ_Z80_UN(0x4cu), WZ_Z80_UN(0x4du), WZ_Z80_UN(0x4eu),
    WZ_Z80_UN(0x4fu), WZ_Z80_UN(0x50u), WZ_Z80_UN(0x51u), WZ_Z80_UN(0x52u),
    WZ_Z80_UN(0x53u), WZ_Z80_UN(0x54u), WZ_Z80_UN(0x55u), WZ_Z80_UN(0x56u),
    WZ_Z80_UN(0x57u), WZ_Z80_UN(0x58u), WZ_Z80_UN(0x59u), WZ_Z80_UN(0x5au),
    WZ_Z80_UN(0x5bu), WZ_Z80_UN(0x5cu), WZ_Z80_UN(0x5du), WZ_Z80_UN(0x5eu),
    WZ_Z80_UN(0x5fu), WZ_Z80_UN(0x60u), WZ_Z80_UN(0x61u), WZ_Z80_UN(0x62u),
    WZ_Z80_UN(0x63u), WZ_Z80_UN(0x64u), WZ_Z80_UN(0x65u), WZ_Z80_UN(0x66u),
    WZ_Z80_UN(0x67u), WZ_Z80_UN(0x68u), WZ_Z80_UN(0x69u), WZ_Z80_UN(0x6au),
    WZ_Z80_UN(0x6bu), WZ_Z80_UN(0x6cu), WZ_Z80_UN(0x6du), WZ_Z80_UN(0x6eu),
    WZ_Z80_UN(0x6fu), WZ_Z80_UN(0x70u), WZ_Z80_UN(0x71u), WZ_Z80_UN(0x72u),
    WZ_Z80_UN(0x73u), WZ_Z80_UN(0x74u), WZ_Z80_UN(0x75u), WZ_Z80_UN(0x76u),
    WZ_Z80_UN(0x77u), WZ_Z80_UN(0x78u), WZ_Z80_UN(0x79u), WZ_Z80_UN(0x7au),
    WZ_Z80_UN(0x7bu), WZ_Z80_UN(0x7cu), WZ_Z80_UN(0x7du), WZ_Z80_UN(0x7eu),
    WZ_Z80_UN(0x7fu), WZ_Z80_UN(0x80u), WZ_Z80_UN(0x81u), WZ_Z80_UN(0x82u),
    WZ_Z80_UN(0x83u), WZ_Z80_UN(0x84u), WZ_Z80_UN(0x85u), WZ_Z80_UN(0x86u),
    WZ_Z80_UN(0x87u), WZ_Z80_UN(0x88u), WZ_Z80_UN(0x89u), WZ_Z80_UN(0x8au),
    WZ_Z80_UN(0x8bu), WZ_Z80_UN(0x8cu), WZ_Z80_UN(0x8du), WZ_Z80_UN(0x8eu),
    WZ_Z80_UN(0x8fu), WZ_Z80_UN(0x90u), WZ_Z80_UN(0x91u), WZ_Z80_UN(0x92u),
    WZ_Z80_UN(0x93u), WZ_Z80_UN(0x94u), WZ_Z80_UN(0x95u), WZ_Z80_UN(0x96u),
    WZ_Z80_UN(0x97u), WZ_Z80_UN(0x98u), WZ_Z80_UN(0x99u), WZ_Z80_UN(0x9au),
    WZ_Z80_UN(0x9bu), WZ_Z80_UN(0x9cu), WZ_Z80_UN(0x9du), WZ_Z80_UN(0x9eu),
    WZ_Z80_UN(0x9fu), WZ_Z80_UN(0xa0u), WZ_Z80_UN(0xa1u), WZ_Z80_UN(0xa2u),
    WZ_Z80_UN(0xa3u), WZ_Z80_UN(0xa4u), WZ_Z80_UN(0xa5u), WZ_Z80_UN(0xa6u),
    WZ_Z80_UN(0xa7u), WZ_Z80_UN(0xa8u), WZ_Z80_UN(0xa9u), WZ_Z80_UN(0xaau),
    WZ_Z80_UN(0xabu), WZ_Z80_UN(0xacu), WZ_Z80_UN(0xadu), WZ_Z80_UN(0xaeu),
    WZ_Z80_UN(0xafu), WZ_Z80_UN(0xb0u), WZ_Z80_UN(0xb1u), WZ_Z80_UN(0xb2u),
    WZ_Z80_UN(0xb3u), WZ_Z80_UN(0xb4u), WZ_Z80_UN(0xb5u), WZ_Z80_UN(0xb6u),
    WZ_Z80_UN(0xb7u), WZ_Z80_UN(0xb8u), WZ_Z80_UN(0xb9u), WZ_Z80_UN(0xbau),
    WZ_Z80_UN(0xbbu), WZ_Z80_UN(0xbcu), WZ_Z80_UN(0xbdu), WZ_Z80_UN(0xbeu),
    WZ_Z80_UN(0xbfu), WZ_Z80_UN(0xc0u), WZ_Z80_UN(0xc1u), WZ_Z80_UN(0xc2u),
    WZ_Z80_UN(0xc3u), WZ_Z80_UN(0xc4u), WZ_Z80_UN(0xc5u), WZ_Z80_UN(0xc6u),
    WZ_Z80_UN(0xc7u), WZ_Z80_UN(0xc8u), WZ_Z80_UN(0xc9u), WZ_Z80_UN(0xcau),
    WZ_Z80_PREFIX(0xcbu, WZ_Z80_PRIMARY_OP_PREFIX_CB),
    WZ_Z80_UN(0xccu), WZ_Z80_UN(0xcdu), WZ_Z80_UN(0xceu), WZ_Z80_UN(0xcfu),
    WZ_Z80_UN(0xd0u), WZ_Z80_UN(0xd1u), WZ_Z80_UN(0xd2u), WZ_Z80_UN(0xd3u),
    WZ_Z80_UN(0xd4u), WZ_Z80_UN(0xd5u), WZ_Z80_UN(0xd6u), WZ_Z80_UN(0xd7u),
    WZ_Z80_UN(0xd8u), WZ_Z80_UN(0xd9u), WZ_Z80_UN(0xdau), WZ_Z80_UN(0xdbu),
    WZ_Z80_UN(0xdcu),
    WZ_Z80_PREFIX(0xddu, WZ_Z80_PRIMARY_OP_PREFIX_DD),
    WZ_Z80_UN(0xdeu), WZ_Z80_UN(0xdfu), WZ_Z80_UN(0xe0u), WZ_Z80_UN(0xe1u),
    WZ_Z80_UN(0xe2u), WZ_Z80_UN(0xe3u), WZ_Z80_UN(0xe4u), WZ_Z80_UN(0xe5u),
    WZ_Z80_UN(0xe6u), WZ_Z80_UN(0xe7u), WZ_Z80_UN(0xe8u), WZ_Z80_UN(0xe9u),
    WZ_Z80_UN(0xeau), WZ_Z80_UN(0xebu), WZ_Z80_UN(0xecu),
    WZ_Z80_PREFIX(0xedu, WZ_Z80_PRIMARY_OP_PREFIX_ED),
    WZ_Z80_UN(0xeeu), WZ_Z80_UN(0xefu), WZ_Z80_UN(0xf0u), WZ_Z80_UN(0xf1u),
    WZ_Z80_UN(0xf2u), WZ_Z80_UN(0xf3u), WZ_Z80_UN(0xf4u), WZ_Z80_UN(0xf5u),
    WZ_Z80_UN(0xf6u), WZ_Z80_UN(0xf7u), WZ_Z80_UN(0xf8u), WZ_Z80_UN(0xf9u),
    WZ_Z80_UN(0xfau), WZ_Z80_UN(0xfbu), WZ_Z80_UN(0xfcu),
    WZ_Z80_PREFIX(0xfdu, WZ_Z80_PRIMARY_OP_PREFIX_FD),
    WZ_Z80_UN(0xfeu), WZ_Z80_UN(0xffu)
};

#define WZ_Z80_CB_ROW(base_value, operation_value, status_value, bit_value) \
    { (wz_byte_t)((base_value) + 0u), (operation_value), 0u, (bit_value), (status_value) }, \
    { (wz_byte_t)((base_value) + 1u), (operation_value), 1u, (bit_value), (status_value) }, \
    { (wz_byte_t)((base_value) + 2u), (operation_value), 2u, (bit_value), (status_value) }, \
    { (wz_byte_t)((base_value) + 3u), (operation_value), 3u, (bit_value), (status_value) }, \
    { (wz_byte_t)((base_value) + 4u), (operation_value), 4u, (bit_value), (status_value) }, \
    { (wz_byte_t)((base_value) + 5u), (operation_value), 5u, (bit_value), (status_value) }, \
    { (wz_byte_t)((base_value) + 6u), (operation_value), 6u, (bit_value), (status_value) }, \
    { (wz_byte_t)((base_value) + 7u), (operation_value), 7u, (bit_value), (status_value) }

static const wz_z80_cb_opcode_decode_t wz_z80_cb_opcode_table[256] = {
    WZ_Z80_CB_ROW(0x00u, WZ_Z80_CB_OP_RLC, WZ_Z80_OPCODE_IMPLEMENTED, 0u),
    WZ_Z80_CB_ROW(0x08u, WZ_Z80_CB_OP_RRC, WZ_Z80_OPCODE_IMPLEMENTED, 0u),
    WZ_Z80_CB_ROW(0x10u, WZ_Z80_CB_OP_RL, WZ_Z80_OPCODE_IMPLEMENTED, 0u),
    WZ_Z80_CB_ROW(0x18u, WZ_Z80_CB_OP_RR, WZ_Z80_OPCODE_IMPLEMENTED, 0u),
    WZ_Z80_CB_ROW(0x20u, WZ_Z80_CB_OP_SLA, WZ_Z80_OPCODE_IMPLEMENTED, 0u),
    WZ_Z80_CB_ROW(0x28u, WZ_Z80_CB_OP_SRA, WZ_Z80_OPCODE_IMPLEMENTED, 0u),
    WZ_Z80_CB_ROW(0x30u, WZ_Z80_CB_OP_SLL, WZ_Z80_OPCODE_UNDOCUMENTED, 0u),
    WZ_Z80_CB_ROW(0x38u, WZ_Z80_CB_OP_SRL, WZ_Z80_OPCODE_IMPLEMENTED, 0u),
    WZ_Z80_CB_ROW(0x40u, WZ_Z80_CB_OP_BIT, WZ_Z80_OPCODE_IMPLEMENTED, 0u),
    WZ_Z80_CB_ROW(0x48u, WZ_Z80_CB_OP_BIT, WZ_Z80_OPCODE_IMPLEMENTED, 1u),
    WZ_Z80_CB_ROW(0x50u, WZ_Z80_CB_OP_BIT, WZ_Z80_OPCODE_IMPLEMENTED, 2u),
    WZ_Z80_CB_ROW(0x58u, WZ_Z80_CB_OP_BIT, WZ_Z80_OPCODE_IMPLEMENTED, 3u),
    WZ_Z80_CB_ROW(0x60u, WZ_Z80_CB_OP_BIT, WZ_Z80_OPCODE_IMPLEMENTED, 4u),
    WZ_Z80_CB_ROW(0x68u, WZ_Z80_CB_OP_BIT, WZ_Z80_OPCODE_IMPLEMENTED, 5u),
    WZ_Z80_CB_ROW(0x70u, WZ_Z80_CB_OP_BIT, WZ_Z80_OPCODE_IMPLEMENTED, 6u),
    WZ_Z80_CB_ROW(0x78u, WZ_Z80_CB_OP_BIT, WZ_Z80_OPCODE_IMPLEMENTED, 7u),
    WZ_Z80_CB_ROW(0x80u, WZ_Z80_CB_OP_RES, WZ_Z80_OPCODE_IMPLEMENTED, 0u),
    WZ_Z80_CB_ROW(0x88u, WZ_Z80_CB_OP_RES, WZ_Z80_OPCODE_IMPLEMENTED, 1u),
    WZ_Z80_CB_ROW(0x90u, WZ_Z80_CB_OP_RES, WZ_Z80_OPCODE_IMPLEMENTED, 2u),
    WZ_Z80_CB_ROW(0x98u, WZ_Z80_CB_OP_RES, WZ_Z80_OPCODE_IMPLEMENTED, 3u),
    WZ_Z80_CB_ROW(0xa0u, WZ_Z80_CB_OP_RES, WZ_Z80_OPCODE_IMPLEMENTED, 4u),
    WZ_Z80_CB_ROW(0xa8u, WZ_Z80_CB_OP_RES, WZ_Z80_OPCODE_IMPLEMENTED, 5u),
    WZ_Z80_CB_ROW(0xb0u, WZ_Z80_CB_OP_RES, WZ_Z80_OPCODE_IMPLEMENTED, 6u),
    WZ_Z80_CB_ROW(0xb8u, WZ_Z80_CB_OP_RES, WZ_Z80_OPCODE_IMPLEMENTED, 7u),
    WZ_Z80_CB_ROW(0xc0u, WZ_Z80_CB_OP_SET, WZ_Z80_OPCODE_IMPLEMENTED, 0u),
    WZ_Z80_CB_ROW(0xc8u, WZ_Z80_CB_OP_SET, WZ_Z80_OPCODE_IMPLEMENTED, 1u),
    WZ_Z80_CB_ROW(0xd0u, WZ_Z80_CB_OP_SET, WZ_Z80_OPCODE_IMPLEMENTED, 2u),
    WZ_Z80_CB_ROW(0xd8u, WZ_Z80_CB_OP_SET, WZ_Z80_OPCODE_IMPLEMENTED, 3u),
    WZ_Z80_CB_ROW(0xe0u, WZ_Z80_CB_OP_SET, WZ_Z80_OPCODE_IMPLEMENTED, 4u),
    WZ_Z80_CB_ROW(0xe8u, WZ_Z80_CB_OP_SET, WZ_Z80_OPCODE_IMPLEMENTED, 5u),
    WZ_Z80_CB_ROW(0xf0u, WZ_Z80_CB_OP_SET, WZ_Z80_OPCODE_IMPLEMENTED, 6u),
    WZ_Z80_CB_ROW(0xf8u, WZ_Z80_CB_OP_SET, WZ_Z80_OPCODE_IMPLEMENTED, 7u)
};

#undef WZ_Z80_CB_ROW

#undef WZ_Z80_PREFIX
#undef WZ_Z80_IMPL
#undef WZ_Z80_UN

static wz_result_t wz_z80_bus(wz_machine_t* machine,
                              wz_bus_cycle_t cycle,
                              wz_master_tick_t offset,
                              wz_word_t address,
                              wz_byte_t* value,
                              wz_byte_t t_states);

size_t wz_z80_primary_opcode_count(void)
{
    return sizeof(wz_z80_primary_opcode_table) / sizeof(wz_z80_primary_opcode_table[0]);
}

const wz_z80_opcode_decode_t* wz_z80_primary_opcode_decode(wz_byte_t opcode)
{
    return &wz_z80_primary_opcode_table[opcode];
}

size_t wz_z80_cb_opcode_count(void)
{
    return sizeof(wz_z80_cb_opcode_table) / sizeof(wz_z80_cb_opcode_table[0]);
}

const wz_z80_cb_opcode_decode_t* wz_z80_cb_opcode_decode(wz_byte_t opcode)
{
    return &wz_z80_cb_opcode_table[opcode];
}

static wz_z80_ed_opcode_decode_t wz_z80_ed_make(wz_byte_t opcode,
                                                wz_z80_ed_operation_t operation,
                                                wz_byte_t operand,
                                                wz_z80_opcode_status_t status)
{
    wz_z80_ed_opcode_decode_t decode;
    decode.opcode = opcode;
    decode.operation = operation;
    decode.operand = operand;
    decode.status = status;
    return decode;
}

size_t wz_z80_ed_opcode_count(void)
{
    return 256u;
}

wz_z80_ed_opcode_decode_t wz_z80_ed_opcode_decode(wz_byte_t opcode)
{
    switch (opcode) {
    case 0x40u: case 0x48u: case 0x50u: case 0x58u:
    case 0x60u: case 0x68u: case 0x78u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_IN_R_C,
                              (wz_byte_t)((opcode >> 3u) & 0x07u),
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0x70u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_IN_R_C,
                              WZ_Z80_TARGET_HL_INDIRECT, WZ_Z80_OPCODE_UNDOCUMENTED);
    case 0x41u: case 0x49u: case 0x51u: case 0x59u:
    case 0x61u: case 0x69u: case 0x79u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_OUT_C_R,
                              (wz_byte_t)((opcode >> 3u) & 0x07u),
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0x71u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_OUT_C_R,
                              WZ_Z80_TARGET_HL_INDIRECT, WZ_Z80_OPCODE_UNDOCUMENTED);
    case 0x42u: case 0x52u: case 0x62u: case 0x72u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_SBC_HL_RR,
                              (wz_byte_t)((opcode >> 4u) & 0x03u),
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0x4au: case 0x5au: case 0x6au: case 0x7au:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_ADC_HL_RR,
                              (wz_byte_t)((opcode >> 4u) & 0x03u),
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0x43u: case 0x53u: case 0x63u: case 0x73u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_LD_NN_RR,
                              (wz_byte_t)((opcode >> 4u) & 0x03u),
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0x4bu: case 0x5bu: case 0x6bu: case 0x7bu:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_LD_RR_NN,
                              (wz_byte_t)((opcode >> 4u) & 0x03u),
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0x44u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_NEG, 0u,
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0x4cu: case 0x54u: case 0x5cu:
    case 0x64u: case 0x6cu: case 0x74u: case 0x7cu:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_NEG, 0u, WZ_Z80_OPCODE_UNDOCUMENTED);
    case 0x45u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_RETN, 0u, WZ_Z80_OPCODE_IMPLEMENTED);
    case 0x55u: case 0x5du: case 0x65u: case 0x6du:
    case 0x75u: case 0x7du:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_RETN, 0u, WZ_Z80_OPCODE_UNDOCUMENTED);
    case 0x4du:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_RETI, 0u, WZ_Z80_OPCODE_IMPLEMENTED);
    case 0x46u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_IM, 0u, WZ_Z80_OPCODE_IMPLEMENTED);
    case 0x56u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_IM, 1u, WZ_Z80_OPCODE_IMPLEMENTED);
    case 0x5eu:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_IM, 2u, WZ_Z80_OPCODE_IMPLEMENTED);
    case 0x4eu: case 0x66u: case 0x6eu:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_IM, 0u, WZ_Z80_OPCODE_UNDOCUMENTED);
    case 0x76u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_IM, 1u, WZ_Z80_OPCODE_UNDOCUMENTED);
    case 0x7eu:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_IM, 2u, WZ_Z80_OPCODE_UNDOCUMENTED);
    case 0x47u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_LD_I_A, 0u, WZ_Z80_OPCODE_IMPLEMENTED);
    case 0x4fu:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_LD_R_A, 0u, WZ_Z80_OPCODE_IMPLEMENTED);
    case 0x57u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_LD_A_I, 0u, WZ_Z80_OPCODE_IMPLEMENTED);
    case 0x5fu:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_LD_A_R, 0u, WZ_Z80_OPCODE_IMPLEMENTED);
    case 0x67u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_RRD, 0u, WZ_Z80_OPCODE_IMPLEMENTED);
    case 0x6fu:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_RLD, 0u, WZ_Z80_OPCODE_IMPLEMENTED);
    case 0xa0u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_LDI, 0u,
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0xa1u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_CPI, 0u,
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0xa2u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_INI, 0u,
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0xa3u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_OUTI, 0u,
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0xa8u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_LDD, 0u,
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0xa9u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_CPD, 0u,
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0xaau:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_IND, 0u,
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0xabu:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_OUTD, 0u,
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0xb0u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_LDIR, 0u,
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0xb1u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_CPIR, 0u,
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0xb2u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_INIR, 0u,
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0xb3u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_OTIR, 0u,
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0xb8u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_LDDR, 0u,
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0xb9u:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_CPDR, 0u,
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0xbau:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_INDR, 0u,
                              WZ_Z80_OPCODE_IMPLEMENTED);
    case 0xbbu:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_OTDR, 0u,
                              WZ_Z80_OPCODE_IMPLEMENTED);
    default:
        return wz_z80_ed_make(opcode, WZ_Z80_ED_OP_UNSUPPORTED, 0u,
                              WZ_Z80_OPCODE_UNDOCUMENTED);
    }
}

static wz_word_t wz_z80_hl(const wz_z80_state_t* state)
{
    return (wz_word_t)state->main.l | ((wz_word_t)state->main.h << 8u);
}

static wz_byte_t* wz_z80_target_register(wz_z80_state_t* state, wz_byte_t target)
{
    switch (target) {
    case 0u:
        return &state->main.b;
    case 1u:
        return &state->main.c;
    case 2u:
        return &state->main.d;
    case 3u:
        return &state->main.e;
    case 4u:
        return &state->main.h;
    case 5u:
        return &state->main.l;
    case 7u:
        return &state->main.a;
    default:
        return 0;
    }
}

static wz_word_t wz_z80_get_rr(const wz_z80_state_t* state, wz_byte_t pair)
{
    switch (pair) {
    case 0u:
        return (wz_word_t)state->main.c | ((wz_word_t)state->main.b << 8u);
    case 1u:
        return (wz_word_t)state->main.e | ((wz_word_t)state->main.d << 8u);
    case 2u:
        return wz_z80_hl(state);
    default:
        return state->stack_pointer;
    }
}

static void wz_z80_set_rr(wz_z80_state_t* state, wz_byte_t pair, wz_word_t value)
{
    switch (pair) {
    case 0u:
        state->main.b = (wz_byte_t)(value >> 8u);
        state->main.c = (wz_byte_t)(value & 0xffu);
        break;
    case 1u:
        state->main.d = (wz_byte_t)(value >> 8u);
        state->main.e = (wz_byte_t)(value & 0xffu);
        break;
    case 2u:
        state->main.h = (wz_byte_t)(value >> 8u);
        state->main.l = (wz_byte_t)(value & 0xffu);
        break;
    default:
        state->stack_pointer = value;
        break;
    }
}

static bool wz_z80_parity_even(wz_byte_t value)
{
    value ^= (wz_byte_t)(value >> 4u);
    value ^= (wz_byte_t)(value >> 2u);
    value ^= (wz_byte_t)(value >> 1u);
    return (value & 1u) == 0u;
}

static wz_byte_t wz_z80_sz53p_flags(wz_byte_t value)
{
    wz_byte_t flags = (wz_byte_t)(value & (WZ_Z80_FLAG_S | WZ_Z80_FLAG_Y | WZ_Z80_FLAG_X));
    if (value == 0u) {
        flags |= WZ_Z80_FLAG_Z;
    }
    if (wz_z80_parity_even(value)) {
        flags |= WZ_Z80_FLAG_PV;
    }
    return flags;
}

static wz_result_t wz_z80_cb_load_target(wz_machine_t* machine,
                                         const wz_z80_cb_opcode_decode_t* decode,
                                         wz_byte_t* value)
{
    wz_byte_t* reg;

    if (decode->target == WZ_Z80_TARGET_HL_INDIRECT) {
        return wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 8u, wz_z80_hl(&machine->cpu), value, 3u);
    }

    reg = wz_z80_target_register(&machine->cpu, decode->target);
    if (reg == 0) {
        return WZ_RESULT_INVALID_STATE;
    }
    *value = *reg;
    return WZ_RESULT_OK;
}

static wz_result_t wz_z80_execute_index_prefix(wz_machine_t* machine,
                                               wz_byte_t initial_prefix)
{
    wz_byte_t active_prefix = initial_prefix;
    wz_byte_t opcode = 0u;
    wz_byte_t low = 0u;
    wz_byte_t high = 0u;
    wz_byte_t value;
    wz_byte_t flags;
    wz_word_t* index;
    wz_word_t address;
    wz_word_t operand;
    wz_word_t result;
    wz_dword_t wide;
    size_t prefix_count = 1u;

    machine->master_tick += 8u;
    while (machine->memory[machine->cpu.program_counter] == 0xddu ||
           machine->memory[machine->cpu.program_counter] == 0xfdu) {
        wz_byte_t prefix = 0u;
        if (prefix_count == 65536u ||
            wz_z80_bus(machine, WZ_BUS_M1_OPCODE_FETCH, 0u,
                       machine->cpu.program_counter, &prefix, 4u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
        wz_z80_increment_r(&machine->cpu);
        machine->master_tick += 8u;
        active_prefix = prefix;
        prefix_count += 1u;
    }

    if (machine->memory[machine->cpu.program_counter] == 0xcbu) {
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }

    opcode = machine->memory[machine->cpu.program_counter];
    switch (opcode) {
    case 0x09u: case 0x19u: case 0x21u: case 0x22u: case 0x23u:
    case 0x29u: case 0x2au: case 0x2bu: case 0x39u: case 0xe9u: case 0xf9u:
        break;
    default:
        return wz_z80_step(machine);
    }

    if (wz_z80_bus(machine, WZ_BUS_M1_OPCODE_FETCH, 0u,
                   machine->cpu.program_counter, &opcode, 4u) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
    wz_z80_increment_r(&machine->cpu);
    index = active_prefix == 0xddu ? &machine->cpu.ix : &machine->cpu.iy;

    switch (opcode) {
    case 0x21u:
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 8u,
                       machine->cpu.program_counter, &low, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 14u,
                       machine->cpu.program_counter, &high, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
        *index = (wz_word_t)low | ((wz_word_t)high << 8u);
        machine->master_tick += 20u;
        return WZ_RESULT_OK;
    case 0x22u:
    case 0x2au:
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 8u,
                       machine->cpu.program_counter, &low, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 14u,
                       machine->cpu.program_counter, &high, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
        address = (wz_word_t)low | ((wz_word_t)high << 8u);
        machine->cpu.memptr = wz_z80_add16(address, 1u);
        if (opcode == 0x22u) {
            value = (wz_byte_t)(*index & 0xffu);
            if (wz_z80_bus(machine, WZ_BUS_MEMORY_WRITE, 20u,
                           address, &value, 3u) != WZ_RESULT_OK) {
                return WZ_RESULT_INVALID_STATE;
            }
            value = (wz_byte_t)(*index >> 8u);
            if (wz_z80_bus(machine, WZ_BUS_MEMORY_WRITE, 26u,
                           wz_z80_add16(address, 1u), &value, 3u) != WZ_RESULT_OK) {
                return WZ_RESULT_INVALID_STATE;
            }
        } else {
            if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 20u,
                           address, &low, 3u) != WZ_RESULT_OK ||
                wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 26u,
                           wz_z80_add16(address, 1u), &high, 3u) != WZ_RESULT_OK) {
                return WZ_RESULT_INVALID_STATE;
            }
            *index = (wz_word_t)low | ((wz_word_t)high << 8u);
        }
        machine->master_tick += 32u;
        return WZ_RESULT_OK;
    case 0x23u:
        *index = wz_z80_add16(*index, 1u);
        machine->master_tick += 12u;
        return WZ_RESULT_OK;
    case 0x2bu:
        *index = wz_z80_add16(*index, 0xffffu);
        machine->master_tick += 12u;
        return WZ_RESULT_OK;
    case 0x09u:
    case 0x19u:
    case 0x29u:
    case 0x39u:
        operand = opcode == 0x29u ? *index :
            wz_z80_get_rr(&machine->cpu, (wz_byte_t)(opcode >> 4u));
        result = (wz_word_t)(*index + operand);
        wide = (wz_dword_t)*index + (wz_dword_t)operand;
        flags = (wz_byte_t)(machine->cpu.main.f &
                            (WZ_Z80_FLAG_S | WZ_Z80_FLAG_Z | WZ_Z80_FLAG_PV));
        flags |= (wz_byte_t)((result >> 8u) & (WZ_Z80_FLAG_Y | WZ_Z80_FLAG_X));
        if (((*index ^ operand ^ result) & 0x1000u) != 0u) {
            flags |= WZ_Z80_FLAG_H;
        }
        if ((wide & 0x10000u) != 0u) {
            flags |= WZ_Z80_FLAG_C;
        }
        machine->cpu.memptr = wz_z80_add16(*index, 1u);
        *index = result;
        machine->cpu.main.f = flags;
        machine->master_tick += 22u;
        return WZ_RESULT_OK;
    case 0xe9u:
        machine->cpu.program_counter = *index;
        machine->master_tick += 8u;
        return WZ_RESULT_OK;
    case 0xf9u:
        machine->cpu.stack_pointer = *index;
        machine->master_tick += 12u;
        return WZ_RESULT_OK;
    default:
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }
}

static wz_result_t wz_z80_cb_store_target(wz_machine_t* machine,
                                          const wz_z80_cb_opcode_decode_t* decode,
                                          wz_byte_t value)
{
    wz_byte_t* reg;

    if (decode->target == WZ_Z80_TARGET_HL_INDIRECT) {
        return wz_z80_bus(machine, WZ_BUS_MEMORY_WRITE, 14u, wz_z80_hl(&machine->cpu), &value, 3u);
    }

    reg = wz_z80_target_register(&machine->cpu, decode->target);
    if (reg == 0) {
        return WZ_RESULT_INVALID_STATE;
    }
    *reg = value;
    return WZ_RESULT_OK;
}

static wz_result_t wz_z80_execute_cb(wz_machine_t* machine,
                                     const wz_z80_cb_opcode_decode_t* decode)
{
    wz_byte_t value = 0u;
    wz_byte_t result = 0u;
    wz_byte_t carry = 0u;
    wz_byte_t bit_flag_source = 0u;
    wz_byte_t old_carry = (wz_byte_t)(machine->cpu.main.f & WZ_Z80_FLAG_C);

    if (wz_z80_cb_load_target(machine, decode, &value) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }

    switch (decode->operation) {
    case WZ_Z80_CB_OP_RLC:
        carry = (wz_byte_t)((value >> 7u) & 1u);
        result = (wz_byte_t)((value << 1u) | carry);
        machine->cpu.main.f = (wz_byte_t)(wz_z80_sz53p_flags(result) | carry);
        break;
    case WZ_Z80_CB_OP_RRC:
        carry = (wz_byte_t)(value & 1u);
        result = (wz_byte_t)((value >> 1u) | (carry << 7u));
        machine->cpu.main.f = (wz_byte_t)(wz_z80_sz53p_flags(result) | carry);
        break;
    case WZ_Z80_CB_OP_RL:
        carry = (wz_byte_t)((value >> 7u) & 1u);
        result = (wz_byte_t)((value << 1u) | old_carry);
        machine->cpu.main.f = (wz_byte_t)(wz_z80_sz53p_flags(result) | carry);
        break;
    case WZ_Z80_CB_OP_RR:
        carry = (wz_byte_t)(value & 1u);
        result = (wz_byte_t)((value >> 1u) | (old_carry << 7u));
        machine->cpu.main.f = (wz_byte_t)(wz_z80_sz53p_flags(result) | carry);
        break;
    case WZ_Z80_CB_OP_SLA:
        carry = (wz_byte_t)((value >> 7u) & 1u);
        result = (wz_byte_t)(value << 1u);
        machine->cpu.main.f = (wz_byte_t)(wz_z80_sz53p_flags(result) | carry);
        break;
    case WZ_Z80_CB_OP_SRA:
        carry = (wz_byte_t)(value & 1u);
        result = (wz_byte_t)((value >> 1u) | (value & WZ_Z80_FLAG_S));
        machine->cpu.main.f = (wz_byte_t)(wz_z80_sz53p_flags(result) | carry);
        break;
    case WZ_Z80_CB_OP_SLL:
        carry = (wz_byte_t)((value >> 7u) & 1u);
        result = (wz_byte_t)((value << 1u) | 1u);
        machine->cpu.main.f = (wz_byte_t)(wz_z80_sz53p_flags(result) | carry);
        break;
    case WZ_Z80_CB_OP_SRL:
        carry = (wz_byte_t)(value & 1u);
        result = (wz_byte_t)(value >> 1u);
        machine->cpu.main.f = (wz_byte_t)(wz_z80_sz53p_flags(result) | carry);
        break;
    case WZ_Z80_CB_OP_BIT:
        result = (wz_byte_t)(value & (wz_byte_t)(1u << decode->bit));
        bit_flag_source = decode->target == WZ_Z80_TARGET_HL_INDIRECT ?
            (wz_byte_t)(wz_z80_hl(&machine->cpu) >> 8u) : value;
        machine->cpu.main.f = (wz_byte_t)((machine->cpu.main.f & WZ_Z80_FLAG_C) |
                                          WZ_Z80_FLAG_H |
                                          (bit_flag_source & (WZ_Z80_FLAG_Y | WZ_Z80_FLAG_X)));
        if (result == 0u) {
            machine->cpu.main.f |= (WZ_Z80_FLAG_Z | WZ_Z80_FLAG_PV);
        }
        if (decode->bit == 7u && result != 0u) {
            machine->cpu.main.f |= WZ_Z80_FLAG_S;
        }
        machine->master_tick += decode->target == WZ_Z80_TARGET_HL_INDIRECT ? 24u : 16u;
        return WZ_RESULT_OK;
    case WZ_Z80_CB_OP_RES:
        result = (wz_byte_t)(value & (wz_byte_t)~(wz_byte_t)(1u << decode->bit));
        break;
    case WZ_Z80_CB_OP_SET:
        result = (wz_byte_t)(value | (wz_byte_t)(1u << decode->bit));
        break;
    default:
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }

    if (wz_z80_cb_store_target(machine, decode, result) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    machine->master_tick += decode->target == WZ_Z80_TARGET_HL_INDIRECT ? 30u : 16u;
    return WZ_RESULT_OK;
}

static wz_byte_t wz_z80_neg_flags(wz_byte_t value, wz_byte_t result)
{
    wz_byte_t flags = (wz_byte_t)(result & (WZ_Z80_FLAG_S | WZ_Z80_FLAG_Y | WZ_Z80_FLAG_X));
    flags |= WZ_Z80_FLAG_N;
    if (result == 0u) {
        flags |= WZ_Z80_FLAG_Z;
    }
    if ((value & 0x0fu) != 0u) {
        flags |= WZ_Z80_FLAG_H;
    }
    if (value == 0x80u) {
        flags |= WZ_Z80_FLAG_PV;
    }
    if (value != 0u) {
        flags |= WZ_Z80_FLAG_C;
    }
    return flags;
}

static wz_byte_t wz_z80_ld_a_ir_flags(const wz_machine_t* machine, wz_byte_t value)
{
    wz_byte_t flags = (wz_byte_t)((machine->cpu.main.f & WZ_Z80_FLAG_C) |
                                  (value & (WZ_Z80_FLAG_S | WZ_Z80_FLAG_Y | WZ_Z80_FLAG_X)));
    if (value == 0u) {
        flags |= WZ_Z80_FLAG_Z;
    }
    if (machine->cpu.iff2 != 0u) {
        flags |= WZ_Z80_FLAG_PV;
    }
    return flags;
}

static wz_byte_t wz_z80_in_flags(const wz_machine_t* machine, wz_byte_t value)
{
    return (wz_byte_t)((machine->cpu.main.f & WZ_Z80_FLAG_C) | wz_z80_sz53p_flags(value));
}

static wz_byte_t wz_z80_add16_flags(wz_word_t left, wz_word_t right, wz_byte_t carry, wz_word_t result)
{
    wz_dword_t wide = (wz_dword_t)left + (wz_dword_t)right + (wz_dword_t)carry;
    wz_byte_t high = (wz_byte_t)(result >> 8u);
    wz_byte_t flags = (wz_byte_t)(high & (WZ_Z80_FLAG_S | WZ_Z80_FLAG_Y | WZ_Z80_FLAG_X));

    if (result == 0u) {
        flags |= WZ_Z80_FLAG_Z;
    }
    if (((left ^ right ^ result) & 0x1000u) != 0u) {
        flags |= WZ_Z80_FLAG_H;
    }
    if (((~(left ^ right) & (left ^ result)) & 0x8000u) != 0u) {
        flags |= WZ_Z80_FLAG_PV;
    }
    if ((wide & 0x10000u) != 0u) {
        flags |= WZ_Z80_FLAG_C;
    }
    return flags;
}

static wz_byte_t wz_z80_sub16_flags(wz_word_t left, wz_word_t right, wz_byte_t carry, wz_word_t result)
{
    wz_byte_t high = (wz_byte_t)(result >> 8u);
    wz_byte_t flags = (wz_byte_t)(WZ_Z80_FLAG_N |
                                  (high & (WZ_Z80_FLAG_S | WZ_Z80_FLAG_Y | WZ_Z80_FLAG_X)));

    if (result == 0u) {
        flags |= WZ_Z80_FLAG_Z;
    }
    if (((left ^ right ^ result) & 0x1000u) != 0u) {
        flags |= WZ_Z80_FLAG_H;
    }
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u) {
        flags |= WZ_Z80_FLAG_PV;
    }
    if ((wz_dword_t)left < ((wz_dword_t)right + (wz_dword_t)carry)) {
        flags |= WZ_Z80_FLAG_C;
    }
    return flags;
}

static wz_result_t wz_z80_execute_ld_block(wz_machine_t* machine,
                                           bool decrement,
                                           bool repeat)
{
    wz_word_t source = wz_z80_hl(&machine->cpu);
    wz_word_t destination = wz_z80_get_rr(&machine->cpu, 1u);
    wz_word_t count = wz_z80_get_rr(&machine->cpu, 0u);
    wz_byte_t value = 0u;
    wz_byte_t sum;
    wz_byte_t flags;

    if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 8u, source, &value, 3u) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (wz_z80_bus(machine, WZ_BUS_MEMORY_WRITE, 14u, destination, &value, 3u) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }

    wz_z80_set_rr(&machine->cpu, 2u, wz_z80_add16(source, decrement ? 0xffffu : 1u));
    wz_z80_set_rr(&machine->cpu, 1u, wz_z80_add16(destination, decrement ? 0xffffu : 1u));
    count = wz_z80_add16(count, 0xffffu);
    wz_z80_set_rr(&machine->cpu, 0u, count);

    sum = (wz_byte_t)(machine->cpu.main.a + value);
    flags = (wz_byte_t)(machine->cpu.main.f & (WZ_Z80_FLAG_S | WZ_Z80_FLAG_Z | WZ_Z80_FLAG_C));
    flags |= (wz_byte_t)(sum & WZ_Z80_FLAG_X);
    flags |= (wz_byte_t)((sum & 0x02u) << 4u);
    if (count != 0u) {
        flags |= WZ_Z80_FLAG_PV;
    }
    machine->cpu.main.f = flags;

    if (repeat && count != 0u) {
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 0xfffeu);
        machine->cpu.memptr = wz_z80_add16(machine->cpu.program_counter, 1u);
        machine->master_tick += 42u;
    } else {
        machine->master_tick += 32u;
    }
    return WZ_RESULT_OK;
}

static wz_result_t wz_z80_execute_cp_block(wz_machine_t* machine,
                                           bool decrement,
                                           bool repeat)
{
    wz_word_t address = wz_z80_hl(&machine->cpu);
    wz_word_t count = wz_z80_get_rr(&machine->cpu, 0u);
    wz_byte_t value = 0u;
    wz_byte_t result;
    wz_byte_t adjusted;
    wz_byte_t flags;

    if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 8u, address, &value, 3u) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }

    result = (wz_byte_t)(machine->cpu.main.a - value);
    flags = (wz_byte_t)((machine->cpu.main.f & WZ_Z80_FLAG_C) | WZ_Z80_FLAG_N);
    flags |= (wz_byte_t)(result & WZ_Z80_FLAG_S);
    if (result == 0u) {
        flags |= WZ_Z80_FLAG_Z;
    }
    if (((machine->cpu.main.a ^ value ^ result) & 0x10u) != 0u) {
        flags |= WZ_Z80_FLAG_H;
    }

    wz_z80_set_rr(&machine->cpu, 2u, wz_z80_add16(address, decrement ? 0xffffu : 1u));
    count = wz_z80_add16(count, 0xffffu);
    wz_z80_set_rr(&machine->cpu, 0u, count);
    if (count != 0u) {
        flags |= WZ_Z80_FLAG_PV;
    }

    adjusted = (wz_byte_t)(result - ((flags & WZ_Z80_FLAG_H) != 0u ? 1u : 0u));
    flags |= (wz_byte_t)(adjusted & WZ_Z80_FLAG_X);
    flags |= (wz_byte_t)((adjusted & 0x02u) << 4u);
    machine->cpu.main.f = flags;

    if (repeat && count != 0u && result != 0u) {
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 0xfffeu);
        machine->cpu.memptr = wz_z80_add16(machine->cpu.program_counter, 1u);
        machine->master_tick += 42u;
    } else {
        machine->cpu.memptr = wz_z80_add16(machine->cpu.memptr, decrement ? 0xffffu : 1u);
        machine->master_tick += 32u;
    }
    return WZ_RESULT_OK;
}

static wz_byte_t wz_z80_block_io_flags(wz_byte_t counter, wz_byte_t value, wz_word_t sum)
{
    wz_byte_t flags = (wz_byte_t)(counter & (WZ_Z80_FLAG_S | WZ_Z80_FLAG_Y | WZ_Z80_FLAG_X));
    if (counter == 0u) {
        flags |= WZ_Z80_FLAG_Z;
    }
    if (wz_z80_parity_even((wz_byte_t)(((wz_byte_t)sum & 0x07u) ^ counter))) {
        flags |= WZ_Z80_FLAG_PV;
    }
    if (sum > 0xffu) {
        flags |= (WZ_Z80_FLAG_H | WZ_Z80_FLAG_C);
    }
    if ((value & 0x80u) != 0u) {
        flags |= WZ_Z80_FLAG_N;
    }
    return flags;
}

static wz_result_t wz_z80_execute_in_block(wz_machine_t* machine,
                                           bool decrement,
                                           bool repeat)
{
    wz_word_t address = wz_z80_hl(&machine->cpu);
    wz_word_t port = wz_z80_get_rr(&machine->cpu, 0u);
    wz_word_t sum;
    wz_byte_t value = 0u;

    if (wz_z80_bus(machine, WZ_BUS_IO_READ, 8u, port, &value, 4u) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (wz_z80_bus(machine, WZ_BUS_MEMORY_WRITE, 16u, address, &value, 3u) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }

    machine->cpu.main.b = (wz_byte_t)(machine->cpu.main.b - 1u);
    machine->cpu.memptr = wz_z80_add16(port, decrement ? 0xffffu : 1u);
    wz_z80_set_rr(&machine->cpu, 2u, wz_z80_add16(address, decrement ? 0xffffu : 1u));
    sum = (wz_word_t)value + (wz_byte_t)(machine->cpu.main.c + (decrement ? 0xffu : 1u));
    machine->cpu.main.f = wz_z80_block_io_flags(machine->cpu.main.b, value, sum);

    if (repeat && machine->cpu.main.b != 0u) {
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 0xfffeu);
        machine->master_tick += 42u;
    } else {
        machine->master_tick += 32u;
    }
    return WZ_RESULT_OK;
}

static wz_result_t wz_z80_execute_out_block(wz_machine_t* machine,
                                            bool decrement,
                                            bool repeat)
{
    wz_word_t address = wz_z80_hl(&machine->cpu);
    wz_word_t port;
    wz_word_t sum;
    wz_byte_t value = 0u;

    if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 8u, address, &value, 3u) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    machine->cpu.main.b = (wz_byte_t)(machine->cpu.main.b - 1u);
    wz_z80_set_rr(&machine->cpu, 2u, wz_z80_add16(address, decrement ? 0xffffu : 1u));
    port = wz_z80_get_rr(&machine->cpu, 0u);
    machine->cpu.memptr = wz_z80_add16(port, decrement ? 0xffffu : 1u);
    if (wz_z80_bus(machine, WZ_BUS_IO_WRITE, 14u, port, &value, 4u) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }

    sum = (wz_word_t)value + machine->cpu.main.l;
    machine->cpu.main.f = wz_z80_block_io_flags(machine->cpu.main.b, value, sum);
    if (repeat && machine->cpu.main.b != 0u) {
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 0xfffeu);
        machine->master_tick += 42u;
    } else {
        machine->master_tick += 32u;
    }
    return WZ_RESULT_OK;
}

static wz_result_t wz_z80_execute_ed(wz_machine_t* machine,
                                     wz_z80_ed_opcode_decode_t decode)
{
    wz_byte_t* target;
    wz_byte_t carry_in;
    wz_byte_t value;
    wz_byte_t low = 0u;
    wz_byte_t high = 0u;
    wz_word_t address;
    wz_word_t hl;
    wz_word_t pair_value;
    wz_word_t result16;

    switch (decode.operation) {
    case WZ_Z80_ED_OP_NEG:
        value = machine->cpu.main.a;
        machine->cpu.main.a = (wz_byte_t)(0u - value);
        machine->cpu.main.f = wz_z80_neg_flags(value, machine->cpu.main.a);
        machine->master_tick += 16u;
        return WZ_RESULT_OK;
    case WZ_Z80_ED_OP_IM:
        if (decode.operand > (wz_byte_t)WZ_Z80_INTERRUPT_MODE_2) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.interrupt_mode = decode.operand;
        machine->master_tick += 16u;
        return WZ_RESULT_OK;
    case WZ_Z80_ED_OP_LD_I_A:
        machine->cpu.i = machine->cpu.main.a;
        machine->master_tick += 18u;
        return WZ_RESULT_OK;
    case WZ_Z80_ED_OP_LD_R_A:
        machine->cpu.r = machine->cpu.main.a;
        machine->master_tick += 18u;
        return WZ_RESULT_OK;
    case WZ_Z80_ED_OP_LD_A_I:
        machine->cpu.main.a = machine->cpu.i;
        machine->cpu.main.f = wz_z80_ld_a_ir_flags(machine, machine->cpu.main.a);
        machine->master_tick += 18u;
        return WZ_RESULT_OK;
    case WZ_Z80_ED_OP_LD_A_R:
        machine->cpu.main.a = machine->cpu.r;
        machine->cpu.main.f = wz_z80_ld_a_ir_flags(machine, machine->cpu.main.a);
        machine->master_tick += 18u;
        return WZ_RESULT_OK;
    case WZ_Z80_ED_OP_IN_R_C:
        address = wz_z80_get_rr(&machine->cpu, 0u);
        machine->cpu.memptr = wz_z80_add16(address, 1u);
        value = 0u;
        if (wz_z80_bus(machine, WZ_BUS_IO_READ, 8u, address, &value, 4u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        target = wz_z80_target_register(&machine->cpu, decode.operand);
        if (target != 0) {
            *target = value;
        } else if (decode.operand != WZ_Z80_TARGET_HL_INDIRECT) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.main.f = wz_z80_in_flags(machine, value);
        machine->master_tick += 24u;
        return WZ_RESULT_OK;
    case WZ_Z80_ED_OP_OUT_C_R:
        address = wz_z80_get_rr(&machine->cpu, 0u);
        machine->cpu.memptr = wz_z80_add16(address, 1u);
        target = wz_z80_target_register(&machine->cpu, decode.operand);
        if (target != 0) {
            value = *target;
        } else if (decode.operand == WZ_Z80_TARGET_HL_INDIRECT) {
            value = 0u;
        } else {
            return WZ_RESULT_INVALID_STATE;
        }
        if (wz_z80_bus(machine, WZ_BUS_IO_WRITE, 8u, address, &value, 4u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->master_tick += 24u;
        return WZ_RESULT_OK;
    case WZ_Z80_ED_OP_ADC_HL_RR:
        hl = wz_z80_hl(&machine->cpu);
        pair_value = wz_z80_get_rr(&machine->cpu, decode.operand);
        carry_in = (wz_byte_t)(machine->cpu.main.f & WZ_Z80_FLAG_C);
        result16 = (wz_word_t)(hl + pair_value + carry_in);
        machine->cpu.memptr = wz_z80_add16(hl, 1u);
        wz_z80_set_rr(&machine->cpu, 2u, result16);
        machine->cpu.main.f = wz_z80_add16_flags(hl, pair_value, carry_in, result16);
        machine->master_tick += 30u;
        return WZ_RESULT_OK;
    case WZ_Z80_ED_OP_SBC_HL_RR:
        hl = wz_z80_hl(&machine->cpu);
        pair_value = wz_z80_get_rr(&machine->cpu, decode.operand);
        carry_in = (wz_byte_t)(machine->cpu.main.f & WZ_Z80_FLAG_C);
        result16 = (wz_word_t)(hl - pair_value - carry_in);
        machine->cpu.memptr = wz_z80_add16(hl, 1u);
        wz_z80_set_rr(&machine->cpu, 2u, result16);
        machine->cpu.main.f = wz_z80_sub16_flags(hl, pair_value, carry_in, result16);
        machine->master_tick += 30u;
        return WZ_RESULT_OK;
    case WZ_Z80_ED_OP_RRD:
        address = wz_z80_hl(&machine->cpu);
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 8u, address, &value, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        low = (wz_byte_t)(machine->cpu.main.a & 0x0fu);
        machine->cpu.main.a = (wz_byte_t)((machine->cpu.main.a & 0xf0u) | (value & 0x0fu));
        value = (wz_byte_t)((low << 4u) | (value >> 4u));
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_WRITE, 14u, address, &value, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.main.f = (wz_byte_t)((machine->cpu.main.f & WZ_Z80_FLAG_C) |
                                          wz_z80_sz53p_flags(machine->cpu.main.a));
        machine->cpu.memptr = wz_z80_add16(address, 1u);
        machine->master_tick += 36u;
        return WZ_RESULT_OK;
    case WZ_Z80_ED_OP_RLD:
        address = wz_z80_hl(&machine->cpu);
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 8u, address, &value, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        low = (wz_byte_t)(machine->cpu.main.a & 0x0fu);
        machine->cpu.main.a = (wz_byte_t)((machine->cpu.main.a & 0xf0u) | (value >> 4u));
        value = (wz_byte_t)((value << 4u) | low);
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_WRITE, 14u, address, &value, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.main.f = (wz_byte_t)((machine->cpu.main.f & WZ_Z80_FLAG_C) |
                                          wz_z80_sz53p_flags(machine->cpu.main.a));
        machine->cpu.memptr = wz_z80_add16(address, 1u);
        machine->master_tick += 36u;
        return WZ_RESULT_OK;
    case WZ_Z80_ED_OP_LDI:
        return wz_z80_execute_ld_block(machine, false, false);
    case WZ_Z80_ED_OP_LDD:
        return wz_z80_execute_ld_block(machine, true, false);
    case WZ_Z80_ED_OP_LDIR:
        return wz_z80_execute_ld_block(machine, false, true);
    case WZ_Z80_ED_OP_LDDR:
        return wz_z80_execute_ld_block(machine, true, true);
    case WZ_Z80_ED_OP_CPI:
        return wz_z80_execute_cp_block(machine, false, false);
    case WZ_Z80_ED_OP_CPD:
        return wz_z80_execute_cp_block(machine, true, false);
    case WZ_Z80_ED_OP_CPIR:
        return wz_z80_execute_cp_block(machine, false, true);
    case WZ_Z80_ED_OP_CPDR:
        return wz_z80_execute_cp_block(machine, true, true);
    case WZ_Z80_ED_OP_INI:
        return wz_z80_execute_in_block(machine, false, false);
    case WZ_Z80_ED_OP_IND:
        return wz_z80_execute_in_block(machine, true, false);
    case WZ_Z80_ED_OP_INIR:
        return wz_z80_execute_in_block(machine, false, true);
    case WZ_Z80_ED_OP_INDR:
        return wz_z80_execute_in_block(machine, true, true);
    case WZ_Z80_ED_OP_OUTI:
        return wz_z80_execute_out_block(machine, false, false);
    case WZ_Z80_ED_OP_OUTD:
        return wz_z80_execute_out_block(machine, true, false);
    case WZ_Z80_ED_OP_OTIR:
        return wz_z80_execute_out_block(machine, false, true);
    case WZ_Z80_ED_OP_OTDR:
        return wz_z80_execute_out_block(machine, true, true);
    case WZ_Z80_ED_OP_RETN:
    case WZ_Z80_ED_OP_RETI:
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 8u,
                       machine->cpu.stack_pointer, &low, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.stack_pointer = wz_z80_add16(machine->cpu.stack_pointer, 1u);
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 14u,
                       machine->cpu.stack_pointer, &high, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.stack_pointer = wz_z80_add16(machine->cpu.stack_pointer, 1u);
        machine->cpu.program_counter = (wz_word_t)low | ((wz_word_t)high << 8u);
        machine->cpu.memptr = machine->cpu.program_counter;
        machine->cpu.iff1 = machine->cpu.iff2;
        machine->master_tick += 28u;
        return WZ_RESULT_OK;
    case WZ_Z80_ED_OP_LD_NN_RR:
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 8u,
                       machine->cpu.program_counter, &low, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 14u,
                       machine->cpu.program_counter, &high, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
        address = (wz_word_t)low | ((wz_word_t)high << 8u);
        machine->cpu.memptr = wz_z80_add16(address, 1u);
        pair_value = wz_z80_get_rr(&machine->cpu, decode.operand);
        value = (wz_byte_t)(pair_value & 0xffu);
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_WRITE, 20u, address, &value, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        value = (wz_byte_t)(pair_value >> 8u);
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_WRITE, 26u,
                       wz_z80_add16(address, 1u), &value, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->master_tick += 40u;
        return WZ_RESULT_OK;
    case WZ_Z80_ED_OP_LD_RR_NN:
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 8u,
                       machine->cpu.program_counter, &low, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 14u,
                       machine->cpu.program_counter, &high, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
        address = (wz_word_t)low | ((wz_word_t)high << 8u);
        machine->cpu.memptr = wz_z80_add16(address, 1u);
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 20u, address, &low, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 26u,
                       wz_z80_add16(address, 1u), &high, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        wz_z80_set_rr(&machine->cpu, decode.operand,
                      (wz_word_t)low | ((wz_word_t)high << 8u));
        machine->master_tick += 40u;
        return WZ_RESULT_OK;
    default:
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }
}

static wz_result_t wz_z80_bus(wz_machine_t* machine,
                              wz_bus_cycle_t cycle,
                              wz_master_tick_t offset,
                              wz_word_t address,
                              wz_byte_t* value,
                              wz_byte_t t_states)
{
    wz_bus_request_t request;
    wz_bus_request_init(&request, cycle, machine->master_tick + offset, address,
                        value == 0 ? 0u : *value, t_states);
    if (wz_machine_bus_request(machine, &request) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (value != 0) {
        *value = request.value;
    }
    return WZ_RESULT_OK;
}

void wz_z80_state_init(wz_z80_state_t* state)
{
    if (state == 0) {
        return;
    }

    state->main.a = 0u;
    state->main.f = 0u;
    state->main.b = 0u;
    state->main.c = 0u;
    state->main.d = 0u;
    state->main.e = 0u;
    state->main.h = 0u;
    state->main.l = 0u;
    state->alternate.a = 0u;
    state->alternate.f = 0u;
    state->alternate.b = 0u;
    state->alternate.c = 0u;
    state->alternate.d = 0u;
    state->alternate.e = 0u;
    state->alternate.h = 0u;
    state->alternate.l = 0u;
    state->ix = 0u;
    state->iy = 0u;
    state->stack_pointer = 0xffffu;
    state->program_counter = 0u;
    state->memptr = 0u;
    state->i = 0u;
    state->r = 0u;
    state->iff1 = 0u;
    state->iff2 = 0u;
    state->interrupt_mode = (wz_byte_t)WZ_Z80_INTERRUPT_MODE_0;
    state->halted = 0u;
}

wz_result_t wz_z80_state_validate(const wz_z80_state_t* state)
{
    if (state == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (state->iff1 > 1u || state->iff2 > 1u || state->halted > 1u) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (state->interrupt_mode > (wz_byte_t)WZ_Z80_INTERRUPT_MODE_2) {
        return WZ_RESULT_INVALID_STATE;
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_z80_step(wz_machine_t* machine)
{
    wz_byte_t opcode = 0u;
    wz_byte_t value = 0u;
    wz_byte_t low = 0u;
    wz_byte_t high = 0u;
    wz_byte_t cb_opcode = 0u;
    const wz_z80_opcode_decode_t* decode;
    wz_word_t pc;
    wz_word_t address;

    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (wz_z80_state_validate(&machine->cpu) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }

    pc = machine->cpu.program_counter;
    if (wz_z80_bus(machine, WZ_BUS_M1_OPCODE_FETCH, 0u, pc, &opcode, 4u) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    machine->cpu.program_counter = wz_z80_add16(pc, 1u);
    wz_z80_increment_r(&machine->cpu);

    decode = wz_z80_primary_opcode_decode(opcode);
    switch (decode->operation) {
    case WZ_Z80_PRIMARY_OP_NOP:
        machine->master_tick += 8u;
        return WZ_RESULT_OK;
    case WZ_Z80_PRIMARY_OP_LD_A_N:
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 8u,
                       machine->cpu.program_counter, &value, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
        machine->cpu.main.a = value;
        machine->master_tick += 14u;
        return WZ_RESULT_OK;
    case WZ_Z80_PRIMARY_OP_LD_NN_A:
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 8u,
                       machine->cpu.program_counter, &low, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 14u,
                       machine->cpu.program_counter, &high, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
        address = (wz_word_t)low | ((wz_word_t)high << 8u);
        value = machine->cpu.main.a;
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_WRITE, 20u, address, &value, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->master_tick += 26u;
        return WZ_RESULT_OK;
    case WZ_Z80_PRIMARY_OP_PREFIX_CB:
        if (wz_z80_bus(machine, WZ_BUS_M1_OPCODE_FETCH, 4u,
                       machine->cpu.program_counter, &cb_opcode, 4u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
        wz_z80_increment_r(&machine->cpu);
        return wz_z80_execute_cb(machine, wz_z80_cb_opcode_decode(cb_opcode));
    case WZ_Z80_PRIMARY_OP_PREFIX_ED:
        if (wz_z80_bus(machine, WZ_BUS_M1_OPCODE_FETCH, 4u,
                       machine->cpu.program_counter, &value, 4u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
        wz_z80_increment_r(&machine->cpu);
        return wz_z80_execute_ed(machine, wz_z80_ed_opcode_decode(value));
    case WZ_Z80_PRIMARY_OP_PREFIX_DD:
    case WZ_Z80_PRIMARY_OP_PREFIX_FD:
        return wz_z80_execute_index_prefix(machine, opcode);
    case WZ_Z80_PRIMARY_OP_UNSUPPORTED:
    default:
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }
}
