# ProjectOrganoid — Epitope persistent spine + streaming partitions (UE Editor Python)
#
# Run in Unreal:
#   Tools -> Execute Python Script... -> Content/Python/build_epitope_spine.py
# Or headless:
#   UnrealEditor-Cmd.exe ProjectOrganoid.uproject -run=pythonscript
#       -script="Content/Python/build_epitope_spine.py" -unattended -nosplash
#
# Creates the one continuous facility:
#   /Game/Maps/Lvl_Epitope                  persistent spine (always resident)
#   /Game/Maps/Epitope/SL_Epitope_Admin     five streaming partitions, registered
#   /Game/Maps/Epitope/SL_Epitope_NeuroGenetics
#   /Game/Maps/Epitope/SL_Epitope_Cryo
#   /Game/Maps/Epitope/SL_Epitope_Compute
#   /Game/Maps/Epitope/SL_Epitope_Reactor
#
# The spine owns everything that must survive a partition unloading: vertical
# circulation, seam buffers, the player start, and the streaming volumes that drive
# UProjectOrganoidLevelManagerSubsystem. Room geometry lives in the partitions.
#
# Requires the ProjectOrganoid C++ module compiled (ProjectOrganoidStreamingVolume).

import unreal


MAPS_DIR = "/Game/Maps"
PARTITION_DIR = "/Game/Maps/Epitope"
SPINE_MAP = f"{MAPS_DIR}/Lvl_Epitope"

STREAMING_VOLUME_CLASS = "/Script/ProjectOrganoid.ProjectOrganoidStreamingVolume"
GAMEPLAY_GAME_MODE = "/Script/ProjectOrganoid.ProjectOrganoidGameMode"
CUBE_MESH = "/Engine/BasicShapes/Cube.Cube"

# One building, floors stacked on a shared footprint. Descending Z is descending
# through the facility, which is the only spatial fact the fiction actually requires.
FLOOR_DROP = 1200.0
PLATE_HALF = 3000.0          # region floor plates are 6000 x 6000
REGION_XY_EXTENT = 3400.0    # region volume overhangs the plate so seams are covered
STAIR_X = 5000.0             # circulation tower, clear of every plate
STAIR_Y = 900.0

REGIONS = [
    {
        "key": "Admin",
        "partition": "SL_Epitope_Admin",
        "enum": ["SUB_LEVEL1_ADMIN", "SubLevel1_Admin", "SUBLEVEL1_ADMIN"],
        "tint": unreal.LinearColor(0.55, 0.60, 0.65, 1.0),
    },
    {
        "key": "NeuroGenetics",
        "partition": "SL_Epitope_NeuroGenetics",
        "enum": ["SUB_LEVEL2_NEURO_GENETICS", "SubLevel2_NeuroGenetics", "SUBLEVEL2_NEUROGENETICS"],
        "tint": unreal.LinearColor(0.35, 0.65, 0.45, 1.0),
    },
    {
        "key": "Cryo",
        "partition": "SL_Epitope_Cryo",
        "enum": ["SUB_LEVEL3_CRYO", "SubLevel3_Cryo", "SUBLEVEL3_CRYO"],
        "tint": unreal.LinearColor(0.45, 0.70, 0.85, 1.0),
    },
    {
        "key": "Compute",
        "partition": "SL_Epitope_Compute",
        "enum": ["SUB_LEVEL4_COMPUTE", "SubLevel4_Compute", "SUBLEVEL4_COMPUTE"],
        "tint": unreal.LinearColor(0.70, 0.55, 0.80, 1.0),
    },
    {
        "key": "Reactor",
        "partition": "SL_Epitope_Reactor",
        "enum": ["SUB_LEVEL5_REACTOR", "SubLevel5_Reactor", "SUBLEVEL5_REACTOR"],
        "tint": unreal.LinearColor(0.85, 0.55, 0.35, 1.0),
    },
]


def floor_z(index):
    return -FLOOR_DROP * index


def partition_path(region):
    return f"{PARTITION_DIR}/{region['partition']}"


def stair_side(index):
    """Ramps switch back each flight so one landing per floor serves both directions."""
    return 1.0 if index % 2 == 0 else -1.0


# ----------------------------------------------------------------------------------
# Editor plumbing
# ----------------------------------------------------------------------------------

def get_level_subsystem():
    return unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def get_actor_subsystem():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def get_editor_world():
    try:
        return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    except Exception:
        return unreal.EditorLevelLibrary.get_editor_world()


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def set_property(obj, candidates, value):
    """UE's Python bindings expose properties in snake_case, but not always predictably."""
    if not obj:
        return False
    for name in candidates:
        try:
            obj.set_editor_property(name, value)
            return True
        except Exception:
            continue
    unreal.log_warning(f"Could not set any of {candidates} on {obj}")
    return False


def set_enum_property(obj, candidates, enum_type_name, value_names):
    enum_type = getattr(unreal, enum_type_name, None)
    if not enum_type:
        unreal.log_warning(f"Enum not exposed to Python: {enum_type_name}")
        return False

    value = None
    for candidate in value_names:
        value = getattr(enum_type, candidate, None)
        if value is not None:
            break

    if value is None:
        unreal.log_warning(f"Could not resolve {enum_type_name} from {value_names}")
        return False

    return set_property(obj, candidates, value)


def spawn_box(location, scale, name, tint=None):
    mesh = unreal.EditorAssetLibrary.load_asset(CUBE_MESH)
    if not mesh:
        unreal.log_warning(f"Missing engine cube mesh: {CUBE_MESH}")
        return None

    actor = get_actor_subsystem().spawn_actor_from_class(unreal.StaticMeshActor, location)
    if not actor:
        return None

    actor.set_actor_label(name)
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.set_actor_scale3d(scale)

    if tint:
        try:
            actor.static_mesh_component.set_editor_property("wireframe_color_override", tint)
        except Exception:
            pass

    return actor


def spawn_ramp(location, scale, roll, name):
    mesh = unreal.EditorAssetLibrary.load_asset(CUBE_MESH)
    if not mesh:
        return None

    rotation = unreal.Rotator(roll=roll, pitch=0.0, yaw=0.0)
    actor = get_actor_subsystem().spawn_actor_from_class(unreal.StaticMeshActor, location, rotation)
    if not actor:
        return None

    actor.set_actor_label(name)
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.set_actor_scale3d(scale)
    return actor


def spawn_streaming_volume(location, extent, name, partitions, region_enum=None):
    actor_class = unreal.load_class(None, STREAMING_VOLUME_CLASS)
    if not actor_class:
        unreal.log_error(
            f"Could not load {STREAMING_VOLUME_CLASS}. Compile the C++ module before running this."
        )
        return None

    actor = get_actor_subsystem().spawn_actor_from_class(actor_class, location)
    if not actor:
        return None

    actor.set_actor_label(name)
    set_property(actor, ["RequestedStreamingLevels", "requested_streaming_levels"], partitions)

    if region_enum:
        set_enum_property(
            actor,
            ["RegionContextTag", "region_context_tag"],
            "ProjectOrganoidSubLevelTag",
            region_enum,
        )

    try:
        actor.trigger_volume.set_editor_property("box_extent", extent)
    except Exception as exc:
        unreal.log_warning(f"Could not resize {name}: {exc}")

    return actor


# ----------------------------------------------------------------------------------
# Partitions
# ----------------------------------------------------------------------------------

def build_partition(region, index):
    """Each partition gets a floor plate so residency is visible while walking."""
    path = partition_path(region)
    subsystem = get_level_subsystem()

    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.log(f"Partition exists, opening: {path}")
        subsystem.load_level(path)
    else:
        subsystem.new_level(path)

    z = floor_z(index)
    spawn_box(
        unreal.Vector(0.0, 0.0, z),
        unreal.Vector(PLATE_HALF * 2.0 / 100.0, PLATE_HALF * 2.0 / 100.0, 0.2),
        f"{region['key']}_FloorPlate",
        tint=region["tint"],
    )

    # A marker at the plate edge makes it obvious in PIE which partitions are resident.
    spawn_box(
        unreal.Vector(0.0, -PLATE_HALF + 200.0, z + 200.0),
        unreal.Vector(2.0, 0.5, 4.0),
        f"{region['key']}_Marker",
        tint=region["tint"],
    )

    subsystem.save_current_level()
    unreal.log(f"Built partition {region['partition']} at Z={z}")


# ----------------------------------------------------------------------------------
# Spine
# ----------------------------------------------------------------------------------

def build_spine_geometry():
    """Vertical circulation and seam buffers. Always resident, so keep it cheap."""
    for index, region in enumerate(REGIONS):
        z = floor_z(index)
        side = stair_side(index)
        landing_y = -STAIR_Y * side

        # Bridge from the region plate out to the circulation tower.
        spawn_box(
            unreal.Vector((PLATE_HALF + STAIR_X) * 0.5, landing_y, z),
            unreal.Vector((STAIR_X - PLATE_HALF + 800.0) / 100.0, 6.0, 0.2),
            f"Spine_Bridge_{region['key']}",
        )

        # Tower landing, shared by the arriving and departing flight.
        spawn_box(
            unreal.Vector(STAIR_X, landing_y, z),
            unreal.Vector(16.0, 10.0, 0.2),
            f"Spine_Landing_{region['key']}",
        )

    # Ramps between floors. Roughly 34 degrees, inside the default walkable slope.
    import math

    run = STAIR_Y * 2.0
    angle = math.degrees(math.atan2(FLOOR_DROP, run))
    length = math.sqrt(run * run + FLOOR_DROP * FLOOR_DROP)

    for index in range(len(REGIONS) - 1):
        upper = REGIONS[index]
        lower = REGIONS[index + 1]
        side = stair_side(index)

        spawn_ramp(
            unreal.Vector(STAIR_X, 0.0, floor_z(index) - FLOOR_DROP * 0.5),
            unreal.Vector(16.0, length / 100.0, 0.2),
            roll=-angle * side,
            name=f"Spine_Ramp_{upper['key']}_To_{lower['key']}",
        )


def build_spine_volumes():
    for index, region in enumerate(REGIONS):
        z = floor_z(index)

        # Region volume: holds its own partition resident and owns hazard context.
        spawn_streaming_volume(
            unreal.Vector(0.0, 0.0, z + 500.0),
            unreal.Vector(REGION_XY_EXTENT, REGION_XY_EXTENT, 550.0),
            f"StreamVolume_Region_{region['key']}",
            [region["partition"]],
            region_enum=region["enum"],
        )

    # Seam bands in the tower. Each requests both neighbours, so approaching from
    # either direction warms the far side before it can be seen.
    for index in range(len(REGIONS) - 1):
        upper = REGIONS[index]
        lower = REGIONS[index + 1]

        spawn_streaming_volume(
            unreal.Vector(STAIR_X, 0.0, floor_z(index) - FLOOR_DROP * 0.5),
            unreal.Vector(1200.0, 1400.0, 800.0),
            f"StreamBand_{upper['key']}_{lower['key']}",
            [upper["partition"], lower["partition"]],
        )


def build_spine_support():
    actor_subsystem = get_actor_subsystem()

    start = actor_subsystem.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0.0, 0.0, floor_z(0) + 120.0)
    )
    if start:
        start.set_actor_label("PlayerStart_AdminVestibule")

    key_light = actor_subsystem.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0.0, 0.0, 2000.0), unreal.Rotator(0.0, -50.0, 20.0)
    )
    if key_light:
        key_light.set_actor_label("Spine_KeyLight")

    sky = actor_subsystem.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0.0, 0.0, 1500.0))
    if sky:
        sky.set_actor_label("Spine_SkyLight")


def register_partitions(world):
    """Register each partition on the persistent level so ULevelStreaming exists for it."""
    registered = []

    for index, region in enumerate(REGIONS):
        path = partition_path(region)
        try:
            streaming = unreal.EditorLevelUtils.add_level_to_world(
                world, path, unreal.LevelStreamingDynamic
            )
        except Exception as exc:
            unreal.log_error(f"Could not register {path}: {exc}")
            continue

        if not streaming:
            unreal.log_error(f"add_level_to_world returned nothing for {path}")
            continue

        # Only Admin starts resident. Everything else waits for a volume to ask, and the
        # reconciler converges within UnloadGraceSeconds even if these flags do not stick.
        wanted = index == 0
        set_property(streaming, ["initially_loaded", "bInitiallyLoaded"], wanted)
        set_property(streaming, ["initially_visible", "bInitiallyVisible"], wanted)
        for setter in ("set_should_be_loaded", "set_should_be_visible"):
            try:
                getattr(streaming, setter)(wanted)
            except Exception:
                pass

        registered.append(region["partition"])
        unreal.log(f"Registered partition {region['partition']} (initially resident: {wanted})")

    return registered


def build_spine():
    subsystem = get_level_subsystem()

    if unreal.EditorAssetLibrary.does_asset_exist(SPINE_MAP):
        unreal.log(f"Spine exists, opening: {SPINE_MAP}")
        subsystem.load_level(SPINE_MAP)
    else:
        subsystem.new_level(SPINE_MAP)

    # Spawn spine content before registering partitions, so nothing lands in a sub-level.
    build_spine_support()
    build_spine_geometry()
    build_spine_volumes()

    world = get_editor_world()
    if not world:
        unreal.log_error("No editor world for the spine map")
        return []

    game_mode = unreal.load_class(None, GAMEPLAY_GAME_MODE)
    if game_mode:
        world.get_world_settings().set_editor_property("default_game_mode", game_mode)
    else:
        unreal.log_warning(f"Could not load {GAMEPLAY_GAME_MODE}")

    registered = register_partitions(world)

    subsystem.save_current_level()
    unreal.EditorAssetLibrary.save_directory(MAPS_DIR, only_if_is_dirty=True, recursive=True)
    return registered


def build_epitope_spine():
    unreal.log("=== Epitope spine generation ===")
    ensure_directory(MAPS_DIR)
    ensure_directory(PARTITION_DIR)

    for index, region in enumerate(REGIONS):
        build_partition(region, index)

    registered = build_spine()

    unreal.log("=== Epitope spine complete ===")
    unreal.log(f"Persistent spine: {SPINE_MAP}")
    unreal.log(f"Partitions registered: {len(registered)} / {len(REGIONS)}")
    unreal.log("Walk the tower ramps to watch partitions stream in and out.")


build_epitope_spine()
