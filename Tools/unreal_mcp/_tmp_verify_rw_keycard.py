import json
import sys

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

print("ACTOR", json.dumps(call_unreal("get_actor", {"label": "Pickup_ResearchWingKeycard"}), indent=2)[:3500])
for prop in ("Quantity", "bDestroyOnPickup", "PickupObjectiveEventId"):
    print(prop, json.dumps(call_unreal("get_actor_property", {"actor": "Pickup_ResearchWingKeycard", "property": prop})))
print("ADMIN_CARD", json.dumps(call_unreal("get_actor", {"label": "Pickup_AdminKeycard"}), indent=2)[:800])
print("GATE", json.dumps(call_unreal("get_actor", {"label": "Gate_ResearchWing"}), indent=2)[:500])
print("OVERLAP", json.dumps(call_unreal("overlap_query", {"origin": [2580, -560, 80], "half_height": 44, "radius": 21}), indent=2)[:2000])
print("STATE", json.dumps(call_unreal("get_editor_state", {}), indent=2)[:2000])
