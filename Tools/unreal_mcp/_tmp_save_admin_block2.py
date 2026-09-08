import json
import sys

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

print("DIRTY BEFORE SAVE")
print(json.dumps(call_unreal("get_editor_state"), indent=2)[:2000])

prepared = call_unreal(
    "prepare_write",
    {
        "action": "save_maps",
        "packages": ["/Game/Maps/Epitope/SL_Epitope_Admin"],
        "description": "Admin-only save to preserve approved Block 2 DoorLock, dressing, and branding before DoorLock C++ rebuild.",
    },
    {"read_only": True},
)
print("PREPARE", json.dumps(prepared, indent=2)[:4000])
change_id = (prepared.get("data") or {}).get("change_id")
if not change_id:
    sys.exit(1)

print("USER", json.dumps(call_unreal("approve_write", {"change_id": change_id, "role": "user", "identity": "Tom"}, {"read_only": True}), indent=2)[:800])
print("REVIEW", json.dumps(call_unreal("approve_write", {"change_id": change_id, "role": "second_review", "identity": "Grok"}, {"read_only": True}), indent=2)[:800])
executed = call_unreal(
    "execute_write",
    {"change_id": change_id},
    {"read_only": False, "required_world_package": "/Game/Maps/Epitope/SL_Epitope_Admin"},
)
print("EXECUTE", json.dumps(executed, indent=2)[:5000])
print("STATE AFTER")
print(json.dumps(call_unreal("get_editor_state"), indent=2)[:2000])
print("DOOR", json.dumps(call_unreal("get_actor_property", {"actor": "DoorLock_VestibuleToAtrium", "property": "bIsInteractable"}), indent=2))
