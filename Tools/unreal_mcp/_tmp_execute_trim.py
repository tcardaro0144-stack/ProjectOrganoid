import json
import sys

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

print("STATE", json.dumps(call_unreal("get_editor_state"), indent=2)[:2000])

prepared = call_unreal(
    "prepare_write",
    {
        "action": "trim_spine_landing_admin",
        "spec": "trim_spine_landing_admin_v1",
        "description": "Trim Admin spine landing and bridge so the existing ramp is exposed south of Transit.",
    },
    {"read_only": True},
)
print("PREPARE", json.dumps(prepared, indent=2)[:4000])
change_id = (prepared.get("data") or {}).get("change_id")
if not change_id:
    sys.exit(1)

print("USER", json.dumps(call_unreal("approve_write", {"change_id": change_id, "role": "user", "identity": "Tom"}, {"read_only": True}), indent=2))
print("REVIEW", json.dumps(call_unreal("approve_write", {"change_id": change_id, "role": "second_review", "identity": "Grok"}, {"read_only": True}), indent=2))
executed = call_unreal(
    "execute_write",
    {"change_id": change_id},
    {"read_only": False, "required_world_package": "/Game/Maps/Lvl_Epitope"},
)
print("EXECUTE", json.dumps(executed, indent=2)[:8000])
if not executed.get("ok"):
    sys.exit(2)

for label in (
    "Spine_Landing_Admin",
    "Spine_Bridge_Admin",
    "Spine_Ramp_Admin_To_NeuroGenetics",
    "Gate_ResearchWing",
    "Admin_Transit_Wall_South",
):
    r = call_unreal("get_actor", {"label": label, "prefer_pie": False})
    data = r.get("data") or {}
    print(label, r.get("ok"), data.get("location"), data.get("scale"), data.get("owning_package") or data.get("package"), r.get("error"))

print("STATE_AFTER", json.dumps(call_unreal("get_editor_state"), indent=2)[:2000])
