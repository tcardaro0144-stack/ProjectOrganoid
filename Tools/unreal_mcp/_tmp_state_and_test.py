import json
import sys
import time

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

print("STATE", json.dumps(call_unreal("get_editor_state"), indent=2)[:1500])
started = call_unreal("run_playtest", {"test_id": "AdminToNeuroTraversal_Functional"})
print("START", json.dumps(started, indent=2)[:600])
run_id = (started.get("data") or {}).get("run_id")
if not run_id:
    sys.exit(1)
deadline = time.time() + 180
while time.time() < deadline:
    time.sleep(4)
    status = call_unreal("get_playtest_status", {"run_id": run_id})
    data = status.get("data") or {}
    print(data.get("state"), data.get("current_stage"), data.get("elapsed_ms"))
    if data.get("state") in ("pass", "fail", "blocked", "error"):
        print(json.dumps(call_unreal("get_playtest_result", {"run_id": run_id}), indent=2)[:14000])
        sys.exit(0 if data.get("state") == "pass" else 1)
print("timeout")
call_unreal("stop_playtest", {"run_id": run_id})
sys.exit(2)
