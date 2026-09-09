# ProjectOrganoid — READ-ONLY restoration audit diagnostic.
# DIAGNOSTIC ONLY. Does not spawn, move, delete, rename, save, or import
# any build script. Does not begin Section 13.
#
# Intended map:
#   /Game/Maps/Epitope/SL_Epitope_Admin
# If the active package is anything else, abort with ZERO writes.
#
# Run manually:
#   File -> Execute Python Script...
#   C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Content\Python\diagnose_restoration_audit_admin.py

import unreal

REQUIRED_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
MENU_PACKAGE = "/Game/Maps/Lvl_MainMenu"
CUBE_HALF = 50.0

SECTION_GROUPS = (
    ("S1 Vestibule/Reception", ("Admin_S1_",), ()),
    ("S5 Hub", ("Admin_Hub_", "Admin_S5_"), ()),
    ("S6 Security", ("Admin_SecurityOffice_", "Admin_S6_"), ("Terminal_AdminSecurity",)),
    ("S7 Records", ("Admin_RecordsArchives_", "Admin_S7_"), ("Admin_RecordsArchives_Block",)),
    ("S8 Conference", ("Admin_ConferenceRoom_",), ()),
    ("S9 Director Suite", ("Admin_DirectorSuite_",), ()),
    ("S10 Operations", ("Admin_Operations_",), ()),
    ("S11 Transit", ("Admin_Transit_",), ()),
    ("S12 Service Corridor", ("Admin_ServiceCorridor_",), ()),
    ("S13 FROZEN (must be 0)", ("Admin_S13_",), ()),
)

CAMERA_ARM = 320.0
TACTICAL_RADIUS = 800.0


def _log(msg):
    unreal.log(msg)


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


def _actor_label(actor):
    actor_label = _safe_call(actor, "get_actor_label")
    if actor_label:
        return _safe_str(actor_label)
    return _safe_str(_safe_call(actor, "get_name"))


def _matches_group(label, prefixes, exact):
    if label in exact:
        return True
    for prefix in prefixes:
        if label.startswith(prefix):
            return True
    return False


def _aabb(actor):
    loc = _safe_call(actor, "get_actor_location")
    scale = _safe_call(actor, "get_actor_scale3d")
    if loc is None or scale is None:
        return None
    hx = abs(scale.x) * CUBE_HALF
    hy = abs(scale.y) * CUBE_HALF
    hz = abs(scale.z) * CUBE_HALF
    return (
        (loc.x - hx, loc.y - hy, loc.z - hz),
        (loc.x + hx, loc.y + hy, loc.z + hz),
    )


def _span(aabb):
    mn, mx = aabb
    return (mx[0] - mn[0], mx[1] - mn[1], mx[2] - mn[2])


_log("=== RESTORATION AUDIT DIAGNOSTIC (READ-ONLY) ===")
_log("ZERO writes. ZERO deletes. ZERO saves. Section 13 not implemented.")

world = _get_editor_world()
pkg = _world_package(world)
_log("active package=%s" % pkg)
if pkg != REQUIRED_PACKAGE:
    _log("=== RESTORATION AUDIT DIAGNOSTIC ABORTED — WRONG MAP ===")
    _log("Expected: %s" % REQUIRED_PACKAGE)
    raise RuntimeError("diagnostic aborted: package '%s'" % pkg)

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = actor_sub.get_all_level_actors() if actor_sub else []
_log("level actor count=%d" % len(all_actors))

by_label = {}
for actor in all_actors:
    label = _actor_label(actor)
    by_label.setdefault(label, []).append(actor)

for name, prefixes, exact in SECTION_GROUPS:
    labels = []
    for label, actors in sorted(by_label.items()):
        if _matches_group(label, prefixes, exact):
            labels.append("%s x%d" % (label, len(actors)))
    _log("--- %s count=%d ---" % (name, len(labels)))
    for entry in labels:
        _log("  %s" % entry)

pawn_classes = ("ProjectOrganoidCharacter", "DefaultPawn", "SpectatorPawn")
_log("--- CAMERA / PAWN SNAPSHOT ---")
for actor in all_actors:
    cls = _safe_str(_safe_call(actor, "get_class"))
    label = _actor_label(actor)
    if any(token in cls or token in label for token in pawn_classes):
        loc = _safe_call(actor, "get_actor_location")
        _log("pawn label=%s class=%s loc=%s" % (
            label, cls, loc
        ))

_log("--- TACTICAL / CAMERA FIT FLAGS ---")
_log("Assumed C++ camera boom arm=%.0f  tactical sphere radius=%.0f" % (
    CAMERA_ARM, TACTICAL_RADIUS
))
_log("Rooms whose interior Y or X span is below 2x tactical radius (1600) will clip the sphere.")
_log("Rooms whose interior span is below camera arm (320) will bury the follow camera in walls.")

key_floors = (
    "Admin_Hub_Floor",
    "Admin_Operations_Floor",
    "Admin_Transit_Floor",
    "Admin_ServiceCorridor_Floor",
    "Admin_ConferenceRoom_Floor",
)
for floor_label in key_floors:
    actors = by_label.get(floor_label, [])
    if not actors:
        _log("MISSING floor: %s" % floor_label)
        continue
    aabb = _aabb(actors[0])
    if not aabb:
        _log("no AABB: %s" % floor_label)
        continue
    sx, sy, sz = _span(aabb)
    sphere_clip = "CLIP" if min(sx, sy) < (TACTICAL_RADIUS * 2.0) else "ok"
    camera_clip = "TIGHT" if min(sx, sy) < (CAMERA_ARM * 2.0) else "ok"
    _log("%s span X=%.1f Y=%.1f tactical=%s camera=%s" % (
        floor_label, sx, sy, sphere_clip, camera_clip
    ))

_log("Admin_S13_* count=%d (must remain 0)" % len([
    label for label in by_label if label.startswith("Admin_S13_")
]))
_log("=== RESTORATION AUDIT DIAGNOSTIC COMPLETE ===")
_log("No actors were modified. No save occurred.")
