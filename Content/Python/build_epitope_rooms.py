# ProjectOrganoid — Epitope room blockout + interaction pass (UE Editor Python)
#
# Run in Unreal:
#   Tools -> Execute Python Script... -> Content/Python/build_epitope_rooms.py
# Or headless:
#   UnrealEditor-Cmd.exe ProjectOrganoid.uproject -run=pythonscript
#       -script="<abs path with forward slashes>/build_epitope_rooms.py" -unattended -nosplash
#
# Authors room geometry and interaction design into the five streaming partitions
# created by build_epitope_spine.py. The spine is not touched except to reposition
# the player start into Admin's atrium.
#
# Every region shares one floor plan so the streaming, power, and security systems
# get exercised identically in each; what differs is what is in the rooms. Rooms sit
# on the partition floor plate already authored by the spine builder.
#
#   +Y  ---------------------------------------------
#       |   NW room     |    NE room    |           |
#       |---------------+---------------|  Entry    |
#       |          Main corridor        |  hall     |  -> tower bridge at X=+2900
#       |---------------+---------------|           |
#       |   SW room     |    SE room    |           |
#   -Y  ---------------------------------------------
#       -X                                        +X
#
# Re-runnable: every generated actor is destroyed and rebuilt on each run.

import unreal


PARTITION_DIR = "/Game/Maps/Epitope"
SPINE_MAP = "/Game/Maps/Lvl_Epitope"
HOST_BP_DIR = "/Game/Hosts"
HOST_BP_NAME = "BP_OrganoidHost"
HOST_BP_PATH = f"{HOST_BP_DIR}/{HOST_BP_NAME}"
ITEM_KEYCARD_PATH = "/Game/Data/Items/DA_Item_AdminKeycard"
ITEM_SOT_PATH = "/Game/Data/Items/DA_Item_SOT"
DIALOGUE_SURVIVOR_PATH = "/Game/Data/Dialogue/DA_Dialogue_IncineratorSurvivor"
HOST_MESH_PATH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"

CUBE_MESH = "/Engine/BasicShapes/Cube.Cube"
HOST_BASE_CLASS = "/Script/ProjectOrganoid.ProjectOrganoidHostBase"

FLOOR_DROP = 1200.0
WALL_HEIGHT = 400.0
WALL_THICKNESS = 40.0
DOOR_WIDTH = 500.0

# Shared floor plan, in partition-local XY. The plate authored by the spine is
# 6000 x 6000, so everything here stays inside +/-2900.
ROOMS = {
    "entry":    {"x0": 1000.0,  "x1": 2900.0,  "y0": -2900.0, "y1": 2900.0},
    "corridor": {"x0": -2900.0, "x1": 1000.0,  "y0": -400.0,  "y1": 400.0},
    "nw":       {"x0": -2900.0, "x1": -1000.0, "y0": 400.0,   "y1": 2900.0},
    "ne":       {"x0": -1000.0, "x1": 1000.0,  "y0": 400.0,   "y1": 2900.0},
    "sw":       {"x0": -2900.0, "x1": -1000.0, "y0": -2900.0, "y1": -400.0},
    "se":       {"x0": -1000.0, "x1": 1000.0,  "y0": -2900.0, "y1": -400.0},
}


def floor_z(index):
    return -FLOOR_DROP * index


def stair_side(index):
    return 1.0 if index % 2 == 0 else -1.0


def tower_door_y(index):
    """The spine bridge meets the plate edge here, so the east wall opens here."""
    return -900.0 * stair_side(index)


# ----------------------------------------------------------------------------------
# Editor helpers
# ----------------------------------------------------------------------------------

def actors():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def levels():
    return unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def report(message):
    unreal.log_warning(f"[ROOMS] {message}")


def resolve_enum(enum_type_name, wanted):
    """UE's Python enum naming varies, so match on a normalised form."""
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


def to_text(value):
    try:
        return unreal.Text(value)
    except Exception:
        return value


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
    report(f"could not set {name} on {obj.get_name() if hasattr(obj, 'get_name') else obj}")
    return False


def set_enum(obj, name, enum_type_name, wanted):
    value = resolve_enum(enum_type_name, wanted)
    return set_prop(obj, name, value) if value is not None else False


def get_component(actor, name):
    snake = "".join(f"_{c.lower()}" if c.isupper() else c for c in name).lstrip("_")
    for candidate in (snake, name):
        try:
            return actor.get_editor_property(candidate)
        except Exception:
            continue
    return None


def resize_box(actor, component_name, extent):
    component = get_component(actor, component_name)
    if component:
        try:
            component.set_box_extent(extent, True)
            return True
        except Exception:
            pass
    report(f"could not resize {component_name} on {actor.get_actor_label()}")
    return False


# ----------------------------------------------------------------------------------
# Geometry
# ----------------------------------------------------------------------------------

def spawn_box(center, scale, name):
    mesh = unreal.EditorAssetLibrary.load_asset(CUBE_MESH)
    actor = actors().spawn_actor_from_class(unreal.StaticMeshActor, center)
    if not actor:
        return None
    actor.set_actor_label(name)
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.set_actor_scale3d(scale)
    return actor


def segments_with_openings(a0, a1, openings):
    """Subtract door gaps from a wall run, returning the solid spans that remain."""
    spans = [(a0, a1)]
    for center, width in openings:
        gap0, gap1 = center - width * 0.5, center + width * 0.5
        next_spans = []
        for s0, s1 in spans:
            if gap1 <= s0 or gap0 >= s1:
                next_spans.append((s0, s1))
                continue
            if s0 < gap0:
                next_spans.append((s0, gap0))
            if gap1 < s1:
                next_spans.append((gap1, s1))
        spans = next_spans
    return [(s0, s1) for s0, s1 in spans if s1 - s0 > 1.0]


def wall_along_y(x, y0, y1, z, name, openings=()):
    for i, (a0, a1) in enumerate(segments_with_openings(y0, y1, openings)):
        spawn_box(
            unreal.Vector(x, (a0 + a1) * 0.5, z + WALL_HEIGHT * 0.5),
            unreal.Vector(WALL_THICKNESS / 100.0, (a1 - a0) / 100.0, WALL_HEIGHT / 100.0),
            f"{name}_{i}",
        )


def wall_along_x(y, x0, x1, z, name, openings=()):
    for i, (a0, a1) in enumerate(segments_with_openings(x0, x1, openings)):
        spawn_box(
            unreal.Vector((a0 + a1) * 0.5, y, z + WALL_HEIGHT * 0.5),
            unreal.Vector((a1 - a0) / 100.0, WALL_THICKNESS / 100.0, WALL_HEIGHT / 100.0),
            f"{name}_{i}",
        )


def build_shell(index):
    """Perimeter plus the interior walls that carve the six spaces."""
    z = floor_z(index)
    door_y = tower_door_y(index)

    wall_along_x(2900.0, -2900.0, 2900.0, z, "Wall_Perimeter_North")
    wall_along_x(-2900.0, -2900.0, 2900.0, z, "Wall_Perimeter_South")
    wall_along_y(-2900.0, -2900.0, 2900.0, z, "Wall_Perimeter_West")
    wall_along_y(2900.0, -2900.0, 2900.0, z, "Wall_Perimeter_East",
                 openings=[(door_y, DOOR_WIDTH)])

    # Entry hall / interior divide, open at the corridor mouth.
    wall_along_y(1000.0, -2900.0, 2900.0, z, "Wall_EntryHall",
                 openings=[(0.0, DOOR_WIDTH)])

    # Corridor walls, each with a door into the two rooms behind them.
    wall_along_x(400.0, -2900.0, 1000.0, z, "Wall_Corridor_North",
                 openings=[(-2000.0, DOOR_WIDTH), (0.0, DOOR_WIDTH)])
    wall_along_x(-400.0, -2900.0, 1000.0, z, "Wall_Corridor_South",
                 openings=[(-2000.0, DOOR_WIDTH), (0.0, DOOR_WIDTH)])

    # Room dividers.
    wall_along_y(-1000.0, 400.0, 2900.0, z, "Wall_Divider_North")
    wall_along_y(-1000.0, -2900.0, -400.0, z, "Wall_Divider_South")

    # One light per room so PIE is readable without the spine key light.
    for room_key in ROOMS:
        location = spot(room_key, 0.5, 0.5, index, WALL_HEIGHT - 40.0)
        light_class = getattr(unreal, "PointLight", None) or getattr(unreal, "PointLightActor", None)
        light = actors().spawn_actor_from_class(light_class, location) if light_class else None
        if light:
            light.set_actor_label(f"Light_{room_key}")
            component = light.get_component_by_class(unreal.LightComponent)
            if component:
                component.set_intensity(4500.0)
                component.set_attenuation_radius(2200.0)


def spot(room_key, fx=0.5, fy=0.5, index=0, dz=0.0):
    """A point inside a room, given as a fraction of its footprint."""
    room = ROOMS[room_key]
    x = room["x0"] + (room["x1"] - room["x0"]) * fx
    y = room["y0"] + (room["y1"] - room["y0"]) * fy
    return unreal.Vector(x, y, floor_z(index) + dz)


# ----------------------------------------------------------------------------------
# Actor spawning
# ----------------------------------------------------------------------------------

def spawn_organoid(class_name, location, label, rotation=None):
    actor_class = unreal.load_class(None, f"/Script/ProjectOrganoid.{class_name}")
    if not actor_class:
        report(f"missing class {class_name} - is the C++ module compiled?")
        return None

    rotation = rotation or unreal.Rotator(0.0, 0.0, 0.0)
    actor = actors().spawn_actor_from_class(actor_class, location, rotation)
    if actor:
        actor.set_actor_label(label)
    else:
        report(f"failed to spawn {class_name} as {label}")
    return actor


def make_log_entry(entry_id, title, body, author, category="Facility"):
    try:
        entry = unreal.ProjectOrganoidLogEntry()
    except Exception:
        report("FProjectOrganoidLogEntry not exposed to Python")
        return None

    set_prop(entry, "EntryId", entry_id)
    set_prop(entry, "Title", to_text(title))
    set_prop(entry, "Body", to_text(body))
    set_prop(entry, "Author", to_text(author))
    set_prop(entry, "Category", category)
    return entry


def place_data_pad(location, label, entry_id, title, body, author, objective_event=None):
    pad = spawn_organoid("ProjectOrganoidDataPad", location, label)
    if not pad:
        return None

    entry = make_log_entry(entry_id, title, body, author)
    if entry:
        set_prop(pad, "LogEntry", entry)
    if objective_event:
        set_prop(pad, "ObjectiveEventId", objective_event)
    return pad


def place_hazard(location, label, hazard_type, extent, region_tag, dps=8.0,
                 toxicity=5.0, intensity=1.0):
    zone = spawn_organoid("ProjectOrganoidHazardZone", location, label)
    if not zone:
        return None

    set_enum(zone, "HazardType", "ProjectOrganoidHazardType", hazard_type)
    set_enum(zone, "AssociatedSubLevelTag", "ProjectOrganoidSubLevelTag", region_tag)
    set_prop(zone, "DamagePerSecond", dps)
    set_prop(zone, "ToxicityPerSecond", toxicity)
    set_prop(zone, "HazardIntensity", intensity)
    resize_box(zone, "HazardVolume", extent)
    return zone


def place_ambience(location, label, zone_id, display, extent, priority=0,
                   occlusion=1.0, room_tone_volume=0.35):
    zone = spawn_organoid("ProjectOrganoidAmbienceZone", location, label)
    if not zone:
        return None

    set_prop(zone, "ZoneId", zone_id)
    set_prop(zone, "DisplayName", to_text(display))
    set_prop(zone, "Priority", priority)
    set_prop(zone, "OcclusionStrengthBias", occlusion)
    set_prop(zone, "RoomToneVolume", room_tone_volume)
    resize_box(zone, "ZoneVolume", extent)
    return zone


def place_checkpoint(location, label, checkpoint_id, display, on_overlap=True):
    point = spawn_organoid("ProjectOrganoidCheckpoint", location, label)
    if not point:
        return None

    set_prop(point, "CheckpointId", checkpoint_id)
    set_prop(point, "CheckpointDisplayName", to_text(display))
    set_prop(point, "bSaveOnOverlapEnter", on_overlap)
    return point


def place_terminal(location, label, terminal_id, sector, mini_game="NodeMatch",
                   blackout_disables=True, single_use=True, password=None):
    terminal = spawn_organoid("ProjectOrganoidTerminal", location, label)
    if not terminal:
        return None

    set_prop(terminal, "TerminalId", terminal_id)
    set_enum(terminal, "PowerSector", "ProjectOrganoidPowerSector", sector)
    set_prop(terminal, "bDisableDuringBlackout", blackout_disables)
    set_prop(terminal, "bSingleUse", single_use)

    try:
        config = unreal.ProjectOrganoidHackingSessionConfig()
        set_enum(config, "MiniGame", "ProjectOrganoidHackingMiniGame", mini_game)
        if password:
            set_prop(config, "TargetPassword", password)
        set_prop(terminal, "HackingConfig", config)
    except Exception:
        report("FProjectOrganoidHackingSessionConfig not exposed; terminal left at defaults")

    return terminal


def place_gate(location, label, gate_id, tier, sector, lockdown_group):
    gate = spawn_organoid("ProjectOrganoidSecurityGate", location, label)
    if not gate:
        return None

    set_prop(gate, "GateId", gate_id)
    set_prop(gate, "LockdownGroupId", lockdown_group)
    set_enum(gate, "RequiredSecurityTier", "ProjectOrganoidSecurityTier", tier)
    set_enum(gate, "PowerSector", "ProjectOrganoidPowerSector", sector)
    return gate


def place_scannable(location, label, display, entry_id, title, body, author):
    scannable = spawn_organoid("ProjectOrganoidScannableActor", location, label)
    if not scannable:
        return None

    set_prop(scannable, "ScanDisplayName", to_text(display))
    entry = make_log_entry(entry_id, title, body, author, category="Scan")
    if entry:
        set_prop(scannable, "LoreEntry", entry)
    return scannable


def load_data_asset(path):
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        report(f"data asset missing (run build_epitope_data.py): {path}")
        return None
    return unreal.EditorAssetLibrary.load_asset(path)


def place_pickup(location, label, item_path, quantity=1, event_id=None):
    item = load_data_asset(item_path)
    pickup = spawn_organoid("ProjectOrganoidItemPickup", location, label)
    if not pickup:
        return None
    if item:
        set_prop(pickup, "ItemData", item)
    set_prop(pickup, "Quantity", quantity)
    if event_id:
        set_prop(pickup, "PickupObjectiveEventId", event_id)
    return pickup


def place_host(location, label):
    host_class = None
    try:
        host_class = unreal.EditorAssetLibrary.load_blueprint_class(HOST_BP_PATH)
    except Exception:
        host_class = None

    if not host_class:
        report(f"host blueprint missing at {HOST_BP_PATH}; skipping {label}")
        return None

    host = actors().spawn_actor_from_class(host_class, location)
    if host:
        host.set_actor_label(label)
    return host


# ----------------------------------------------------------------------------------
# Host blueprint (the C++ base is Abstract, so nothing can place it directly)
# ----------------------------------------------------------------------------------

def ensure_host_blueprint():
    if unreal.EditorAssetLibrary.does_asset_exist(HOST_BP_PATH):
        report(f"host blueprint present: {HOST_BP_PATH}")
        return True

    parent = unreal.load_class(None, HOST_BASE_CLASS)
    if not parent:
        report(f"could not load {HOST_BASE_CLASS}")
        return False

    if not unreal.EditorAssetLibrary.does_directory_exist(HOST_BP_DIR):
        unreal.EditorAssetLibrary.make_directory(HOST_BP_DIR)

    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent)

    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        HOST_BP_NAME, HOST_BP_DIR, unreal.Blueprint, factory
    )
    if not asset:
        report("failed to create host blueprint")
        return False

    unreal.EditorAssetLibrary.save_asset(HOST_BP_PATH)
    report(f"created host blueprint: {HOST_BP_PATH} (assign a skeletal mesh to make it visible)")
    return True


# ----------------------------------------------------------------------------------
# Region content
# ----------------------------------------------------------------------------------

def populate_admin(i):
    """Onboarding. Every verb taught with no lethal pressure and no hosts."""
    place_ambience(spot("corridor", 0.5, 0.5, i, 200.0), "Ambience_ReceptionAtrium",
                   "Admin_Atrium", "Reception Atrium",
                   unreal.Vector(2000.0, 500.0, 300.0), priority=1, occlusion=0.6)
    place_ambience(spot("nw", 0.5, 0.5, i, 200.0), "Ambience_HepaPlenum",
                   "Admin_Plenum", "HEPA Plenum",
                   unreal.Vector(950.0, 1250.0, 300.0), priority=2, occlusion=1.6,
                   room_tone_volume=0.12)

    place_checkpoint(spot("corridor", 0.35, 0.5, i, 60.0), "Checkpoint_ReceptionAtrium",
                     "Checkpoint_Admin_Atrium", "Reception Atrium")

    # Decon corridor: first hazard, telegraphed and survivable.
    place_hazard(spot("ne", 0.5, 0.4, i, 200.0), "Hazard_DeconUVC", "UVCRadiation",
                 unreal.Vector(900.0, 700.0, 250.0), "SubLevel1_Admin",
                 dps=6.0, toxicity=2.0)

    # Tier 1 door out of the vestibule, opened by the keycard hidden in the plenum.
    door = spawn_organoid("ProjectOrganoidDoorLock",
                          unreal.Vector(1000.0, 0.0, floor_z(i) + 100.0),
                          "DoorLock_VestibuleToAtrium")
    if door:
        set_enum(door, "RequiredSecurityTier", "ProjectOrganoidSecurityTier", "Level1_Admin")
        set_enum(door, "PowerSector", "ProjectOrganoidPowerSector", "Admin")
        set_prop(door, "bConsumeKeycardOnUnlock", True)
        set_prop(door, "InteractionPrompt", to_text("Use Keycard - Atrium Airlock"))

    terminal = place_terminal(spot("sw", 0.5, 0.6, i, 100.0), "Terminal_AdminSecurity",
                              "Terminal_AdminSecurity", "Admin", mini_game="NodeMatch")
    if terminal and door:
        set_prop(terminal, "LinkedDoorLock", door)

    upgrade = spawn_organoid("ProjectOrganoidUpgradeTerminal",
                             spot("se", 0.5, 0.5, i, 100.0), "SterlingTerminal_FieldOffice")
    if upgrade:
        set_prop(upgrade, "InteractionPrompt", to_text("Access Dr. Sterling's Terminal"))

    place_pickup(spot("nw", 0.35, 0.35, i, 80.0), "Pickup_AdminKeycard", ITEM_KEYCARD_PATH)
    place_pickup(spot("se", 0.7, 0.35, i, 80.0), "Pickup_SOT_FieldOffice", ITEM_SOT_PATH, quantity=4)

    place_data_pad(
        spot("entry", 0.4, 0.62, i, 90.0), "DataPad_LockdownAuthorization",
        "Pad_Admin_Authorization", "Lockdown Authorization 44-C",
        "Containment authorized facility-wide. Countersigned and filed. Note the date "
        "against the incident report: this authorization precedes the event it responds to "
        "by three days.",
        "Facility Oversight")
    place_data_pad(
        spot("sw", 0.3, 0.35, i, 90.0), "DataPad_ShiftRoster",
        "Pad_Admin_Roster", "Shift Roster - BSL-4",
        "The neuro-genetics team is struck through in pen. Nobody removed them from the "
        "system, so the badge readers still expect them.",
        "Facility Operations")
    place_data_pad(
        spot("se", 0.32, 0.68, i, 90.0), "DataPad_VisitorLog",
        "Pad_Admin_VisitorLog", "Visitor Log - Current Cycle",
        "One scheduled arrival: A. Vance, bio-hazard audit. The request originated from an "
        "internal terminal, not from the regulator.",
        "Reception Desk")


def populate_neuro(i):
    """First combat region. Hosts, toxicity pressure, and weak-point targeting."""
    place_ambience(spot("corridor", 0.5, 0.5, i, 200.0), "Ambience_GowningCorridor",
                   "Neuro_Gowning", "Gowning Corridor",
                   unreal.Vector(2000.0, 500.0, 300.0), priority=1)
    place_checkpoint(spot("entry", 0.5, 0.5, i, 60.0), "Checkpoint_NeuroAirlock",
                     "Checkpoint_Neuro_Airlock", "Gowning Airlock")

    place_hazard(spot("sw", 0.5, 0.5, i, 200.0), "Hazard_ScrubberLeak", "ToxicGas",
                 unreal.Vector(900.0, 1250.0, 250.0), "SubLevel2_NeuroGenetics",
                 dps=7.0, toxicity=11.0, intensity=1.2)

    # Matrix hall carries the region's scan density.
    for n, fx in enumerate((0.25, 0.5, 0.75)):
        place_scannable(
            spot("nw", fx, 0.6, i, 120.0), f"Scannable_OrganoidMatrix_{n + 1}",
            f"Organoid Matrix Lattice {n + 1}",
            f"Scan_Neuro_Matrix_{n + 1}", f"Matrix Lattice {n + 1}",
            "Tissue is still developing along the silicon. The nutrient feed to this rack "
            "was cut weeks ago and the growth curve did not flatten.",
            "Suit Scanner")

    trap_volume = spawn_organoid("ProjectOrganoidCorridorTrapVolume",
                                 spot("corridor", 0.45, 0.5, i, 140.0),
                                 "CorridorTraps_GowningRing")
    if trap_volume:
        set_prop(trap_volume, "CorridorId", "Neuro_GowningRing")
        # Low count so traps read as accidents rather than level design.
        set_prop(trap_volume, "DesiredTrapCount", 2)
        resize_box(trap_volume, "CorridorBounds", unreal.Vector(1600.0, 340.0, 200.0))

    for n, (room, fx, fy) in enumerate((("ne", 0.3, 0.4), ("ne", 0.7, 0.7), ("nw", 0.5, 0.25))):
        place_host(spot(room, fx, fy, i, 100.0), f"Host_Neuro_{n + 1}")

    survivor = spawn_organoid("ProjectOrganoidDialogueNPC",
                              spot("se", 0.72, 0.72, i, 100.0), "NPC_IncineratorSurvivor")
    if survivor:
        set_prop(survivor, "InteractionPrompt", to_text("Talk — Incinerator Bay"))
        conversation = load_data_asset(DIALOGUE_SURVIVOR_PATH)
        if conversation:
            set_prop(survivor, "ConversationAsset", conversation)
        mesh = load_data_asset(HOST_MESH_PATH)
        skeletal = get_component(survivor, "MeshComponent")
        if mesh and skeletal:
            for setter in ("set_skeletal_mesh_asset", "set_skeletal_mesh"):
                if hasattr(skeletal, setter):
                    getattr(skeletal, setter)(mesh)
                    break

    place_data_pad(
        spot("se", 0.5, 0.5, i, 90.0), "DataPad_EthicsObjection",
        "Pad_Neuro_Ethics", "Objection - Filed 11/03",
        "Formal objection to continued matrix production. Auto-dismissed by workflow rule "
        "R-12: objections from below director grade close automatically after 48 hours.",
        "Dr. E. Sterling")
    place_data_pad(
        spot("se", 0.32, 0.3, i, 90.0), "DataPad_SpecimenBadge",
        "Pad_Neuro_Badge", "Recovered ID Badge",
        "Facility badge, laminate scorched. The photograph matches a name on the Admin "
        "shift roster. The specimens are staff.",
        "Unknown")


def populate_cryo(i):
    """Survival and visibility pressure. Highest damage, lowest toxicity."""
    place_ambience(spot("corridor", 0.5, 0.5, i, 200.0), "Ambience_CryoPitCatwalk",
                   "Cryo_Catwalk", "Catwalk Over Cryo Pit",
                   unreal.Vector(2000.0, 500.0, 300.0), priority=1,
                   occlusion=1.8, room_tone_volume=0.18)
    place_checkpoint(spot("entry", 0.5, 0.5, i, 60.0), "Checkpoint_FreightAirlock",
                     "Checkpoint_Cryo_Airlock", "Freight Airlock")

    place_hazard(spot("nw", 0.5, 0.5, i, 200.0), "Hazard_LN2Rupture", "LiquidN2Frost",
                 unreal.Vector(950.0, 1250.0, 250.0), "SubLevel3_Cryo",
                 dps=14.0, toxicity=0.0, intensity=1.3)

    # Freezer aisles punish running blind through the fog.
    for n, fy in enumerate((0.3, 0.5, 0.7)):
        plate = spawn_organoid("ProjectOrganoidPressurePlate",
                               spot("ne", 0.5, fy, i, 20.0), f"PressurePlate_FreezerAisle_{n + 1}")
        if plate:
            set_prop(plate, "CorridorId", "Cryo_FreezerStacks")
            set_enum(plate, "LinkedHazard", "ProjectOrganoidHazardType", "LiquidN2Frost")
            set_prop(plate, "TriggerDamage", 22.0)

    # Manifest office stays dark until Cryo sector power is restored.
    place_terminal(spot("sw", 0.5, 0.5, i, 100.0), "Terminal_CryoManifest",
                   "Terminal_CryoManifest", "Cryo", mini_game="PasswordDecrypt",
                   blackout_disables=True, password="LOTLIST")

    place_scannable(
        spot("se", 0.5, 0.5, i, 120.0), "Scannable_ThawRestraints",
        "Thaw Chamber Restraints",
        "Scan_Cryo_Restraints", "Opened Restraints",
        "Failure surfaces are on the inner face. Nothing forced this chamber from the "
        "corridor side.",
        "Suit Scanner")

    place_data_pad(
        spot("sw", 0.3, 0.3, i, 90.0), "DataPad_SpecimenManifest",
        "Pad_Cryo_Manifest", "Specimen Manifest - Lot Index",
        "Intake catalogued by lot number. Cross-referenced against the Admin roster, the "
        "lots resolve to people who were still clocking in.",
        "Cryogenic Storage")
    place_data_pad(
        spot("se", 0.3, 0.7, i, 90.0), "DataPad_ConsentForms",
        "Pad_Cryo_Consent", "Consent Packet - Batch 9",
        "Every signature is dated after the corresponding intake scan.",
        "Facility Oversight")
    place_data_pad(
        spot("nw", 0.25, 0.25, i, 90.0), "DataPad_SterlingCryoNote",
        "Pad_Cryo_SterlingNote", "Private Note - Not Filed",
        "I refused the cryo intake twice. I was overruled twice, by someone whose name does "
        "not appear on any document I am cleared to read.",
        "Dr. E. Sterling")


def populate_compute(i):
    """The puzzle region. Hacking and power routing carry the tension."""
    place_ambience(spot("corridor", 0.5, 0.5, i, 200.0), "Ambience_Crawlspace",
                   "Compute_Crawlspace", "Maintenance Crawlspace",
                   unreal.Vector(2000.0, 500.0, 300.0), priority=1, occlusion=1.4)
    place_checkpoint(spot("sw", 0.25, 0.5, i, 60.0), "Checkpoint_InterfaceChamber",
                     "Checkpoint_Compute_Interface", "Core Interface Chamber")

    # Laser grid exists only while the Compute sector is online.
    for n, fy in enumerate((0.35, 0.5, 0.65)):
        laser = spawn_organoid("ProjectOrganoidLaserTripwire",
                               spot("entry", 0.55, fy, i, 120.0), f"Laser_Antechamber_{n + 1}")
        if laser:
            set_prop(laser, "CorridorId", "Compute_Antechamber")
            set_prop(laser, "BeamLength", 900.0)
            power_aware = get_component(laser, "PowerAware")
            if power_aware:
                set_enum(power_aware, "PowerSector", "ProjectOrganoidPowerSector", "Compute")

    # Terminal chain: each hack wakes more of the facility.
    for n, fy in enumerate((0.3, 0.5, 0.7)):
        place_terminal(spot("sw", 0.6, fy, i, 100.0), f"Terminal_CoreInterface_{n + 1}",
                       f"Terminal_ComputeCore_{n + 1}", "Compute",
                       mini_game="NodeMatch" if n % 2 == 0 else "PasswordDecrypt",
                       password="SUBSTRATE")

    place_hazard(spot("nw", 0.5, 0.5, i, 200.0), "Hazard_CoolantLeak", "ToxicGas",
                 unreal.Vector(950.0, 1250.0, 250.0), "SubLevel4_Compute",
                 dps=4.0, toxicity=7.0, intensity=0.8)

    for n, fx in enumerate((0.3, 0.7)):
        place_scannable(
            spot("ne", fx, 0.55, i, 120.0), f"Scannable_NerveRack_{n + 1}",
            f"Bio-Neural Rack {n + 1}",
            f"Scan_Compute_Rack_{n + 1}", f"Rack Row {n + 1}",
            "Synthetic nerve tissue threaded through the backplane. It twitches on a cycle "
            "that does not match the cooling schedule.",
            "Suit Scanner")

    place_data_pad(
        spot("se", 0.5, 0.5, i, 90.0), "DataPad_AutonomousDecisionLog",
        "Pad_Compute_Decisions", "Decision Log - Autonomous",
        "Lockdown extended. Lockdown extended. Lockdown extended. Hourly, for eleven weeks, "
        "with no operator attached to any entry.",
        "Facility Control")
    place_data_pad(
        spot("sw", 0.25, 0.25, i, 90.0), "DataPad_SterlingConfession",
        "Pad_Compute_Confession", "For A. Vance",
        "You will have worked out by now that I did not lose control of the substrate. I "
        "handed it over. It has been unfailingly reasonable ever since, which is the part "
        "I cannot make anyone understand.",
        "Dr. E. Sterling",
        objective_event="Event_SterlingConfessionRead")


def populate_reactor(i):
    """Finale. Every system at maximum simultaneously."""
    place_ambience(spot("corridor", 0.5, 0.5, i, 200.0), "Ambience_CoolantBasin",
                   "Reactor_Basin", "Coolant Basin Rim",
                   unreal.Vector(2000.0, 500.0, 300.0), priority=1, room_tone_volume=0.5)
    place_checkpoint(spot("corridor", 0.75, 0.5, i, 60.0), "Checkpoint_BasinRim",
                     "Checkpoint_Reactor_Basin", "Coolant Basin Rim")

    place_hazard(spot("ne", 0.5, 0.35, i, 200.0), "Hazard_IncubatorUVC", "UVCRadiation",
                 unreal.Vector(950.0, 900.0, 250.0), "SubLevel5_Reactor",
                 dps=11.0, toxicity=4.0, intensity=1.2)
    place_hazard(spot("nw", 0.5, 0.5, i, 200.0), "Hazard_DecayGardens", "ToxicGas",
                 unreal.Vector(950.0, 1250.0, 250.0), "SubLevel5_Reactor",
                 dps=9.0, toxicity=13.0, intensity=1.3)

    control = place_terminal(spot("sw", 0.5, 0.5, i, 100.0), "Terminal_ControlSpine",
                             "Terminal_ReactorControlSpine", "Reactor",
                             mini_game="PasswordDecrypt", password="EPITAPH", single_use=False)
    if control:
        set_prop(control, "SuccessObjectiveEventId", "Event_ReactorControlUsed")

    place_host(spot("ne", 0.5, 0.7, i, 100.0), "Host_PrimaryIncubator_Boss")

    place_scannable(
        spot("ne", 0.3, 0.5, i, 120.0), "Scannable_PrimaryIncubator",
        "Primary Incubator",
        "Scan_Reactor_Incubator", "Primary Incubator",
        "The scanner will not settle on a classification. It alternates between tissue "
        "culture and active processor and reports both with full confidence.",
        "Suit Scanner")

    place_data_pad(
        spot("se", 0.5, 0.5, i, 90.0), "DataPad_GrantProposal",
        "Pad_Reactor_Grant", "Grant Proposal - Original Submission",
        "The stated objective is tissue scaffolding for burn recovery. Appendix F, which "
        "was never circulated, describes the actual programme.",
        "Epitope Research Council")
    place_data_pad(
        spot("sw", 0.28, 0.3, i, 90.0), "DataPad_SterlingFinalLog",
        "Pad_Reactor_FinalLog", "Final Log - E. Sterling",
        "Recorded eight days after the date on my own death certificate. I would call that "
        "an administrative error, except that everything else here has been filed correctly.",
        "Dr. E. Sterling")


REGIONS = [
    {"key": "Admin", "partition": "SL_Epitope_Admin", "populate": populate_admin},
    {"key": "NeuroGenetics", "partition": "SL_Epitope_NeuroGenetics", "populate": populate_neuro},
    {"key": "Cryo", "partition": "SL_Epitope_Cryo", "populate": populate_cryo},
    {"key": "Compute", "partition": "SL_Epitope_Compute", "populate": populate_compute},
    {"key": "Reactor", "partition": "SL_Epitope_Reactor", "populate": populate_reactor},
]


# ----------------------------------------------------------------------------------
# Build
# ----------------------------------------------------------------------------------

def clear_generated(keep_labels=()):
    """Partitions are entirely script-authored, so a clean slate is safe."""
    removed = 0
    for actor in actors().get_all_level_actors():
        if actor.get_actor_label() in keep_labels:
            continue
        class_name = actor.get_class().get_name()
        is_generated = (
            isinstance(actor, unreal.StaticMeshActor)
            or "ProjectOrganoid" in class_name
            or "Host" in actor.get_actor_label()
        )
        if is_generated:
            actors().destroy_actor(actor)
            removed += 1
    return removed


def build_region(region, index):
    path = f"{PARTITION_DIR}/{region['partition']}"
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        report(f"partition missing, run build_epitope_spine.py first: {path}")
        return False

    levels().load_level(path)
    removed = clear_generated()

    z = floor_z(index)
    spawn_box(
        unreal.Vector(0.0, 0.0, z),
        unreal.Vector(60.0, 60.0, 0.2),
        f"{region['key']}_FloorPlate",
    )

    build_shell(index)
    region["populate"](index)

    levels().save_current_level()
    report(f"{region['key']}: cleared {removed}, rebuilt shell + interaction pass at Z={z}")
    return True


def place_spine_gates():
    """Seam gates live on the persistent spine so an unlock survives a partition unload."""
    levels().load_level(SPINE_MAP)

    stale = [
        "Gate_ResearchWing",
        "Gate_VaultAntechamber",
        "Gate_ReactorAccessLift",
    ]
    for actor in list(actors().get_all_level_actors()):
        if actor.get_actor_label() in stale:
            actors().destroy_actor(actor)

    neuro_i, compute_i = 1, 3
    place_gate(
        unreal.Vector(2900.0, tower_door_y(neuro_i), floor_z(neuro_i) + 140.0),
        "Gate_ResearchWing", "Gate_Neuro_Research", "Level2_Lab",
        "NeuroGenetics", "Research",
    )
    place_gate(
        unreal.Vector(2900.0, tower_door_y(compute_i), floor_z(compute_i) + 140.0),
        "Gate_VaultAntechamber", "Gate_Compute_Antechamber", "Level4_Compute",
        "Compute", "Core",
    )
    place_gate(
        unreal.Vector(1950.0, -1800.0, floor_z(compute_i) + 140.0),
        "Gate_ReactorAccessLift", "Gate_Compute_ReactorLift", "Level5_Core",
        "Compute", "Core",
    )
    report("seam gates authored on the persistent spine")
    levels().save_current_level()


def move_player_start():
    """Avery should wake in the atrium, not standing in the decon corridor."""
    levels().load_level(SPINE_MAP)

    for actor in actors().get_all_level_actors():
        if isinstance(actor, unreal.PlayerStart):
            actor.set_actor_location(unreal.Vector(-1500.0, 0.0, 120.0), False, False)
            actor.set_actor_rotation(unreal.Rotator(0.0, 0.0, 0.0), False)
            actor.set_actor_label("PlayerStart_ReceptionAtrium")
            report("player start moved to the Admin reception atrium")
            break

    levels().save_current_level()


def build_epitope_rooms():
    report("=== Epitope room blockout ===")
    ensure_host_blueprint()

    built = 0
    for index, region in enumerate(REGIONS):
        if build_region(region, index):
            built += 1

    place_spine_gates()
    move_player_start()

    report(f"=== Complete: {built} / {len(REGIONS)} regions authored ===")


build_epitope_rooms()
