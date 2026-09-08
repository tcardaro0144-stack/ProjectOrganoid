import json
import sys

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

CHANGE = "chg_93877b31-4db3-4e91-2e60-04b7b2508ab9"

print("USER", json.dumps(call_unreal("approve_write", {"change_id": CHANGE, "role": "user", "identity": "Tom"}, {"read_only": True}), indent=2))
print("REVIEW", json.dumps(call_unreal("approve_write", {"change_id": CHANGE, "role": "second_review", "identity": "Grok"}, {"read_only": True}), indent=2))
print(
    "EXECUTE",
    json.dumps(
        call_unreal(
            "execute_write",
            {"change_id": CHANGE},
            {"read_only": False, "required_world_package": "/Game/Maps/Epitope/SL_Epitope_Admin"},
        ),
        indent=2,
    ),
)

labels = [
    "Admin_ServiceCorridor_Wall_East",
    "Admin_ServiceCorridor_Wall_East_South",
    "Admin_ServiceCorridor_Wall_East_North",
    "Admin_ServiceCorridor_Wall_East_Header",
    "Admin_ResearchWing_Connector_Floor",
    "Admin_ResearchWing_Connector_Ceiling",
    "Admin_ResearchWing_Connector_Wall_South",
    "Admin_ResearchWing_Connector_Wall_North",
    "Admin_ResearchWing_Connector_Threshold",
    "Admin_Transit_Wall_East",
]
for label in labels:
    r = call_unreal("get_actor", {"label": label, "prefer_pie": False})
    data = r.get("data") or {}
    print(label, r.get("ok"), data.get("location"), data.get("scale"), r.get("error"))

print("STATE", json.dumps(call_unreal("get_editor_state"), indent=2)[:1500])
