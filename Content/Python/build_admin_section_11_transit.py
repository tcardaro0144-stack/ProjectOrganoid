# ProjectOrganoid — Section 11 Transit Control for SL_Epitope_Admin.
# Does not touch Lvl_MainMenu. Does not build Section 12 Service Corridor.
# Does not rebuild Sections 1–10 except removing the sealed S10 east plug
# Admin_Operations_ToTransit after Transit geometry and the opening validate.
# Do not import this module from another script; execution starts on load.

import unreal

REQUIRED_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
MENU_PACKAGE = "/Game/Maps/Lvl_MainMenu"
CUBE_PATH = "/Engine/BasicShapes/Cube.Cube"
CUBE_HALF = 50.0
TRANSFORM_EPS = 0.05
SPAN_EPS = 0.05
S11_PREFIX = "Admin_Transit_"
S10_PREFIX = "Admin_Operations_"
S9_PREFIX = "Admin_DirectorSuite_"
S8_PREFIX = "Admin_ConferenceRoom_"
PLUG_LABEL = "Admin_Operations_ToTransit"
S10_EAST_NORTH = "Admin_Operations_Wall_East_North"
S10_EAST_SOUTH = "Admin_Operations_Wall_East_South"
S10_EAST_HEADER = "Admin_Operations_Wall_East_Header"
S10_FLOOR_LABEL = "Admin_Operations_Floor"
S10_EAST_KEEP = (S10_EAST_NORTH, S10_EAST_SOUTH, S10_EAST_HEADER)

ROOM_SIZE_X = 900.0
ROOM_SIZE_Y = 700.0
WALL_T = 25.0
EXPECTED_DOOR_WIDTH = 260.0
EXPECTED_DOOR_HEIGHT = 300.0
EXPECTED_DOOR_CENTER_Y = 0.0
FLOOR_Z = 10.0
CEILING_Z = 470.0
WALL_Z = 225.0
WALL_H = 4.5
FLOOR_SLAB_SCALE_Z = 0.20
WALKABLE_CLEARANCE_Z = 5.0
EXPECTED_FLOOR_TOP_Z = FLOOR_Z + FLOOR_SLAB_SCALE_Z * CUBE_HALF
WALKABLE_Z0 = EXPECTED_FLOOR_TOP_Z + WALKABLE_CLEARANCE_Z

S6_SNAPSHOT_PREFIXES = ("Admin_SecurityOffice_", "Admin_S6_")
S6_SNAPSHOT_EXACT = ("Terminal_AdminSecurity",)
S7_SNAPSHOT_PREFIXES = ("Admin_RecordsArchives_", "Admin_S7_")
S7_SNAPSHOT_EXACT = ("Admin_RecordsArchives_Block",)

DISPLAY_LABELS = (
    "Admin_Transit_Display_Admin",
    "Admin_Transit_Display_NeuroGenetics",
    "Admin_Transit_Display_Cryo",
    "Admin_Transit_Display_Compute",
    "Admin_Transit_Display_Reactor",
)


def _log(msg):
    unreal.log(msg)


def _fmt_vec(v):
    return "(%.6f, %.6f, %.6f)" % (v.x, v.y, v.z)


def _fmt_rot(r):
    return "(%.6f, %.6f, %.6f)" % (r.pitch, r.yaw, r.roll)


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


def _assert_admin_map(stage):
    world = _get_editor_world()
    pkg = _world_package(world)
    if pkg != REQUIRED_PACKAGE:
        _log("=== SECTION 11 ABORTED — WRONG MAP ===")
        _log("Expected: %s" % REQUIRED_PACKAGE)
        _log("Actual: %s" % pkg)
        _log("stage=%s ZERO actor writes, deletes, or saves." % stage)
        raise RuntimeError(
            "SECTION 11 aborted at %s: current package '%s', expected '%s'"
            % (stage, pkg, REQUIRED_PACKAGE)
        )
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


def _aabb_from_actor_loc_scale(actor):
    loc = actor.get_actor_location()
    scale = actor.get_actor_scale3d()
    return _aabb_from_loc_scale((loc.x, loc.y, loc.z), (scale.x, scale.y, scale.z))


def _aabb_union(a, b):
    if a is None:
        return b
    if b is None:
        return a
    return (
        (min(a[0][0], b[0][0]), min(a[0][1], b[0][1]), min(a[0][2], b[0][2])),
        (max(a[1][0], b[1][0]), max(a[1][1], b[1][1]), max(a[1][2], b[1][2])),
    )


def _aabb_overlaps(a, b, eps=1.0):
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
    return "min=(%.2f, %.2f, %.2f) max=(%.2f, %.2f, %.2f)" % (
        mn[0], mn[1], mn[2], mx[0], mx[1], mx[2]
    )


def _vec_close(actual, expected_xyz):
    return (
        abs(actual.x - expected_xyz[0]) <= TRANSFORM_EPS
        and abs(actual.y - expected_xyz[1]) <= TRANSFORM_EPS
        and abs(actual.z - expected_xyz[2]) <= TRANSFORM_EPS
    )


def _rot_close(actual, expected_pyr):
    return (
        abs(actual.pitch - expected_pyr[0]) <= TRANSFORM_EPS
        and abs(actual.yaw - expected_pyr[1]) <= TRANSFORM_EPS
        and abs(actual.roll - expected_pyr[2]) <= TRANSFORM_EPS
    )


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


def _apply_blockout(actor, spec, cube):
    label, loc_xyz, rot_pyr, scale_xyz, profile, enabled = spec
    actor.set_actor_label(label)
    actor.set_actor_location(unreal.Vector(*loc_xyz), False, False)
    actor.set_actor_rotation(unreal.Rotator(rot_pyr[0], rot_pyr[1], rot_pyr[2]), False)
    actor.set_actor_scale3d(unreal.Vector(*scale_xyz))
    mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
    if not mesh:
        raise RuntimeError(label + " has no StaticMeshComponent")
    mesh.set_static_mesh(cube)
    mesh.set_collision_profile_name(profile)
    mesh.set_collision_enabled(getattr(unreal.CollisionEnabled, enabled))


def _matches_spec(actor, spec):
    label, loc_xyz, rot_pyr, scale_xyz, profile, enabled = spec
    reasons = []
    if actor.get_actor_label() != label:
        reasons.append("label=%s" % actor.get_actor_label())
    if not _vec_close(actor.get_actor_location(), loc_xyz):
        reasons.append("loc=%s expected=%s" % (_fmt_vec(actor.get_actor_location()), str(loc_xyz)))
    if not _rot_close(actor.get_actor_rotation(), rot_pyr):
        reasons.append("rot=%s expected=%s" % (_fmt_rot(actor.get_actor_rotation()), str(rot_pyr)))
    if not _vec_close(actor.get_actor_scale3d(), scale_xyz):
        reasons.append("scale=%s expected=%s" % (_fmt_vec(actor.get_actor_scale3d()), str(scale_xyz)))
    mesh_path = _mesh_path(actor)
    if not mesh_path.endswith("Cube.Cube"):
        reasons.append("mesh=%s" % mesh_path)
    actual_profile = _collision_profile(actor)
    if actual_profile != profile and actual_profile != "None":
        reasons.append("collision_profile=%s expected=%s" % (actual_profile, profile))
    actual_enabled = _collision_enabled(actor)
    if actual_enabled != enabled:
        reasons.append("collision_enabled=%s expected=%s" % (actual_enabled, enabled))
    owner = _actor_owner_package(actor)
    if owner != REQUIRED_PACKAGE:
        reasons.append("owner=%s" % owner)
    return reasons


def _collect_by_label(actor_sub):
    by_label = {}
    for actor in actor_sub.get_all_level_actors():
        if not actor:
            continue
        by_label.setdefault(actor.get_actor_label(), []).append(actor)
    return by_label


def _fail(message):
    _log("=== SECTION 11 ABORTED ===")
    _log(message)
    raise RuntimeError(message)


def _solid(label, loc, scale):
    return (label, loc, (0.0, 0.0, 0.0), scale, "BlockAll", "QUERY_AND_PHYSICS")


def _snapshot_actor(actor):
    loc = actor.get_actor_location()
    rot = actor.get_actor_rotation()
    scale = actor.get_actor_scale3d()
    return (
        actor.get_actor_label(),
        _safe_str(_safe_call(actor, "get_path_name")),
        (loc.x, loc.y, loc.z),
        (rot.pitch, rot.yaw, rot.roll),
        (scale.x, scale.y, scale.z),
        _collision_profile(actor),
        _collision_enabled(actor),
    )


def _snapshot_protected(actor_sub):
    rows = []
    for actor in actor_sub.get_all_level_actors():
        if not actor:
            continue
        if _actor_owner_package(actor) != REQUIRED_PACKAGE:
            continue
        label = actor.get_actor_label()
        if label == PLUG_LABEL:
            continue
        if label.startswith(S11_PREFIX) or label.startswith("Admin_S11_"):
            continue
        rows.append(_snapshot_actor(actor))
    rows.sort()
    return rows


def _snapshot_group(actor_sub, prefixes, exact_labels):
    rows = []
    for actor in actor_sub.get_all_level_actors():
        label = actor.get_actor_label()
        hit = label in exact_labels
        for prefix in prefixes:
            if label.startswith(prefix):
                hit = True
        if not hit:
            continue
        if _actor_owner_package(actor) != REQUIRED_PACKAGE:
            continue
        rows.append(_snapshot_actor(actor))
    rows.sort()
    return rows


def _group_aabb(actor_sub, prefixes, exact_labels=()):
    acc = None
    count = 0
    for actor in actor_sub.get_all_level_actors():
        label = actor.get_actor_label()
        hit = label in exact_labels
        for prefix in prefixes:
            if label.startswith(prefix):
                hit = True
        if not hit:
            continue
        if _actor_owner_package(actor) != REQUIRED_PACKAGE:
            continue
        acc = _aabb_union(acc, _aabb_from_actor(actor))
        count += 1
        loc = actor.get_actor_location()
        scale = actor.get_actor_scale3d()
        _log("AUDIT label=%s loc=%s scale=%s owner=%s" % (
            label, _fmt_vec(loc), _fmt_vec(scale), _actor_owner_package(actor)
        ))
    return acc, count


def _admin_one(by_label, label):
    found = [a for a in by_label.get(label, []) if _actor_owner_package(a) == REQUIRED_PACKAGE]
    if len(found) != 1:
        _fail("Precondition failed: %s count=%d expected=1. ZERO modifications." % (label, len(found)))
    return found[0]


def _plan_creates(by_label, specs):
    to_create = []
    already_ok = []
    mismatched = []
    for spec in specs:
        label = spec[0]
        found = by_label.get(label, [])
        if len(found) > 1:
            mismatched.append("%s has %d copies" % (label, len(found)))
            continue
        if len(found) == 1:
            reasons = _matches_spec(found[0], spec)
            if reasons:
                mismatched.append("%s exists but does not match spec: %s" % (label, "; ".join(reasons)))
            else:
                already_ok.append(label)
            continue
        to_create.append(spec)
    return to_create, already_ok, mismatched


def _spawn_specs(actor_sub, cube, specs_to_create):
    created = []
    for spec in specs_to_create:
        _assert_admin_map("pre-spawn " + spec[0])
        actor = actor_sub.spawn_actor_from_class(
            unreal.StaticMeshActor,
            unreal.Vector(*spec[1]),
            unreal.Rotator(spec[2][0], spec[2][1], spec[2][2]),
        )
        if not actor:
            _fail("Failed to spawn " + spec[0] + ". Original %s was not deleted." % PLUG_LABEL)
        _apply_blockout(actor, spec, cube)
        created.append(spec[0])
        _log("created %s" % spec[0])
    return created


def _verify_specs_present(actor_sub, specs):
    by_label = _collect_by_label(actor_sub)
    errors = []
    rows = []
    union = None
    for spec in specs:
        label = spec[0]
        found = by_label.get(label, [])
        if len(found) != 1:
            errors.append("%s count=%d" % (label, len(found)))
            continue
        actor = found[0]
        reasons = _matches_spec(actor, spec)
        owner = _actor_owner_package(actor)
        if owner == MENU_PACKAGE:
            errors.append("%s owned by Lvl_MainMenu" % label)
        if reasons:
            errors.append("%s verify failed: %s" % (label, "; ".join(reasons)))
        rows.append(actor)
        union = _aabb_union(union, _aabb_from_actor(actor))
        _log(
            "VERIFY label=%s class=%s loc=%s rot=%s scale=%s owner=%s collision_profile=%s collision_enabled=%s"
            % (
                label,
                _safe_str(_safe_call(actor.get_class(), "get_path_name")),
                _fmt_vec(actor.get_actor_location()),
                _fmt_rot(actor.get_actor_rotation()),
                _fmt_vec(actor.get_actor_scale3d()),
                owner,
                _collision_profile(actor),
                _collision_enabled(actor),
            )
        )
    return errors, rows, union, by_label


def _walkable_opening_aabb(door_aabb):
    return (
        (door_aabb[0][0], door_aabb[0][1], WALKABLE_Z0),
        (door_aabb[1][0], door_aabb[1][1], door_aabb[1][2]),
    )


def _blocking_walkable_opening(actor_sub, door_aabb, extra_ignore=()):
    walk_aabb = _walkable_opening_aabb(door_aabb)
    ignore = set(extra_ignore)
    blockers = []
    for actor in actor_sub.get_all_level_actors():
        label = actor.get_actor_label()
        if label in ignore:
            continue
        if _collision_enabled(actor) == "NO_COLLISION":
            continue
        profile = _collision_profile(actor)
        if profile == "NoCollision":
            continue
        actor_aabb = _aabb_from_actor(actor)
        if _aabb_overlaps(actor_aabb, walk_aabb, eps=0.5):
            blockers.append("%s topZ=%.3f" % (label, actor_aabb[1][2]))
    return blockers, walk_aabb


def _walkable_ignore_labels():
    return set([
        "Admin_FloorPlate",
        "Admin_Hub_Floor",
        "Admin_S5_Hub_Floor",
        "Admin_Operations_Floor",
        "Admin_Transit_Floor",
        "Admin_Transit_Threshold",
    ])


def _is_s6_label(label):
    if label in S6_SNAPSHOT_EXACT:
        return True
    return any(label.startswith(prefix) for prefix in S6_SNAPSHOT_PREFIXES)


def _is_s7_label(label):
    if label in S7_SNAPSHOT_EXACT:
        return True
    return any(label.startswith(prefix) for prefix in S7_SNAPSHOT_PREFIXES)


def _is_s8_label(label):
    return label.startswith(S8_PREFIX)


def _is_s9_label(label):
    return label.startswith(S9_PREFIX)


def _section_actor_aabbs(actor_sub, predicate):
    rows = []
    for actor in actor_sub.get_all_level_actors():
        if not actor:
            continue
        if _actor_owner_package(actor) != REQUIRED_PACKAGE:
            continue
        label = actor.get_actor_label()
        if not predicate(label):
            continue
        rows.append((label, _aabb_from_actor(actor)))
    rows.sort(key=lambda item: item[0])
    return rows


def _capture_live_spec(actor):
    loc = actor.get_actor_location()
    rot = actor.get_actor_rotation()
    scale = actor.get_actor_scale3d()
    profile = _collision_profile(actor)
    enabled = _collision_enabled(actor)
    if profile in ("None", ""):
        profile = "BlockAll"
    if enabled in ("None", ""):
        enabled = "QUERY_AND_PHYSICS"
    return (
        actor.get_actor_label(),
        (loc.x, loc.y, loc.z),
        (rot.pitch, rot.yaw, rot.roll),
        (scale.x, scale.y, scale.z),
        profile,
        enabled,
    )


def _build_s11_specs(east_face_x, door_center_y, door_width, door_height):
    door_half = door_width * 0.5
    door_y0 = door_center_y - door_half
    door_y1 = door_center_y + door_half
    west_wall_x = east_face_x + WALL_T * 0.5
    floor_min_x = east_face_x + WALL_T
    floor_max_x = floor_min_x + ROOM_SIZE_X
    floor_cx = (floor_min_x + floor_max_x) * 0.5
    floor_cy = door_center_y
    floor_min_y = door_center_y - ROOM_SIZE_Y * 0.5
    floor_max_y = door_center_y + ROOM_SIZE_Y * 0.5
    east_wall_x = floor_max_x + WALL_T * 0.5
    north_y = floor_max_y + WALL_T * 0.5
    south_y = floor_min_y - WALL_T * 0.5
    wall_top = WALL_Z + WALL_H * CUBE_HALF
    header_z = (door_height + wall_top) * 0.5
    header_h = wall_top - door_height
    west_north_len = floor_max_y - door_y1
    west_south_len = door_y0 - floor_min_y
    display_x = floor_max_x - 15.0
    display_z = 130.0
    display_scale = (0.30, 0.80, 2.20)
    display_ys = (
        door_center_y - 240.0,
        door_center_y - 120.0,
        door_center_y,
        door_center_y + 120.0,
        door_center_y + 240.0,
    )
    specs = [
        _solid("Admin_Transit_Floor", (floor_cx, floor_cy, FLOOR_Z), (ROOM_SIZE_X / 100.0, ROOM_SIZE_Y / 100.0, FLOOR_SLAB_SCALE_Z)),
        _solid("Admin_Transit_Ceiling", (floor_cx, floor_cy, CEILING_Z), (ROOM_SIZE_X / 100.0, ROOM_SIZE_Y / 100.0, FLOOR_SLAB_SCALE_Z)),
        _solid("Admin_Transit_Wall_North", (floor_cx, north_y, WALL_Z), (ROOM_SIZE_X / 100.0, WALL_T / 100.0, WALL_H)),
        _solid("Admin_Transit_Wall_South", (floor_cx, south_y, WALL_Z), (ROOM_SIZE_X / 100.0, WALL_T / 100.0, WALL_H)),
        _solid("Admin_Transit_Wall_East", (east_wall_x, floor_cy, WALL_Z), (WALL_T / 100.0, ROOM_SIZE_Y / 100.0, WALL_H)),
        _solid(
            "Admin_Transit_Wall_West_North",
            (west_wall_x, (door_y1 + floor_max_y) * 0.5, WALL_Z),
            (WALL_T / 100.0, west_north_len / 100.0, WALL_H),
        ),
        _solid(
            "Admin_Transit_Wall_West_South",
            (west_wall_x, (floor_min_y + door_y0) * 0.5, WALL_Z),
            (WALL_T / 100.0, west_south_len / 100.0, WALL_H),
        ),
        _solid(
            "Admin_Transit_Wall_West_Header",
            (west_wall_x, door_center_y, header_z),
            (WALL_T / 100.0, door_width / 100.0, header_h / 100.0),
        ),
        _solid("Admin_Transit_Threshold", (east_face_x, door_center_y, FLOOR_Z), (0.50, door_width / 100.0, FLOOR_SLAB_SCALE_Z)),
        _solid(DISPLAY_LABELS[0], (display_x, display_ys[0], display_z), display_scale),
        _solid(DISPLAY_LABELS[1], (display_x, display_ys[1], display_z), display_scale),
        _solid(DISPLAY_LABELS[2], (display_x, display_ys[2], display_z), display_scale),
        _solid(DISPLAY_LABELS[3], (display_x, display_ys[3], display_z), display_scale),
        _solid(DISPLAY_LABELS[4], (display_x, display_ys[4], display_z), display_scale),
    ]
    room_aabb = (
        (east_face_x, floor_min_y - WALL_T, 0.0),
        (floor_max_x + WALL_T, floor_max_y + WALL_T, CEILING_Z + 10.0),
    )
    opening_aabb = (
        (east_face_x - WALL_T, door_y0, 0.0),
        (floor_min_x, door_y1, door_height),
    )
    return specs, room_aabb, opening_aabb, (floor_min_x, floor_max_x, floor_min_y, floor_max_y)


# ---------------------------------------------------------------------------
# 1. Map guard
# ---------------------------------------------------------------------------
world, world_pkg = _assert_admin_map("precondition-package")
_log("SECTION 11 map guard passed: %s" % world_pkg)

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not actor_sub or not level_sub:
    raise RuntimeError("EditorActorSubsystem / LevelEditorSubsystem unavailable")

by_label = _collect_by_label(actor_sub)
protected_before = _snapshot_protected(actor_sub)
s6_before = _snapshot_group(actor_sub, S6_SNAPSHOT_PREFIXES, S6_SNAPSHOT_EXACT)
s7_before = _snapshot_group(actor_sub, S7_SNAPSHOT_PREFIXES, S7_SNAPSHOT_EXACT)
s8_before = _snapshot_group(actor_sub, (S8_PREFIX,), ())
s9_before = _snapshot_group(actor_sub, (S9_PREFIX,), ())
s10_before = _snapshot_group(actor_sub, (S10_PREFIX,), ())
s10_keep_before = [row for row in s10_before if row[0] != PLUG_LABEL]

_log("=== SECTION 11 PRE-BUILD SPATIAL AUDIT ===")
s6_aabb, s6_count = _group_aabb(actor_sub, S6_SNAPSHOT_PREFIXES, S6_SNAPSHOT_EXACT)
s7_aabb, s7_count = _group_aabb(actor_sub, S7_SNAPSHOT_PREFIXES, S7_SNAPSHOT_EXACT)
s8_aabb, s8_count = _group_aabb(actor_sub, (S8_PREFIX,))
s9_aabb, s9_count = _group_aabb(actor_sub, (S9_PREFIX,))
s10_aabb, s10_count = _group_aabb(actor_sub, (S10_PREFIX,))
_log("Section 6 count=%d bounds=%s" % (s6_count, _aabb_str(s6_aabb)))
_log("Section 7 count=%d bounds=%s" % (s7_count, _aabb_str(s7_aabb)))
_log("Section 8 count=%d bounds=%s" % (s8_count, _aabb_str(s8_aabb)))
_log("Section 9 count=%d bounds=%s" % (s9_count, _aabb_str(s9_aabb)))
_log("Section 10 count=%d bounds=%s" % (s10_count, _aabb_str(s10_aabb)))
_log("protected Admin actors (excluding plug/S11)=%d" % len(protected_before))

if s8_count < 1 or s9_count < 1 or s10_count < 1:
    _fail("Precondition failed: verified Sections 8/9/10 missing. ZERO modifications.")
if any(label.startswith("Admin_S12_") or label.startswith("Admin_ServiceCorridor") for label in by_label):
    _fail("Precondition failed: Section 12 labels already exist. ZERO modifications.")
if any(label.startswith("Admin_S11_") for label in by_label):
    _fail("Precondition failed: unexpected Admin_S11_ labels exist. ZERO modifications.")

# ---------------------------------------------------------------------------
# 2. Live Section 10 east interface
# ---------------------------------------------------------------------------
east_north = _admin_one(by_label, S10_EAST_NORTH)
east_south = _admin_one(by_label, S10_EAST_SOUTH)
east_header = _admin_one(by_label, S10_EAST_HEADER)
s10_floor = _admin_one(by_label, S10_FLOOR_LABEL)
east_keep_specs = [
    _capture_live_spec(east_north),
    _capture_live_spec(east_south),
    _capture_live_spec(east_header),
]

north_aabb = _aabb_from_actor_loc_scale(east_north)
south_aabb = _aabb_from_actor_loc_scale(east_south)
header_aabb = _aabb_from_actor_loc_scale(east_header)
east_union = _aabb_union(_aabb_union(north_aabb, south_aabb), header_aabb)
east_face_x = east_union[1][0]
door_y0 = south_aabb[1][1]
door_y1 = north_aabb[0][1]
door_width = door_y1 - door_y0
door_center_y = (door_y0 + door_y1) * 0.5
door_height = EXPECTED_DOOR_HEIGHT
header_z0 = header_aabb[0][2]

_log("live S10 east union %s" % _aabb_str(east_union))
_log("live S10 east face X=%.6f" % east_face_x)
_log("live opening Y=%.6f to %.6f width=%.6f centerY=%.6f headerZ0=%.6f" % (
    door_y0, door_y1, door_width, door_center_y, header_z0
))

if abs(door_width - EXPECTED_DOOR_WIDTH) > 1.0:
    _fail("Precondition failed: live S10 east opening width=%.3f expected=%.3f. ZERO modifications." % (
        door_width, EXPECTED_DOOR_WIDTH
    ))
if abs(door_center_y - EXPECTED_DOOR_CENTER_Y) > 1.0:
    _fail("Precondition failed: live S10 east opening center Y=%.3f expected=%.3f. ZERO modifications." % (
        door_center_y, EXPECTED_DOOR_CENTER_Y
    ))
if abs(header_z0 - EXPECTED_DOOR_HEIGHT) > 1.0:
    _fail("Precondition failed: live S10 east header Z0=%.3f expected=%.3f. ZERO modifications." % (
        header_z0, EXPECTED_DOOR_HEIGHT
    ))
if east_face_x < 4400.0 or east_face_x > 4700.0:
    _fail("Precondition failed: live S10 east face X=%.3f outside expected Operations range. ZERO modifications." % east_face_x)

plug_actors = [a for a in by_label.get(PLUG_LABEL, []) if _actor_owner_package(a) == REQUIRED_PACKAGE]
if len(plug_actors) > 1:
    _fail("Precondition failed: %s count=%d expected 0 or 1. ZERO modifications." % (PLUG_LABEL, len(plug_actors)))
original_plug = plug_actors[0] if plug_actors else None
if original_plug:
    plug_aabb = _aabb_from_actor_loc_scale(original_plug)
    _log("INSPECT %s loc=%s scale=%s aabb=%s owner=%s collision=%s/%s" % (
        PLUG_LABEL,
        _fmt_vec(original_plug.get_actor_location()),
        _fmt_vec(original_plug.get_actor_scale3d()),
        _aabb_str(plug_aabb),
        _actor_owner_package(original_plug),
        _collision_profile(original_plug),
        _collision_enabled(original_plug),
    ))
    if abs(plug_aabb[0][1] - door_y0) > 1.0 or abs(plug_aabb[1][1] - door_y1) > 1.0:
        _fail("Precondition failed: %s Y does not match the live S10 east opening. ZERO modifications." % PLUG_LABEL)
else:
    _log("%s already absent; treating as rerun after unseal." % PLUG_LABEL)

s11_specs, room_aabb, opening_aabb, floor_extents = _build_s11_specs(
    east_face_x, door_center_y, EXPECTED_DOOR_WIDTH, EXPECTED_DOOR_HEIGHT
)
expected_s11_labels = [spec[0] for spec in s11_specs]
_log("proposed Section 11 bounds: %s" % _aabb_str(room_aabb))
_log("proposed interior floor X=%.1f to %.1f Y=%.1f to %.1f" % floor_extents)
_log("opening AABB: %s" % _aabb_str(opening_aabb))
_log("proposed Section 11 actor count=%d" % len(s11_specs))

# ---------------------------------------------------------------------------
# 3. Envelope / overlap vs S6–S10 (S11 threshold may kiss S10 east face)
# ---------------------------------------------------------------------------
s11_interface = set([
    "Admin_Transit_Threshold",
    "Admin_Transit_Wall_West_North",
    "Admin_Transit_Wall_West_South",
    "Admin_Transit_Wall_West_Header",
])
conflict = []
s6_actor_aabbs = _section_actor_aabbs(actor_sub, _is_s6_label)
s7_actor_aabbs = _section_actor_aabbs(actor_sub, _is_s7_label)
s8_actor_aabbs = _section_actor_aabbs(actor_sub, _is_s8_label)
s9_actor_aabbs = _section_actor_aabbs(actor_sub, _is_s9_label)
for spec in s11_specs:
    aabb = _aabb_from_loc_scale(spec[1], spec[3])
    for label, other in s6_actor_aabbs:
        if _aabb_overlaps(aabb, other):
            conflict.append("%s overlaps Section 6 actor %s" % (spec[0], label))
    for label, other in s7_actor_aabbs:
        if _aabb_overlaps(aabb, other):
            conflict.append("%s overlaps Section 7 actor %s" % (spec[0], label))
    for label, other in s8_actor_aabbs:
        if _aabb_overlaps(aabb, other):
            conflict.append("%s overlaps Section 8 actor %s" % (spec[0], label))
    for label, other in s9_actor_aabbs:
        if _aabb_overlaps(aabb, other):
            conflict.append("%s overlaps Section 9 actor %s" % (spec[0], label))
    if spec[0] not in s11_interface:
        s10_body = _aabb_from_actor_loc_scale(s10_floor)
        if _aabb_overlaps(aabb, s10_body, eps=1.0):
            conflict.append("%s overlaps Section 10 floor" % spec[0])
if conflict:
    _fail("Precondition failed: spatial conflict. ZERO modifications. " + " | ".join(conflict))

unexpected = []
for label in by_label:
    if label.startswith(S11_PREFIX) and label not in expected_s11_labels:
        unexpected.append(label)
    if label.startswith("Admin_S11_"):
        unexpected.append(label)
if unexpected:
    _fail(
        "Precondition failed: unexpected Transit labels already exist. ZERO modifications. labels=%s"
        % ",".join(sorted(set(unexpected)))
    )

s11_to_create, s11_already_ok, s11_mismatch = _plan_creates(by_label, s11_specs)
if s11_mismatch:
    _fail(
        "Precondition failed: existing labels conflict with intended spec. ZERO modifications. "
        + " | ".join(s11_mismatch)
    )

_log("=== SECTION 11 PRECONDITIONS PASSED ===")
_log("will create s11=%d; already-ok s11=%d; plug_present=%s" % (
    len(s11_to_create), len(s11_already_ok), bool(original_plug)
))

cube = unreal.EditorAssetLibrary.load_asset(CUBE_PATH)
if not cube:
    raise RuntimeError("Missing mesh: " + CUBE_PATH)

# ---------------------------------------------------------------------------
# EXECUTION — S11 first, unseal ToTransit last
# ---------------------------------------------------------------------------
created = []
created.extend(_spawn_specs(actor_sub, cube, s11_to_create))
for label in s11_already_ok:
    _log("exists, skip spawn: %s" % label)

s11_errors, s11_rows, s11_union, by_label = _verify_specs_present(actor_sub, s11_specs)
if s11_errors:
    _fail("Section 11 failed validation. Original %s NOT deleted. " % PLUG_LABEL + " | ".join(s11_errors))

east_keep_errors, _, _, by_label = _verify_specs_present(actor_sub, east_keep_specs)
if east_keep_errors:
    _fail("Section 10 east jambs/header changed during S11 spawn. %s NOT deleted. " % PLUG_LABEL + " | ".join(east_keep_errors))

pre_delete_ignore = _walkable_ignore_labels() | set([PLUG_LABEL])
pre_blockers, pre_walk = _blocking_walkable_opening(actor_sub, opening_aabb, pre_delete_ignore)
_log("pre-unseal walkable opening AABB: %s" % _aabb_str(pre_walk))
if pre_blockers:
    _fail(
        "Opening still blocked by replacement/other geometry. Original %s NOT deleted. blockers=%s"
        % (PLUG_LABEL, ",".join(pre_blockers))
    )

deleted_plug = False
if original_plug is not None:
    _assert_admin_map("pre-delete-totransit-plug")
    still = [
        a for a in actor_sub.get_all_level_actors()
        if a.get_actor_label() == PLUG_LABEL and _actor_owner_package(a) == REQUIRED_PACKAGE
    ]
    if len(still) != 1:
        _fail("Refusing to delete: %s count changed to %d before delete." % (PLUG_LABEL, len(still)))
    destroyed = actor_sub.destroy_actor(still[0])
    if not destroyed:
        _fail("destroy_actor returned false for %s." % PLUG_LABEL)
    deleted_plug = True
    _log("deleted original %s after Transit and opening validation" % PLUG_LABEL)
else:
    _log("no %s delete performed" % PLUG_LABEL)

# ---------------------------------------------------------------------------
# Final verification
# ---------------------------------------------------------------------------
_assert_admin_map("pre-final-verify")
by_label = _collect_by_label(actor_sub)
final_errors = []

if by_label.get(PLUG_LABEL):
    final_errors.append("original %s still exists after unseal" % PLUG_LABEL)

keep_errors, _, _, by_label = _verify_specs_present(actor_sub, east_keep_specs)
final_errors.extend(keep_errors)

ignore_opening = _walkable_ignore_labels()
blockers, walk_aabb = _blocking_walkable_opening(actor_sub, opening_aabb, ignore_opening)
_log("walkable opening AABB: %s" % _aabb_str(walk_aabb))
_log("walkable Z starts at %.3f (expected floor top %.3f + clearance %.3f)" % (
    WALKABLE_Z0, EXPECTED_FLOOR_TOP_Z, WALKABLE_CLEARANCE_Z
))
if blockers:
    final_errors.append("walkable Operations-Transit opening still blocked by: " + ",".join(blockers))

transit_labels = [label for label in by_label if label.startswith(S11_PREFIX)]
dupes = [label for label in transit_labels if len(by_label.get(label, [])) > 1]
if dupes:
    final_errors.append("duplicate Admin_Transit_* labels: " + ",".join(sorted(set(dupes))))
extra = [label for label in transit_labels if label not in expected_s11_labels]
if extra:
    final_errors.append("unexpected Admin_Transit_* labels: " + ",".join(sorted(set(extra))))
missing = [label for label in expected_s11_labels if label not in transit_labels]
if missing:
    final_errors.append("missing Section 11 labels: " + ",".join(missing))

missing_displays = [label for label in DISPLAY_LABELS if len(by_label.get(label, [])) != 1]
if missing_displays:
    final_errors.append("missing displays: " + ",".join(missing_displays))

if any(label.startswith("Admin_S12_") or label.startswith("Admin_ServiceCorridor") for label in by_label):
    final_errors.append("Section 12 was built; it must not be")
if any(label.startswith("Admin_S11_") for label in by_label):
    final_errors.append("unexpected Admin_S11_ labels exist")

protected_after = _snapshot_protected(actor_sub)
s6_after = _snapshot_group(actor_sub, S6_SNAPSHOT_PREFIXES, S6_SNAPSHOT_EXACT)
s7_after = _snapshot_group(actor_sub, S7_SNAPSHOT_PREFIXES, S7_SNAPSHOT_EXACT)
s8_after = _snapshot_group(actor_sub, (S8_PREFIX,), ())
s9_after = _snapshot_group(actor_sub, (S9_PREFIX,), ())
s10_after = _snapshot_group(actor_sub, (S10_PREFIX,), ())
s10_keep_after = [row for row in s10_after if row[0] != PLUG_LABEL]
if protected_after != protected_before:
    final_errors.append("protected Admin actors changed during Section 11 build")
if s6_after != s6_before:
    final_errors.append("Section 6 actors changed during Section 11 build")
if s7_after != s7_before:
    final_errors.append("Section 7 actors changed during Section 11 build")
if s8_after != s8_before:
    final_errors.append("Section 8 actors changed during Section 11 build")
if s9_after != s9_before:
    final_errors.append("Section 9 actors changed during Section 11 build")
if s10_keep_after != s10_keep_before:
    final_errors.append("Section 10 actors other than %s changed during Section 11 build" % PLUG_LABEL)

for spec in s11_specs:
    found = by_label.get(spec[0], [])
    if len(found) != 1:
        continue
    if _actor_owner_package(found[0]) != REQUIRED_PACKAGE:
        final_errors.append("%s owned by %s" % (spec[0], _actor_owner_package(found[0])))
    if _actor_owner_package(found[0]) == MENU_PACKAGE:
        final_errors.append("%s owned by Lvl_MainMenu" % spec[0])

if final_errors:
    _log("=== SECTION 11 VERIFICATION FAILED ===")
    _fail(" | ".join(final_errors))

_, world_pkg = _assert_admin_map("pre-save")
if world_pkg != REQUIRED_PACKAGE:
    _fail("Refusing to save: current package is %s" % world_pkg)
saved = level_sub.save_current_level()
_log("saved current level only; package=%s result=%s" % (world_pkg, _safe_str(saved)))
_log("Lvl_MainMenu was not saved")

_log("=== SECTION 11 TRANSIT CONTROL VERIFIED IN SL_EPITOPE_ADMIN ===")
_log("Section 11 actor count=%d" % len(s11_specs))
_log("created=%d already-existing s11=%d" % (len(created), len(s11_already_ok)))
_log("plug deleted=%s kept S10 east pieces=%s" % (deleted_plug, ",".join(S10_EAST_KEEP)))
_log("interior=%.0f x %.0f ceiling=%.0f" % (ROOM_SIZE_X, ROOM_SIZE_Y, CEILING_Z))
_log("opening door=%.0f x %.0f at S10 east face X=%.1f Y=%.1f to %.1f" % (
    EXPECTED_DOOR_WIDTH, EXPECTED_DOOR_HEIGHT, east_face_x, door_y0, door_y1
))
_log("displays=%s" % ",".join(DISPLAY_LABELS))
_log("Sections 1–10 protected snapshots unchanged except %s. Section 12 was not built. Lvl_MainMenu was not saved." % PLUG_LABEL)
_log("=== SECTION 11 TRANSIT CONTROL COMPLETE ===")
