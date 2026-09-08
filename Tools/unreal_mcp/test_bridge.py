"""Simulates a real MCP client talking to server.py over stdio.

Usage:
    python test_bridge.py "C:\\path\\to\\server.py"

Sends a properly framed `initialize` request, then `tools/list`,
and prints exactly what the server sends back (or the raw stderr
if it crashes). This mirrors what Cline actually does at launch,
unlike typing into an interactive terminal.
"""

import json
import subprocess
import sys


def frame(message: dict) -> bytes:
    encoded = json.dumps(message).encode("utf-8")
    header = f"Content-Length: {len(encoded)}\r\n\r\n".encode("ascii")
    return header + encoded


def read_one(stream) -> dict | None:
    headers = {}
    while True:
        line = stream.readline()
        if not line:
            return None
        if line in (b"\r\n", b"\n"):
            break
        decoded = line.decode("utf-8", errors="replace")
        if ":" in decoded:
            k, v = decoded.split(":", 1)
            headers[k.strip().lower()] = v.strip()
    length = int(headers.get("content-length", "0") or 0)
    if length <= 0:
        return None
    body = stream.read(length)
    return json.loads(body.decode("utf-8"))


def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: python test_bridge.py <path-to-server.py>")
        sys.exit(1)

    server_path = sys.argv[1]

    proc = subprocess.Popen(
        [sys.executable, server_path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    try:
        proc.stdin.write(frame({
            "jsonrpc": "2.0",
            "id": 1,
            "method": "initialize",
            "params": {},
        }))
        proc.stdin.flush()

        reply = read_one(proc.stdout)
        print("=== initialize reply ===")
        print(json.dumps(reply, indent=2))

        proc.stdin.write(frame({
            "jsonrpc": "2.0",
            "id": 2,
            "method": "tools/list",
            "params": {},
        }))
        proc.stdin.flush()

        reply = read_one(proc.stdout)
        print("=== tools/list reply ===")
        tool_count = len(reply["result"]["tools"]) if reply and "result" in reply else 0
        print(f"Got {tool_count} tools back.")

    finally:
        proc.stdin.close()
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()

        stderr_output = proc.stderr.read().decode("utf-8", errors="replace")
        if stderr_output.strip():
            print("=== stderr from server.py ===")
            print(stderr_output)


if __name__ == "__main__":
    main()
