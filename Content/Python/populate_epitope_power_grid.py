# ProjectOrganoid — fill facility lights + Neuro/Cryo breakers without wiping rooms.
#
# Places the nine power-aware fixtures in each streaming partition (six sector
# lights + three emergency) and the two missing breakers. Existing labels are
# left alone; only gaps are spawned.
#
# Headless (forward slashes only):
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript
#       -script="<abs>/populate_epitope_power_grid.py"

import unreal


PARTITION_DIR = "/Game/Maps/Epitope"
FLOOR_DROP = 1200.0
WALL_HEIGHT = 400.0

ROOMS = {
    "entry":    {"x0": 1000.0,  "x1": 2900.0,  "y0": -2900.0, "y1": 2900.0},
    "corridor": {"x0": -2900.0, "x1": 1000.0,  "y0": -400.0,  "y1": 400.0},
    "nw":       {"x0": -2900.0, "x1": -1000.0, "y0": 400.0,   "y1": 2900.0},
    "ne":       {"x0": -1000.0, "x1": 1000.0,  "y0": 400.0,   "y1": 2900.0},
    "sw":       {"x0": -2900.0, "x1": -1000.0, "y0": -2900.0, "y1": -400.0},
    "se":       {"x0": -1000.0, "x1": 1000.0,  "y0": -2900.0, "y1": -400.0},
}

REGIONS = (
    {"key": "Admin", "partition": "SL_Epitope_Admin", "index": 0},
    {"key": "NeuroGenetics", "partition": "SL_Epitope_NeuroGenetics", "index": 1},
    {"key": "Cryo", "partition": "SL_Epitope_Cryo", "index": 2},
    {"key": "Compute", "partition": "SL_Epitope_Compute", "index": 3},
    {"key": "Reactor", "partition": "SL_Epitope_Reactor", "index": 4},
)


def report(message):
    unreal.log_warning(f"[POWER GRID] {message}")


def actors():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def levels():
    return unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def floor_z(index):
    return -FLOOR_DROP * index


def spot(room_key, fx, fy, index, dz):
    room = ROOMS[room_key]
    return unreal.Vector(
        room["x0"] + (room["x1"] - room["x0"]) * fx,
        room["y0"] + (room["y1"] - room["y0"]) * fy,
        floor_z(index) + dz,
    )


def resolve_enum(enum_type_name, wanted):
    enum_type = getattr(unreal, enum_type_name, None)
    if not enum_type:
        report(f"enum type not exposed: {enum_type_name}")
        return None
    target = wanted.replace("_", "").lower()
    for attr in dir(enum_type):
        if attr.startswith("_"):
            continue
        if attr.replace("_", "").lower() == target:
            return getattr(enum_type, attr)
    report(f"could not resolve {enum_type_name}.{wanted}")
    return None


def set_prop(obj, name, value):
    if obj is None:
        return False
    snake = "".join(f"_{c.lower()}" if c.isupper() else c for c in name).lstrip("_")
    for candidate in (snake, name):
        try:
            obj.set_editor_property(candidate, value)
            return True
        except Exception:
            continue
    report(f"could not set {name} on {obj.get_name()}")
    return False


def set_enum(obj, name, enum_type_name, wanted):
    value = resolve_enum(enum_type_name, wanted)
    return set_prop(obj, name, value) if value is not None else False


def to_text(value):
    try:
        return unreal.Text(value)
    except Exception:
        return value


def current_labels():
    return {actor.get_actor_label() for actor in actors().get_all_level_actors()}


def strip_legacy_point_lights():
    """The first room pass spawned always-on PointLights with the same Light_* labels."""
    removed = 0
    point_light_types = tuple(
        cls for cls in (
            getattr(unreal, "PointLight", None),
            getattr(unreal, "PointLightActor", None),
        ) if cls
    )
    if not point_light_types:
        return 0

    for actor in list(actors().get_all_level_actors()):
        label = actor.get_actor_label()
        if not (label.startswith("Light_") or label.startswith("EmergencyLight_")):
            continue
        if not isinstance(actor, point_light_types):
            continue
        actors().destroy_actor(actor)
        removed += 1
        report(f"  removed leftover PointLight {label}")
    return removed


def spawn_organoid(class_name, location, label):
    actor_class = unreal.load_class(None, f"/Script/ProjectOrganoid.{class_name}")
    if not actor_class:
        report(f"missing class {class_name} - is the C++ module compiled?")
        return None
    actor = actors().spawn_actor_from_class(actor_class, location)
    if actor:
        actor.set_actor_label(label)
    else:
        report(f"failed to spawn {class_name} as {label}")
    return actor


def expected_lights(index):
    fixtures = []
    for room_key in ROOMS:
        fixtures.append((
            f"Light_{room_key}",
            spot(room_key, 0.5, 0.5, index, WALL_HEIGHT - 40.0),
            "Sector",
        ))
    fixtures.extend((
        ("EmergencyLight_CorridorWest", spot("corridor", 0.25, 0.5, index, WALL_HEIGHT - 40.0), "Emergency"),
        ("EmergencyLight_CorridorEast", spot("corridor", 0.75, 0.5, index, WALL_HEIGHT - 40.0), "Emergency"),
        ("EmergencyLight_Entry", spot("entry", 0.5, 0.18, index, WALL_HEIGHT - 40.0), "Emergency"),
    ))
    return fixtures


def expected_panels(region_key, index):
    if region_key == "NeuroGenetics":
        return [(
            "PowerPanel_NeuroBackup",
            spot("se", 0.25, 0.25, index, 100.0),
            "NeuroGenetics",
            "Online",
            "Event_NeuroPowerRestored",
            "Restore Lab Power",
        )]
    if region_key == "Cryo":
        return [(
            "PowerPanel_CryoBackup",
            spot("entry", 0.35, 0.35, index, 100.0),
            "Cryo",
            "Emergency",
            "Event_CryoBackupEngaged",
            "Engage Cryo Backup",
        )]
    return []


def fill_region(region):
    path = f"{PARTITION_DIR}/{region['partition']}"
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        report(f"partition missing: {path}")
        return 0, 0

    levels().load_level(path)
    stripped = strip_legacy_point_lights()
    if stripped:
        report(f"{region['key']}: stripped {stripped} leftover PointLights")
    labels = current_labels()
    added_lights = 0
    added_panels = 0

    for label, location, role in expected_lights(region["index"]):
        if label in labels:
            continue
        light = spawn_organoid("ProjectOrganoidFacilityLight", location, label)
        if not light:
            continue
        set_enum(light, "PowerSector", "ProjectOrganoidPowerSector", region["key"])
        set_enum(light, "LightRole", "ProjectOrganoidFacilityLightRole", role)
        added_lights += 1
        report(f"  placed {label} ({role})")

    for label, location, sector, restored, event_id, prompt in expected_panels(region["key"], region["index"]):
        if label in labels:
            continue
        panel = spawn_organoid("ProjectOrganoidPowerPanel", location, label)
        if not panel:
            continue
        set_enum(panel, "PowerSector", "ProjectOrganoidPowerSector", sector)
        set_enum(panel, "RestoredState", "ProjectOrganoidPowerState", restored)
        set_prop(panel, "InteractionPrompt", to_text(prompt))
        set_prop(panel, "SuccessObjectiveEventId", event_id)
        added_panels += 1
        report(f"  placed {label}")

    labels = current_labels()
    light_labels = sorted(label for label, _, _ in expected_lights(region["index"]) if label in labels)
    panel_labels = sorted(label for label, *_ in expected_panels(region["key"], region["index"]) if label in labels)
    report(
        f"{region['key']}: lights {len(light_labels)}/9 ({', '.join(light_labels) or 'none'})"
    )
    if panel_labels or region["key"] in ("NeuroGenetics", "Cryo"):
        report(f"{region['key']}: panels {len(panel_labels)}/1 ({', '.join(panel_labels) or 'none'})")

    levels().save_current_level()
    return added_lights, added_panels


def populate():
    report("=== Fill facility lights and breakers ===")
    total_lights = 0
    total_panels = 0
    for region in REGIONS:
        lights, panels = fill_region(region)
        total_lights += lights
        total_panels += panels
    report(f"=== Complete: added {total_lights} lights, {total_panels} panels ===")


populate()
