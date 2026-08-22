# Read-only check that Phase 3 data assets exist and are wired.

import unreal


ASSETS = {
    "/Game/Data/Items/DA_Item_AdminKeycard": "ProjectOrganoidItemData",
    "/Game/Data/Items/DA_Item_SOT": "ProjectOrganoidItemData",
    "/Game/Data/Missions/DA_Mission_TheAudit": "ProjectOrganoidObjectiveDataAsset",
    "/Game/Data/Missions/DA_Mission_TheProduction": "ProjectOrganoidObjectiveDataAsset",
    "/Game/Data/Missions/DA_Mission_TheHandover": "ProjectOrganoidObjectiveDataAsset",
    "/Game/Data/Missions/DA_Mission_TheConclusion": "ProjectOrganoidObjectiveDataAsset",
    "/Game/Data/Dialogue/DA_Dialogue_IncineratorSurvivor": "ProjectOrganoidDialogueDataAsset",
    "/Game/Hosts/BP_OrganoidHost": "Blueprint",
}


def report(line):
    unreal.log_warning(f"[DATA CHECK] {line}")


def verify():
    missing = 0
    for path, expected in ASSETS.items():
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            report(f"MISSING {path}")
            missing += 1
            continue
        asset = unreal.EditorAssetLibrary.load_asset(path)
        report(f"ok {path} ({asset.get_class().get_name() if asset else expected})")

    audit = unreal.EditorAssetLibrary.load_asset("/Game/Data/Missions/DA_Mission_TheAudit")
    if audit:
        try:
            tasks = list(audit.get_editor_property("tasks"))
            report(f"TheAudit tasks={len(tasks)}")
        except Exception:
            report("TheAudit tasks unreadable")

    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.load_level("/Game/Maps/Epitope/SL_Epitope_Admin")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    pickups = [a for a in actors if "ItemPickup" in a.get_class().get_name()]
    report(f"Admin pickups={len(pickups)}")
    if not pickups:
        missing += 1

    levels.load_level("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    survivors = [a for a in actors if a.get_actor_label() == "NPC_IncineratorSurvivor"]
    if survivors:
        try:
            conversation = survivors[0].get_editor_property("conversation_asset")
            report(f"survivor conversation={conversation}")
            if not conversation:
                missing += 1
        except Exception as exc:
            report(f"survivor conversation unreadable: {exc}")
            missing += 1
    else:
        report("survivor MISSING")
        missing += 1

    report(f"=== Data check complete, missing={missing} ===")


verify()
