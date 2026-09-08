import json
import sys
import time

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

TESTS = [
    ("NeuroAccess_Functional", 360),
    ("AdminToNeuroTraversal_Functional", 300),
    ("ResearchStation_Functional", 240),
    ("NeuroResearchStationPlacement_Functional", 240),
    ("BiologicalAdaptation_Functional", 240),
    ("PETactical_Functional", 180),
    ("AmmoReload_Functional", 180),
    ("HostCombatLoop_Functional", 180),
    ("S17_AccessDoor_Functional", 180),
]


def summarize(result):
    data = result.get("data") or {}
    record = data.get("record") or data
    assertions = record.get("assertions") or []
    passed = sum(1 for a in assertions if a.get("passed") is True)
    failed = sum(1 for a in assertions if a.get("passed") is False)
    return {
        "state": data.get("state") or record.get("state"),
        "run_id": data.get("run_id") or record.get("run_id"),
        "passed": passed,
        "failed": failed,
        "total": len(assertions),
        "failure": record.get("failure_reason") or data.get("failure_reason"),
        "headline": record.get("headline") or data.get("headline"),
    }


def run_one(test_id, timeout):
    print("=== START", test_id)
    started = call_unreal("run_playtest", {"test_id": test_id})
    print(json.dumps(started, indent=2)[:800])
    run_id = (started.get("data") or {}).get("run_id")
    if not run_id:
        return False, {"state": "error", "failure": "no run_id"}
    deadline = time.time() + timeout
    while time.time() < deadline:
        time.sleep(5)
        status = call_unreal("get_playtest_status", {"run_id": run_id})
        data = status.get("data") or {}
        state = data.get("state")
        print(test_id, state, data.get("current_stage"), data.get("elapsed_ms"))
        if state in ("pass", "fail", "blocked", "error"):
            result = call_unreal("get_playtest_result", {"run_id": run_id})
            summary = summarize(result)
            print("RESULT", json.dumps(summary, indent=2))
            if state != "pass":
                print(json.dumps(result, indent=2)[:8000])
            return state == "pass", summary
    print("TIMEOUT", test_id)
    call_unreal("stop_playtest", {"run_id": run_id})
    return False, {"state": "timeout"}


results = []
ok = True
for test_id, timeout in TESTS:
    passed, summary = run_one(test_id, timeout)
    results.append((test_id, summary))
    if not passed:
        ok = False
        break
    time.sleep(3)

print("=== SUITE ===")
for test_id, summary in results:
    print(test_id, json.dumps(summary))
print("STATE", json.dumps(call_unreal("get_editor_state", {}), indent=2)[:1200])
sys.exit(0 if ok else 1)
