# ProjectOrganoid — Section 14 Admin Sector Controller placement.
# Reuses Section 13 BP_AdminSectorController. Does not recreate S13 assets.
# Does not spawn room triggers. Does not touch S1–S12 geometry.
# Does not save Lvl_MainMenu. Does not import build_epitope_rooms.py.
#
# Requires compiled AProjectOrganoidAdminSectorController.
# Current editor map must be /Game/Maps/Epitope/SL_Epitope_Admin.
#
# Run:
#   File -> Execute Python Script...
#   Content/Python/build_admin_section_14_sector_controller.py

import unreal

REQUIRED_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
MENU_PACKAGE = "/Game/Maps/Lvl_MainMenu"
BP_PATH = "/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminSectorController"
PARENT_CLASS = "/Script/ProjectOrganoid.ProjectOrganoidAdminSectorController"
INSTANCE_LABEL = "Admin_SectorController"
LOCATION = (2280.0, 0.0, 200.0)
LOCATION_EPS = 1.0
TRANSFORM_EPS = 0.05


def _log(msg):
    unreal.log("[S14] " + msg)


def _fail(msg):
    _log("=== SECTION 14 ABORTED ===")
    _log(msg)
    raise RuntimeError(msg)


def _safe_call(obj, method_name, *args):
    if obj is None:
        return None
    method = getattr(obj, method_name, None)
    if not callable(method):
        return None
    try:
        return method(*args)
    except Exception:
        return None


def _safe_str(value, fallback="None"):
    if value is None:
        return fallback
    try:
        text = str(value)
        if text:
            return text
    except Exception:
        pass
    return fallback


def _package_from_path(path):
    cleaned = _safe_str(path, "")
    cleaned = cleaned.replace("\\", "/")
    if not cleaned or cleaned == "None":
        return ""
    if "." in cleaned:
        cleaned = cleaned.split(".", 1)[0]
    return cleaned


def _classify_package(pkg):
    if not pkg:
        return "cannot be determined"
    if pkg == REQUIRED_PACKAGE or pkg.endswith("/SL_Epitope_Admin"):
        return REQUIRED_PACKAGE
    if pkg == MENU_PACKAGE or pkg.endswith("/Lvl_MainMenu"):
        return MENU_PACKAGE
    return pkg


def _get_editor_world():
    sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    if sub:
        world = _safe_call(sub, "get_editor_world")
        if world:
            return world
    return None


def _world_package(world):
    if not world:
        return "cannot be determined"
    outermost = _safe_call(world, "get_outermost")
    candidates = [
        _package_from_path(_safe_call(outermost, "get_name") if outermost else None),
        _package_from_path(_safe_call(outermost, "get_path_name") if outermost else None),
        _package_from_path(_safe_call(world, "get_path_name")),
        _package_from_path(_safe_call(world, "get_name")),
    ]
    classified_any = "cannot be determined"
    for candidate in candidates:
        classified = _classify_package(candidate)
        if classified == REQUIRED_PACKAGE:
            return REQUIRED_PACKAGE
        if classified == MENU_PACKAGE:
            return MENU_PACKAGE
        if classified != "cannot be determined" and classified.startswith("/Game/"):
            classified_any = classified
    return classified_any


def _assert_admin_map(stage):
    world = _get_editor_world()
    pkg = _world_package(world)
    if pkg != REQUIRED_PACKAGE:
        _fail("WRONG MAP at %s. Expected %s, actual %s. Open SL_Epitope_Admin. ZERO writes." % (
            stage, REQUIRED_PACKAGE, pkg
        ))
    return world, pkg


def _actor_owner_package(actor):
    level = _safe_call(actor, "get_level")
    if level is None:
        level = _safe_call(actor, "get_outer")
    outermost_actor = _safe_call(actor, "get_outermost")
    outermost_level = _safe_call(level, "get_outermost")
    candidates = [
        _package_from_path(_safe_call(outermost_actor, "get_name") if outermost_actor else None),
        _package_from_path(_safe_call(outermost_actor, "get_path_name") if outermost_actor else None),
        _package_from_path(_safe_call(outermost_level, "get_name") if outermost_level else None),
        _package_from_path(_safe_call(outermost_level, "get_path_name") if outermost_level else None),
        _package_from_path(_safe_call(actor, "get_path_name")),
        _package_from_path(_safe_call(level, "get_path_name")),
    ]
    hits = []
    for candidate in candidates:
        classified = _classify_package(candidate)
        if classified == "cannot be determined":
            continue
        if classified not in hits:
            hits.append(classified)
    if len(hits) == 1:
        return hits[0]
    if hits:
        return "mixed:" + ",".join(hits)
    return "cannot be determined"


def _actor_label(actor):
    label = _safe_call(actor, "get_actor_label")
    if label:
        return _safe_str(label)
    return _safe_str(_safe_call(actor, "get_name"))


def _compile_blueprint(bp):
    for lib_name in ("KismetEditorLibrary", "BlueprintEditorLibrary"):
        lib = getattr(unreal, lib_name, None)
        method = getattr(lib, "compile_blueprint", None) if lib else None
        if callable(method):
            method(bp)
            return True
    return False


def _vec_close(vec, xyz, eps=LOCATION_EPS):
    return (
        abs(vec.x - xyz[0]) <= eps
        and abs(vec.y - xyz[1]) <= eps
        and abs(vec.z - xyz[2]) <= eps
    )


_log("=== SECTION 14 SECTOR CONTROLLER BEGIN ===")
_log("ZERO S1-S12 geometry edits. ZERO Admin_S13_* rooms. ZERO room triggers.")
_log("ZERO audio edits. ZERO build_epitope_rooms.py.")

world, pkg = _assert_admin_map("start")
_log("active package=%s" % pkg)

if not unreal.EditorAssetLibrary.does_asset_exist(BP_PATH):
    _fail("Section 13 asset missing: %s" % BP_PATH)
_log("reusing %s" % BP_PATH)

parent = unreal.load_class(None, PARENT_CLASS)
if not parent:
    _fail("C++ parent missing: %s  Compile ProjectOrganoidEditor first (no Live Coding)." % PARENT_CLASS)

bp = unreal.EditorAssetLibrary.load_asset(BP_PATH)
if not bp:
    _fail("Failed to load %s" % BP_PATH)

parent_ok = False
try:
    current_parent = bp.get_editor_property("parent_class")
    parent_ok = current_parent == parent
    _log("current parent=%s" % _safe_str(current_parent))
except Exception:
    parent_ok = False

if not parent_ok:
    reparent = getattr(unreal.BlueprintEditorLibrary, "reparent_blueprint", None)
    if not callable(reparent):
        _fail("BlueprintEditorLibrary.reparent_blueprint unavailable")
    reparent(bp, parent)
    _log("reparented BP_AdminSectorController -> ProjectOrganoidAdminSectorController")
else:
    _log("parent already ProjectOrganoidAdminSectorController")

compiled = _compile_blueprint(bp)
saved_bp = unreal.EditorAssetLibrary.save_asset(BP_PATH, only_if_is_dirty=False)
_log("blueprint compile_called=%s saved=%s" % (compiled, saved_bp))

bp_class = unreal.EditorAssetLibrary.load_blueprint_class(BP_PATH)
if not bp_class:
    _fail("Failed to load generated class for %s" % BP_PATH)

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = actor_sub.get_all_level_actors() if actor_sub else []

room_triggers = []
s13_rooms = []
existing_controllers = []
for actor in all_actors:
    label = _actor_label(actor)
    if label.startswith("Admin_S13_"):
        s13_rooms.append(label)
    if label.startswith("BP_AdminRoomTrigger") or label == "Admin_RoomTrigger":
        if "RoomTrigger" in label:
            room_triggers.append(label)
    cls_name = _safe_str(_safe_call(actor, "get_class"))
    if label == INSTANCE_LABEL or "AdminSectorController" in cls_name:
        if _actor_owner_package(actor) == REQUIRED_PACKAGE:
            existing_controllers.append(actor)

if s13_rooms:
    _fail("Admin_S13_* geometry present; Section 14 will not continue: %s" % ", ".join(s13_rooms))

placed_room_triggers = [
    a for a in all_actors
    if "AdminRoomTrigger" in _safe_str(_safe_call(a, "get_class"))
    or _actor_label(a).startswith("BP_AdminRoomTrigger")
]
if placed_room_triggers:
    _fail("BP_AdminRoomTrigger already placed; Section 15 is not authorized.")

if len(existing_controllers) > 1:
    _fail("Multiple Admin sector controllers already in Admin: %d" % len(existing_controllers))

if len(existing_controllers) == 1:
    actor = existing_controllers[0]
    actor.set_actor_label(INSTANCE_LABEL)
    actor.set_actor_location(unreal.Vector(*LOCATION), False, False)
    actor.set_actor_rotation(unreal.Rotator(0.0, 0.0, 0.0), False)
    actor.set_actor_scale3d(unreal.Vector(1.0, 1.0, 1.0))
    actor.set_actor_enable_collision(False)
    _log("reconfigured existing %s" % INSTANCE_LABEL)
    created = False
else:
    _assert_admin_map("pre-spawn")
    actor = actor_sub.spawn_actor_from_class(
        bp_class,
        unreal.Vector(*LOCATION),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    if not actor:
        _fail("Failed to spawn Admin_SectorController")
    actor.set_actor_label(INSTANCE_LABEL)
    actor.set_actor_scale3d(unreal.Vector(1.0, 1.0, 1.0))
    actor.set_actor_enable_collision(False)
    created = True
    _log("placed %s" % INSTANCE_LABEL)

owner = _actor_owner_package(actor)
if owner != REQUIRED_PACKAGE:
    _fail("Controller owner package is %s, expected %s. Actor not left in wrong map." % (
        owner, REQUIRED_PACKAGE
    ))

loc = actor.get_actor_location()
if not _vec_close(loc, LOCATION):
    _fail("Controller location %s not within %.1f of %s" % (_safe_str(loc), LOCATION_EPS, LOCATION))

actor.set_actor_enable_collision(False)

# Save Admin map only (current level already asserted as SL_Epitope_Admin).
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not level_sub:
    _fail("LevelEditorSubsystem unavailable; refusing to save")
saved_map = level_sub.save_current_level()
_log("saved current Admin map result=%s" % _safe_str(saved_map))

# Re-count
all_actors = actor_sub.get_all_level_actors()
controllers = [
    a for a in all_actors
    if _actor_label(a) == INSTANCE_LABEL and _actor_owner_package(a) == REQUIRED_PACKAGE
]
s13_count = len([a for a in all_actors if _actor_label(a).startswith("Admin_S13_")])
trigger_count = len([
    a for a in all_actors
    if "AdminRoomTrigger" in _safe_str(_safe_call(a, "get_class"))
])

if len(controllers) != 1:
    _fail("Expected 1 Admin_SectorController, found %d" % len(controllers))
if s13_count != 0:
    _fail("Admin_S13_* count=%d" % s13_count)
if trigger_count != 0:
    _fail("Room trigger count=%d" % trigger_count)

_log("instance count=1 loc=(%.3f, %.3f, %.3f) collision=%s created=%s" % (
    loc.x, loc.y, loc.z, _safe_str(_safe_call(actor, "get_actor_enable_collision")), created
))
_log("Admin_S13_* count=0")
_log("BP_AdminRoomTrigger placed count=0")
_log("=== SECTION 14 SECTOR CONTROLLER COMPLETE ===")
_log("Section 15 was not started.")
