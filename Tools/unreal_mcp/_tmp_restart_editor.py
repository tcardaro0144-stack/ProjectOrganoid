import json
import subprocess
import sys
import time
import urllib.request

EDITOR = r"C:\Users\tomca\Desktop\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
PROJECT = r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\ProjectOrganoid.uproject"
BRIDGE = "http://127.0.0.1:8732/v1/command"


def bridge_ok():
    payload = json.dumps({"command": "get_editor_state", "args": {}, "session": {"read_only": True}}).encode("utf-8")
    req = urllib.request.Request(BRIDGE, data=payload, headers={"Content-Type": "application/json"}, method="POST")
    try:
        with urllib.request.urlopen(req, timeout=5) as response:
            return json.loads(response.read().decode("utf-8"))
    except Exception as exc:
        return {"ok": False, "error": str(exc)}


print("Force-stopping all UnrealEditor processes")
subprocess.run(["taskkill", "/IM", "UnrealEditor.exe", "/F", "/T"], check=False)
for _ in range(40):
    time.sleep(2)
    ping = bridge_ok()
    if not ping.get("ok"):
        print("bridge down")
        break
else:
    print("bridge still up after kill")
    sys.exit(2)

time.sleep(3)
print("Starting UnrealEditor")
subprocess.Popen([EDITOR, PROJECT], cwd=r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid")
deadline = time.time() + 240
while time.time() < deadline:
    time.sleep(5)
    state = bridge_ok()
    data = state.get("data") or {}
    print("wait", state.get("ok"), data.get("map"), data.get("dirty_count"), state.get("error"))
    if state.get("ok") and data.get("has_editor_world"):
        print(json.dumps(state, indent=2)[:2000])
        sys.exit(0)

print("Editor did not become reachable")
sys.exit(1)
