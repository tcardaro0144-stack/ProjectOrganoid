import json
import sys
from collections import Counter

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal

print("PIE", json.dumps(call_unreal("get_pie_state"), indent=2)[:500])
print("PING", json.dumps(call_unreal("ping"), indent=2)[:2500])

tests = call_unreal("list_playtests")
ids = [t.get("test_id") for t in ((tests.get("data") or {}).get("tests") or [])]
print("HAS_INVESTIGATION", "OpeningInvestigation_Functional" in ids)
print("HAS_DOORLOCK_POWER", "DoorLockPowerInteractable_Functional" in ids)

print("DOOR", json.dumps(call_unreal("get_actor_property", {
    "actor": "DoorLock_VestibuleToAtrium",
    "property": "bIsInteractable",
}), indent=2))

labels = [
    "Admin_Block2_Reception_Chair",
    "Admin_Block2_Reception_Mug",
    "Admin_Block2_Security_Chair",
    "Admin_Block2_Security_Mug",
    "Admin_Block2_Security_Headset",
    "Admin_Brand_PreparedImmunity",
    "Admin_Brand_VisitExpected",
]
counts = Counter()
for origin, radius in (
    ([1450, -150, 110], 500),
    ([2680, -400, 110], 500),
    ([450, 380, 210], 250),
    ([1450, 495, 250], 200),
    ([1000, 0, 100], 200),
):
    data = (call_unreal("list_actors_near", {"origin": origin, "radius": radius, "max": 50}).get("data") or {})
    for actor in data.get("actors") or []:
        label = actor.get("label") or ""
        if label in labels or label == "DoorLock_VestibuleToAtrium":
            counts[label] += 1
            print("FOUND", label, actor.get("location"), actor.get("class"))

print("COUNTS", dict(counts))
print("MISSING", [label for label in labels if counts[label] == 0])
print("DUPES", {k: v for k, v in counts.items() if v > 1})
