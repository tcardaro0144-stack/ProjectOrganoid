import json
import sys
import time

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

TESTS = [
    "NeuroResearchStationPlacement_Functional",
    "ResearchStation_Functional",
    "AmmoReload_Functional",
    "HostCombatLoop_Functional",
    "AdminToNeuroTraversal_Functional",
    "S17_AccessDoor_Functional",
]


def summarize_result(result):
    data = result.get("data") or {}
    record = data.get("record") or data
    assertions = record.get("assertions") or []
    passed = sum(1 for a in assertions if a.get("passed"))
    failed = [a for a in assertions if not a.get("passed")]
    return {
        "run_id": data.get("run_id") or record.get("run_id"),
        "state": data.get("state") or record.get("state"),
        "passed": passed,
        "failed": len(failed),
        "total": len(assertions),
        "failure_reason": record.get("failure_reason") or data.get("failure_reason"),
        "failed_ids": [a.get("id") for a in failed],
        "dirty_count": next((a.get("actual") for a in (record.get("actors") or []) if False), None),
        "actors": record.get("actors") or data.get("actors"),
    }


def save_neuro_if_dirty():
    state = call_unreal("get_editor_state")
    data = state.get("data") or {}
    dirty = [p.get("package") for p in (data.get("dirty_packages") or [])]
    if not dirty:
        return True
    if dirty != ["/Game/Maps/Epitope/SL_Epitope_NeuroGenetics"]:
        print("UNEXPECTED_DIRTY", dirty)
        return False
    print("NEURO_DIRTY_ON_LOAD saving targeted Neuro-only")
    prepared = call_unreal(
        "prepare_write",
        {
            "action": "save_maps",
            "packages": ["/Game/Maps/Epitope/SL_Epitope_NeuroGenetics"],
            "description": "Neuro-only Recast/post-load dirty clear before next playtest.",
        },
        {"read_only": True},
    )
    cid = (prepared.get("data") or {}).get("change_id")
    if not cid:
        print("PREPARE FAIL", json.dumps(prepared, indent=2)[:1500])
        return False
    call_unreal("approve_write", {"change_id": cid, "role": "user", "identity": "Tom"}, {"read_only": True})
    call_unreal("approve_write", {"change_id": cid, "role": "second_review", "identity": "Grok"}, {"read_only": True})
    saved = call_unreal(
        "execute_write",
        {"change_id": cid},
        {"read_only": False, "required_world_package": "/Game/Maps/Epitope/SL_Epitope_NeuroGenetics"},
    )
    print("SAVED", saved.get("ok"), cid)
    return bool(saved.get("ok"))


def run_one(test_id):
    if not save_neuro_if_dirty():
        return False, {}
    print("=== START", test_id)
    started = call_unreal("run_playtest", {"test_id": test_id})
    run_id = (started.get("data") or {}).get("run_id")
    if not run_id:
        print(json.dumps(started, indent=2)[:2000])
        return False, {}
    deadline = time.time() + 240
    while time.time() < deadline:
        time.sleep(5)
        status = call_unreal("get_playtest_status", {"run_id": run_id})
        data = status.get("data") or {}
        state = data.get("state")
        print(test_id, state, data.get("current_stage"), data.get("elapsed_ms"))
        if state in ("pass", "fail", "blocked", "error"):
            result = call_unreal("get_playtest_result", {"run_id": run_id})
            summary = summarize_result(result)
            summary["run_id"] = run_id
            summary["state"] = state
            print("RESULT", json.dumps(summary, indent=2))
            if state != "pass":
                print(json.dumps(result, indent=2)[:12000])
            return state == "pass", summary
    print("TIMEOUT", test_id)
    call_unreal("stop_playtest", {"run_id": run_id})
    return False, {"run_id": run_id, "state": "timeout"}


summaries = []
ok = True
for test_id in TESTS:
    passed, summary = run_one(test_id)
    summaries.append({"test_id": test_id, **summary})
    if not passed:
        ok = False
        break
    time.sleep(3)

print("SUMMARIES", json.dumps(summaries, indent=2))
print("FINAL", json.dumps(call_unreal("get_editor_state"), indent=2)[:1500])
sys.exit(0 if ok else 1)
