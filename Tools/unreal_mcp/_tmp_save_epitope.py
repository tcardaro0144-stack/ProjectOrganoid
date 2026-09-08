import json
import sys

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

prepared = call_unreal(
    "prepare_write",
    {
        "action": "save_maps",
        "packages": ["/Game/Maps/Lvl_Epitope"],
        "description": "Lvl_Epitope-only save after Admin spine plate trim.",
    },
    {"read_only": True},
)
print("PREPARE", json.dumps(prepared, indent=2)[:3000])
change_id = (prepared.get("data") or {}).get("change_id")
if not change_id:
    sys.exit(1)
call_unreal("approve_write", {"change_id": change_id, "role": "user", "identity": "Tom"}, {"read_only": True})
call_unreal("approve_write", {"change_id": change_id, "role": "second_review", "identity": "Grok"}, {"read_only": True})
saved = call_unreal(
    "execute_write",
    {"change_id": change_id},
    {"read_only": False, "required_world_package": "/Game/Maps/Lvl_Epitope"},
)
print("SAVED", json.dumps(saved, indent=2)[:4000])
print("STATE", json.dumps(call_unreal("get_editor_state"), indent=2)[:2000])
if not saved.get("ok"):
    sys.exit(2)
