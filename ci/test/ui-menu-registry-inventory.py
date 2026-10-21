#!/usr/bin/env python3
# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.
"""Compare the production MENU TREE with the canonical UI ID inventory."""

import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess
import tempfile


helpers = runpy.run_path(str(Path(__file__).with_name(
    "live-telnet-acceptance.py")))
connect = helpers["connect"]
Control = helpers["Control"]


def read_id_block(document, heading):
    if heading not in document:
        raise RuntimeError(f"canonical inventory heading is missing: {heading}")
    section = document.split(heading, 1)[1]
    if "```text" not in section:
        raise RuntimeError(f"canonical ID block is missing after: {heading}")
    block = section.split("```text", 1)[1].split("```", 1)[0]
    return [line.strip() for line in block.splitlines() if line.strip()]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--architecture", type=Path, required=True)
    parser.add_argument("--proof", type=Path, required=True)
    args = parser.parse_args()
    if not args.binary.is_file() or not args.rom.is_file() or \
            not args.architecture.is_file():
        raise RuntimeError("required runner input is unavailable")

    document = args.architecture.read_text(encoding="utf-8")
    roots = read_id_block(document, "The top-level menu-node IDs are:")
    child_ids = read_id_block(document,
        "Required initial child-node/action IDs include:")
    semantic_ids = read_id_block(document,
        "Required initial non-menu semantic command IDs")
    required_ids = list(dict.fromkeys(child_ids + semantic_ids))
    group_ids = {
        item for item in child_ids
        if any(other.startswith(item + ".") for other in required_ids)
    }
    group_ids.update({
        "file.recent",
        "settings.peripherals.networking",
        "settings.peripherals.zx_printer",
    })
    assertions = 0
    process = None
    connection = None
    args.proof.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="wz-menu-registry-inventory-") as temporary:
        environment = os.environ.copy()
        environment["WZSN_ROM_PATH"] = str(args.rom.resolve())
        environment["XDG_CONFIG_HOME"] = str(Path(temporary) / "config")
        try:
            with (Path(temporary) / "application.log").open(
                    "w", encoding="utf-8") as log:
                process = subprocess.Popen([str(args.binary.resolve())],
                    cwd=temporary, env=environment, stdout=log,
                    stderr=subprocess.STDOUT)
                connection = connect(process)
                control = Control(connection)
                tree = control.send("MENU TREE", multiline=True)
                item_records = {}
                for line in tree:
                    match = re.match(r"ITEM ([a-z0-9_.]+) PARENT=([^ ]+) ", line)
                    if match is None:
                        continue
                    item_records.setdefault(match.group(1), []).append(line)

                for root_id in roots:
                    records = item_records.get(root_id, [])
                    if len(records) != 1 or "TYPE=MENU" not in records[0]:
                        raise RuntimeError(
                            f"canonical root is missing or duplicated: {root_id}")
                    assertions += 1

                dynamic_recent_group = "file.recent"
                if dynamic_recent_group in item_records:
                    recent_records = item_records[dynamic_recent_group]
                    if any("TYPE=COMMAND" in line for line in recent_records):
                        raise RuntimeError(
                            "dynamic Recent paths must remain local presentation state")
                assertions += 1

                missing_actions = []
                for identifier in required_ids:
                    records = item_records.get(identifier, [])
                    if records:
                        if len(records) != 1 or "TYPE=COMMAND" not in records[0]:
                            raise RuntimeError(
                                f"required action is not one unique command: {identifier}")
                        line = records[0]
                        if not re.search(r" PARENT=(ROOT|[a-z0-9_.]+) TYPE=COMMAND "
                                         r"STATE=(ENABLED|DISABLED) "
                                         r"REMOTE=(ALLOWED|DENIED) "
                                         r"CLASS=(REMOTE_SAFE|HOST_READ|HOST_WRITE|"
                                         r"MEDIA_DESTRUCTIVE|APPLICATION_CONTROL|LOCAL_ONLY) "
                                         r"LABEL=\"[^\"]+\"", line):
                            raise RuntimeError(
                                f"command metadata violates the menu contract: {identifier}")
                        description = control.send(
                            f"DESCRIBE {identifier}", multiline=True)
                        fields = {line.split("=", 1)[0]: line.split("=", 1)[1]
                                  for line in description if "=" in line}
                        required_fields = {
                            "ID", "LABEL", "DESCRIPTION", "PARAMETERS", "ENABLED",
                            "REMOTE_CLASS", "REMOTE_ALLOWED", "AFFECTS_MACHINE_STATE",
                            "RECORDABLE",
                        }
                        if not description or \
                                description[0] != f"DESCRIBE {identifier}" or \
                                not required_fields.issubset(fields) or \
                                fields["ID"] != identifier or \
                                not fields["LABEL"].startswith('"') or \
                                not fields["DESCRIPTION"].startswith('"') or \
                                fields["ENABLED"] not in {"YES", "NO"} or \
                                fields["REMOTE_ALLOWED"] not in {"YES", "NO"} or \
                                fields["AFFECTS_MACHINE_STATE"] not in {"YES", "NO"} or \
                                fields["RECORDABLE"] not in {"YES", "NO"}:
                            raise RuntimeError(
                                f"DESCRIBE metadata is incomplete: {identifier}")
                        if ("CLASS=" + fields["REMOTE_CLASS"]) not in line or \
                                ("REMOTE=" + ("ALLOWED" if fields["REMOTE_ALLOWED"] == "YES"
                                               else "DENIED")) not in line or \
                                ("STATE=" + ("ENABLED" if fields["ENABLED"] == "YES"
                                             else "DISABLED")) not in line:
                            raise RuntimeError(
                                f"MENU TREE and DESCRIBE metadata disagree: {identifier}")
                        assertions += 2
                    elif identifier in group_ids:
                        # Group IDs describe local menu/toolbar structure; they
                        # are not registry commands with DESCRIBE metadata.
                        continue
                    else:
                        missing_actions.append(identifier)

                if missing_actions:
                    raise RuntimeError(
                        f"required command IDs are absent: {missing_actions}")
                args.proof.write_text(json.dumps({
                    "testId": "ui-menu-registry-inventory",
                    "status": "pass",
                    "commit": os.environ.get("GITHUB_SHA", ""),
                    "runId": int(os.environ.get("GITHUB_RUN_ID", "0")),
                    "runner": os.environ.get("RUNNER_OS", ""),
                    "architecture": os.environ.get("RUNNER_ARCH", ""),
                    "assertions": assertions,
                    "topLevelMenuIds": roots,
                    "requiredSection71Ids": required_ids,
                    "groupNodeIds": sorted(group_ids),
                }, indent=2) + "\n", encoding="utf-8")
        finally:
            if connection is not None:
                connection.close()
            if process is not None and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


if __name__ == "__main__":
    main()
