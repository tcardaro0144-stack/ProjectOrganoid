import json
import sys

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

prepared = call_unreal(
    "prepare_write",
    {
        "action": "save_maps",
        "packages": ["/Game/Maps/Epitope/SL_Epitope_NeuroGenetics"],
        "description": "Neuro-only save to clear MainMenu Recast dirty before playtests. No Neuro geometry mutation.",
    },
    {"read_only": True},
)
print("PREPARE", json.dumps(prepared, indent=2)[:2000])
change_id = (prepared.get("data") or {}).get("change_id")
if not change_id:
    print(json.dumps(prepared, indent=2))
    sys.exit(1)

call_unreal("approve_write", {"change_id": change_id, "role": "user", "identity": "Tom"}, {"read_only": True})
call_unreal("approve_write", {"change_id": change_id, "role": "second_review", "identity": "Grok"}, {"read_only": True})
print(
    "EXECUTE",
    json.dumps(
        call_unreal(
            "execute_write",
            {"change_id": change_id},
            {"read_only": False, "required_world_package": "/Game/Maps/Epitope/SL_Epitope_NeuroGenetics"},
        ),
        indent=2,
    )[:2500],
)
print("STATE", json.dumps(call_unreal("get_editor_state"), indent=2)[:1200])
