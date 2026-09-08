import json
import sys

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal


def gated(action, args, session_package):
    prepared = call_unreal("prepare_write", args, {"read_only": True})
    print("PREPARE", action, json.dumps(prepared, indent=2)[:2000])
    change_id = (prepared.get("data") or {}).get("change_id")
    if not change_id:
        raise SystemExit(f"{action} prepare failed")
    print("USER", json.dumps(call_unreal("approve_write", {"change_id": change_id, "role": "user", "identity": "Tom"}, {"read_only": True}), indent=2)[:400])
    print("REVIEW", json.dumps(call_unreal("approve_write", {"change_id": change_id, "role": "second_review", "identity": "Grok"}, {"read_only": True}), indent=2)[:400])
    executed = call_unreal(
        "execute_write",
        {"change_id": change_id},
        {"read_only": False, "required_world_package": session_package},
    )
    print("EXECUTE", action, json.dumps(executed, indent=2)[:2500])
    return change_id, executed


item_id, _ = gated(
    "save_asset",
    {
        "action": "save_asset",
        "required_package": "/Game/Data/Items/DA_Item_ResearchWingKeycard",
        "description": "Save DA_Item_ResearchWingKeycard only.",
    },
    "/Game/Maps/Epitope/SL_Epitope_Admin",
)
admin_id, _ = gated(
    "save_maps",
    {
        "action": "save_maps",
        "packages": ["/Game/Maps/Epitope/SL_Epitope_Admin"],
        "description": "Admin-only save after Research Wing keycard placement.",
    },
    "/Game/Maps/Epitope/SL_Epitope_Admin",
)
print("ITEM_CHANGE", item_id)
print("ADMIN_CHANGE", admin_id)
print("STATE", json.dumps(call_unreal("get_editor_state", {}), indent=2)[:2000])
