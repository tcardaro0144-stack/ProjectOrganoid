import json
import sys

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

print("STATE", json.dumps(call_unreal("get_editor_state"), indent=2)[:1800])
print("PIE", json.dumps(call_unreal("get_pie_state"), indent=2)[:400])

queries = [
    ("SECURITY", [2680, -400, 110], 450),
    ("RECORDS", [2680, 400, 110], 400),
    ("HUB", [2280, 0, 80], 400),
    ("RECEPTION", [1450, -150, 110], 400),
    ("CHECKPOINT_WEST", [-1535, 0, 60], 250),
    ("NEURO_AIRLOCK_GUESS", [5000, -900, 220], 400),
]

interesting = (
    "Checkpoint", "Pickup", "Terminal", "Hologram", "DoorLock", "Gate",
    "Block2", "Brand", "Locker", "Cabinet", "FirstAid", "Med", "Ammo",
    "Chair", "Mug", "Headset", "Keycard", "RoomTrigger", "Security",
    "Records", "Host",
)

for title, origin, radius in queries:
    print("====", title, origin, radius)
    data = (call_unreal("list_actors_near", {"origin": origin, "radius": radius, "max": 60}).get("data") or {})
    actors = data.get("actors") or []
    print("COUNT", len(actors))
    for actor in actors:
        label = actor.get("label") or ""
        cls = actor.get("class") or ""
        if any(k.lower() in label.lower() or k.lower() in cls.lower() for k in interesting):
            print(" ", label, actor.get("location"), cls)

for label in (
    "Checkpoint_ReceptionAtrium",
    "Checkpoint_NeuroAirlock",
    "Pickup_ResearchWingKeycard",
    "Admin_Terminal_Security",
    "Admin_Terminal_Records",
    "Admin_FacilityHologram",
    "Admin_RoomTrigger_Security",
    "Admin_RoomTrigger_Records",
    "DoorLock_VestibuleToAtrium",
):
    print("----", label)
    actor = call_unreal("get_actor", {"name": label})
    print(json.dumps(actor, indent=2)[:1600])
    for prop in (
        "bRestoreHealthOnSave",
        "bSaveOnInteract",
        "bSaveOnOverlapEnter",
        "CheckpointId",
        "Quantity",
        "bIsInteractable",
        "ItemData",
    ):
        result = call_unreal("get_actor_property", {"actor": label, "property": prop})
        if result.get("ok"):
            print("PROP", prop, json.dumps(result.get("data"), indent=2)[:500])
