import json
import sys
import time

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

TESTS = [
    ("OpeningFoundation_Functional", 240),
    ("DoorLockPowerInteractable_Functional", 180),
    ("S17_AccessDoor_Functional", 180),
    ("S18_ReceptionTerminal_Functional", 180),
    ("S18_SecurityTerminal_Functional", 180),
    ("S18_RecordsTerminal_Functional", 180),
    ("S18_ExecutiveTerminal_Functional", 180),
    ("S18_OperationsTerminal_Functional", 180),
    ("S18_TransitTerminal_Functional", 180),
    ("S19_FacilityHologram_Functional", 180),
    ("NeuroAccess_Functional", 240),
    ("AdminToNeuroTraversal_Functional", 240),
    ("AmmoReload_Functional", 180),
    ("HostCombatLoop_Functional", 240),
    ("ResearchStation_Functional", 180),
    ("NeuroResearchStationPlacement_Functional", 180),
    ("BiologicalAdaptation_Functional", 180),
    ("PETactical_Functional", 180),
]


def run_one(test_id, timeout):
    print("=== START", test_id)
    started = call_unreal("run_playtest", {"test_id": test_id})
    run_id = (started.get("data") or {}).get("run_id")
    if not run_id:
        print("NO_RUN_ID", json.dumps(started)[:800])
        return {
            "test_id": test_id,
            "state": "error",
            "assertions": 0,
            "failures": 1,
            "failed_ids": ["no_run_id"],
            "headline": str(started)[:400],
        }
    deadline = time.time() + timeout
    last = ""
    while time.time() < deadline:
        time.sleep(4)
        status = call_unreal("get_playtest_status", {"run_id": run_id})
        data = status.get("data") or {}
        line = "%s %s %s" % (data.get("state"), data.get("current_stage"), data.get("elapsed_ms"))
        if line != last:
            print(test_id, line)
            last = line
        if data.get("state") in ("pass", "fail", "blocked", "error"):
            result = call_unreal("get_playtest_result", {"run_id": run_id})
            rec = result.get("data") or {}
            assertions = rec.get("assertions") or []
            failed = [a for a in assertions if not a.get("passed")]
            summary = {
                "test_id": test_id,
                "state": rec.get("state") or data.get("state"),
                "assertions": len(assertions),
                "failures": len(failed),
                "failed_ids": [a.get("id") for a in failed],
                "headline": rec.get("headline"),
                "failure_reason": rec.get("failure_reason"),
            }
            print("RESULT", json.dumps(summary, indent=2))
            return summary
    print("TIMEOUT", test_id)
    call_unreal("stop_playtest", {"run_id": run_id})
    return {
        "test_id": test_id,
        "state": "timeout",
        "assertions": 0,
        "failures": 1,
        "failed_ids": ["timeout"],
        "headline": "TIMEOUT",
    }


summaries = []
ok = True
for test_id, timeout in TESTS:
    summary = run_one(test_id, timeout)
    summaries.append(summary)
    if summary.get("state") != "pass":
        ok = False
        break
    time.sleep(2)

print("==== SUITE ====")
print(json.dumps(summaries, indent=2))
print("STATE", json.dumps(call_unreal("get_editor_state"), indent=2)[:1500])
sys.exit(0 if ok else 2)
