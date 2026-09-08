import json
import sys

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

labels = [
    "Admin_ServiceCorridor_Floor",
    "Admin_ServiceCorridor_Ceiling",
    "Admin_ServiceCorridor_Wall_East",
    "Admin_ServiceCorridor_Wall_South",
    "Admin_ServiceCorridor_Wall_West",
    "Admin_ServiceCorridor_Threshold",
    "Admin_ServiceCorridor_Cabinet_East",
    "Admin_Transit_Wall_East",
    "Admin_Transit_Wall_South",
    "Admin_Transit_Floor",
    "Checkpoint_NeuroAirlock",
    "NeuroGenetics_FloorPlate",
    "Spine_Landing_Admin",
    "Spine_Ramp_Admin_To_NeuroGenetics",
    "Spine_Landing_NeuroGenetics",
    "Spine_Bridge_NeuroGenetics",
    "Gate_ResearchWing",
]

for label in labels:
    r = call_unreal("get_actor", {"label": label})
    data = r.get("data") or {}
    print(
        json.dumps(
            {
                "label": label,
                "ok": r.get("ok"),
                "error": r.get("error"),
                "class": data.get("class") or data.get("class_name"),
                "package": data.get("owning_package") or data.get("package"),
                "location": data.get("location"),
                "scale": data.get("scale"),
                "rotation": data.get("rotation"),
                "bounds_origin": data.get("bounds_origin"),
                "bounds_extent": data.get("bounds_extent"),
            },
            indent=2,
        )
    )
