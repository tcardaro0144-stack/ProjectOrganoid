# ProjectOrganoid — READ-ONLY live survey for Section 11 planning.
# Inspects SL_Epitope_Admin. Performs ZERO writes and ZERO saves.
# Do not import build_admin_section_10_operations.py or any Section 11 build script.

import unreal

REQUIRED_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
MENU_PACKAGE = "/Game/Maps/Lvl_MainMenu"
ADJACENT_X_EPS = 50.0
PROPOSED_S11_AABB = (
    (4530.0, -362.5, 0.0),
    (5480.0, 362.5, 460.0),
)
S10_EAST_FACE_X = 4530.0


def _log(msg):
    unreal.log(msg)


def _fmt_vec(v):
    return "(%.6f, %.6f, %.6f)" % (v.x, v.y, v.z)


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


def _aabb_from_actor(actor):
    origin, extent = actor.get_actor_bounds(False, False)
    mn = (origin.x - extent.x, origin.y - extent.y, origin.z - extent.z)
    mx = (origin.x + extent.x, origin.y + extent.y, origin.z + extent.z)
    return mn, mx


def _aabb_str(aabb):
    if not aabb:
        return "MISSING"
    mn, mx = aabb
    return "min=(%.6f, %.6f, %.6f) max=(%.6f, %.6f, %.6f)" % (
        mn[0], mn[1], mn[2], mx[0], mx[1], mx[2]
    )


def _aabb_overlaps(a, b, eps=0.5):
    if not a or not b:
        return False
    return (
        a[0][0] < b[1][0] - eps and a[1][0] > b[0][0] + eps
        and a[0][1] < b[1][1] - eps and a[1][1] > b[0][1] + eps
        and a[0][2] < b[1][2] - eps and a[1][2] > b[0][2] + eps
    )


def _aabb_union(a, b):
    if a is None:
        return b
    if b is None:
        return a
    return (
        (min(a[0][0], b[0][0]), min(a[0][1], b[0][1]), min(a[0][2], b[0][2])),
        (max(a[1][0], b[1][0]), max(a[1][1], b[1][1]), max(a[1][2], b[1][2])),
    )


world = _get_editor_world()
pkg = _world_package(world)
if pkg != REQUIRED_PACKAGE:
    _log("=== SECTION 11 SURVEY ABORTED — WRONG MAP ===")
    _log("Expected: %s" % REQUIRED_PACKAGE)
    _log("Actual: %s" % pkg)
    _log("ZERO actor writes, deletes, or saves.")
    raise RuntimeError("diagnostic aborted: wrong map %s" % pkg)

_log("=== SECTION 11 LIVE SURVEY (READ-ONLY) ===")
_log("editor_world_package=%s" % pkg)
_log("zero writes: no spawn/destroy/move/rename/collision-change/save")
_log("proposed S11 envelope for overlap test: %s" % _aabb_str(PROPOSED_S11_AABB))

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not actor_sub:
    raise RuntimeError("EditorActorSubsystem unavailable")

interface_labels = (
    "Admin_Operations_ToTransit",
    "Admin_Operations_Wall_East_North",
    "Admin_Operations_Wall_East_South",
    "Admin_Operations_Wall_East_Header",
    "Admin_Operations_Station_SectorAccess",
)
east_of_ops = []
proposed_hits = []
s6 = []
s7 = []
s8 = []
s9 = []
s10 = []
hub = []
perimeter = []
s11_existing = []
interface = []
union_east = None

for actor in actor_sub.get_all_level_actors():
    if not actor:
        continue
    if _actor_owner_package(actor) != REQUIRED_PACKAGE:
        continue
    label = actor.get_actor_label()
    loc = actor.get_actor_location()
    scale = actor.get_actor_scale3d()
    aabb = _aabb_from_actor(actor)
    row = (label, loc, scale, aabb)
    if label in interface_labels:
        interface.append(row)
    if label.startswith("Admin_Operations_"):
        s10.append(row)
    if label.startswith("Admin_DirectorSuite_"):
        s9.append(row)
    if label.startswith("Admin_ConferenceRoom_"):
        s8.append(row)
    if label.startswith("Admin_SecurityOffice_") or label.startswith("Admin_S6_") or label == "Terminal_AdminSecurity":
        s6.append(row)
    if label.startswith("Admin_RecordsArchives_") or label.startswith("Admin_S7_") or label == "Admin_RecordsArchives_Block":
        s7.append(row)
    if label in ("Admin_Hub_Floor", "Admin_S5_Hub_Floor", "Admin_Hub_Opening_Cut") or label.startswith("Admin_Hub_"):
        hub.append(row)
    if label.startswith("Wall_Perimeter_East_1"):
        perimeter.append(row)
    if label.startswith("Admin_Transit") or label.startswith("Admin_S11_") or label.startswith("Admin_TransitControl_"):
        s11_existing.append(row)
    if aabb[1][0] > S10_EAST_FACE_X - 1.0:
        east_of_ops.append(row)
        union_east = _aabb_union(union_east, aabb)
    if _aabb_overlaps(aabb, PROPOSED_S11_AABB) and not label.startswith("Admin_Operations_"):
        proposed_hits.append(row)

def _dump(tag, rows):
    _log("--- %s count=%d ---" % (tag, len(rows)))
    rows = sorted(rows, key=lambda r: r[0])
    for label, loc, scale, aabb in rows:
        _log(
            "%s label=%s loc=%s scale=%s aabb=%s"
            % (tag, label, _fmt_vec(loc), _fmt_vec(scale), _aabb_str(aabb))
        )

_dump("S6", s6)
_dump("S7", s7)
_dump("S8", s8)
_dump("S9", s9)
_dump("HUB", hub)
_dump("PERIMETER", perimeter)
_dump("S10", s10)
_dump("INTERFACE", interface)
_dump("S11_EXISTING", s11_existing)
_dump("EAST_OF_S10_FACE", east_of_ops)
_dump("NON_S10_IN_PROPOSED_S11", proposed_hits)
_log("east-of-S10-face union=%s" % _aabb_str(union_east))
_log("S11 existing actor count=%d (expected 0)" % len(s11_existing))
_log("non-S10 actors overlapping proposed S11 envelope=%d (expected 0)" % len(proposed_hits))
_log("diagnostic complete. no actors were spawned, moved, deleted, renamed, or saved.")
_log("=== SECTION 11 LIVE SURVEY COMPLETE ===")
