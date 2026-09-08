import json
import sys
import time

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

TESTS = [
    "ResearchStation_Functional",
    "AmmoReload_Functional",
    "HostCombatLoop_Functional",
    "AdminToNeuroTraversal_Functional",
    "S17_AccessDoor_Functional",
]


def run_one(test_id, timeout_s=240):
    print("=== START", test_id, flush=True)
    started = call_unreal("run_playtest", {"test_id": test_id})
    print(json.dumps(started, indent=2)[:800], flush=True)
    run_id = (started.get("data") or {}).get("run_id")
    if not run_id:
        return False, started
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        time.sleep(4)
        status = call_unreal("get_playtest_status", {"run_id": run_id})
        data = status.get("data") or {}
        state = data.get("state")
        print(test_id, state, data.get("current_stage"), data.get("elapsed_ms"), flush=True)
        if state in ("pass", "fail", "blocked", "error"):
            result = call_unreal("get_playtest_result", {"run_id": run_id})
            print(json.dumps(result, indent=2)[:16000], flush=True)
            return state == "pass", result
    print("TIMEOUT", test_id, flush=True)
    call_unreal("stop_playtest", {"run_id": run_id})
    return False, {"timeout": True, "run_id": run_id}


ok = True
results = []
for test_id in TESTS:
    passed, payload = run_one(test_id)
    results.append((test_id, passed))
    if not passed:
        ok = False
        break
    time.sleep(3)

print("SUMMARY", json.dumps(results, indent=2), flush=True)
print("STATE", json.dumps(call_unreal("get_editor_state"), indent=2)[:1200], flush=True)
sys.exit(0 if ok else 1)
