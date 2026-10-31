#!/usr/bin/env python3
"""Writes docs/tools.json from the add-in's own tool registry.

The registry (src/apps/registry.c) is the only hand-written list of tools.  The
home screen draws from it, the help pages read from it, and this script asks the
host preview to print it, so the site and the README can never describe a tool
the add-in does not have.

    make sim && python3 tools/gen_tools_json.py

The JSON is checked in because the site build has no compiler; CI regenerates it
and fails when it differs from the committed file.
"""

import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SIM = ROOT / "build" / "sim" / "bench"


def main() -> int:
    if not SIM.exists():
        print("build/sim/bench is missing: run make sim", file=sys.stderr)
        return 1

    env = {"BENCH_DUMP_TOOLS": "1", "PATH": "/usr/bin:/bin"}
    out = subprocess.run([str(SIM)], capture_output=True, text=True, env=env)
    if out.returncode != 0:
        print(out.stderr, file=sys.stderr)
        return out.returncode

    tools = json.loads(out.stdout)
    lines = [line.rstrip() for tool in tools for line in tool["help"].split("\n")]
    for line in lines:
        if len(line) > 40:
            print(f"help line too wide ({len(line)}): {line!r}", file=sys.stderr)
            return 1

    target = ROOT / "docs" / "tools.json"
    target.write_text(json.dumps(tools, indent=2) + "\n", encoding="utf-8")
    print(f"wrote {target.relative_to(ROOT)}: {len(tools)} tools")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
