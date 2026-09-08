import json
import sys
import time

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

deadline = time.time() + 240
last = ""
while time.time() < deadline:
    try:
        state = call_unreal("get_editor_state")
    except Exception as exc:
        line = "exc %s" % exc
        if line != last:
            print(line)
            last = line
        time.sleep(5)
        continue
    data = state.get("data") or {}
    line = "%s map=%s dirty=%s pie=%s" % (
        state.get("ok"),
        data.get("map"),
        data.get("dirty_count"),
        data.get("has_editor_world"),
    )
    if line != last:
        print(line)
        last = line
    if state.get("ok") and data.get("has_editor_world"):
        print(json.dumps(state, indent=2)[:2500])
        sys.exit(0)
    time.sleep(5)

print("TIMEOUT waiting for editor")
sys.exit(1)
