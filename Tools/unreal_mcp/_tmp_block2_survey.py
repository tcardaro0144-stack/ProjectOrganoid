import json
import sys

sys.path.insert(0, r"C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Tools\unreal_mcp")
from server import call_unreal


def dump(title, result, limit=4000):
    print("===", title, "===", flush=True)
    print(json.dumps(result, indent=2)[:limit], flush=True)


dump("STATE", call_unreal("get_editor_state"))
dump("PIE", call_unreal("get_pie_state"))

for label in [
    "Admin_Terminal_Reception",
    "Admin_Terminal_Security",
    "Admin_Terminal_Records",
    "Admin_FacilityHologram",
    "Admin_SectorDisplay",
    "Admin_SectorController",
    "Admin_S1_Reception_Desk",
    "Admin_RoomTrigger_Reception",
    "Admin_RoomTrigger_Hub",
    "Admin_RoomTrigger_Security",
    "Admin_RoomTrigger_Records",
    "Checkpoint_ReceptionAtrium",
    "BP_AdminAccessDoor",
    "DoorLock_VestibuleToAtrium",
    "Pickup_AdminKeycard",
    "Pickup_ResearchWingKeycard",
    "DataPad_VisitorLog",
    "DataPad_ShiftRoster",
    "DataPad_LockdownAuthorization",
]:
    dump("ACTOR " + label, call_unreal("get_actor", {"name": label}), 2500)

for actor, props in [
    ("Admin_Terminal_Reception", ["TerminalID", "Title", "TerminalType", "bInitiallyPowered", "bIsPowered", "bHasActivated", "bOneShot", "bIsInteractable", "InteractionPrompt", "InteractionRange"]),
    ("Admin_Terminal_Security", ["TerminalID", "Title", "TerminalType", "bInitiallyPowered", "bIsPowered", "bHasActivated", "bOneShot", "InteractionPrompt"]),
    ("Admin_Terminal_Records", ["TerminalID", "Title", "TerminalType", "bInitiallyPowered", "bIsPowered", "bHasActivated", "bOneShot", "InteractionPrompt"]),
    ("Admin_FacilityHologram", ["bAdminOnline", "bNeuroOnline", "bCryoOnline", "bComputeOnline", "bReactorOnline"]),
]:
    for prop in props:
        dump("PROP %s.%s" % (actor, prop), call_unreal("get_actor_property", {"actor": actor, "property": prop}), 800)

for origin, radius, name in [
    ([200, 0, 80], 700, "VESTIBULE"),
    ([1200, 0, 80], 700, "RECEPTION"),
    ([2400, 0, 80], 800, "HUB"),
    ([2680, -400, 80], 700, "SECURITY"),
    ([2680, 400, 80], 700, "RECORDS"),
]:
    dump("NEAR " + name, call_unreal("list_actors_near", {"origin": origin, "radius": radius, "max": 80}), 6000)

dump("ENUM", call_unreal("get_user_defined_enum", {
    "path": "/Game/ProjectOrganoid/Environment/Admin/Blueprints/E_AdminTerminalType",
    "blueprint_path": "/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminTerminal",
}))
dump("BP_TERMINAL", call_unreal("find_blueprint", {"path": "/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminTerminal"}))
dump("BP_MEMBERS", call_unreal("get_blueprint_members", {"path": "/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminTerminal"}), 4000)
dump("BP_HOLO", call_unreal("find_blueprint", {"path": "/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminFacilityHologram"}))
dump("BP_HOLO_MEMBERS", call_unreal("get_blueprint_members", {"path": "/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminFacilityHologram"}), 3000)
