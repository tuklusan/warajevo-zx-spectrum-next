<!-- Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. -->

# Application command boundary acceptance

`application-command-boundary.c` verifies four requirements using a single
registry and a mutation-counting machine fixture:

1. The application-test projection invokes the registered reset handler.
2. A hit-tested GUI toolbar click invokes that same registered handler and checks the right-edge boundary.
3. The Telnet `DO machine.reset` projection invokes the same handler.
4. A worker thread cannot rebind the finalized registry or execute the handler;
   dispatch returns `wrong-thread` and leaves machine state unchanged.

Run through the hosted matrix with:

```text
cmake -S ci/test/application-command-boundary -B dist/application-command-boundary -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build dist/application-command-boundary --config Release --target wz_application_command_boundary --parallel 2
dist/application-command-boundary/wz_application_command_boundary
```

The Windows and POSIX test workers use native thread creation. The command
registry owner identity itself is generated with C11 thread-local storage and
atomics and does not inspect machine state from a worker thread. Dispatch is
rejected until an owner is bound; thread ID zero is reserved for the unbound
state, and identity exhaustion fails closed.
