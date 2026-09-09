# ProjectOrganoid — READ-ONLY live survey for Section 12 planning.
# Inspects SL_Epitope_Admin. Performs ZERO writes and ZERO saves.
# Do not import any Section 10/11/12 build script.
# Candidate envelopes are DERIVED from live S8–S11 actor AABBs, then
# tested for overlap. This script does not spawn a Service Corridor.

import unreal

REQUIRED_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
MENU_PACKAGE = "/Game/Maps/Lvl_MainMenu"
ADJACENT_EPS = 50.0
CUBE_HALF = 50.0


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


def _aabb_from_loc_scale(loc, scale):
    hx = abs(scale[0]) * CUBE_HALF
    hy = abs(scale[1]) * CUBE_HALF
    hz = abs(scale[2]) * CUBE_HALF
    return (
        (loc[0] - hx, loc[1] - hy, loc[2] - hz),
        (loc[0] + hx, loc[1] + hy, loc[2] + hz),
    )


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


def _y_overlap(a, b, eps=0.5):
    if not a or not b:
        return False
    return a[0][1] < b[1][1] - eps and a[1][1] > b[0][1] + eps


def _x_overlap(a, b, eps=0.5):
    if not a or not b:
        return False
    return a[0][0] < b[1][0] - eps and a[1][0] > b[0][0] + eps


def _x_near_span(a, wall, eps=ADJACENT_EPS):
    if not a or not wall:
        return False
    return a[0][0] < wall[1][0] + eps and a[1][0] > wall[0][0] - eps


def _y_near_span(a, wall, eps=ADJACENT_EPS):
    if not a or not wall:
        return False
    return a[0][1] < wall[1][1] + eps and a[1][1] > wall[0][1] - eps


def _collision_profile(actor):
    mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
    if not mesh:
        return "None"
    return _safe_str(_safe_call(mesh, "get_collision_profile_name"))


def _normalize_collision_enabled(value):
    text = _safe_str(value).upper().replace(" ", "")
    if "NO_COLLISION" in text or "NOCOLLISION" in text:
        return "NO_COLLISION"
    if "QUERY_AND_PHYSICS" in text or "QUERYANDPHYSICS" in text:
        return "QUERY_AND_PHYSICS"
    if "QUERY_ONLY" in text or "QUERYONLY" in text:
        return "QUERY_ONLY"
    if "PHYSICS_ONLY" in text or "PHYSICSONLY" in text:
        return "PHYSICS_ONLY"
    return text


def _collision_enabled(actor):
    mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
    if not mesh:
        return "None"
    value = _safe_call(mesh, "get_collision_enabled")
    if value is None:
        try:
            value = mesh.get_editor_property("collision_enabled")
        except Exception:
            value = None
    return _normalize_collision_enabled(value)


def _mesh_path(actor):
    mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
    if not mesh:
        return "None"
    try:
        static_mesh = mesh.get_editor_property("static_mesh")
    except Exception:
        static_mesh = None
    if not static_mesh:
        return "None"
    return _safe_str(_safe_call(static_mesh, "get_path_name"))


world = _get_editor_world()
pkg = _world_package(world)
if pkg != REQUIRED_PACKAGE:
    _log("=== SECTION 12 SURVEY ABORTED — WRONG MAP ===")
    _log("Expected: %s" % REQUIRED_PACKAGE)
    _log("Actual: %s" % pkg)
    _log("ZERO actor writes, deletes, or saves.")
    raise RuntimeError("diagnostic aborted: wrong map %s" % pkg)

_log("=== SECTION 12 LIVE SURVEY (READ-ONLY) ===")
_log("editor_world_package=%s" % pkg)
_log("zero writes: no spawn/destroy/move/rename/collision-change/save")
_log("do not import or run a Section 12 build script from this diagnostic")

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not actor_sub:
    raise RuntimeError("EditorActorSubsystem unavailable")

s6 = []
s7 = []
s8 = []
s9 = []
s10 = []
s11 = []
hub = []
perimeter = []
s12_existing = []
all_admin = []
by_label = {}

for actor in actor_sub.get_all_level_actors():
    if not actor:
        continue
    if _actor_owner_package(actor) != REQUIRED_PACKAGE:
        continue
    label = actor.get_actor_label()
    loc = actor.get_actor_location()
    rot = actor.get_actor_rotation()
    scale = actor.get_actor_scale3d()
    aabb = _aabb_from_actor(actor)
    row = {
        "actor": actor,
        "label": label,
        "loc": loc,
        "rot": rot,
        "scale": scale,
        "aabb": aabb,
        "owner": _actor_owner_package(actor),
        "collision": "%s/%s" % (_collision_profile(actor), _collision_enabled(actor)),
        "mesh": _mesh_path(actor),
    }
    all_admin.append(row)
    by_label.setdefault(label, []).append(row)
    if label.startswith("Admin_SecurityOffice_") or label.startswith("Admin_S6_") or label == "Terminal_AdminSecurity":
        s6.append(row)
    if label.startswith("Admin_RecordsArchives_") or label.startswith("Admin_S7_") or label == "Admin_RecordsArchives_Block":
        s7.append(row)
    if label.startswith("Admin_ConferenceRoom_"):
        s8.append(row)
    if label.startswith("Admin_DirectorSuite_"):
        s9.append(row)
    if label.startswith("Admin_Operations_"):
        s10.append(row)
    if label.startswith("Admin_Transit_") or label.startswith("Admin_S11_") or label.startswith("Admin_TransitControl_"):
        s11.append(row)
    if label.startswith("Admin_Hub_") or label in ("Admin_S5_Hub_Floor", "Admin_FloorPlate"):
        hub.append(row)
    if label.startswith("Wall_Perimeter_"):
        perimeter.append(row)
    if label.startswith("Admin_S12_") or label.startswith("Admin_ServiceCorridor"):
        s12_existing.append(row)


def _dump(tag, rows):
    _log("--- %s count=%d ---" % (tag, len(rows)))
    rows = sorted(rows, key=lambda r: r["label"])
    for row in rows:
        _log(
            "%s label=%s loc=%s rot=(%.6f, %.6f, %.6f) scale=%s aabb=%s owner=%s collision=%s mesh=%s"
            % (
                tag,
                row["label"],
                _fmt_vec(row["loc"]),
                row["rot"].pitch,
                row["rot"].yaw,
                row["rot"].roll,
                _fmt_vec(row["scale"]),
                _aabb_str(row["aabb"]),
                row["owner"],
                row["collision"],
                row["mesh"],
            )
        )


def _one(label):
    rows = by_label.get(label, [])
    if len(rows) != 1:
        _log("INTERFACE %s count=%d (expected 1)" % (label, len(rows)))
        return None
    return rows[0]


def _union_rows(rows):
    acc = None
    for row in rows:
        acc = _aabb_union(acc, row["aabb"])
    return acc


_dump("S6", s6)
_dump("S7", s7)
_dump("S8", s8)
_dump("S9", s9)
_dump("HUB", hub)
_dump("PERIMETER", perimeter)
_dump("S10", s10)
_dump("S11", s11)
_dump("S12_EXISTING", s12_existing)

s10_south = _one("Admin_Operations_Wall_South")
s10_north = _one("Admin_Operations_Wall_North")
s10_east_n = _one("Admin_Operations_Wall_East_North")
s10_east_s = _one("Admin_Operations_Wall_East_South")
s10_east_h = _one("Admin_Operations_Wall_East_Header")
s10_plug = by_label.get("Admin_Operations_ToTransit", [])
s11_south = _one("Admin_Transit_Wall_South")
s11_north = _one("Admin_Transit_Wall_North")
s11_east = _one("Admin_Transit_Wall_East")
s8_hub_wall = _one("Admin_ConferenceRoom_Wall_Hub")
s8_posx = _one("Admin_ConferenceRoom_Wall_PosX")
s9_north = _one("Admin_DirectorSuite_Wall_North")
s9_support = _one("Admin_DirectorSuite_Support_Floor")
s7_block = _one("Admin_RecordsArchives_Block")

_log("--- LIVE INTERFACE ACTORS ---")
for tag, row in (
    ("S10_SOUTH", s10_south),
    ("S10_NORTH", s10_north),
    ("S10_EAST_NORTH", s10_east_n),
    ("S10_EAST_SOUTH", s10_east_s),
    ("S10_EAST_HEADER", s10_east_h),
    ("S11_SOUTH", s11_south),
    ("S11_NORTH", s11_north),
    ("S11_EAST", s11_east),
    ("S8_WALL_HUB", s8_hub_wall),
    ("S8_WALL_POSX", s8_posx),
    ("S9_WALL_NORTH", s9_north),
    ("S7_BLOCK", s7_block),
):
    if row is None:
        _log("%s MISSING" % tag)
        continue
    _log(
        "%s label=%s loc=%s rot=(%.6f, %.6f, %.6f) scale=%s aabb=%s owner=%s collision=%s"
        % (
            tag,
            row["label"],
            _fmt_vec(row["loc"]),
            row["rot"].pitch,
            row["rot"].yaw,
            row["rot"].roll,
            _fmt_vec(row["scale"]),
            _aabb_str(row["aabb"]),
            row["owner"],
            row["collision"],
        )
    )
_log("S10_TOTRANSIT_PLUG count=%d (expected 0 after Section 11)" % len(s10_plug))

s10_union = _union_rows(s10)
s11_union = _union_rows(s11)
s8_union = _union_rows(s8)
s9_union = _union_rows(s9)
_log("S10 union=%s" % _aabb_str(s10_union))
_log("S11 union=%s" % _aabb_str(s11_union))
_log("S8 union=%s" % _aabb_str(s8_union))
_log("S9 union=%s" % _aabb_str(s9_union))

# Candidate envelopes derived from live faces. These are survey boxes, not a build spec.
candidates = {}
if s10_south and s8_hub_wall:
    south_gap = (
        (
            max(s10_south["aabb"][0][0], s8_hub_wall["aabb"][0][0]),
            s8_hub_wall["aabb"][1][1],
            0.0,
        ),
        (
            s10_south["aabb"][1][0],
            s10_south["aabb"][0][1],
            480.0,
        ),
    )
    candidates["SOUTH_GAP_S8_NORTH_TO_S10_SOUTH"] = south_gap
    _log(
        "derived SOUTH_GAP_S8_NORTH_TO_S10_SOUTH %s  (Y from Conference north face to Operations south face; X clipped to overlap of those walls)"
        % _aabb_str(south_gap)
    )
    if south_gap[1][1] <= south_gap[0][1]:
        _log("SOUTH_GAP inverted or zero; live faces do not leave a +Y gap")

if s10_south and s11_south:
    s11_south_flank = (
        (
            s10_south["aabb"][1][0],
            min(s10_south["aabb"][0][1], s11_south["aabb"][0][1]),
            0.0,
        ),
        (
            s11_south["aabb"][1][0],
            s11_south["aabb"][0][1],
            480.0,
        ),
    )
    candidates["S11_SOUTH_FLANK"] = s11_south_flank
    _log("derived S11_SOUTH_FLANK %s" % _aabb_str(s11_south_flank))

if s10_north:
    north_flank = (
        (s10_north["aabb"][0][0], s10_north["aabb"][1][1], 0.0),
        (s10_north["aabb"][1][0], s10_north["aabb"][1][1] + 800.0, 480.0),
    )
    candidates["S10_NORTH_FLANK_800"] = north_flank
    _log("derived S10_NORTH_FLANK_800 %s  (800cm north of Operations north face; probe only)" % _aabb_str(north_flank))

if s11_east:
    east_of_s11 = (
        (s11_east["aabb"][1][0], s11_east["aabb"][0][1], 0.0),
        (s11_east["aabb"][1][0] + 400.0, s11_east["aabb"][1][1], 480.0),
    )
    candidates["EAST_OF_S11_400"] = east_of_s11
    _log("derived EAST_OF_S11_400 %s  (probe only; Transit east is intended sealed terminus)" % _aabb_str(east_of_s11))

if s10_south and s8_posx:
    east_of_conference = (
        (s8_posx["aabb"][1][0], s8_union[0][1] if s8_union else s8_posx["aabb"][0][1], 0.0),
        (s10_south["aabb"][1][0], s10_south["aabb"][0][1], 480.0),
    )
    candidates["EAST_OF_S8_TO_S10_SOUTH"] = east_of_conference
    _log("derived EAST_OF_S8_TO_S10_SOUTH %s" % _aabb_str(east_of_conference))

_log("--- CANDIDATE ENVELOPE OVERLAPS (Admin-owned actors) ---")
for name, box in candidates.items():
    hits = []
    for row in all_admin:
        if _aabb_overlaps(row["aabb"], box):
            hits.append(row)
    _log("%s overlap_count=%d" % (name, len(hits)))
    for row in sorted(hits, key=lambda r: r["label"]):
        _log(
            "%s HIT label=%s aabb=%s"
            % (name, row["label"], _aabb_str(row["aabb"]))
        )


def _adjacent_to_south_wall(row, wall):
    if wall is None:
        return False
    if row["label"] == wall["label"]:
        return False
    return _x_overlap(row["aabb"], wall["aabb"]) and _y_near_span(row["aabb"], wall["aabb"])


def _adjacent_to_north_wall(row, wall):
    if wall is None:
        return False
    if row["label"] == wall["label"]:
        return False
    return _x_overlap(row["aabb"], wall["aabb"]) and _y_near_span(row["aabb"], wall["aabb"])


def _adjacent_to_east_wall(row, wall):
    if wall is None:
        return False
    if row["label"] == wall["label"]:
        return False
    return _y_overlap(row["aabb"], wall["aabb"]) and _x_near_span(row["aabb"], wall["aabb"])


def _dump_adj(tag, wall, predicate, groups):
    _log("--- WALL_ADJACENT %s wall=%s ---" % (tag, wall["label"] if wall else "MISSING"))
    if wall is None:
        _log("%s wall MISSING; cannot compute adjacency" % tag)
        return
    _log("%s wall aabb=%s" % (tag, _aabb_str(wall["aabb"])))
    for group_name, rows in groups:
        hits = [row for row in rows if predicate(row, wall)]
        if not hits:
            _log("%s %s adjacent=MISSING" % (tag, group_name))
            continue
        union = _union_rows(hits)
        _log("%s %s adjacent_count=%d union=%s" % (tag, group_name, len(hits), _aabb_str(union)))
        for row in sorted(hits, key=lambda r: r["label"]):
            _log("%s %s label=%s aabb=%s" % (tag, group_name, row["label"], _aabb_str(row["aabb"])))


groups = (
    ("S6", s6),
    ("S7", s7),
    ("S8", s8),
    ("S9", s9),
    ("S11", s11),
    ("HUB", hub),
    ("PERIMETER", perimeter),
)

_dump_adj("S10_SOUTH", s10_south, _adjacent_to_south_wall, groups)
_dump_adj("S10_NORTH", s10_north, _adjacent_to_north_wall, groups)
_dump_adj("S11_SOUTH", s11_south, _adjacent_to_south_wall, groups + (("S10", s10),))
_dump_adj("S11_NORTH", s11_north, _adjacent_to_north_wall, groups + (("S10", s10),))
_dump_adj("S11_EAST", s11_east, _adjacent_to_east_wall, groups + (("S10", s10),))

_log("S12 existing actor count=%d (expected 0)" % len(s12_existing))
_log("S11 actor count=%d" % len(s11))
_log("S10 actor count=%d" % len(s10))
_log("diagnostic complete. no actors were spawned, moved, deleted, renamed, or saved.")
_log("=== SECTION 12 LIVE SURVEY COMPLETE ===")
