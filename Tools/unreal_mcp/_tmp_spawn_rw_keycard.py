import json
import sys

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

print("PIE", json.dumps(call_unreal("get_pie_state", {}), indent=2)[:800])

prepared = call_unreal(
    "prepare_write",
    {
        "action": "spawn_admin_research_wing_keycard",
        "label": "Pickup_ResearchWingKeycard",
        "location": [2580.0, -560.0, 80.0],
        "rotation": [0.0, 0.0, 0.0],
        "destination_package": "/Game/Maps/Epitope/SL_Epitope_Admin",
        "save": False,
        "compile": False,
        "require_pie_stopped": True,
        "description": "Create DA_Item_ResearchWingKeycard and place Pickup_ResearchWingKeycard at approved Security alcove.",
    },
    {"read_only": True},
)
print("PREPARE", json.dumps(prepared, indent=2))
change_id = (prepared.get("data") or {}).get("change_id")
if not change_id:
    sys.exit(1)

print("USER", json.dumps(call_unreal("approve_write", {"change_id": change_id, "role": "user", "identity": "Tom"}, {"read_only": True}), indent=2)[:1200])
print("REVIEW", json.dumps(call_unreal("approve_write", {"change_id": change_id, "role": "second_review", "identity": "Grok"}, {"read_only": True}), indent=2)[:1200])
executed = call_unreal(
    "execute_write",
    {"change_id": change_id},
    {"read_only": False, "required_world_package": "/Game/Maps/Epitope/SL_Epitope_Admin"},
)
print("EXECUTE", json.dumps(executed, indent=2))
print("CHANGE_ID", change_id)
