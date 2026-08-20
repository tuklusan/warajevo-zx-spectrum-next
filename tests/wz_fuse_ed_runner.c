/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/wz_machine.h"

static wz_word_t make_word(unsigned value)
{
    return (wz_word_t)(value & 0xffffu);
}

static void set_pair(wz_byte_t* high, wz_byte_t* low, unsigned value)
{
    *high = (wz_byte_t)(value >> 8u);
    *low = (wz_byte_t)value;
}

static unsigned get_pair(wz_byte_t high, wz_byte_t low)
{
    return ((unsigned)high << 8u) | low;
}

static wz_byte_t fuse_bus_input(wz_bus_cycle_t cycle,
                                wz_word_t address,
                                void* context)
{
    (void)context;
    return cycle == WZ_BUS_IO_READ ? (wz_byte_t)(address >> 8u) : 0xffu;
}

static int load_memory_line(wz_machine_t* machine, const char* line)
{
    char copy[1024];
    char* token;
    unsigned address;

    if (strlen(line) >= sizeof(copy)) {
        return 0;
    }
    strcpy(copy, line);
    token = strtok(copy, " \t\r\n");
    if (token == NULL || sscanf(token, "%x", &address) != 1) {
        return 0;
    }
    while ((token = strtok(NULL, " \t\r\n")) != NULL) {
        unsigned value;
        if (strcmp(token, "-1") == 0) {
            return 1;
        }
        if (sscanf(token, "%x", &value) != 1) {
            return 0;
        }
        machine->memory[(wz_word_t)address] = (wz_byte_t)value;
        address = (address + 1u) & 0xffffu;
    }
    return 0;
}

int main(int argc, char** argv)
{
    FILE* input;
    char line[1024];
    char name[128];
    unsigned registers[13];
    unsigned i_value, r_value, iff1, iff2, interrupt_mode, halted;
    unsigned long target_tstates;
    wz_machine_t machine;
    wz_bus_input_t bus_input;
    wz_byte_t initial_memory[65536u];
    size_t steps = 0u;

    if (argc != 2) {
        fputs("usage: wz_fuse_ed_runner <single-case-file>\n", stderr);
        return 2;
    }
    input = fopen(argv[1], "r");
    if (input == NULL || fgets(name, sizeof(name), input) == NULL ||
        fgets(line, sizeof(line), input) == NULL ||
        sscanf(line, "%x %x %x %x %x %x %x %x %x %x %x %x %x",
               &registers[0], &registers[1], &registers[2], &registers[3],
               &registers[4], &registers[5], &registers[6], &registers[7],
               &registers[8], &registers[9], &registers[10], &registers[11],
               &registers[12]) != 13 ||
        fgets(line, sizeof(line), input) == NULL ||
        sscanf(line, "%x %x %u %u %u %u %lu", &i_value, &r_value,
               &iff1, &iff2, &interrupt_mode, &halted, &target_tstates) != 7) {
        fputs("invalid Fuse input case header\n", stderr);
        if (input != NULL) {
            fclose(input);
        }
        return 2;
    }
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        fclose(input);
        return 2;
    }
    for (size_t index = 0u; index < sizeof(machine.memory); index += 4u) {
        machine.memory[index] = 0xdeu;
        machine.memory[index + 1u] = 0xadu;
        machine.memory[index + 2u] = 0xbeu;
        machine.memory[index + 3u] = 0xefu;
    }
    while (fgets(line, sizeof(line), input) != NULL) {
        if (strcmp(line, "-1\n") == 0 || strcmp(line, "-1\r\n") == 0) {
            break;
        }
        if (!load_memory_line(&machine, line)) {
            fputs("invalid Fuse memory setup\n", stderr);
            fclose(input);
            return 2;
        }
    }
    fclose(input);
    memcpy(initial_memory, machine.memory, sizeof(initial_memory));

    set_pair(&machine.cpu.main.a, &machine.cpu.main.f, registers[0]);
    set_pair(&machine.cpu.main.b, &machine.cpu.main.c, registers[1]);
    set_pair(&machine.cpu.main.d, &machine.cpu.main.e, registers[2]);
    set_pair(&machine.cpu.main.h, &machine.cpu.main.l, registers[3]);
    set_pair(&machine.cpu.alternate.a, &machine.cpu.alternate.f, registers[4]);
    set_pair(&machine.cpu.alternate.b, &machine.cpu.alternate.c, registers[5]);
    set_pair(&machine.cpu.alternate.d, &machine.cpu.alternate.e, registers[6]);
    set_pair(&machine.cpu.alternate.h, &machine.cpu.alternate.l, registers[7]);
    machine.cpu.ix = make_word(registers[8]);
    machine.cpu.iy = make_word(registers[9]);
    machine.cpu.stack_pointer = make_word(registers[10]);
    machine.cpu.program_counter = make_word(registers[11]);
    machine.cpu.memptr = make_word(registers[12]);
    machine.cpu.i = (wz_byte_t)i_value;
    machine.cpu.r = (wz_byte_t)r_value;
    machine.cpu.iff1 = (wz_byte_t)iff1;
    machine.cpu.iff2 = (wz_byte_t)iff2;
    machine.cpu.interrupt_mode = (wz_byte_t)interrupt_mode;
    machine.cpu.halted = (wz_byte_t)halted;
    wz_bus_input_init(&bus_input, fuse_bus_input, NULL);
    wz_machine_set_bus_input(&machine, &bus_input);

    while ((machine.master_tick / 2u) < target_tstates && steps < 65536u) {
        if (wz_z80_step(&machine) != WZ_RESULT_OK) {
            fputs("unsupported or invalid instruction during Fuse case\n", stderr);
            return 1;
        }
        steps += 1u;
    }
    if (steps == 65536u) {
        fputs("Fuse case exceeded instruction limit\n", stderr);
        return 1;
    }

    printf("RESULT %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x\n",
           get_pair(machine.cpu.main.a, machine.cpu.main.f),
           get_pair(machine.cpu.main.b, machine.cpu.main.c),
           get_pair(machine.cpu.main.d, machine.cpu.main.e),
           get_pair(machine.cpu.main.h, machine.cpu.main.l),
           get_pair(machine.cpu.alternate.a, machine.cpu.alternate.f),
           get_pair(machine.cpu.alternate.b, machine.cpu.alternate.c),
           get_pair(machine.cpu.alternate.d, machine.cpu.alternate.e),
           get_pair(machine.cpu.alternate.h, machine.cpu.alternate.l),
           machine.cpu.ix, machine.cpu.iy, machine.cpu.stack_pointer,
           machine.cpu.program_counter, machine.cpu.memptr);
    printf("AUX %02x %02x %u %u %u %u %llu\n",
           machine.cpu.i, machine.cpu.r, machine.cpu.iff1, machine.cpu.iff2,
           machine.cpu.interrupt_mode, machine.cpu.halted,
           (unsigned long long)(machine.master_tick / 2u));
    for (size_t index = 0u; index < sizeof(machine.memory); ++index) {
        if (machine.memory[index] != initial_memory[index]) {
            printf("MEM %04x %02x\n", (unsigned)index, machine.memory[index]);
        }
    }
    puts("END");
    return 0;
}
