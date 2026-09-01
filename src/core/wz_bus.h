/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_BUS_H
#define WZ_CORE_WZ_BUS_H

#include "core/wz_types.h"

typedef enum {
    WZ_BUS_M1_OPCODE_FETCH = 0,
    WZ_BUS_MEMORY_READ,
    WZ_BUS_MEMORY_WRITE,
    WZ_BUS_IO_READ,
    WZ_BUS_IO_WRITE,
    WZ_BUS_INTERRUPT_ACKNOWLEDGE,
    WZ_BUS_INTERNAL
} wz_bus_cycle_t;

typedef struct {
    wz_bus_cycle_t cycle;
    wz_master_tick_t master_tick;
    wz_word_t address;
    wz_byte_t value;
    wz_byte_t t_states;
    wz_byte_t contention_delay;
} wz_bus_request_t;

typedef void (*wz_bus_observer_fn)(const wz_bus_request_t* request, void* context);

typedef struct {
    wz_bus_observer_fn record;
    void* context;
} wz_bus_observer_t;

typedef wz_byte_t (*wz_bus_input_fn)(wz_bus_cycle_t cycle,
                                     wz_word_t address,
                                     void* context);

typedef struct {
    wz_bus_input_fn read;
    void* context;
} wz_bus_input_t;

typedef struct wz_machine wz_machine_t;

void wz_bus_request_init(wz_bus_request_t* request,
                         wz_bus_cycle_t cycle,
                         wz_master_tick_t master_tick,
                         wz_word_t address,
                         wz_byte_t value,
                         wz_byte_t t_states);
void wz_bus_observer_init(wz_bus_observer_t* observer,
                          wz_bus_observer_fn record,
                          void* context);
void wz_bus_input_init(wz_bus_input_t* input,
                       wz_bus_input_fn read,
                       void* context);
wz_result_t wz_machine_set_bus_observer(wz_machine_t* machine,
                                        const wz_bus_observer_t* observer);
wz_result_t wz_machine_set_bus_input(wz_machine_t* machine,
                                     const wz_bus_input_t* input);
wz_result_t wz_machine_bus_request(wz_machine_t* machine,
                                   wz_bus_request_t* request);

#endif
