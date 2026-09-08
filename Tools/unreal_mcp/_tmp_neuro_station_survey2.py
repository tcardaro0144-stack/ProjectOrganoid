import json
import sys

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal


def dump(label, r, n=3500):
    print("\n===", label, "===")
    print(json.dumps(r, indent=2)[:n])


# Ownership / collision / extents
for actor, comp in [
    ("Checkpoint_NeuroAirlock", "InteractionSphere"),
    ("Checkpoint_NeuroAirlock", "CheckpointMesh"),
    ("Checkpoint_NeuroAirlock", "AutosaveVolume"),
    ("Hazard_ScrubberLeak", "HazardVolume"),
    ("CorridorTraps_GowningRing", "CorridorBounds"),
    ("PowerPanel_NeuroBackup", "PanelMesh"),
    ("PowerPanel_NeuroBackup", "InteractionSphere"),
    ("Host_Neuro_1", "CollisionCylinder"),
    ("Host_Neuro_1", "HostPerception"),
    ("NavMeshBounds_NeuroGenetics", None),
    ("SterlingTerminal_FieldOffice", "TerminalMesh"),
    ("SterlingTerminal_FieldOffice", None),
    ("NPC_IncineratorSurvivor", None),
    ("DataPad_EthicsObjection", None),
    ("DataPad_SpecimenBadge", None),
    ("Scannable_OrganoidMatrix_3", "MeshComponent"),
    ("Ambience_GowningCorridor", None),
]:
    if comp:
        dump(f"{actor}.{comp}", call_unreal("get_component", {"actor": actor, "component": comp}))
    else:
        dump(actor, call_unreal("get_actor", {"label": actor, "prefer_pie": False}))

# Geometry around candidate spots
spots = [
    ("entry_doorway", [1000, 0, -1140], 700),
    ("entry_sw_wall", [1200, -900, -1140], 700),
    ("se_east_wall", [800, -1600, -1140], 800),
    ("corridor_pretrap", [500, -200, -1140], 700),
    ("ne_door", [0, 400, -1140], 700),
    ("neuro_east_wall", [2900, 900, -1140], 900),
    ("neuro_floor", [0, 0, -1200], 500),
]
for name, origin, radius in spots:
    r = call_unreal("list_actors_near", {"origin": origin, "radius": radius, "max": 50})
    data = r.get("data") or {}
    actors = data.get("actors") or data.get("items") or (data if isinstance(data, list) else [])
    slim = []
    for a in actors:
        lab = a.get("label") or a.get("name") or ""
        if any(k in lab for k in ("Light_", "Billboard", "Brush", "WorldSettings", "Recast", "DefaultPhysics", "Buoyancy", "Sky", "Cloud", "Floor")):
            continue
        slim.append({
            "label": lab,
            "class": a.get("class"),
            "location": a.get("location"),
            "distance": a.get("distance"),
        })
    print("\n###", name, origin)
    print(json.dumps(slim, indent=2)[:3500])

# Named Neuro walls / plate
for label in [
    "NeuroGenetics_FloorPlate",
    "Wall_EntryHall",
    "Wall_EntryHall_0",
    "Wall_EntryHall_1",
    "Wall_Perimeter_East",
    "Wall_Perimeter_East_0",
    "Wall_Perimeter_East_1",
    "Wall_Corridor_North_0",
    "Wall_Corridor_South_0",
    "Wall_Divider_South",
    "Wall_Divider_North",
]:
    r = call_unreal("get_actor", {"label": label, "prefer_pie": False})
    print("\n##", label, "ok" if r.get("ok") else r.get("error"))
    if r.get("ok"):
        d = r.get("data") or {}
        print(json.dumps({
            "label": d.get("label") or d.get("name"),
            "location": d.get("location"),
            "rotation": d.get("rotation"),
            "scale": d.get("scale"),
            "class": d.get("class"),
        }, indent=2))
