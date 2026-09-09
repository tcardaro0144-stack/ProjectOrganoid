# ProjectOrganoid — READ-ONLY east-perimeter split recalculation for Section 10.
# Inspects live SL_Epitope_Admin geometry. Performs ZERO writes and ZERO saves.
# Do not import build_admin_section_10_operations.py (that module executes).

import unreal

REQUIRED_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
MENU_PACKAGE = "/Game/Maps/Lvl_MainMenu"
CUBE_HALF = 50.0
SPAN_EPS = 0.05
OVERLAP_EPS = 0.5
ADJACENT_X_EPS = 50.0

PERIMETER_EAST_LABEL = "Wall_Perimeter_East_1"
PERIMETER_SOUTH_LABEL = "Wall_Perimeter_East_1_South"
PERIMETER_NORTH_LABEL = "Wall_Perimeter_East_1_North"
PERIMETER_HEADER_LABEL = "Wall_Perimeter_East_1_Header"

S6_PREFIXES = ("Admin_SecurityOffice_", "Admin_S6_")
S6_EXACT = ("Terminal_AdminSecurity",)
S7_PREFIXES = ("Admin_RecordsArchives_", "Admin_S7_")
S7_EXACT = ("Admin_RecordsArchives_Block",)
HUB_FLOOR_LABELS = ("Admin_Hub_Floor", "Admin_S5_Hub_Floor")
HUB_OPENING_LABEL = "Admin_Hub_Opening_Cut"
S10_PREFIX = "Admin_Operations_"

INTENDED_DOOR_CENTER_Y = 0.0
INTENDED_DOOR_WIDTH = 260.0
INTENDED_DOOR_HEIGHT = 300.0
MIN_JAMB_LEN = 40.0
MIN_HEADER_H = 20.0


def _log(msg):
    unreal.log(msg)


def _fmt_vec(v):
    return "(%.6f, %.6f, %.6f)" % (v.x, v.y, v.z)


def _fmt_xyz(xyz):
    return "(%.6f, %.6f, %.6f)" % (xyz[0], xyz[1], xyz[2])


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


def _aabb_from_loc_scale(loc_xyz, scale_xyz):
    hx = abs(scale_xyz[0]) * CUBE_HALF
    hy = abs(scale_xyz[1]) * CUBE_HALF
    hz = abs(scale_xyz[2]) * CUBE_HALF
    mn = (loc_xyz[0] - hx, loc_xyz[1] - hy, loc_xyz[2] - hz)
    mx = (loc_xyz[0] + hx, loc_xyz[1] + hy, loc_xyz[2] + hz)
    return mn, mx


def _aabb_from_actor(actor):
    origin, extent = actor.get_actor_bounds(False, False)
    mn = (origin.x - extent.x, origin.y - extent.y, origin.z - extent.z)
    mx = (origin.x + extent.x, origin.y + extent.y, origin.z + extent.z)
    return mn, mx


def _aabb_union(a, b):
    if a is None:
        return b
    if b is None:
        return a
    return (
        (min(a[0][0], b[0][0]), min(a[0][1], b[0][1]), min(a[0][2], b[0][2])),
        (max(a[1][0], b[1][0]), max(a[1][1], b[1][1]), max(a[1][2], b[1][2])),
    )


def _aabb_overlaps(a, b, eps=OVERLAP_EPS):
    if not a or not b:
        return False
    return (
        a[0][0] < b[1][0] - eps and a[1][0] > b[0][0] + eps
        and a[0][1] < b[1][1] - eps and a[1][1] > b[0][1] + eps
        and a[0][2] < b[1][2] - eps and a[1][2] > b[0][2] + eps
    )


def _aabb_str(aabb):
    if not aabb:
        return "MISSING"
    mn, mx = aabb
    return "min=(%.6f, %.6f, %.6f) max=(%.6f, %.6f, %.6f)" % (
        mn[0], mn[1], mn[2], mx[0], mx[1], mx[2]
    )


def _y_span_str(aabb):
    if not aabb:
        return "MISSING"
    return "Y=%.6f to %.6f (span=%.6f)" % (aabb[0][1], aabb[1][1], aabb[1][1] - aabb[0][1])


def _z_span_str(aabb):
    if not aabb:
        return "MISSING"
    return "Z=%.6f to %.6f (span=%.6f)" % (aabb[0][2], aabb[1][2], aabb[1][2] - aabb[0][2])


def _covers_y(piece_y0, piece_y1, target_y0, target_y1, eps=SPAN_EPS):
    return (piece_y0 - target_y0) <= eps and (target_y1 - piece_y1) <= eps


def _clip_y_to_wall(aabb, wall_aabb):
    if not aabb or not wall_aabb:
        return None
    y0 = max(aabb[0][1], wall_aabb[0][1])
    y1 = min(aabb[1][1], wall_aabb[1][1])
    if y1 <= y0:
        return None
    return (
        (aabb[0][0], y0, aabb[0][2]),
        (aabb[1][0], y1, aabb[1][2]),
    )


def _is_s6(label):
    if label in S6_EXACT:
        return True
    return any(label.startswith(prefix) for prefix in S6_PREFIXES)


def _is_s7(label):
    if label in S7_EXACT:
        return True
    return any(label.startswith(prefix) for prefix in S7_PREFIXES)


def _is_hub_geom(label):
    return label in HUB_FLOOR_LABELS or label == HUB_OPENING_LABEL or label.startswith("Admin_Hub_") or label.startswith("Admin_S5_")


def _adjacent_to_east_wall(actor_aabb, wall_aabb):
    if not actor_aabb or not wall_aabb:
        return False
    y_overlap = actor_aabb[0][1] < wall_aabb[1][1] and actor_aabb[1][1] > wall_aabb[0][1]
    x_near = (
        actor_aabb[1][0] >= wall_aabb[0][0] - ADJACENT_X_EPS
        and actor_aabb[0][0] <= wall_aabb[1][0] + ADJACENT_X_EPS
    )
    return y_overlap and x_near


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


def _log_actor(tag, actor):
    loc = actor.get_actor_location()
    rot = actor.get_actor_rotation()
    scale = actor.get_actor_scale3d()
    loc_xyz = (loc.x, loc.y, loc.z)
    scale_xyz = (scale.x, scale.y, scale.z)
    bounds_aabb = _aabb_from_actor(actor)
    loc_scale_aabb = _aabb_from_loc_scale(loc_xyz, scale_xyz)
    _log("%s label=%s" % (tag, actor.get_actor_label()))
    _log("%s owner=%s" % (tag, _actor_owner_package(actor)))
    _log("%s loc=%s" % (tag, _fmt_vec(loc)))
    _log("%s rot=(%.6f, %.6f, %.6f)" % (tag, rot.pitch, rot.yaw, rot.roll))
    _log("%s scale=%s" % (tag, _fmt_vec(scale)))
    _log("%s actor_bounds AABB=%s" % (tag, _aabb_str(bounds_aabb)))
    _log("%s loc*scale AABB=%s" % (tag, _aabb_str(loc_scale_aabb)))
    _log("%s %s" % (tag, _y_span_str(bounds_aabb)))
    _log("%s %s" % (tag, _z_span_str(bounds_aabb)))
    _log("%s mesh=%s" % (tag, _mesh_path(actor)))
    _log("%s collision=%s/%s" % (tag, _collision_profile(actor), _collision_enabled(actor)))
    return {
        "label": actor.get_actor_label(),
        "actor": actor,
        "loc": loc_xyz,
        "scale": scale_xyz,
        "bounds": bounds_aabb,
        "loc_scale": loc_scale_aabb,
        "owner": _actor_owner_package(actor),
    }


def _union_bounds(rows):
    acc = None
    for row in rows:
        acc = _aabb_union(acc, row["bounds"])
    return acc


def _split_from_wall(wall_loc_scale_aabb, wall_loc, wall_scale, door_center_y, door_width, door_height):
    orig = wall_loc_scale_aabb
    door_y0 = door_center_y - door_width * 0.5
    door_y1 = door_center_y + door_width * 0.5
    south_y0 = orig[0][1]
    south_y1 = door_y0
    north_y0 = door_y1
    north_y1 = orig[1][1]
    header_z0 = door_height
    header_z1 = orig[1][2]
    south_len = south_y1 - south_y0
    north_len = north_y1 - north_y0
    header_h = header_z1 - header_z0
    south_aabb = (
        (orig[0][0], south_y0, orig[0][2]),
        (orig[1][0], south_y1, orig[1][2]),
    )
    north_aabb = (
        (orig[0][0], north_y0, orig[0][2]),
        (orig[1][0], north_y1, orig[1][2]),
    )
    header_aabb = (
        (orig[0][0], door_y0, header_z0),
        (orig[1][0], door_y1, header_z1),
    )
    door_aabb = (
        (orig[0][0], door_y0, orig[0][2]),
        (orig[1][0], door_y1, door_height),
    )
    south_spec = (
        PERIMETER_SOUTH_LABEL,
        (wall_loc[0], (south_y0 + south_y1) * 0.5, wall_loc[2]),
        (wall_scale[0], south_len / 100.0, wall_scale[2]),
    )
    north_spec = (
        PERIMETER_NORTH_LABEL,
        (wall_loc[0], (north_y0 + north_y1) * 0.5, wall_loc[2]),
        (wall_scale[0], north_len / 100.0, wall_scale[2]),
    )
    header_spec = (
        PERIMETER_HEADER_LABEL,
        (wall_loc[0], door_center_y, (header_z0 + header_z1) * 0.5),
        (wall_scale[0], door_width / 100.0, header_h / 100.0),
    )
    return {
        "door_center_y": door_center_y,
        "door_width": door_width,
        "door_height": door_height,
        "door_y0": door_y0,
        "door_y1": door_y1,
        "south_y0": south_y0,
        "south_y1": south_y1,
        "north_y0": north_y0,
        "north_y1": north_y1,
        "header_y0": door_y0,
        "header_y1": door_y1,
        "header_z0": header_z0,
        "header_z1": header_z1,
        "south_len": south_len,
        "north_len": north_len,
        "header_h": header_h,
        "south_aabb": south_aabb,
        "north_aabb": north_aabb,
        "header_aabb": header_aabb,
        "door_aabb": door_aabb,
        "south_spec": south_spec,
        "north_spec": north_spec,
        "header_spec": header_spec,
    }


def _current_check_errors(split, orig_aabb, s6_aabb, s7_aabb):
    errors = []
    door_y0 = split["door_y0"]
    door_y1 = split["door_y1"]
    expected_y0 = -INTENDED_DOOR_WIDTH * 0.5
    expected_y1 = INTENDED_DOOR_WIDTH * 0.5
    if abs(door_y0 - expected_y0) > SPAN_EPS or abs(door_y1 - expected_y1) > SPAN_EPS:
        errors.append("doorway Y=%.6f to %.6f expected=%.6f to %.6f" % (
            door_y0, door_y1, expected_y0, expected_y1
        ))
    if abs(split["south_y0"] - orig_aabb[0][1]) > SPAN_EPS or abs(split["north_y1"] - orig_aabb[1][1]) > SPAN_EPS:
        errors.append("south+north Y footprint != original wall")
    if abs(split["south_y1"] - door_y0) > SPAN_EPS:
        errors.append("south piece does not stop at doorway Y=%.6f" % door_y0)
    if abs(split["north_y0"] - door_y1) > SPAN_EPS:
        errors.append("north piece does not start at doorway Y=%.6f" % door_y1)
    if s6_aabb and not _covers_y(split["south_y0"], split["south_y1"], s6_aabb[0][1], s6_aabb[1][1]):
        errors.append(
            "south replacement does not cover Section 6 Y range (south Y=%.6f to %.6f, S6 Y=%.6f to %.6f)"
            % (split["south_y0"], split["south_y1"], s6_aabb[0][1], s6_aabb[1][1])
        )
    if s7_aabb and not _covers_y(split["north_y0"], split["north_y1"], s7_aabb[0][1], s7_aabb[1][1]):
        errors.append(
            "north replacement does not cover Section 7 Y range (north Y=%.6f to %.6f, S7 Y=%.6f to %.6f)"
            % (split["north_y0"], split["north_y1"], s7_aabb[0][1], s7_aabb[1][1])
        )
    if split["south_len"] < MIN_JAMB_LEN:
        errors.append("south jamb too short (%.6f)" % split["south_len"])
    if split["north_len"] < MIN_JAMB_LEN:
        errors.append("north jamb too short (%.6f)" % split["north_len"])
    if split["header_h"] < MIN_HEADER_H:
        errors.append("header too short (%.6f)" % split["header_h"])
    return errors


def _choose_door(orig_aabb, s6_cover, s7_cover):
    wall_y0 = orig_aabb[0][1]
    wall_y1 = orig_aabb[1][1]
    intended_y0 = INTENDED_DOOR_CENTER_Y - INTENDED_DOOR_WIDTH * 0.5
    intended_y1 = INTENDED_DOOR_CENTER_Y + INTENDED_DOOR_WIDTH * 0.5
    reasons = []
    intended_ok = True

    if intended_y0 < wall_y0 - SPAN_EPS or intended_y1 > wall_y1 + SPAN_EPS:
        intended_ok = False
        reasons.append("intended doorway is not fully inside original wall Y")

    if s6_cover:
        if wall_y0 - s6_cover[0][1] > SPAN_EPS:
            intended_ok = False
            reasons.append("S6 Y min=%.6f is south of wall Y min=%.6f; this wall cannot cover that S6 span" % (
                s6_cover[0][1], wall_y0
            ))
        if s6_cover[1][1] - intended_y0 > SPAN_EPS:
            intended_ok = False
            reasons.append("intended south jamb ends at Y=%.6f but S6 Y max=%.6f; doorway would expose Security" % (
                intended_y0, s6_cover[1][1]
            ))
    if s7_cover:
        if s7_cover[1][1] - wall_y1 > SPAN_EPS:
            intended_ok = False
            reasons.append("S7 Y max=%.6f is north of wall Y max=%.6f; this wall cannot cover that S7 span" % (
                s7_cover[1][1], wall_y1
            ))
        if intended_y1 - s7_cover[0][1] > SPAN_EPS:
            intended_ok = False
            reasons.append("intended north jamb starts at Y=%.6f but S7 Y min=%.6f; doorway would expose Records" % (
                intended_y1, s7_cover[0][1]
            ))

    if intended_ok:
        return INTENDED_DOOR_CENTER_Y, INTENDED_DOOR_WIDTH, True, ["intended doorway Y=-130 to +130 remains valid against wall-adjacent S6/S7"]

    gap0 = wall_y0
    gap1 = wall_y1
    if s6_cover:
        gap0 = max(gap0, s6_cover[1][1])
    if s7_cover:
        gap1 = min(gap1, s7_cover[0][1])
    gap_w = gap1 - gap0
    if gap_w + SPAN_EPS < INTENDED_DOOR_WIDTH:
        reasons.append("no %.3f-wide gap between protected S6 Y max and S7 Y min (gap=%.6f to %.6f width=%.6f)" % (
            INTENDED_DOOR_WIDTH, gap0, gap1, gap_w
        ))
        return None, None, False, reasons

    ideal_y0 = INTENDED_DOOR_CENTER_Y - INTENDED_DOOR_WIDTH * 0.5
    door_y0 = min(max(ideal_y0, gap0), gap1 - INTENDED_DOOR_WIDTH)
    door_center = door_y0 + INTENDED_DOOR_WIDTH * 0.5
    reasons.append(
        "shifted doorway to nearest %.3f-wide window inside protected gap Y=%.6f to %.6f"
        % (INTENDED_DOOR_WIDTH, gap0, gap1)
    )
    return door_center, INTENDED_DOOR_WIDTH, False, reasons


world = _get_editor_world()
pkg = _world_package(world)
if pkg != REQUIRED_PACKAGE:
    _log("=== SECTION 10 EAST WALL SPLIT DIAGNOSIS ABORTED — WRONG MAP ===")
    _log("Expected: %s" % REQUIRED_PACKAGE)
    _log("Actual: %s" % pkg)
    _log("ZERO actor writes, deletes, or saves.")
    raise RuntimeError("diagnostic aborted: wrong map %s" % pkg)

_log("=== SECTION 10 EAST WALL SPLIT DIAGNOSIS (READ-ONLY) ===")
_log("editor_world_package=%s" % pkg)
_log("zero writes: no spawn/destroy/move/rename/collision-change/save")

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not actor_sub:
    raise RuntimeError("EditorActorSubsystem unavailable")

wall_rows = []
s6_rows = []
s7_rows = []
hub_rows = []
s10_rows = []
repl_rows = []

for actor in actor_sub.get_all_level_actors():
    if not actor:
        continue
    if _actor_owner_package(actor) != REQUIRED_PACKAGE:
        continue
    label = actor.get_actor_label()
    if label == PERIMETER_EAST_LABEL:
        wall_rows.append(_log_actor("WALL", actor))
    elif label in (PERIMETER_SOUTH_LABEL, PERIMETER_NORTH_LABEL, PERIMETER_HEADER_LABEL):
        repl_rows.append(_log_actor("REPL", actor))
    elif _is_s6(label):
        s6_rows.append(_log_actor("S6", actor))
    elif _is_s7(label):
        s7_rows.append(_log_actor("S7", actor))
    elif label in HUB_FLOOR_LABELS or label == HUB_OPENING_LABEL:
        hub_rows.append(_log_actor("HUB", actor))
    elif label.startswith(S10_PREFIX):
        s10_rows.append(_log_actor("S10", actor))

s6_rows.sort(key=lambda r: r["label"])
s7_rows.sort(key=lambda r: r["label"])
hub_rows.sort(key=lambda r: r["label"])
s10_rows.sort(key=lambda r: r["label"])
repl_rows.sort(key=lambda r: r["label"])

s6_union = _union_bounds(s6_rows)
s7_union = _union_bounds(s7_rows)
hub_union = _union_bounds(hub_rows)
s10_union = _union_bounds(s10_rows)

_log("--- GROUP UNIONS (actor_bounds, same method as Section 10 build script) ---")
_log("S6 count=%d union=%s" % (len(s6_rows), _aabb_str(s6_union)))
_log("S7 count=%d union=%s" % (len(s7_rows), _aabb_str(s7_union)))
_log("Hub floor/opening count=%d union=%s" % (len(hub_rows), _aabb_str(hub_union)))
_log("S10 count=%d union=%s" % (len(s10_rows), _aabb_str(s10_union)))
_log("existing replacement pieces count=%d" % len(repl_rows))

if len(wall_rows) != 1:
    _log("Wall_Perimeter_East_1 count=%d expected=1. Cannot recalculate split." % len(wall_rows))
    _log("=== SECTION 10 EAST WALL SPLIT RECALCULATION COMPLETE ===")
    raise RuntimeError("diagnostic aborted: Wall_Perimeter_East_1 count=%d" % len(wall_rows))

wall = wall_rows[0]
orig_aabb = wall["loc_scale"]
orig_bounds = wall["bounds"]
_log("--- ORIGINAL WALL ---")
_log("original wall loc*scale AABB (used by split math)=%s" % _aabb_str(orig_aabb))
_log("original wall actor_bounds AABB=%s" % _aabb_str(orig_bounds))
_log("original wall Y min/max=%.6f / %.6f" % (orig_aabb[0][1], orig_aabb[1][1]))
_log("original wall Z min/max=%.6f / %.6f" % (orig_aabb[0][2], orig_aabb[1][2]))
_log("original wall X min/max=%.6f / %.6f" % (orig_aabb[0][0], orig_aabb[1][0]))
_log("original wall Y span=%.6f" % (orig_aabb[1][1] - orig_aabb[0][1]))

s6_adjacent_rows = [row for row in s6_rows if _adjacent_to_east_wall(row["bounds"], orig_aabb)]
s7_adjacent_rows = [row for row in s7_rows if _adjacent_to_east_wall(row["bounds"], orig_aabb)]
s6_adjacent = _union_bounds(s6_adjacent_rows)
s7_adjacent = _union_bounds(s7_adjacent_rows)
s6_clipped = _clip_y_to_wall(s6_adjacent, orig_aabb)
s7_clipped = _clip_y_to_wall(s7_adjacent, orig_aabb)

_log("--- SECTION 6 ADJACENT TO EAST WALL ---")
_log("S6 adjacent count=%d / %d" % (len(s6_adjacent_rows), len(s6_rows)))
for row in s6_rows:
    adj = _adjacent_to_east_wall(row["bounds"], orig_aabb)
    _log("S6 adjacency label=%s adjacent=%s Y=%.6f to %.6f X=%.6f to %.6f" % (
        row["label"], adj, row["bounds"][0][1], row["bounds"][1][1], row["bounds"][0][0], row["bounds"][1][0]
    ))
_log("S6 full union %s" % _aabb_str(s6_union))
_log("S6 wall-adjacent union %s" % _aabb_str(s6_adjacent))
_log("S6 wall-adjacent clipped to wall Y %s" % _aabb_str(s6_clipped))

_log("--- SECTION 7 ADJACENT TO EAST WALL ---")
_log("S7 adjacent count=%d / %d" % (len(s7_adjacent_rows), len(s7_rows)))
for row in s7_rows:
    adj = _adjacent_to_east_wall(row["bounds"], orig_aabb)
    _log("S7 adjacency label=%s adjacent=%s Y=%.6f to %.6f X=%.6f to %.6f" % (
        row["label"], adj, row["bounds"][0][1], row["bounds"][1][1], row["bounds"][0][0], row["bounds"][1][0]
    ))
_log("S7 full union %s" % _aabb_str(s7_union))
_log("S7 wall-adjacent union %s" % _aabb_str(s7_adjacent))
_log("S7 wall-adjacent clipped to wall Y %s" % _aabb_str(s7_clipped))

intended = _split_from_wall(
    orig_aabb, wall["loc"], wall["scale"],
    INTENDED_DOOR_CENTER_Y, INTENDED_DOOR_WIDTH, INTENDED_DOOR_HEIGHT,
)
_log("--- CURRENT BUILD-SCRIPT SPLIT (door Y=-130 to +130) ---")
_log("intended doorway Y=%.6f to %.6f width=%.6f height=%.6f" % (
    intended["door_y0"], intended["door_y1"], intended["door_width"], intended["door_height"]
))
_log("intended South Y=%.6f to %.6f AABB=%s" % (intended["south_y0"], intended["south_y1"], _aabb_str(intended["south_aabb"])))
_log("intended North Y=%.6f to %.6f AABB=%s" % (intended["north_y0"], intended["north_y1"], _aabb_str(intended["north_aabb"])))
_log("intended Header Y=%.6f to %.6f Z=%.6f to %.6f AABB=%s" % (
    intended["header_y0"], intended["header_y1"], intended["header_z0"], intended["header_z1"], _aabb_str(intended["header_aabb"])
))

full_errors = _current_check_errors(intended, orig_aabb, s6_union, s7_union)
adj_errors = _current_check_errors(intended, orig_aabb, s6_clipped or s6_adjacent, s7_clipped or s7_adjacent)
_log("current check vs FULL S6/S7 union errors=%s" % ("NONE" if not full_errors else " | ".join(full_errors)))
_log("current check vs WALL-ADJACENT S6/S7 errors=%s" % ("NONE" if not adj_errors else " | ".join(adj_errors)))

s6_cover = s6_clipped or s6_adjacent
s7_cover = s7_clipped or s7_adjacent
door_center, door_width, kept_intended, choose_reasons = _choose_door(orig_aabb, s6_cover, s7_cover)
for reason in choose_reasons:
    _log("DOOR CHOICE: %s" % reason)

corrected = None
if door_center is None:
    _log("corrected doorway: NONE — no safe opening satisfies protection + reconstruction")
else:
    corrected = _split_from_wall(
        orig_aabb, wall["loc"], wall["scale"],
        door_center, door_width, INTENDED_DOOR_HEIGHT,
    )

conflicts = []
s10_valid = None
if corrected:
    orig_span = orig_aabb[1][1] - orig_aabb[0][1]
    recon_span = corrected["south_len"] + corrected["door_width"] + corrected["north_len"]
    span_delta = recon_span - orig_span
    span_ok = abs(span_delta) <= SPAN_EPS
    _log("--- CORRECTED SPLIT ---")
    _log("corrected doorway center Y=%.6f" % corrected["door_center_y"])
    _log("corrected doorway Y min/max=%.6f / %.6f" % (corrected["door_y0"], corrected["door_y1"]))
    _log("corrected doorway width=%.6f" % corrected["door_width"])
    _log("corrected doorway height=%.6f" % corrected["door_height"])
    _log("corrected doorway AABB=%s" % _aabb_str(corrected["door_aabb"]))
    _log("corrected South Y min/max=%.6f / %.6f AABB=%s" % (
        corrected["south_y0"], corrected["south_y1"], _aabb_str(corrected["south_aabb"])
    ))
    _log("corrected North Y min/max=%.6f / %.6f AABB=%s" % (
        corrected["north_y0"], corrected["north_y1"], _aabb_str(corrected["north_aabb"])
    ))
    _log("corrected Header Y min/max=%.6f / %.6f Z min/max=%.6f / %.6f AABB=%s" % (
        corrected["header_y0"], corrected["header_y1"], corrected["header_z0"], corrected["header_z1"],
        _aabb_str(corrected["header_aabb"])
    ))
    _log("South spec loc=%s scale=%s" % (_fmt_xyz(corrected["south_spec"][1]), _fmt_xyz(corrected["south_spec"][2])))
    _log("North spec loc=%s scale=%s" % (_fmt_xyz(corrected["north_spec"][1]), _fmt_xyz(corrected["north_spec"][2])))
    _log("Header spec loc=%s scale=%s" % (_fmt_xyz(corrected["header_spec"][1]), _fmt_xyz(corrected["header_spec"][2])))
    _log("span check south(%.6f) + door(%.6f) + north(%.6f) = %.6f original=%.6f delta=%.6f ok=%s" % (
        corrected["south_len"], corrected["door_width"], corrected["north_len"], recon_span, orig_span, span_delta, span_ok
    ))
    if not span_ok:
        conflicts.append("reconstructed Y span != original wall Y span")

    s6_protected = True
    if s6_cover:
        s6_protected = _covers_y(corrected["south_y0"], corrected["south_y1"], s6_cover[0][1], s6_cover[1][1])
        _log("South covers wall-adjacent S6 Y (%.6f to %.6f): %s" % (
            s6_cover[0][1], s6_cover[1][1], s6_protected
        ))
        if not s6_protected:
            conflicts.append("corrected South does not cover wall-adjacent Section 6 Y")
        if _aabb_overlaps(corrected["door_aabb"], s6_cover, eps=OVERLAP_EPS):
            conflicts.append("corrected doorway overlaps wall-adjacent Section 6")
            _log("doorway overlaps adjacent S6: YES")
        else:
            _log("doorway overlaps adjacent S6: NO")
    else:
        _log("South covers wall-adjacent S6 Y: N/A (no adjacent S6)")

    s7_protected = True
    if s7_cover:
        s7_protected = _covers_y(corrected["north_y0"], corrected["north_y1"], s7_cover[0][1], s7_cover[1][1])
        _log("North covers wall-adjacent S7 Y (%.6f to %.6f): %s" % (
            s7_cover[0][1], s7_cover[1][1], s7_protected
        ))
        if not s7_protected:
            conflicts.append("corrected North does not cover wall-adjacent Section 7 Y")
        if _aabb_overlaps(corrected["door_aabb"], s7_cover, eps=OVERLAP_EPS):
            conflicts.append("corrected doorway overlaps wall-adjacent Section 7")
            _log("doorway overlaps adjacent S7: YES")
        else:
            _log("doorway overlaps adjacent S7: NO")
    else:
        _log("North covers wall-adjacent S7 Y: N/A (no adjacent S7)")

    still_fails_full_union = _current_check_errors(corrected, orig_aabb, s6_union, s7_union)
    if still_fails_full_union:
        _log("NOTE: current build-script FULL-UNION coverage check would still fail: %s" % (
            " | ".join(still_fails_full_union)
        ))
        _log("That check is measuring S6/S7 actors not adjacent to this wall. Do not bypass it; clip coverage to wall-adjacent Y.")
        conflicts.append("build script still uses full S6/S7 union; wall-adjacent clip is required before rerun")

    _log("--- SECTION 10 PLACEMENT VS CORRECTED DOORWAY ---")
    if not s10_rows:
        s10_valid = False
        _log("current Section 10 actors: NONE")
        conflicts.append("no Admin_Operations_* actors present")
    else:
        opening_labels = (
            "Admin_Operations_Threshold",
            "Admin_Operations_Connector_Floor",
            "Admin_Operations_Wall_West_Header",
            "Admin_Operations_Wall_West_North",
            "Admin_Operations_Wall_West_South",
        )
        opening_rows = [row for row in s10_rows if row["label"] in opening_labels]
        s10_open_union = _union_bounds(opening_rows) if opening_rows else None
        _log("S10 opening-related count=%d union=%s" % (len(opening_rows), _aabb_str(s10_open_union)))
        door_y_match = (
            abs(corrected["door_y0"] - intended["door_y0"]) <= SPAN_EPS
            and abs(corrected["door_y1"] - intended["door_y1"]) <= SPAN_EPS
        )
        s10_y_ok = True
        if s10_open_union:
            if s10_open_union[0][1] < corrected["door_y0"] - 1.0 or s10_open_union[1][1] > corrected["door_y1"] + 1.0:
                s10_y_ok = False
                _log("S10 opening Y=%.6f to %.6f is not contained in corrected doorway Y=%.6f to %.6f" % (
                    s10_open_union[0][1], s10_open_union[1][1], corrected["door_y0"], corrected["door_y1"]
                ))
            else:
                _log("S10 opening Y=%.6f to %.6f fits corrected doorway Y=%.6f to %.6f" % (
                    s10_open_union[0][1], s10_open_union[1][1], corrected["door_y0"], corrected["door_y1"]
                ))
        if door_y_match and s10_y_ok:
            s10_valid = True
            _log("current Section 10 placement REMAINS VALID with the corrected doorway")
            _log("Section 10 actors must NOT be moved")
        else:
            s10_valid = False
            _log("current Section 10 placement DOES NOT MATCH the corrected doorway")
            _log("Section 10 actors were NOT moved by this diagnostic")
            conflicts.append("corrected doorway Y differs from current Operations Control opening; do not move S10 until approved")
else:
    conflicts.append("no corrected doorway could be calculated")
    s10_valid = False

if corrected:
    for row in s6_adjacent_rows + s7_adjacent_rows:
        if _aabb_overlaps(corrected["south_aabb"], row["bounds"]) and row in s7_adjacent_rows:
            conflicts.append("South replacement overlaps adjacent S7 actor %s" % row["label"])
        if _aabb_overlaps(corrected["north_aabb"], row["bounds"]) and row in s6_adjacent_rows:
            conflicts.append("North replacement overlaps adjacent S6 actor %s" % row["label"])

_log("--- FINAL SUMMARY ---")
_log("original wall bounds: %s" % _aabb_str(orig_aabb))
_log("Section 6 full bounds: %s" % _aabb_str(s6_union))
_log("Section 6 wall-adjacent bounds: %s" % _aabb_str(s6_cover))
_log("Section 7 full bounds: %s" % _aabb_str(s7_union))
_log("Section 7 wall-adjacent bounds: %s" % _aabb_str(s7_cover))
if corrected:
    _log("corrected doorway bounds: %s" % _aabb_str(corrected["door_aabb"]))
    _log("corrected South bounds: %s" % _aabb_str(corrected["south_aabb"]))
    _log("corrected North bounds: %s" % _aabb_str(corrected["north_aabb"]))
    _log("corrected Header bounds: %s" % _aabb_str(corrected["header_aabb"]))
    _log("mathematical span check: south+door+north vs original Y  (see span check line)")
    _log("intended doorway preserved: %s" % kept_intended)
else:
    _log("corrected doorway bounds: NONE")
    _log("corrected South/North/Header bounds: NONE")
    _log("mathematical span check: N/A")
    _log("intended doorway preserved: False")
_log("current Section 10 placement remains valid: %s" % s10_valid)
if conflicts:
    _log("remaining conflicts:")
    for item in conflicts:
        _log("  CONFLICT: %s" % item)
else:
    _log("remaining conflicts: NONE")

_log("diagnostic complete. no actors were spawned, moved, deleted, renamed, or saved.")
_log("=== SECTION 10 EAST WALL SPLIT RECALCULATION COMPLETE ===")
