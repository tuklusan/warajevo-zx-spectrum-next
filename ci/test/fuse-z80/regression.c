/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/wz_bus.h"
#include "core/wz_machine.h"

#define FUSE_MEMORY_SIZE 65536u
#define FUSE_EVENT_CAPACITY 32768u
#define FUSE_LINE_CAPACITY 256u
#define FUSE_MAX_STEPS 10000000u

typedef struct {
    uint32_t time;
    uint16_t address;
    uint8_t data;
    char type[3];
    bool has_data;
} fuse_event_t;

typedef struct {
    fuse_event_t events[FUSE_EVENT_CAPACITY];
    size_t event_count;
    uint32_t ticks_per_tstate;
    bool overflow;
} event_log_t;

typedef struct {
    uint16_t words[13];
    uint8_t i;
    uint8_t r;
    uint8_t iff1;
    uint8_t iff2;
    uint8_t interrupt_mode;
    uint8_t halted;
    uint32_t tstates;
    uint8_t memory[FUSE_MEMORY_SIZE];
    event_log_t events;
    char description[FUSE_LINE_CAPACITY];
} fuse_case_t;

static bool read_nonempty_line(FILE* stream, char* line, size_t capacity)
{
    while (fgets(line, (int)capacity, stream) != NULL) {
        size_t length = strlen(line);
        while (length > 0u && (line[length - 1u] == '\n' ||
                               line[length - 1u] == '\r')) {
            line[--length] = '\0';
        }
        for (size_t index = 0u; index < length; ++index) {
            if (!isspace((unsigned char)line[index])) {
                return true;
            }
        }
    }
    return false;
}

static bool parse_words(const char* line, uint16_t* words, size_t count)
{
    const char* cursor = line;
    for (size_t index = 0u; index < count; ++index) {
        char* end = NULL;
        unsigned long value;
        while (isspace((unsigned char)*cursor)) {
            ++cursor;
        }
        errno = 0;
        value = strtoul(cursor, &end, 16);
        if (end == cursor || errno != 0 || value > UINT16_MAX) {
            return false;
        }
        words[index] = (uint16_t)value;
        cursor = end;
    }
    while (isspace((unsigned char)*cursor)) {
        ++cursor;
    }
    return *cursor == '\0';
}

static bool parse_cpu_line(const char* line, fuse_case_t* test)
{
    unsigned int i;
    unsigned int r;
    unsigned int iff1;
    unsigned int iff2;
    unsigned int interrupt_mode;
    unsigned int halted;
    unsigned int tstates;
    int consumed = 0;
    if (sscanf(line, "%x %x %u %u %u %u %u %n", &i, &r, &iff1, &iff2,
               &interrupt_mode, &halted, &tstates, &consumed) != 7) {
        return false;
    }
    for (const char* cursor = line + consumed; *cursor != '\0'; ++cursor) {
        if (!isspace((unsigned char)*cursor)) {
            return false;
        }
    }
    if (i > UINT8_MAX || r > UINT8_MAX || iff1 > 1u || iff2 > 1u ||
        interrupt_mode > 2u || halted > 1u) {
        return false;
    }
    test->i = (uint8_t)i;
    test->r = (uint8_t)r;
    test->iff1 = (uint8_t)iff1;
    test->iff2 = (uint8_t)iff2;
    test->interrupt_mode = (uint8_t)interrupt_mode;
    test->halted = (uint8_t)halted;
    test->tstates = tstates;
    return true;
}

static bool read_memory_changes(FILE* stream, uint8_t* memory)
{
    char token[32];
    while (fscanf(stream, "%31s", token) == 1) {
        char* end = NULL;
        long address;
        errno = 0;
        address = strtol(token, &end, 16);
        if (end == token || *end != '\0' || errno != 0) {
            return false;
        }
        if (address < 0) {
            return address == -1;
        }
        if (address >= (long)FUSE_MEMORY_SIZE) {
            return false;
        }
        for (;;) {
            long value;
            if (fscanf(stream, "%31s", token) != 1) {
                return false;
            }
            errno = 0;
            value = strtol(token, &end, 16);
            if (end == token || *end != '\0' || errno != 0) {
                return false;
            }
            if (value < 0) {
                if (value != -1) {
                    return false;
                }
                break;
            }
            if (value > UINT8_MAX || address >= (long)FUSE_MEMORY_SIZE) {
                return false;
            }
            memory[address++] = (uint8_t)value;
        }
    }
    return false;
}

static bool read_input_case(FILE* stream, fuse_case_t* test)
{
    unsigned int words[13];
    char cpu_line[FUSE_LINE_CAPACITY];
    if (!read_nonempty_line(stream, test->description,
                            sizeof(test->description))) {
        return false;
    }
    if (fscanf(stream,
               "%x %x %x %x %x %x %x %x %x %x %x %x %x",
               &words[0], &words[1], &words[2], &words[3], &words[4],
               &words[5], &words[6], &words[7], &words[8], &words[9],
               &words[10], &words[11], &words[12]) != 13) {
        return false;
    }
    for (size_t index = 0u; index < 13u; ++index) {
        if (words[index] > UINT16_MAX) {
            return false;
        }
        test->words[index] = (uint16_t)words[index];
    }
    if (!read_nonempty_line(stream, cpu_line, sizeof(cpu_line)) ||
        !parse_cpu_line(cpu_line, test)) {
        return false;
    }
    for (size_t index = 0u; index < FUSE_MEMORY_SIZE; index += 4u) {
        test->memory[index] = 0xdeu;
        test->memory[index + 1u] = 0xadu;
        test->memory[index + 2u] = 0xbeu;
        test->memory[index + 3u] = 0xefu;
    }
    return read_memory_changes(stream, test->memory);
}

static bool add_event(event_log_t* log, uint32_t time, const char* type,
                      uint16_t address, uint8_t data, bool has_data)
{
    fuse_event_t* event;
    if (log->event_count >= FUSE_EVENT_CAPACITY) {
        log->overflow = true;
        return false;
    }
    event = &log->events[log->event_count++];
    event->time = time;
    event->address = address;
    event->data = data;
    event->type[0] = type[0];
    event->type[1] = type[1];
    event->type[2] = '\0';
    event->has_data = has_data;
    return true;
}

static bool parse_expected_event(const char* line, fuse_event_t* event)
{
    unsigned int time;
    unsigned int address;
    unsigned int data;
    char type[16];
    int fields = sscanf(line, "%u %15s %x %x", &time, type, &address, &data);
    bool has_data;
    if (fields < 3 || strlen(type) != 2u || address > UINT16_MAX) {
        return false;
    }
    if (strcmp(type, "MC") != 0 && strcmp(type, "MR") != 0 &&
        strcmp(type, "MW") != 0 && strcmp(type, "PC") != 0 &&
        strcmp(type, "PR") != 0 && strcmp(type, "PW") != 0) {
        return false;
    }
    has_data = strcmp(type, "MR") == 0 || strcmp(type, "MW") == 0 ||
               strcmp(type, "PR") == 0 || strcmp(type, "PW") == 0;
    if (has_data && (fields != 4 || data > UINT8_MAX)) {
        return false;
    }
    if (!has_data && fields != 3) {
        return false;
    }
    event->time = time;
    event->address = (uint16_t)address;
    event->data = has_data ? (uint8_t)data : 0u;
    event->type[0] = type[0];
    event->type[1] = type[1];
    event->type[2] = '\0';
    event->has_data = has_data;
    return true;
}

static bool read_expected_case(FILE* stream, fuse_case_t* test)
{
    char line[FUSE_LINE_CAPACITY];
    char cpu_line[FUSE_LINE_CAPACITY];
    if (!read_nonempty_line(stream, test->description,
                            sizeof(test->description))) {
        return false;
    }
    test->events.event_count = 0u;
    test->events.overflow = false;
    while (read_nonempty_line(stream, line, sizeof(line))) {
        fuse_event_t event;
        if (!parse_expected_event(line, &event)) {
            if (!parse_words(line, test->words, 13u)) {
                return false;
            }
            break;
        }
        if (test->events.event_count >= FUSE_EVENT_CAPACITY) {
            return false;
        }
        test->events.events[test->events.event_count++] = event;
    }
    if (!read_nonempty_line(stream, cpu_line, sizeof(cpu_line)) ||
        !parse_cpu_line(cpu_line, test)) {
        return false;
    }
    return read_memory_changes(stream, test->memory);
}

static uint8_t read_port(wz_bus_cycle_t cycle, wz_word_t address,
                         void* context)
{
    (void)cycle;
    (void)context;
    return (uint8_t)(address >> 8u);
}

static void record_bus_request(const wz_bus_request_t* request, void* context)
{
    event_log_t* log = (event_log_t*)context;
    uint32_t ticks_per_tstate = log->ticks_per_tstate;
    uint32_t event_tick;
    uint32_t delay = request->contention_delay;
    uint32_t start_tick;
    if (ticks_per_tstate == 0u) {
        log->overflow = true;
        return;
    }
    event_tick = (uint32_t)(request->master_tick / ticks_per_tstate);
    start_tick = event_tick >= delay ? event_tick - delay : 0u;

    if (request->cycle == WZ_BUS_M1_OPCODE_FETCH ||
        request->cycle == WZ_BUS_MEMORY_READ ||
        request->cycle == WZ_BUS_MEMORY_WRITE) {
        for (uint32_t index = 0u; index < delay; ++index) {
            (void)add_event(log, start_tick + index, "MC", request->address,
                            0u, false);
        }
        (void)add_event(log, event_tick, "MC", request->address, 0u, false);
        (void)add_event(log, event_tick + request->t_states,
                        request->cycle == WZ_BUS_MEMORY_WRITE ? "MW" : "MR",
                        request->address, request->value, true);
        return;
    }
    if (request->cycle == WZ_BUS_IO_READ || request->cycle == WZ_BUS_IO_WRITE) {
        bool contended_high = (request->address & 0xc000u) == 0x4000u;
        uint32_t io_time = event_tick + 1u;
        if (contended_high) {
            (void)add_event(log, start_tick, "PC", request->address, 0u, false);
        }
        (void)add_event(log, io_time,
                        request->cycle == WZ_BUS_IO_WRITE ? "PW" : "PR",
                        request->address, request->value, true);
        if ((request->address & 1u) == 0u || contended_high) {
            (void)add_event(log, io_time + 1u, "PC", request->address, 0u, false);
            (void)add_event(log, io_time + 2u, "PC", request->address, 0u, false);
            (void)add_event(log, io_time + 3u, "PC", request->address, 0u, false);
        }
    }
}

static void set_word_registers(wz_z80_state_t* cpu, const uint16_t* words)
{
    cpu->main.a = (uint8_t)(words[0] >> 8u);
    cpu->main.f = (uint8_t)words[0];
    cpu->main.b = (uint8_t)(words[1] >> 8u);
    cpu->main.c = (uint8_t)words[1];
    cpu->main.d = (uint8_t)(words[2] >> 8u);
    cpu->main.e = (uint8_t)words[2];
    cpu->main.h = (uint8_t)(words[3] >> 8u);
    cpu->main.l = (uint8_t)words[3];
    cpu->alternate.a = (uint8_t)(words[4] >> 8u);
    cpu->alternate.f = (uint8_t)words[4];
    cpu->alternate.b = (uint8_t)(words[5] >> 8u);
    cpu->alternate.c = (uint8_t)words[5];
    cpu->alternate.d = (uint8_t)(words[6] >> 8u);
    cpu->alternate.e = (uint8_t)words[6];
    cpu->alternate.h = (uint8_t)(words[7] >> 8u);
    cpu->alternate.l = (uint8_t)words[7];
    cpu->ix = words[8];
    cpu->iy = words[9];
    cpu->stack_pointer = words[10];
    cpu->program_counter = words[11];
    cpu->memptr = words[12];
}

static void get_word_registers(const wz_z80_state_t* cpu, uint16_t* words)
{
    words[0] = (uint16_t)(((uint16_t)cpu->main.a << 8u) | cpu->main.f);
    words[1] = (uint16_t)(((uint16_t)cpu->main.b << 8u) | cpu->main.c);
    words[2] = (uint16_t)(((uint16_t)cpu->main.d << 8u) | cpu->main.e);
    words[3] = (uint16_t)(((uint16_t)cpu->main.h << 8u) | cpu->main.l);
    words[4] = (uint16_t)(((uint16_t)cpu->alternate.a << 8u) | cpu->alternate.f);
    words[5] = (uint16_t)(((uint16_t)cpu->alternate.b << 8u) | cpu->alternate.c);
    words[6] = (uint16_t)(((uint16_t)cpu->alternate.d << 8u) | cpu->alternate.e);
    words[7] = (uint16_t)(((uint16_t)cpu->alternate.h << 8u) | cpu->alternate.l);
    words[8] = cpu->ix;
    words[9] = cpu->iy;
    words[10] = cpu->stack_pointer;
    words[11] = cpu->program_counter;
    words[12] = cpu->memptr;
}

static bool execute_case(fuse_case_t* input, const fuse_case_t* expected,
                         uint32_t case_number)
{
    wz_machine_t machine;
    wz_bus_observer_t observer;
    wz_bus_input_t port_input;
    wz_master_tick_t target_tick;
    uint16_t actual_words[13];
    uint8_t actual_memory[FUSE_MEMORY_SIZE];
    uint32_t actual_tstates;
    size_t steps = 0u;
    bool success = false;

    memset(&machine, 0, sizeof(machine));
    input->events.event_count = 0u;
    input->events.overflow = false;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        fprintf(stderr, "FAIL %s: machine initialization\n", input->description);
        return false;
    }
    wz_bus_observer_init(&observer, record_bus_request, &input->events);
    wz_bus_input_init(&port_input, read_port, NULL);
    if (wz_machine_set_bus_observer(&machine, &observer) != WZ_RESULT_OK ||
        wz_machine_set_bus_input(&machine, &port_input) != WZ_RESULT_OK ||
        wz_machine_set_hardware_io_decode(&machine, false) != WZ_RESULT_OK) {
        fprintf(stderr, "FAIL %s: bus setup\n", input->description);
        goto cleanup;
    }
    input->events.ticks_per_tstate =
        machine.profile->master_ticks_per_cpu_tstate;
    for (size_t address = 0u; address < FUSE_MEMORY_SIZE; ++address) {
        wz_machine_memory_write(&machine, (wz_word_t)address, input->memory[address]);
    }
    set_word_registers(&machine.cpu, input->words);
    machine.cpu.i = input->i;
    machine.cpu.r = input->r;
    machine.cpu.iff1 = input->iff1;
    machine.cpu.iff2 = input->iff2;
    machine.cpu.interrupt_mode = input->interrupt_mode;
    machine.cpu.halted = input->halted;
    target_tick = (wz_master_tick_t)input->tstates *
        machine.profile->master_ticks_per_cpu_tstate;
    while (machine.master_tick < target_tick) {
        wz_result_t result = wz_z80_step(&machine);
        if (result != WZ_RESULT_OK) {
            fprintf(stderr, "FAIL %s: CPU returned %d at T=%" PRIu64 "\n",
                    input->description, (int)result,
                    (uint64_t)(machine.master_tick /
                               machine.profile->master_ticks_per_cpu_tstate));
            goto cleanup;
        }
        if (++steps > FUSE_MAX_STEPS) {
            fprintf(stderr, "FAIL %s: step limit exceeded\n", input->description);
            goto cleanup;
        }
    }

    get_word_registers(&machine.cpu, actual_words);
    actual_tstates = (uint32_t)(machine.master_tick /
                                machine.profile->master_ticks_per_cpu_tstate);
    if (memcmp(actual_words, expected->words, sizeof(actual_words)) != 0 ||
        machine.cpu.i != expected->i || machine.cpu.r != expected->r ||
        machine.cpu.iff1 != expected->iff1 || machine.cpu.iff2 != expected->iff2 ||
        machine.cpu.interrupt_mode != expected->interrupt_mode ||
        machine.cpu.halted != expected->halted || actual_tstates != expected->tstates) {
        fprintf(stderr,
                "FAIL %s: final CPU state or T-state count differs (case %" PRIu32 ")\n",
                input->description, case_number);
        goto cleanup;
    }
    if (input->events.overflow ||
        input->events.event_count != expected->events.event_count) {
        fprintf(stderr, "FAIL %s: bus event count differs (%zu vs %zu)\n",
                input->description, input->events.event_count,
                expected->events.event_count);
        goto cleanup;
    }
    for (size_t index = 0u; index < expected->events.event_count; ++index) {
        const fuse_event_t* actual = &input->events.events[index];
        const fuse_event_t* wanted = &expected->events.events[index];
        if (actual->time != wanted->time || actual->address != wanted->address ||
            actual->data != wanted->data || actual->has_data != wanted->has_data ||
            strcmp(actual->type, wanted->type) != 0) {
            fprintf(stderr,
                    "FAIL %s: event %zu differs: %" PRIu32 " %s %04x vs "
                    "%" PRIu32 " %s %04x (case %" PRIu32 ")\n",
                    input->description, index, actual->time, actual->type,
                    actual->address, wanted->time, wanted->type, wanted->address,
                    case_number);
            goto cleanup;
        }
    }
    for (size_t address = 0u; address < FUSE_MEMORY_SIZE; ++address) {
        actual_memory[address] = wz_machine_memory_read(&machine, (wz_word_t)address);
    }
    if (memcmp(actual_memory, expected->memory, sizeof(actual_memory)) != 0) {
        fprintf(stderr, "FAIL %s: final memory differs (case %" PRIu32 ")\n",
                input->description, case_number);
        goto cleanup;
    }
    success = true;

cleanup:
    wz_machine_destroy(&machine);
    return success;
}

int main(int argc, char** argv)
{
    FILE* input_stream;
    FILE* expected_stream;
    fuse_case_t* input_case;
    fuse_case_t* expected_case;
    uint32_t cases = 0u;
    if (argc != 3) {
        fprintf(stderr, "usage: fuse-z80-regression tests.in tests.expected\n");
        return 2;
    }
    input_stream = fopen(argv[1], "r");
    expected_stream = fopen(argv[2], "r");
    if (input_stream == NULL || expected_stream == NULL) {
        fprintf(stderr, "could not open pinned Fuse test corpus\n");
        if (input_stream != NULL) fclose(input_stream);
        if (expected_stream != NULL) fclose(expected_stream);
        return 2;
    }
    input_case = (fuse_case_t*)calloc(1u, sizeof(*input_case));
    expected_case = (fuse_case_t*)calloc(1u, sizeof(*expected_case));
    if (input_case == NULL || expected_case == NULL) {
        fprintf(stderr, "could not allocate Fuse case buffers\n");
        free(input_case);
        free(expected_case);
        fclose(input_stream);
        fclose(expected_stream);
        return 2;
    }
    for (;;) {
        bool have_input = read_input_case(input_stream, input_case);
        bool have_expected;
        if (have_input) {
            memcpy(expected_case->memory, input_case->memory,
                   sizeof(expected_case->memory));
        } else {
            memset(expected_case->memory, 0, sizeof(expected_case->memory));
        }
        have_expected = read_expected_case(expected_stream, expected_case);
        if (!have_input || !have_expected) {
            if (have_input != have_expected) {
                fprintf(stderr, "Fuse input and expected case counts differ\n");
                free(input_case);
                free(expected_case);
                fclose(input_stream);
                fclose(expected_stream);
                return 1;
            }
            break;
        }
        if (strcmp(input_case->description, expected_case->description) != 0) {
            fprintf(stderr, "Fuse case names differ at case %" PRIu32 "\n", cases);
            free(input_case);
            free(expected_case);
            fclose(input_stream);
            fclose(expected_stream);
            return 1;
        }
        if (!execute_case(input_case, expected_case, cases)) {
            free(input_case);
            free(expected_case);
            fclose(input_stream);
            fclose(expected_stream);
            return 1;
        }
        ++cases;
    }
    free(input_case);
    free(expected_case);
    fclose(input_stream);
    fclose(expected_stream);
    printf("PASS Fuse Z80 cases: %" PRIu32 "\n", cases);
    return cases == 0u ? 1 : 0;
}
