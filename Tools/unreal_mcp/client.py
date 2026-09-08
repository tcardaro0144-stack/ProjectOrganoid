"""CLI client for OrganoidAIBridge. Does not go through Cursor.

Run from the project root:
  python Tools/unreal_mcp/client.py ping
  python Tools/unreal_mcp/client.py list_playtests
  python Tools/unreal_mcp/client.py run_playtest "{\"test_id\":\"S18_ReceptionTerminal_Functional\"}"
  python Tools/unreal_mcp/client.py get_playtest_result "{\"run_id\":\"ptr_...\"}"
"""
from __future__ import annotations

import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from server import call_unreal  # noqa: E402


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: python Tools/unreal_mcp/client.py <command> [json-args] [json-session]")
        return 2
    command = sys.argv[1]
    args = json.loads(sys.argv[2]) if len(sys.argv) > 2 else {}
    session = {"read_only": True}
    if len(sys.argv) > 3:
        session = json.loads(sys.argv[3])
    result = call_unreal(command, args, session)
    print(json.dumps(result, indent=2))
    return 0 if result.get("ok") else 1


if __name__ == "__main__":
    raise SystemExit(main())
