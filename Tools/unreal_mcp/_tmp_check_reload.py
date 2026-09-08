import json
import sys

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

print("EDITOR", json.dumps(call_unreal("get_editor_state"), indent=2))
print("PLAYTESTS", json.dumps(call_unreal("list_playtests"), indent=2)[:4000])
prepared = call_unreal(
    "prepare_write",
    {
        "action": "spawn_admin_research_wing_connector",
        "spec": "admin_research_wing_connector_v1",
        "destination_package": "/Game/Maps/Epitope/SL_Epitope_Admin",
        "save": False,
        "compile": False,
        "require_pie_stopped": True,
        "description": "Open S12 east wall and spawn Research Wing connector on Admin.",
    },
    {"read_only": True},
)
print("PREPARE", json.dumps(prepared, indent=2))
