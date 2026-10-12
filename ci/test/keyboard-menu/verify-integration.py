# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

"""Check that the host exposes required workflows through keyboard actions."""

from pathlib import Path
import re


source = Path("src/app/wz_sokol_main.c").read_text(encoding="utf-8")
checks = [
    "SAPP_KEYCODE_F1",
    "SAPP_KEYCODE_UP",
    "SAPP_KEYCODE_DOWN",
    "SAPP_KEYCODE_ENTER",
    "SAPP_KEYCODE_ESCAPE",
    '"file.open_run"',
    '"snapshot.load"',
    '"snapshot.save"',
    '"machine.pause_resume"',
    '"machine.reset"',
    '"machine.speed.set"',
    '"machine.model.set"',
    '"media.tape.insert"',
    '"media.tape.eject"',
    '"media.tape.loading_mode.normal"',
    '"media.tape.loading_mode.instant"',
    '"view.fullscreen"',
    '"settings.display.open"',
    '"settings.audio.open"',
    '"settings.input"',
    '"settings.rom.open"',
    '"settings.networking.open"',
    '"media.tape.manager"',
    '"media.microdrive.manager"',
    '"media.microdrive.eject.1"',
    '"media.microdrive.eject.8"',
    '< focus',
    'wz_command_registry_state(',
    'wz_ui_keyboard_menu_open(',
    'wz_ui_keyboard_menu_move(',
    'wz_ui_keyboard_menu_close(',
    'wz_ui_keyboard_menu_set_return_target(',
    'wz_host_release_local_keys();',
    'SAPP_KEYCODE_D',
    'SAPP_KEYCODE_S',
    'Confirm operation (Enter)',
    'Cancel (Esc)',
    'Save and Eject (Enter/S)',
    'Discard and Eject (D)',
    'Eject (Enter)',
]
checks.extend(f'"media.microdrive.eject.{slot}"' for slot in range(1, 9))
missing = [item for item in checks if item not in source]
if missing:
    raise SystemExit("Keyboard integration missing: " + ", ".join(missing))
action_table = re.search(
    r"static const wz_host_keyboard_action_t wz_host_keyboard_actions\[\] = \{(.*?)\n\};",
    source, re.DOTALL)
if action_table is None:
    raise SystemExit("Keyboard action table is missing")
action_count = len(re.findall(r"^\s+\{", action_table.group(1), re.MULTILINE))
if action_count != 38:
    raise SystemExit(f"Expected 38 keyboard actions, found {action_count}")
checks.append("38 action menu count")
print(f"Keyboard workflow source integration passed ({len(checks)} checks)")
