/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_control_port.h"
#include "app/wz_host_socket.h"
#include "app/wz_input_arbiter.h"
#include "app/wz_telnet_client.h"
#include "app/wz_telnet_input.h"

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET native_socket_t;
#define NATIVE_INVALID INVALID_SOCKET
static void native_close(native_socket_t* socket_handle)
{
    if (*socket_handle != NATIVE_INVALID) closesocket(*socket_handle);
    *socket_handle = NATIVE_INVALID;
}
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
typedef int native_socket_t;
#define NATIVE_INVALID (-1)
static void native_close(native_socket_t* socket_handle)
{
    if (*socket_handle != NATIVE_INVALID) close(*socket_handle);
    *socket_handle = NATIVE_INVALID;
}
#endif

#define REQUIRE(test) do { \
    if (!(test)) { \
        fprintf(stderr, "Telnet client gate failed at line %d: %s\n", \
                __LINE__, #test); \
        result = 1; \
        goto cleanup; \
    } \
    ++cases; \
} while (0)

static native_socket_t connect_client(uint16_t port, bool ipv6)
{
    native_socket_t client;
    if (ipv6) {
        struct sockaddr_in6 address;
        client = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
        if (client == NATIVE_INVALID) return NATIVE_INVALID;
        memset(&address, 0, sizeof(address));
        address.sin6_family = AF_INET6;
        address.sin6_port = htons(port);
        address.sin6_addr = in6addr_loopback;
        if (connect(client, (struct sockaddr*)&address, sizeof(address)) != 0) {
            native_close(&client);
            return NATIVE_INVALID;
        }
    } else {
        struct sockaddr_in address;
        client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (client == NATIVE_INVALID) return NATIVE_INVALID;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (connect(client, (struct sockaddr*)&address, sizeof(address)) != 0) {
            native_close(&client);
            return NATIVE_INVALID;
        }
    }
    return client;
}

static int native_send(native_socket_t client, const char* text, size_t length)
{
#if defined(_WIN32)
    return send(client, text, (int)length, 0);
#else
    return (int)send(client, text, length, 0);
#endif
}

static int native_receive(native_socket_t client, char* output, size_t capacity)
{
#if defined(_WIN32)
    return recv(client, output, (int)capacity, 0);
#else
    return (int)recv(client, output, capacity, 0);
#endif
}

static bool set_receive_timeout(native_socket_t client)
{
#if defined(_WIN32)
    DWORD timeout_ms = 5000u;
    return setsockopt(client, SOL_SOCKET, SO_RCVTIMEO,
                      (const char*)&timeout_ms, sizeof(timeout_ms)) == 0;
#else
    struct timeval timeout = {5, 0};
    return setsockopt(client, SOL_SOCKET, SO_RCVTIMEO,
                      &timeout, sizeof(timeout)) == 0;
#endif
}

static bool wait_readable(wz_host_socket_t socket_handle)
{
    fd_set read_set;
    struct timeval timeout = {5, 0};
    FD_ZERO(&read_set);
#if defined(_WIN32)
    FD_SET((SOCKET)socket_handle, &read_set);
    return select(0, &read_set, NULL, NULL, &timeout) > 0;
#else
    FD_SET((int)socket_handle, &read_set);
    return select((int)socket_handle + 1, &read_set, NULL, NULL, &timeout) > 0;
#endif
}

int main(void)
{
    unsigned cases = 0u;
    int result = 0;
    wz_control_port_owner_t owner;
    wz_telnet_client_gate_t gate;
    wz_input_arbiter_t input;
    native_socket_t first = NATIVE_INVALID;
    native_socket_t second = NATIVE_INVALID;
    native_socket_t third = NATIVE_INVALID;
    char response[16];
    int received;

    wz_control_port_owner_init(&owner);
    wz_telnet_client_gate_init(&gate);
    wz_input_arbiter_init(&input);
    REQUIRE(wz_host_socket_system_init());
    REQUIRE(wz_control_port_probe(&owner) == WZ_CONTROL_PORT_PROBE_AVAILABLE);
    wz_telnet_client_bind_input(&gate, &input);
    REQUIRE(owner.ipv4_active || owner.ipv6_active);
    first = connect_client(owner.selected_port, !owner.ipv4_active);
    REQUIRE(first != NATIVE_INVALID);
    REQUIRE(wz_telnet_client_accept(&gate, owner.ipv4_active ?
                owner.ipv4_socket : owner.ipv6_socket) != WZ_HOST_SOCKET_INVALID);
    REQUIRE(wz_telnet_client_is_active(&gate));

    second = connect_client(owner.selected_port, !owner.ipv4_active);
    REQUIRE(second != NATIVE_INVALID);
    REQUIRE(set_receive_timeout(second));
    REQUIRE(wz_telnet_client_accept(&gate, owner.ipv4_active ?
                owner.ipv4_socket : owner.ipv6_socket) == WZ_HOST_SOCKET_INVALID);
    REQUIRE(wz_telnet_client_is_active(&gate));
    received = native_receive(second, response, sizeof(response));
    REQUIRE(received == 6 && memcmp(response, "BUSY\r\n", 6u) == 0);
    REQUIRE(native_receive(second, response, sizeof(response)) == 0);

    REQUIRE(native_send(first, "ping", 4u) == 4);
    REQUIRE(wait_readable(gate.active_client));
    received = wz_host_socket_receive(gate.active_client, response, sizeof(response));
    REQUIRE(received == 4 && memcmp(response, "ping", 4u) == 0);

    REQUIRE(wz_input_arbiter_set(&input, WZ_INPUT_SOURCE_LOCAL, 3u, true));
    REQUIRE(wz_telnet_input_set_key(&input, 3u, true));
    wz_telnet_client_disconnect(&gate);
    REQUIRE(!wz_telnet_client_is_active(&gate));
    REQUIRE(wz_input_arbiter_key_down(&input, 3u));
    REQUIRE(wz_input_arbiter_source_key_down(&input, WZ_INPUT_SOURCE_LOCAL, 3u));
    REQUIRE(!wz_input_arbiter_source_key_down(&input, WZ_INPUT_SOURCE_TELNET, 3u));

    third = connect_client(owner.selected_port, !owner.ipv4_active);
    REQUIRE(third != NATIVE_INVALID);
    REQUIRE(wz_telnet_client_accept(&gate, owner.ipv4_active ?
                owner.ipv4_socket : owner.ipv6_socket) != WZ_HOST_SOCKET_INVALID);
    REQUIRE(wz_telnet_client_is_active(&gate));
    printf("Telnet client gate regression passed (%u cases)\n", cases);

cleanup:
    native_close(&first);
    native_close(&second);
    native_close(&third);
    wz_telnet_client_disconnect(&gate);
    wz_control_port_owner_close(&owner);
    wz_host_socket_system_shutdown();
    return result;
}
