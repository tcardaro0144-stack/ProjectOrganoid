import json
import sys
import time

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

TESTS = [
    "AdminToNeuroTraversal_Functional",
    "HostCombatLoop_Functional",
    "S17_AccessDoor_Functional",
]


def run_one(test_id):
    print("=== START", test_id)
    started = call_unreal("run_playtest", {"test_id": test_id})
    print(json.dumps(started, indent=2)[:800])
    run_id = (started.get("data") or {}).get("run_id")
    if not run_id:
        return False
    deadline = time.time() + 180
    while time.time() < deadline:
        time.sleep(4)
        status = call_unreal("get_playtest_status", {"run_id": run_id})
        data = status.get("data") or {}
        state = data.get("state")
        print(test_id, state, data.get("current_stage"), data.get("elapsed_ms"))
        if state in ("pass", "fail", "blocked", "error"):
            result = call_unreal("get_playtest_result", {"run_id": run_id})
            print(json.dumps(result, indent=2)[:12000])
            return state == "pass"
    print("TIMEOUT", test_id)
    call_unreal("stop_playtest", {"run_id": run_id})
    return False


ok = True
for test_id in TESTS:
    if not run_one(test_id):
        ok = False
        break
    time.sleep(3)

print("STATE", json.dumps(call_unreal("get_editor_state"), indent=2)[:1200])
sys.exit(0 if ok else 1)
