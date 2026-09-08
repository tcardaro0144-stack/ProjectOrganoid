import json
import sys

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

LABELS = [
    "Gate_ResearchWing",
    "Spine_Landing_Admin",
    "Spine_Landing_NeuroGenetics",
    "Spine_Bridge_Admin",
    "Spine_Bridge_NeuroGenetics",
    "Spine_Ramp_Admin_To_NeuroGenetics",
    "Checkpoint_NeuroAirlock",
    "Host_Neuro_1",
    "Host_Neuro_2",
    "Host_Neuro_3",
    "PowerPanel_NeuroBackup",
    "Hazard_ScrubberLeak",
    "CorridorTraps_GowningRing",
    "Scannable_OrganoidMatrix_1",
    "Scannable_OrganoidMatrix_2",
    "Scannable_OrganoidMatrix_3",
    "DataPad_EthicsObjection",
    "DataPad_SpecimenBadge",
    "NPC_IncineratorSurvivor",
    "Ambience_GowningCorridor",
    "NavMeshBounds_NeuroGenetics",
    "SterlingTerminal_FieldOffice",
]


def compact(actor):
    if not actor or not actor.get("ok", True):
        return actor
    data = actor.get("data") or actor
    loc = data.get("location") or data.get("world_location")
    rot = data.get("rotation") or data.get("world_rotation")
    bounds = data.get("bounds") or {}
    return {
        "label": data.get("label") or data.get("name"),
        "name": data.get("name"),
        "class": data.get("class"),
        "package": data.get("package") or data.get("owning_package") or data.get("level"),
        "location": loc,
        "rotation": rot,
        "scale": data.get("scale"),
        "bounds": bounds,
        "components": [
            {
                "name": c.get("name"),
                "class": c.get("class"),
                "location": c.get("location") or c.get("world_location"),
            }
            for c in (data.get("components") or [])[:12]
        ],
    }


print("STATE", json.dumps(call_unreal("get_editor_state"), indent=2)[:2500])
print("PIE", json.dumps(call_unreal("get_pie_state"), indent=2)[:800])

for label in LABELS:
    r = call_unreal("get_actor", {"label": label, "prefer_pie": False})
    print("\n===", label, "===")
    if not r.get("ok"):
        print(json.dumps(r, indent=2)[:1200])
        continue
    print(json.dumps(compact(r), indent=2)[:2500])

# Sample Neuro floor around expected rooms
samples = [
    ("neuro_entry_est", [1950, 0, -1140], 900),
    ("neuro_gate_est", [2900, 900, -1060], 600),
    ("neuro_landing_est", [3200, 900, -1170], 800),
    ("neuro_corridor_est", [-950, 0, -1140], 900),
    ("neuro_se_est", [0, -1650, -1140], 900),
    ("neuro_ne_est", [0, 1650, -1140], 900),
    ("neuro_sw_est", [-1950, -1650, -1140], 900),
    ("neuro_nw_est", [-1950, 1650, -1140], 900),
    ("checkpoint_live", None, 700),
]
for name, origin, radius in samples:
    args = {"radius": radius, "max": 60}
    if origin:
        args["origin"] = origin
    r = call_unreal("list_actors_near", args)
    print("\n### NEAR", name, origin, "r", radius)
    data = r.get("data") or r
    actors = data.get("actors") or data.get("items") or []
    if isinstance(data, list):
        actors = data
    slim = []
    for a in actors[:50]:
        slim.append({
            "label": a.get("label") or a.get("name"),
            "class": a.get("class"),
            "location": a.get("location"),
            "package": a.get("package") or a.get("owning_package"),
            "distance": a.get("distance"),
        })
    print(json.dumps(slim, indent=2)[:4000])
