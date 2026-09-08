import json
import sys
import time

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

print("=== start HostCombatLoop for Lvl_Epitope PIE (read-only inspect) ===")
started = call_unreal("run_playtest", {"test_id": "HostCombatLoop_Functional"})
print(json.dumps(started, indent=2))
run_id = (started.get("data") or {}).get("run_id")
if not run_id:
    sys.exit(1)

spine_labels = [
    "Spine_Landing_Admin",
    "Spine_Ramp_Admin_To_NeuroGenetics",
    "Spine_Landing_NeuroGenetics",
    "Spine_Bridge_Admin",
    "Spine_Bridge_NeuroGenetics",
    "Gate_ResearchWing",
    "StreamBand_Admin_NeuroGenetics",
    "Wall_Perimeter_East_0",
    "Wall_Perimeter_East_1",
    "Wall_Perimeter_East_2",
    "Admin_ServiceCorridor_Wall_East",
]

deadline = time.time() + 90
pie_ready = False
while time.time() < deadline:
    pie = call_unreal("get_pie_state")
    data = pie.get("data") or {}
    playing = data.get("playing") or data.get("pie_running") or data.get("is_playing")
    world = data.get("world") or data.get("pie_world") or data.get("map")
    print("pie:", json.dumps({"ok": pie.get("ok"), "playing": playing, "world": world, "keys": list(data.keys())}))
    if playing:
        pie_ready = True
        break
    time.sleep(2)

if not pie_ready:
    print("PIE did not start")
    call_unreal("stop_playtest", {"run_id": run_id})
    sys.exit(2)

# Give streaming a moment
time.sleep(4)

for label in spine_labels:
    r = call_unreal("get_actor", {"label": label, "prefer_pie": True})
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
            }
        )
    )
    if label == "Gate_ResearchWing" and r.get("ok"):
        for prop in ("GateId", "RequiredSecurityTier", "GateState", "PowerSector"):
            pr = call_unreal("get_actor_property", {"actor": label, "property": prop, "prefer_pie": True})
            print("  prop", prop, json.dumps((pr.get("data") or {}), default=str))

near = call_unreal(
    "list_actors_near",
    {"origin": [5000, -900, 0], "radius": 900, "max": 40, "prefer_pie": True},
)
print("NEAR_LANDING", json.dumps(near, indent=2)[:4000])

near_gate = call_unreal(
    "list_actors_near",
    {"origin": [2900, 900, -1060], "radius": 700, "max": 40, "prefer_pie": True},
)
print("NEAR_GATE", json.dumps(near_gate, indent=2)[:4000])

print("=== stop playtest ===")
print(json.dumps(call_unreal("stop_playtest", {"run_id": run_id}), indent=2))
time.sleep(3)
print(json.dumps(call_unreal("get_pie_state"), indent=2))
print(json.dumps(call_unreal("get_editor_state"), indent=2)[:2000])
