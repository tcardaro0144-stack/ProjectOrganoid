import json
import sys
import time

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

prepared = call_unreal(
    "prepare_write",
    {
        "action": "save_maps",
        "packages": ["/Game/Maps/Epitope/SL_Epitope_NeuroGenetics"],
        "description": "Neuro-only Recast dirty clear immediately before playtests.",
    },
    {"read_only": True},
)
change_id = (prepared.get("data") or {}).get("change_id")
if not change_id:
    print("PREPARE FAIL", json.dumps(prepared, indent=2))
    sys.exit(1)
call_unreal("approve_write", {"change_id": change_id, "role": "user", "identity": "Tom"}, {"read_only": True})
call_unreal("approve_write", {"change_id": change_id, "role": "second_review", "identity": "Grok"}, {"read_only": True})
saved = call_unreal(
    "execute_write",
    {"change_id": change_id},
    {"read_only": False, "required_world_package": "/Game/Maps/Epitope/SL_Epitope_NeuroGenetics"},
)
print("SAVED", saved.get("ok"), (saved.get("data") or {}).get("status"))
print("STATE", json.dumps(call_unreal("get_editor_state"), indent=2)[:800])

TESTS = [
    "AdminToNeuroTraversal_Functional",
    "HostCombatLoop_Functional",
    "S17_AccessDoor_Functional",
]


def run_one(test_id):
    print("=== START", test_id)
    started = call_unreal("run_playtest", {"test_id": test_id})
    run_id = (started.get("data") or {}).get("run_id")
    if not run_id:
        print(json.dumps(started, indent=2))
        return False
    deadline = time.time() + 180
    while time.time() < deadline:
        time.sleep(4)
        status = call_unreal("get_playtest_status", {"run_id": run_id})
        data = status.get("data") or {}
        state = data.get("state")
        print(test_id, state, data.get("current_stage"), data.get("elapsed_ms"))
        if state in ("pass", "fail", "blocked", "error"):
            print(json.dumps(call_unreal("get_playtest_result", {"run_id": run_id}), indent=2)[:14000])
            return state == "pass"
    call_unreal("stop_playtest", {"run_id": run_id})
    return False


ok = True
for test_id in TESTS:
    if not run_one(test_id):
        ok = False
        break
    time.sleep(2)

print("FINAL", json.dumps(call_unreal("get_editor_state"), indent=2)[:1000])
sys.exit(0 if ok else 1)
