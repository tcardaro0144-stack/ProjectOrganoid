# RETIRED / UNSAFE — do not run this script.
# UE 5.8 UEditorLevelUtils::MoveActorsToLevel destroys the source actor (cut/paste).
# This script retained Python UObject wrappers (actor, matches, all_actors) and later
# crashed in PythonScriptPlugin GC (OnPreGarbageCollect -> PyGC_Collect).
# Use OrganoidAIBridge 0.2.4 native game-thread action move_actor_to_level instead.

raise SystemExit(
    "RETIRED: repair_admin_section_18_reception_ownership.py is unsafe. "
    "Use OrganoidAIBridge move_actor_to_level on the game thread."
)

# ProjectOrganoid — Section 18 Phase 1 ownership repair ONLY.
# Moves the existing Admin_Terminal_Reception into SL_Epitope_Admin.
# Does not rerun build_admin_section_18_terminal.py.
# Does not spawn, compile, PIE, Save All, or save any map.
# Does not touch BP_AdminAccessDoor.

import unreal

ADMIN_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
PERSISTENT_PACKAGE = "/Game/Maps/Lvl_Epitope"
INSTANCE_LABEL = "Admin_Terminal_Reception"
INSTANCE_NAME = "BP_AdminTerminal_C_0"
DOOR_LABEL = "BP_AdminAccessDoor"
EXPECTED_LOC = unreal.Vector(1450.0, -150.0, 110.0)
LOC_EPS = 0.51


def _log(msg):
    unreal.log("[S18-REPAIR] " + msg)


def _fail(msg):
    _log("=== SECTION 18 OWNERSHIP REPAIR ABORTED ===")
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
    if pkg == ADMIN_PACKAGE or pkg.endswith("/SL_Epitope_Admin"):
        return ADMIN_PACKAGE
    if pkg == PERSISTENT_PACKAGE or pkg.endswith("/Lvl_Epitope"):
        return PERSISTENT_PACKAGE
    return pkg


def _outermost_package(obj):
    outermost = _safe_call(obj, "get_outermost")
    candidates = [
        _package_from_path(_safe_call(outermost, "get_name") if outermost else None),
        _package_from_path(_safe_call(outermost, "get_path_name") if outermost else None),
        _package_from_path(_safe_call(obj, "get_path_name")),
    ]
    for candidate in candidates:
        classified = _classify_package(candidate)
        if classified in (ADMIN_PACKAGE, PERSISTENT_PACKAGE):
            return classified
        if classified != "cannot be determined" and classified.startswith("/Game/"):
            return classified
    return "cannot be determined"


def _actor_owner_package(actor):
    level = _safe_call(actor, "get_level")
    if level is None:
        level = _safe_call(actor, "get_outer")
    classified = _outermost_package(level) if level is not None else "cannot be determined"
    if classified == "cannot be determined":
        classified = _outermost_package(actor)
    return classified


def _actor_label(actor):
    label = _safe_call(actor, "get_actor_label")
    if label:
        return _safe_str(label)
    return _safe_str(_safe_call(actor, "get_name"))


def _loc_matches(actual, expected):
    return (
        abs(actual.x - expected.x) <= LOC_EPS
        and abs(actual.y - expected.y) <= LOC_EPS
        and abs(actual.z - expected.z) <= LOC_EPS
    )


def _get_editor_world():
    sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    if sub:
        world = _safe_call(sub, "get_editor_world")
        if world:
            return world
    return None


_log("=== SECTION 18 RECEPTION OWNERSHIP REPAIR BEGIN ===")
_log("NO builder rerun. NO second spawn. NO compile. NO PIE. NO map save. NO Access Door edits.")

world = _get_editor_world()
if not world:
    _fail("No editor world")
world_pkg = _outermost_package(world)
_log("editor world package=%s" % world_pkg)
if world_pkg != PERSISTENT_PACKAGE:
    _fail("Expected persistent %s current, got %s. ZERO moves." % (PERSISTENT_PACKAGE, world_pkg))

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = list(actor_sub.get_all_level_actors() if actor_sub else [])
matches = [a for a in all_actors if _actor_label(a) == INSTANCE_LABEL]
if len(matches) != 1:
    _fail("Admin_Terminal_Reception count=%d (need exactly 1). ZERO moves." % len(matches))

actor = matches[0]
actor_name = _safe_str(_safe_call(actor, "get_name"))
loc = actor.get_actor_location()
rot = actor.get_actor_rotation()
scale = actor.get_actor_scale3d()
owner_before = _actor_owner_package(actor)
_log("found label=%s name=%s loc=(%.3f, %.3f, %.3f) rot=(%.3f, %.3f, %.3f) scale=(%.3f, %.3f, %.3f) owner=%s path=%s" % (
    INSTANCE_LABEL,
    actor_name,
    loc.x, loc.y, loc.z,
    rot.pitch, rot.yaw, rot.roll,
    scale.x, scale.y, scale.z,
    owner_before,
    _safe_str(_safe_call(actor, "get_path_name")),
))
if actor_name != INSTANCE_NAME:
    _log("WARNING: instance name is %s (expected %s); continuing because label is unique" % (actor_name, INSTANCE_NAME))
if not _loc_matches(loc, EXPECTED_LOC):
    _fail("Transform is (%.3f, %.3f, %.3f), expected (1450, -150, 110). ZERO moves." % (loc.x, loc.y, loc.z))
if owner_before != PERSISTENT_PACKAGE:
    _fail("Actor owner is %s, expected persistent %s. ZERO moves." % (owner_before, PERSISTENT_PACKAGE))

door_actors = [a for a in all_actors if DOOR_LABEL in _actor_label(a)]
door_before = None
if len(door_actors) == 1:
    door_before = (
        door_actors[0].get_actor_location(),
        door_actors[0].get_actor_rotation(),
        door_actors[0].get_actor_scale3d(),
    )
    _log("door snapshot loc=%s" % door_before[0])
else:
    _log("WARNING: BP_AdminAccessDoor count=%d; continuing without treating as fatal" % len(door_actors))

streaming = unreal.GameplayStatics.get_streaming_level(world, ADMIN_PACKAGE)
if streaming is None:
    streaming = unreal.GameplayStatics.get_streaming_level(world, "SL_Epitope_Admin")
if streaming is None:
    _fail("GameplayStatics.get_streaming_level did not return SL_Epitope_Admin. ZERO moves.")
stream_name = _safe_str(
    _safe_call(streaming, "get_world_asset_package_f_name")
    or _safe_call(streaming, "get_world_asset_package_name")
    or _safe_call(streaming, "get_name")
)
_log("streaming object=%s package=%s" % (_safe_str(type(streaming).__name__), stream_name))
if "SL_Epitope_Admin" not in stream_name and "SL_Epitope_Admin" not in _safe_str(_safe_call(streaming, "get_path_name")):
    _fail("Streaming object is not Admin: %s. ZERO moves." % stream_name)

levels = list(unreal.EditorLevelUtils.get_levels(world) or [])
admin_levels = [lvl for lvl in levels if _outermost_package(lvl) == ADMIN_PACKAGE]
_log("get_levels count=%d admin_levels=%d" % (len(levels), len(admin_levels)))
if len(admin_levels) != 1:
    _fail("EditorLevelUtils.get_levels found %d Admin ULevel(s). ZERO moves." % len(admin_levels))
loaded = _safe_call(streaming, "get_loaded_level")
if loaded is not None and loaded != admin_levels[0]:
    _fail("Streaming loaded level does not match get_levels Admin ULevel. ZERO moves.")
_log("Admin ULevel confirmed package=%s" % _outermost_package(admin_levels[0]))

move_fn = unreal.EditorLevelUtils.move_actors_to_level
_log("move_actors_to_level live=%s" % _safe_str(move_fn))
moved_count = None
try:
    moved_count = move_fn([actor], streaming, False, False)
except TypeError:
    moved_count = move_fn([actor], streaming, False)
_log("move_actors_to_level returned=%s" % _safe_str(moved_count))
if not moved_count:
    _fail("move_actors_to_level moved 0 actors. Existing actor was left in place.")

all_actors_after = list(actor_sub.get_all_level_actors() if actor_sub else [])
matches_after = [a for a in all_actors_after if _actor_label(a) == INSTANCE_LABEL]
if len(matches_after) != 1:
    _fail("After move, Admin_Terminal_Reception count=%d (need exactly 1)." % len(matches_after))
moved = matches_after[0]
loc_after = moved.get_actor_location()
rot_after = moved.get_actor_rotation()
scale_after = moved.get_actor_scale3d()
owner_after = _actor_owner_package(moved)
_log("after move name=%s loc=(%.3f, %.3f, %.3f) rot=(%.3f, %.3f, %.3f) scale=(%.3f, %.3f, %.3f) owner=%s path=%s" % (
    _safe_str(_safe_call(moved, "get_name")),
    loc_after.x, loc_after.y, loc_after.z,
    rot_after.pitch, rot_after.yaw, rot_after.roll,
    scale_after.x, scale_after.y, scale_after.z,
    owner_after,
    _safe_str(_safe_call(moved, "get_path_name")),
))
if not _loc_matches(loc_after, EXPECTED_LOC):
    _fail("Transform changed after move to (%.3f, %.3f, %.3f)." % (loc_after.x, loc_after.y, loc_after.z))
if abs(rot_after.pitch) > 0.05 or abs(rot_after.yaw) > 0.05 or abs(rot_after.roll) > 0.05:
    _fail("Rotation changed after move.")
if abs(scale_after.x - 1.0) > 0.05 or abs(scale_after.y - 1.0) > 0.05 or abs(scale_after.z - 1.0) > 0.05:
    _fail("Scale changed after move.")
if owner_after != ADMIN_PACKAGE:
    _fail("Owning package after move is %s, expected %s." % (owner_after, ADMIN_PACKAGE))

if door_before and door_actors:
    door_now = None
    for candidate in all_actors_after:
        if DOOR_LABEL in _actor_label(candidate):
            door_now = candidate
            break
    if door_now:
        loc_now = door_now.get_actor_location()
        if abs(loc_now.x - door_before[0].x) > 0.05 or abs(loc_now.y - door_before[0].y) > 0.05 or abs(loc_now.z - door_before[0].z) > 0.05:
            _fail("BP_AdminAccessDoor moved during ownership repair")
        _log("Access Door unchanged loc=%s" % loc_now)

_log("map NOT saved. PIE not started. builder not rerun.")
_log("=== SECTION 18 RECEPTION OWNERSHIP REPAIR COMPLETE ===")
