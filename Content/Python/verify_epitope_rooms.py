# ProjectOrganoid — verify room blockout inside each streaming partition.
# Read-only. Safe to run any time.

import unreal


PARTITION_DIR = "/Game/Maps/Epitope"
SPINE_MAP = "/Game/Maps/Lvl_Epitope"

REGIONS = [
    "SL_Epitope_Admin",
    "SL_Epitope_NeuroGenetics",
    "SL_Epitope_Cryo",
    "SL_Epitope_Compute",
    "SL_Epitope_Reactor",
]

EXPECTED = {
    "SL_Epitope_Admin": (
        "ProjectOrganoidHazardZone",
        "ProjectOrganoidCheckpoint",
        "ProjectOrganoidDoorLock",
        "ProjectOrganoidTerminal",
        "ProjectOrganoidUpgradeTerminal",
        "ProjectOrganoidDataPad",
        "ProjectOrganoidAmbienceZone",
    ),
    "SL_Epitope_NeuroGenetics": (
        "ProjectOrganoidHazardZone",
        "ProjectOrganoidCheckpoint",
        "ProjectOrganoidScannableActor",
        "ProjectOrganoidCorridorTrapVolume",
        "ProjectOrganoidDataPad",
        "ProjectOrganoidDialogueNPC",
        "ProjectOrganoidAmbienceZone",
    ),
    "SL_Epitope_Cryo": (
        "ProjectOrganoidHazardZone",
        "ProjectOrganoidCheckpoint",
        "ProjectOrganoidPressurePlate",
        "ProjectOrganoidTerminal",
        "ProjectOrganoidDataPad",
        "ProjectOrganoidScannableActor",
        "ProjectOrganoidAmbienceZone",
    ),
    "SL_Epitope_Compute": (
        "ProjectOrganoidHazardZone",
        "ProjectOrganoidCheckpoint",
        "ProjectOrganoidLaserTripwire",
        "ProjectOrganoidTerminal",
        "ProjectOrganoidScannableActor",
        "ProjectOrganoidDataPad",
        "ProjectOrganoidAmbienceZone",
    ),
    "SL_Epitope_Reactor": (
        "ProjectOrganoidHazardZone",
        "ProjectOrganoidCheckpoint",
        "ProjectOrganoidTerminal",
        "ProjectOrganoidScannableActor",
        "ProjectOrganoidDataPad",
        "ProjectOrganoidAmbienceZone",
    ),
}

SPINE_GATES = (
    "Gate_ResearchWing",
    "Gate_VaultAntechamber",
    "Gate_ReactorAccessLift",
)


def report(line):
    unreal.log_warning(f"[ROOMS CHECK] {line}")


def class_name(actor):
    return actor.get_class().get_name()


def verify_level(path, expected_classes=()):
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.load_level(path)

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    counts = {}
    for actor in actors:
        name = class_name(actor)
        counts[name] = counts.get(name, 0) + 1

    report(f"{path}: {len(actors)} actors")
    missing = [cls for cls in expected_classes if counts.get(cls, 0) == 0]
    if missing:
        report(f"  MISSING {missing}")
    else:
        report(f"  expected classes present")

    walls = sum(1 for a in actors if a.get_actor_label().startswith("Wall_"))
    report(f"  walls={walls} meshes={counts.get('StaticMeshActor', 0)}")
    return missing


def verify():
    report("=== Epitope room check ===")
    missing_total = 0
    for region in REGIONS:
        missing = verify_level(f"{PARTITION_DIR}/{region}", EXPECTED[region])
        missing_total += len(missing)

    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.load_level(SPINE_MAP)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    labels = {a.get_actor_label() for a in actors}
    for gate in SPINE_GATES:
        report(f"spine {gate}: {'present' if gate in labels else 'MISSING'}")
        if gate not in labels:
            missing_total += 1

    starts = [a for a in actors if isinstance(a, unreal.PlayerStart)]
    if starts:
        report(f"player start {starts[0].get_actor_label()} at {starts[0].get_actor_location()}")
    else:
        report("player start MISSING")
        missing_total += 1

    report(f"=== Room check complete, missing={missing_total} ===")


verify()
