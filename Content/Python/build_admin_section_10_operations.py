# ProjectOrganoid — Section 10 Operations Control for SL_Epitope_Admin.
# Does not touch Lvl_MainMenu. Does not build Section 11 Transit Control.
# Does not rebuild Sections 1–9. The Admin plate east perimeter wall
# (Wall_Perimeter_East_1) is split around the hub-forward Operations doorway.

import unreal

REQUIRED_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
MENU_PACKAGE = "/Game/Maps/Lvl_MainMenu"
CUBE_PATH = "/Engine/BasicShapes/Cube.Cube"
CUBE_HALF = 50.0
TRANSFORM_EPS = 0.05
SPAN_EPS = 0.05
ADJACENT_X_EPS = 50.0
S10_PREFIX = "Admin_Operations_"
S9_PREFIX = "Admin_DirectorSuite_"
S8_PREFIX = "Admin_ConferenceRoom_"

HUB_FLOOR_LABELS = ("Admin_Hub_Floor", "Admin_S5_Hub_Floor")
FLOOR_PLATE_LABEL = "Admin_FloorPlate"
PERIMETER_EAST_LABEL = "Wall_Perimeter_East_1"
PERIMETER_SOUTH_LABEL = "Wall_Perimeter_East_1_South"
PERIMETER_NORTH_LABEL = "Wall_Perimeter_East_1_North"
PERIMETER_HEADER_LABEL = "Wall_Perimeter_East_1_Header"
PERIMETER_REPLACEMENT_LABELS = (PERIMETER_SOUTH_LABEL, PERIMETER_NORTH_LABEL, PERIMETER_HEADER_LABEL)

ROOM_SIZE_X = 1600.0
ROOM_SIZE_Y = 1200.0
WALL_T = 25.0
DOOR_WIDTH = 260.0
DOOR_HEIGHT = 300.0
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
S9_REQUIRED_LABEL = "Admin_DirectorSuite_Floor"

STATION_LABELS = (
    "Admin_Operations_Station_Personnel",
    "Admin_Operations_Station_Security",
    "Admin_Operations_Station_Power",
    "Admin_Operations_Station_SectorAccess",
    "Admin_Operations_Station_Logistics",
    "Admin_Operations_Station_Environment",
    "Admin_Operations_Station_Incident",
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
        _log("=== SECTION 10 ABORTED — WRONG MAP ===")
        _log("Expected: %s" % REQUIRED_PACKAGE)
        _log("Actual: %s" % pkg)
        _log("stage=%s ZERO actor writes, deletes, or saves." % stage)
        raise RuntimeError(
            "SECTION 10 aborted at %s: current package '%s', expected '%s'"
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


def _span_y(aabb):
    return aabb[1][1] - aabb[0][1]


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
    _log("=== SECTION 10 ABORTED ===")
    _log(message)
    raise RuntimeError(message)


def _solid(label, loc, scale):
    return (label, loc, (0.0, 0.0, 0.0), scale, "BlockAll", "QUERY_AND_PHYSICS")


def _visual(label, loc, scale):
    return (label, loc, (0.0, 0.0, 0.0), scale, "NoCollision", "NO_COLLISION")


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
        loc = actor.get_actor_location()
        rot = actor.get_actor_rotation()
        scale = actor.get_actor_scale3d()
        rows.append((
            label,
            _safe_str(_safe_call(actor, "get_path_name")),
            (loc.x, loc.y, loc.z),
            (rot.pitch, rot.yaw, rot.roll),
            (scale.x, scale.y, scale.z),
            _collision_profile(actor),
            _collision_enabled(actor),
        ))
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


def _is_s6_label(label):
    if label in S6_SNAPSHOT_EXACT:
        return True
    return any(label.startswith(prefix) for prefix in S6_SNAPSHOT_PREFIXES)


def _is_s7_label(label):
    if label in S7_SNAPSHOT_EXACT:
        return True
    return any(label.startswith(prefix) for prefix in S7_SNAPSHOT_PREFIXES)


def _adjacent_to_east_wall(actor_aabb, wall_aabb):
    # Same methodology as diagnose_admin_section_10_east_wall_split.py.
    if not actor_aabb or not wall_aabb:
        return False
    y_overlap = actor_aabb[0][1] < wall_aabb[1][1] and actor_aabb[1][1] > wall_aabb[0][1]
    x_near = (
        actor_aabb[1][0] >= wall_aabb[0][0] - ADJACENT_X_EPS
        and actor_aabb[0][0] <= wall_aabb[1][0] + ADJACENT_X_EPS
    )
    return y_overlap and x_near


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


def _aabb_from_actor_loc_scale(actor):
    loc = actor.get_actor_location()
    scale = actor.get_actor_scale3d()
    return _aabb_from_loc_scale((loc.x, loc.y, loc.z), (scale.x, scale.y, scale.z))


def _wall_adjacent_cover(actor_sub, wall_aabb, predicate):
    adjacent = None
    count = 0
    labels = []
    for actor in actor_sub.get_all_level_actors():
        if not actor:
            continue
        if _actor_owner_package(actor) != REQUIRED_PACKAGE:
            continue
        label = actor.get_actor_label()
        if not predicate(label):
            continue
        bounds = _aabb_from_actor(actor)
        if not _adjacent_to_east_wall(bounds, wall_aabb):
            continue
        adjacent = _aabb_union(adjacent, bounds)
        count += 1
        labels.append(label)
    labels.sort()
    cover = _clip_y_to_wall(adjacent, wall_aabb) or adjacent
    return cover, count, labels


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
            _fail("Failed to spawn " + spec[0] + ". Original %s was not deleted." % PERIMETER_EAST_LABEL)
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


def _find_hub_floor(by_label):
    found = []
    for label in HUB_FLOOR_LABELS:
        found.extend(by_label.get(label, []))
    admin_owned = [a for a in found if _actor_owner_package(a) == REQUIRED_PACKAGE]
    return admin_owned


def _is_ignored_entrance_shell(label):
    if label in HUB_FLOOR_LABELS or label == FLOOR_PLATE_LABEL:
        return True
    if label == "Admin_Hub_Opening_Cut":
        return True
    if label.startswith(S10_PREFIX):
        return True
    if label in PERIMETER_REPLACEMENT_LABELS:
        return True
    return False


def _split_wall_specs(wall_actor, door_center_y, door_width, door_height, south_label, north_label, header_label):
    loc = wall_actor.get_actor_location()
    scale = wall_actor.get_actor_scale3d()
    loc_xyz = (loc.x, loc.y, loc.z)
    scale_xyz = (scale.x, scale.y, scale.z)
    orig_aabb = _aabb_from_loc_scale(loc_xyz, scale_xyz)
    door_y0 = door_center_y - door_width * 0.5
    door_y1 = door_center_y + door_width * 0.5
    south_y0 = orig_aabb[0][1]
    south_y1 = door_y0
    north_y0 = door_y1
    north_y1 = orig_aabb[1][1]
    south_len = south_y1 - south_y0
    north_len = north_y1 - north_y0
    header_z0 = door_height
    header_z1 = orig_aabb[1][2]
    header_h = header_z1 - header_z0
    if south_len < 40.0 or north_len < 40.0 or header_h < 20.0:
        return None, orig_aabb, None, "wall cannot host %.0fx%.0f doorway at Y=%.1f (south=%.1f north=%.1f header=%.1f)" % (
            door_width, door_height, door_center_y, south_len, north_len, header_h
        )
    specs = [
        _solid(south_label, (loc.x, (south_y0 + south_y1) * 0.5, loc.z), (scale.x, south_len / 100.0, scale.z)),
        _solid(north_label, (loc.x, (north_y0 + north_y1) * 0.5, loc.z), (scale.x, north_len / 100.0, scale.z)),
        _solid(
            header_label,
            (loc.x, door_center_y, (header_z0 + header_z1) * 0.5),
            (scale.x, door_width / 100.0, header_h / 100.0),
        ),
    ]
    door_aabb = (
        (orig_aabb[0][0], door_y0, orig_aabb[0][2]),
        (orig_aabb[1][0], door_y1, door_height),
    )
    return specs, orig_aabb, door_aabb, None


def _verify_replacement_footprint(repl_specs, orig_aabb, door_aabb, s6_adj_aabb, s7_adj_aabb):
    errors = []
    by_name = {}
    for spec in repl_specs:
        by_name[spec[0]] = spec
    south = by_name.get(PERIMETER_SOUTH_LABEL)
    north = by_name.get(PERIMETER_NORTH_LABEL)
    header = by_name.get(PERIMETER_HEADER_LABEL)
    if not south or not north or not header:
        return ["replacement specs missing south/north/header"]
    south_aabb = _aabb_from_loc_scale(south[1], south[3])
    north_aabb = _aabb_from_loc_scale(north[1], north[3])
    header_aabb = _aabb_from_loc_scale(header[1], header[3])
    door_y0 = door_aabb[0][1]
    door_y1 = door_aabb[1][1]
    if abs(door_y0 - (-DOOR_WIDTH * 0.5)) > SPAN_EPS or abs(door_y1 - (DOOR_WIDTH * 0.5)) > SPAN_EPS:
        errors.append("doorway Y=%.3f to %.3f expected=%.3f to %.3f" % (
            door_y0, door_y1, -DOOR_WIDTH * 0.5, DOOR_WIDTH * 0.5
        ))
    if abs(south_aabb[0][1] - orig_aabb[0][1]) > SPAN_EPS or abs(north_aabb[1][1] - orig_aabb[1][1]) > SPAN_EPS:
        errors.append("south+north Y footprint != original wall")
    if abs(south_aabb[1][1] - door_y0) > SPAN_EPS:
        errors.append("south piece does not stop at doorway Y=%.1f" % door_y0)
    if abs(north_aabb[0][1] - door_y1) > SPAN_EPS:
        errors.append("north piece does not start at doorway Y=%.1f" % door_y1)
    south_span = south_aabb[1][1] - south_aabb[0][1]
    door_span = door_y1 - door_y0
    north_span = north_aabb[1][1] - north_aabb[0][1]
    orig_span = orig_aabb[1][1] - orig_aabb[0][1]
    if abs((south_span + door_span + north_span) - orig_span) > SPAN_EPS:
        errors.append(
            "south+door+north Y span=%.6f != original wall Y span=%.6f"
            % (south_span + door_span + north_span, orig_span)
        )
    if abs(south_aabb[0][0] - orig_aabb[0][0]) > SPAN_EPS or abs(south_aabb[1][0] - orig_aabb[1][0]) > SPAN_EPS:
        errors.append("south piece X != original wall")
    if abs(north_aabb[0][0] - orig_aabb[0][0]) > SPAN_EPS or abs(north_aabb[1][0] - orig_aabb[1][0]) > SPAN_EPS:
        errors.append("north piece X != original wall")
    if abs(header_aabb[0][1] - door_y0) > SPAN_EPS or abs(header_aabb[1][1] - door_y1) > SPAN_EPS:
        errors.append("header Y does not match doorway")
    if abs(header_aabb[0][2] - DOOR_HEIGHT) > SPAN_EPS:
        errors.append("header does not start at Z=%.1f" % DOOR_HEIGHT)
    if abs(header_aabb[1][2] - orig_aabb[1][2]) > SPAN_EPS:
        errors.append("header does not reach original wall top")
    if _aabb_overlaps(south_aabb, door_aabb, eps=0.5):
        errors.append("south piece occupies doorway")
    if _aabb_overlaps(north_aabb, door_aabb, eps=0.5):
        errors.append("north piece occupies doorway")
    walk_door = _walkable_opening_aabb(door_aabb)
    if _aabb_overlaps(header_aabb, walk_door, eps=0.5):
        errors.append("header occupies walkable opening")
    # Cover only geometry adjacent to this perimeter wall. None is valid.
    if s6_adj_aabb:
        if south_aabb[0][1] - s6_adj_aabb[0][1] > SPAN_EPS or s6_adj_aabb[1][1] - south_aabb[1][1] > SPAN_EPS:
            errors.append("south replacement does not cover wall-adjacent Section 6 Y range")
        if s6_adj_aabb[1][1] - door_y0 > SPAN_EPS:
            errors.append("doorway would expose wall-adjacent Section 6")
    if s7_adj_aabb:
        if north_aabb[0][1] - s7_adj_aabb[0][1] > SPAN_EPS or s7_adj_aabb[1][1] - north_aabb[1][1] > SPAN_EPS:
            errors.append("north replacement does not cover wall-adjacent Section 7 Y range")
        if door_y1 - s7_adj_aabb[0][1] > SPAN_EPS:
            errors.append("doorway would expose wall-adjacent Section 7")
    return errors


def _walkable_ignore_labels():
    # Designed walking-surface slabs only. Do not whitelist S10 furniture,
    # Operations walls, or the replacement jambs/header.
    return set(HUB_FLOOR_LABELS) | set([
        FLOOR_PLATE_LABEL,
        "Admin_Hub_Opening_Cut",
        "Admin_Operations_Threshold",
        "Admin_Operations_Connector_Floor",
    ])


def _build_s10_specs(hub_east_x, ops_west_x, center_y):
    door_half = DOOR_WIDTH * 0.5
    door_y0 = center_y - door_half
    door_y1 = center_y + door_half
    west_wall_x = ops_west_x + WALL_T * 0.5
    floor_min_x = ops_west_x + WALL_T
    floor_max_x = floor_min_x + ROOM_SIZE_X
    floor_cx = (floor_min_x + floor_max_x) * 0.5
    floor_cy = center_y
    floor_min_y = center_y - ROOM_SIZE_Y * 0.5
    floor_max_y = center_y + ROOM_SIZE_Y * 0.5
    east_wall_x = floor_max_x + WALL_T * 0.5
    north_y = floor_max_y + WALL_T * 0.5
    south_y = floor_min_y - WALL_T * 0.5
    wall_top = WALL_Z + WALL_H * CUBE_HALF
    header_z = (DOOR_HEIGHT + wall_top) * 0.5
    header_h = wall_top - DOOR_HEIGHT
    west_north_len = floor_max_y - door_y1
    west_south_len = door_y0 - floor_min_y
    east_north_len = west_north_len
    east_south_len = west_south_len
    conn_len = ops_west_x - hub_east_x
    conn_cx = (hub_east_x + ops_west_x) * 0.5
    station_z = 55.0
    station_scale = (1.60, 0.70, 1.10)
    north_station_y = floor_max_y - 55.0
    south_station_y = floor_min_y + 55.0
    specs = [
        _solid("Admin_Operations_Floor", (floor_cx, floor_cy, FLOOR_Z), (ROOM_SIZE_X / 100.0, ROOM_SIZE_Y / 100.0, FLOOR_SLAB_SCALE_Z)),
        _solid("Admin_Operations_Ceiling", (floor_cx, floor_cy, CEILING_Z), (ROOM_SIZE_X / 100.0, ROOM_SIZE_Y / 100.0, FLOOR_SLAB_SCALE_Z)),
        _solid("Admin_Operations_Wall_North", (floor_cx, north_y, WALL_Z), (ROOM_SIZE_X / 100.0, WALL_T / 100.0, WALL_H)),
        _solid("Admin_Operations_Wall_South", (floor_cx, south_y, WALL_Z), (ROOM_SIZE_X / 100.0, WALL_T / 100.0, WALL_H)),
        _solid(
            "Admin_Operations_Wall_West_North",
            (west_wall_x, (door_y1 + floor_max_y) * 0.5, WALL_Z),
            (WALL_T / 100.0, west_north_len / 100.0, WALL_H),
        ),
        _solid(
            "Admin_Operations_Wall_West_South",
            (west_wall_x, (floor_min_y + door_y0) * 0.5, WALL_Z),
            (WALL_T / 100.0, west_south_len / 100.0, WALL_H),
        ),
        _solid(
            "Admin_Operations_Wall_West_Header",
            (west_wall_x, center_y, header_z),
            (WALL_T / 100.0, DOOR_WIDTH / 100.0, header_h / 100.0),
        ),
        _solid(
            "Admin_Operations_Wall_East_North",
            (east_wall_x, (door_y1 + floor_max_y) * 0.5, WALL_Z),
            (WALL_T / 100.0, east_north_len / 100.0, WALL_H),
        ),
        _solid(
            "Admin_Operations_Wall_East_South",
            (east_wall_x, (floor_min_y + door_y0) * 0.5, WALL_Z),
            (WALL_T / 100.0, east_south_len / 100.0, WALL_H),
        ),
        _solid(
            "Admin_Operations_Wall_East_Header",
            (east_wall_x, center_y, header_z),
            (WALL_T / 100.0, DOOR_WIDTH / 100.0, header_h / 100.0),
        ),
        _solid("Admin_Operations_Threshold", (ops_west_x, center_y, FLOOR_Z), (0.50, DOOR_WIDTH / 100.0, FLOOR_SLAB_SCALE_Z)),
        _solid(
            "Admin_Operations_ToTransit",
            (east_wall_x, center_y, DOOR_HEIGHT * 0.5),
            (0.08, DOOR_WIDTH / 100.0, DOOR_HEIGHT / 100.0),
        ),
        _solid("Admin_Operations_Station_Personnel", (floor_min_x + 520.0, north_station_y, station_z), station_scale),
        _solid("Admin_Operations_Station_Security", (floor_cx, north_station_y, station_z), station_scale),
        _solid("Admin_Operations_Station_Power", (floor_max_x - 220.0, north_station_y, station_z), station_scale),
        _solid("Admin_Operations_Station_Logistics", (floor_min_x + 520.0, south_station_y, station_z), station_scale),
        _solid("Admin_Operations_Station_Environment", (floor_cx, south_station_y, station_z), station_scale),
        _solid("Admin_Operations_Station_Incident", (floor_max_x - 220.0, south_station_y, station_z), station_scale),
        _solid("Admin_Operations_Station_SectorAccess", (floor_max_x - 70.0, center_y + 280.0, station_z), (0.70, 1.60, 1.10)),
        _solid("Admin_Operations_CentralDisplay", (floor_cx, floor_cy, 45.0), (3.20, 2.00, 0.90)),
        _visual("Admin_Operations_CentralHolo", (floor_cx, floor_cy, 210.0), (0.35, 0.35, 2.40)),
    ]
    if conn_len > 1.0:
        specs.extend([
            _solid(
                "Admin_Operations_Connector_Floor",
                (conn_cx, center_y, FLOOR_Z),
                (conn_len / 100.0, DOOR_WIDTH / 100.0, FLOOR_SLAB_SCALE_Z),
            ),
            _solid(
                "Admin_Operations_Connector_Ceiling",
                (conn_cx, center_y, CEILING_Z),
                (conn_len / 100.0, DOOR_WIDTH / 100.0, FLOOR_SLAB_SCALE_Z),
            ),
            _solid(
                "Admin_Operations_Connector_Wall_North",
                (conn_cx, door_y1 + WALL_T * 0.5, WALL_Z),
                (conn_len / 100.0, WALL_T / 100.0, WALL_H),
            ),
            _solid(
                "Admin_Operations_Connector_Wall_South",
                (conn_cx, door_y0 - WALL_T * 0.5, WALL_Z),
                (conn_len / 100.0, WALL_T / 100.0, WALL_H),
            ),
        ])
    room_aabb = (
        (min(hub_east_x, floor_min_x) - WALL_T, floor_min_y - WALL_T, 0.0),
        (floor_max_x + WALL_T, floor_max_y + WALL_T, CEILING_Z + 10.0),
    )
    entrance_aabb = (
        (hub_east_x, door_y0, 0.0),
        (floor_min_x, door_y1, DOOR_HEIGHT),
    )
    transit_aabb = (
        (floor_max_x, door_y0, 0.0),
        (floor_max_x + WALL_T, door_y1, DOOR_HEIGHT),
    )
    return specs, room_aabb, entrance_aabb, transit_aabb, (floor_min_x, floor_max_x, floor_min_y, floor_max_y)


# ---------------------------------------------------------------------------
# 1. Map guard
# ---------------------------------------------------------------------------
world, world_pkg = _assert_admin_map("precondition-package")
_log("SECTION 10 map guard passed: %s" % world_pkg)

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not actor_sub or not level_sub:
    raise RuntimeError("EditorActorSubsystem / LevelEditorSubsystem unavailable")

by_label = _collect_by_label(actor_sub)
s6_before = _snapshot_group(actor_sub, S6_SNAPSHOT_PREFIXES, S6_SNAPSHOT_EXACT)
s7_before = _snapshot_group(actor_sub, S7_SNAPSHOT_PREFIXES, S7_SNAPSHOT_EXACT)
s8_before = _snapshot_group(actor_sub, (S8_PREFIX,), ())
s9_before = _snapshot_group(actor_sub, (S9_PREFIX,), ())

_log("=== SECTION 10 PRE-BUILD SPATIAL AUDIT ===")
s6_aabb, s6_count = _group_aabb(actor_sub, S6_SNAPSHOT_PREFIXES, S6_SNAPSHOT_EXACT)
s7_aabb, s7_count = _group_aabb(actor_sub, (), S7_SNAPSHOT_EXACT)
s8_aabb, s8_count = _group_aabb(actor_sub, (S8_PREFIX,))
s9_aabb, s9_count = _group_aabb(actor_sub, (S9_PREFIX,))
hub_aabb, hub_count = _group_aabb(actor_sub, ("Admin_Hub_", "Admin_S5_"), HUB_FLOOR_LABELS)
_log("Section 6 count=%d bounds=%s" % (s6_count, _aabb_str(s6_aabb)))
_log("Section 7 count=%d bounds=%s" % (s7_count, _aabb_str(s7_aabb)))
_log("Section 8 count=%d bounds=%s" % (s8_count, _aabb_str(s8_aabb)))
_log("Section 9 count=%d bounds=%s" % (s9_count, _aabb_str(s9_aabb)))
_log("Hub-related count=%d bounds=%s" % (hub_count, _aabb_str(hub_aabb)))

if s6_count < 1 or s7_count < 1:
    _fail("Precondition failed: Section 6/7 missing. ZERO modifications.")
if s8_count < 1:
    _fail("Precondition failed: Section 8 missing. ZERO modifications.")
if s9_count < 1 or not by_label.get(S9_REQUIRED_LABEL):
    _fail("Precondition failed: verified Section 9 Director Suite missing. ZERO modifications.")
if any(label.startswith("Admin_Transit") or label.startswith("Admin_S11_") for label in by_label):
    _fail("Precondition failed: Section 11 labels already exist. ZERO modifications.")

hub_floors = _find_hub_floor(by_label)
if len(hub_floors) != 1:
    _fail("Precondition failed: hub floor count=%d expected=1. ZERO modifications." % len(hub_floors))
hub_floor = hub_floors[0]
hub_floor_aabb = _aabb_from_actor(hub_floor)
hub_east_x = hub_floor_aabb[1][0]
hub_center_y = (hub_floor_aabb[0][1] + hub_floor_aabb[1][1]) * 0.5
if abs(hub_center_y) > 25.0:
    _fail("Precondition failed: hub center Y=%.3f is not on the +X circulation axis. ZERO modifications." % hub_center_y)
door_center_y = 0.0
if abs(hub_center_y - door_center_y) > 25.0:
    _fail("Precondition failed: hub center Y=%.3f cannot host doorway at Y=0. ZERO modifications." % hub_center_y)
if hub_east_x < 2500.0 or hub_east_x > 3100.0:
    _fail("Precondition failed: hub east face X=%.3f outside expected Admin range. ZERO modifications." % hub_east_x)

clear_x = max(hub_east_x, s6_aabb[1][0], s7_aabb[1][0])
ops_west_x = clear_x
_log("derived hub east X=%.3f S6/S7 clear X=%.3f ops west X=%.3f" % (hub_east_x, clear_x, ops_west_x))
_log("doorway locked at Y=%.3f to %.3f width=%.3f (not derived from full S6/S7 union)" % (
    door_center_y - DOOR_WIDTH * 0.5, door_center_y + DOOR_WIDTH * 0.5, DOOR_WIDTH
))

s10_specs, room_aabb, entrance_aabb, transit_aabb, floor_extents = _build_s10_specs(
    hub_east_x, ops_west_x, door_center_y
)
expected_s10_labels = [spec[0] for spec in s10_specs]
_log("proposed Section 10 bounds: %s" % _aabb_str(room_aabb))
_log("proposed interior floor X=%.1f to %.1f Y=%.1f to %.1f" % floor_extents)
_log("entrance AABB: %s" % _aabb_str(entrance_aabb))
_log("sealed S11 AABB: %s" % _aabb_str(transit_aabb))
_log("proposed Section 10 actor count=%d" % len(s10_specs))

# ---------------------------------------------------------------------------
# 2. Split Wall_Perimeter_East_1 around the Operations doorway
#    This is the Admin plate east shell from build_epitope_rooms.py, not an
#    Admin_Hub_* wall. It sits at X=2900, Y -650 to +2900, Z 0-400 and crosses
#    the connector at Y=0. Do not delete the whole wall: S6/S7 stay behind
#    the remaining north/south segments.
# ---------------------------------------------------------------------------
walk_entrance = _walkable_opening_aabb(entrance_aabb)
_log("walkable entrance AABB: %s" % _aabb_str(walk_entrance))

perimeter_actors = [a for a in by_label.get(PERIMETER_EAST_LABEL, []) if _actor_owner_package(a) == REQUIRED_PACKAGE]
have_replacements = all(len(by_label.get(label, [])) == 1 for label in PERIMETER_REPLACEMENT_LABELS)
repl_specs = []
original_wall = None
original_wall_label = PERIMETER_EAST_LABEL
repl_labels = PERIMETER_REPLACEMENT_LABELS
original_already_migrated = False
orig_aabb = None
hub_door_aabb = None
s6_adj_aabb = None
s7_adj_aabb = None
s6_adj_count = 0
s7_adj_count = 0

if len(perimeter_actors) > 1:
    _fail("Precondition failed: %s count=%d expected=1. ZERO modifications." % (
        PERIMETER_EAST_LABEL, len(perimeter_actors)
    ))

if len(perimeter_actors) == 1:
    original_wall = perimeter_actors[0]
    loc = original_wall.get_actor_location()
    scale = original_wall.get_actor_scale3d()
    wall_aabb = _aabb_from_actor(original_wall)
    _log(
        "INSPECT %s loc=%s scale=%s aabb=%s owner=%s collision=%s/%s"
        % (
            PERIMETER_EAST_LABEL,
            _fmt_vec(loc),
            _fmt_vec(scale),
            _aabb_str(wall_aabb),
            _actor_owner_package(original_wall),
            _collision_profile(original_wall),
            _collision_enabled(original_wall),
        )
    )
    if not _aabb_overlaps(wall_aabb, walk_entrance, eps=0.5):
        _fail(
            "Precondition failed: %s does not intersect the Operations entrance. ZERO modifications."
            % PERIMETER_EAST_LABEL
        )
    x_span = wall_aabb[1][0] - wall_aabb[0][0]
    if x_span > 80.0:
        _fail(
            "Precondition failed: %s X span=%.1f is not a thin east wall. ZERO modifications."
            % (PERIMETER_EAST_LABEL, x_span)
        )
    if wall_aabb[0][1] > door_center_y - DOOR_WIDTH * 0.5 - 40.0:
        _fail("Precondition failed: %s does not extend south of the doorway. ZERO modifications." % PERIMETER_EAST_LABEL)
    if wall_aabb[1][1] < door_center_y + DOOR_WIDTH * 0.5 + 40.0:
        _fail("Precondition failed: %s does not extend north of the doorway. ZERO modifications." % PERIMETER_EAST_LABEL)
    repl_specs, orig_aabb, hub_door_aabb, split_err = _split_wall_specs(
        original_wall,
        door_center_y,
        DOOR_WIDTH,
        DOOR_HEIGHT,
        PERIMETER_SOUTH_LABEL,
        PERIMETER_NORTH_LABEL,
        PERIMETER_HEADER_LABEL,
    )
    if split_err:
        _fail("Precondition failed: %s. ZERO modifications." % split_err)
    s6_adj_aabb, s6_adj_count, s6_adj_labels = _wall_adjacent_cover(actor_sub, orig_aabb, _is_s6_label)
    s7_adj_aabb, s7_adj_count, s7_adj_labels = _wall_adjacent_cover(actor_sub, orig_aabb, _is_s7_label)
    _log("Section 6 full-union bounds=%s (NOT used for perimeter cover)" % _aabb_str(s6_aabb))
    _log("Section 6 wall-adjacent count=%d bounds=%s labels=%s" % (
        s6_adj_count, _aabb_str(s6_adj_aabb), ",".join(s6_adj_labels) if s6_adj_labels else "NONE"
    ))
    _log("Section 7 full-union bounds=%s (NOT used for perimeter cover)" % _aabb_str(s7_aabb))
    _log("Section 7 wall-adjacent count=%d bounds=%s labels=%s" % (
        s7_adj_count, _aabb_str(s7_adj_aabb), ",".join(s7_adj_labels) if s7_adj_labels else "NONE"
    ))
    if s6_adj_count < 1:
        _log("Section 6 wall-adjacent=MISSING; South is not required to cover a non-existent adjacent S6 Y range")
    footprint_errors = _verify_replacement_footprint(repl_specs, orig_aabb, hub_door_aabb, s6_adj_aabb, s7_adj_aabb)
    if footprint_errors:
        _fail(
            "Precondition failed: replacement pieces do not reconstruct the perimeter wall. ZERO modifications. "
            + " | ".join(footprint_errors)
        )
    _log(
        "will split %s around doorway Y=%.1f to %.1f header Z=%.1f to original top; keep south/north coverage"
        % (PERIMETER_EAST_LABEL, door_center_y - DOOR_WIDTH * 0.5, door_center_y + DOOR_WIDTH * 0.5, DOOR_HEIGHT)
    )
    _log("perimeter reconstruct AABB %s" % _aabb_str(orig_aabb))
elif have_replacements:
    original_already_migrated = True
    _log("%s already removed; replacement pieces present. Skipping wall delete." % PERIMETER_EAST_LABEL)
    south_ls = _aabb_from_actor_loc_scale(by_label[PERIMETER_SOUTH_LABEL][0])
    north_ls = _aabb_from_actor_loc_scale(by_label[PERIMETER_NORTH_LABEL][0])
    orig_aabb = _aabb_union(south_ls, north_ls)
    s6_adj_aabb, s6_adj_count, s6_adj_labels = _wall_adjacent_cover(actor_sub, orig_aabb, _is_s6_label)
    s7_adj_aabb, s7_adj_count, s7_adj_labels = _wall_adjacent_cover(actor_sub, orig_aabb, _is_s7_label)
    _log("Section 6 wall-adjacent count=%d bounds=%s" % (s6_adj_count, _aabb_str(s6_adj_aabb)))
    _log("Section 7 wall-adjacent count=%d bounds=%s" % (s7_adj_count, _aabb_str(s7_adj_aabb)))
    if s6_adj_count < 1:
        _log("Section 6 wall-adjacent=MISSING; South is not required to cover a non-existent adjacent S6 Y range")
else:
    extra_blockers = []
    for actor in actor_sub.get_all_level_actors():
        label = actor.get_actor_label()
        if _is_ignored_entrance_shell(label):
            continue
        if _collision_enabled(actor) == "NO_COLLISION":
            continue
        if _collision_profile(actor) == "NoCollision":
            continue
        if _aabb_overlaps(_aabb_from_actor(actor), walk_entrance, eps=0.5):
            extra_blockers.append("%s topZ=%.3f" % (label, _aabb_from_actor(actor)[1][2]))
    if extra_blockers:
        _fail(
            "Precondition failed: %s missing and entrance still blocked by: %s. ZERO modifications."
            % (PERIMETER_EAST_LABEL, ",".join(extra_blockers))
        )
    _fail("Precondition failed: %s missing and no perimeter replacements found. ZERO modifications." % PERIMETER_EAST_LABEL)

# ---------------------------------------------------------------------------
# 3. Spatial conflicts against verified S6–S9
# ---------------------------------------------------------------------------
s10_connection = set([
    "Admin_Operations_Threshold",
    "Admin_Operations_Connector_Floor",
    "Admin_Operations_Connector_Ceiling",
    "Admin_Operations_Connector_Wall_North",
    "Admin_Operations_Connector_Wall_South",
    "Admin_Operations_Wall_West_North",
    "Admin_Operations_Wall_West_South",
    "Admin_Operations_Wall_West_Header",
])
conflict = []
s6_actor_aabbs = _section_actor_aabbs(actor_sub, _is_s6_label)
s7_actor_aabbs = _section_actor_aabbs(actor_sub, _is_s7_label)
for spec in s10_specs:
    aabb = _aabb_from_loc_scale(spec[1], spec[3])
    if spec[0] not in s10_connection:
        for label, other in s6_actor_aabbs:
            if _aabb_overlaps(aabb, other):
                conflict.append("%s overlaps Section 6 actor %s" % (spec[0], label))
        for label, other in s7_actor_aabbs:
            if _aabb_overlaps(aabb, other):
                conflict.append("%s overlaps Section 7 actor %s" % (spec[0], label))
    if s8_aabb and _aabb_overlaps(aabb, s8_aabb):
        conflict.append("%s overlaps Section 8" % spec[0])
    if s9_aabb and _aabb_overlaps(aabb, s9_aabb):
        conflict.append("%s overlaps Section 9" % spec[0])
if s8_aabb and _aabb_overlaps(room_aabb, s8_aabb, eps=5.0):
    # Room envelope includes connector Y width only; fail if the 1600x1200 body hits Conference.
    body = (
        (floor_extents[0], floor_extents[2], 0.0),
        (floor_extents[1], floor_extents[3], CEILING_Z),
    )
    if _aabb_overlaps(body, s8_aabb, eps=5.0):
        conflict.append("Operations interior overlaps Conference")
if conflict:
    _fail("Precondition failed: spatial conflict. ZERO modifications. " + " | ".join(conflict))

unexpected = []
for label, actors in by_label.items():
    if label.startswith(S10_PREFIX) and label not in expected_s10_labels:
        unexpected.append(label)
    if label.startswith("Admin_S10_") and label not in expected_s10_labels:
        unexpected.append(label)
if unexpected:
    _fail(
        "Precondition failed: unexpected Operations labels already exist. ZERO modifications. labels=%s"
        % ",".join(sorted(set(unexpected)))
    )

repl_to_create, repl_already_ok, repl_mismatch = _plan_creates(by_label, repl_specs)
s10_to_create, s10_already_ok, s10_mismatch = _plan_creates(by_label, s10_specs)
if repl_mismatch or s10_mismatch:
    _fail(
        "Precondition failed: existing labels conflict with intended spec. ZERO modifications. "
        + " | ".join(repl_mismatch + s10_mismatch)
    )

_log("=== SECTION 10 PRECONDITIONS PASSED ===")
_log("will create perimeter-split=%d s10=%d; already-ok perimeter-split=%d s10=%d" % (
    len(repl_to_create), len(s10_to_create), len(repl_already_ok), len(s10_already_ok)
))

cube = unreal.EditorAssetLibrary.load_asset(CUBE_PATH)
if not cube:
    raise RuntimeError("Missing mesh: " + CUBE_PATH)

# ---------------------------------------------------------------------------
# EXECUTION — S10 first, then replacement pieces, original wall last
# ---------------------------------------------------------------------------
created = []
created.extend(_spawn_specs(actor_sub, cube, s10_to_create))
for label in s10_already_ok:
    _log("exists, skip spawn: %s" % label)

s10_errors, s10_rows, s10_union, by_label = _verify_specs_present(actor_sub, s10_specs)
if s10_errors:
    _fail("Section 10 failed validation. Original %s NOT deleted. " % PERIMETER_EAST_LABEL + " | ".join(s10_errors))
if s8_aabb and s10_union and _aabb_overlaps(s10_union, s8_aabb, eps=5.0):
    body = (
        (floor_extents[0], floor_extents[2], 0.0),
        (floor_extents[1], floor_extents[3], CEILING_Z),
    )
    if _aabb_overlaps(body, s8_aabb, eps=5.0):
        _fail("Section 10 interior overlaps Section 8 after spawn. Original %s NOT deleted." % PERIMETER_EAST_LABEL)
if s9_aabb and s10_union and _aabb_overlaps(s10_union, s9_aabb, eps=1.0):
    _fail("Section 10 overlaps Section 9 after spawn. Original %s NOT deleted." % PERIMETER_EAST_LABEL)

created.extend(_spawn_specs(actor_sub, cube, repl_to_create))
for label in repl_already_ok:
    _log("exists, skip spawn: %s" % label)
if repl_specs:
    repl_errors, repl_rows, repl_union, by_label = _verify_specs_present(actor_sub, repl_specs)
    if repl_errors:
        _fail("Perimeter replacements failed validation. Original %s NOT deleted. " % PERIMETER_EAST_LABEL + " | ".join(repl_errors))
    footprint_live = _verify_replacement_footprint(repl_specs, orig_aabb, hub_door_aabb, s6_adj_aabb, s7_adj_aabb)
    if footprint_live:
        _fail("Perimeter footprint failed after spawn. Original %s NOT deleted. " % PERIMETER_EAST_LABEL + " | ".join(footprint_live))

    pre_delete_ignore = _walkable_ignore_labels() | set([PERIMETER_EAST_LABEL])
    pre_blockers, pre_walk = _blocking_walkable_opening(actor_sub, entrance_aabb, pre_delete_ignore)
    _log("pre-delete walkable entrance AABB: %s" % _aabb_str(pre_walk))
    if pre_blockers:
        _fail(
            "Entrance still blocked by replacement/other geometry. Original %s NOT deleted. blockers=%s"
            % (PERIMETER_EAST_LABEL, ",".join(pre_blockers))
        )

deleted_original = False
if original_wall is not None and original_wall_label and not original_already_migrated:
    _assert_admin_map("pre-delete-perimeter-east-wall")
    still = [a for a in actor_sub.get_all_level_actors() if a.get_actor_label() == original_wall_label]
    if len(still) != 1:
        _fail("Refusing to delete: %s count changed to %d before delete." % (original_wall_label, len(still)))
    destroyed = actor_sub.destroy_actor(still[0])
    if not destroyed:
        _fail("destroy_actor returned false for %s." % original_wall_label)
    deleted_original = True
    _log("deleted original %s after replacement and entrance validation" % original_wall_label)
else:
    _log("no %s delete performed" % PERIMETER_EAST_LABEL)

# ---------------------------------------------------------------------------
# Final verification
# ---------------------------------------------------------------------------
_assert_admin_map("pre-final-verify")
by_label = _collect_by_label(actor_sub)
final_errors = []

if original_wall_label and by_label.get(original_wall_label):
    final_errors.append("original %s still exists after split" % original_wall_label)

for label in PERIMETER_REPLACEMENT_LABELS:
    found = by_label.get(label, [])
    if len(found) != 1:
        final_errors.append("missing perimeter replacement %s count=%d" % (label, len(found)))

ignore_entrance = _walkable_ignore_labels()
blockers, walk_aabb = _blocking_walkable_opening(actor_sub, entrance_aabb, ignore_entrance)
_log("walkable entrance AABB: %s" % _aabb_str(walk_aabb))
_log("walkable Z starts at %.3f (expected floor top %.3f + clearance %.3f)" % (
    WALKABLE_Z0, EXPECTED_FLOOR_TOP_Z, WALKABLE_CLEARANCE_Z
))
if blockers:
    final_errors.append("walkable Operations entrance still blocked by: " + ",".join(blockers))

south_actor = by_label.get(PERIMETER_SOUTH_LABEL, [None])[0]
north_actor = by_label.get(PERIMETER_NORTH_LABEL, [None])[0]
header_actor = by_label.get(PERIMETER_HEADER_LABEL, [None])[0]
if south_actor and north_actor and header_actor:
    south_aabb = _aabb_from_actor(south_actor)
    north_aabb = _aabb_from_actor(north_actor)
    header_aabb = _aabb_from_actor(header_actor)
    if abs(header_aabb[0][2] - DOOR_HEIGHT) > 1.0:
        final_errors.append("header does not sit only above Z=%.1f" % DOOR_HEIGHT)
    if s6_adj_aabb and (south_aabb[0][1] - s6_adj_aabb[0][1] > 1.0 or s6_adj_aabb[1][1] - south_aabb[1][1] > 1.0):
        final_errors.append("south replacement no longer covers wall-adjacent Section 6")
    if s7_adj_aabb and (north_aabb[0][1] - s7_adj_aabb[0][1] > 1.0 or s7_adj_aabb[1][1] - north_aabb[1][1] > 1.0):
        final_errors.append("north replacement no longer covers wall-adjacent Section 7")

ops_labels = [label for label in by_label if label.startswith(S10_PREFIX)]
dupes = [label for label in ops_labels if len(by_label.get(label, [])) > 1]
if dupes:
    final_errors.append("duplicate Admin_Operations_* labels: " + ",".join(sorted(set(dupes))))
extra_ops = [label for label in ops_labels if label not in expected_s10_labels]
if extra_ops:
    final_errors.append("unexpected Admin_Operations_* labels: " + ",".join(sorted(set(extra_ops))))
if len(ops_labels) != len(expected_s10_labels) and not extra_ops and not dupes:
    missing = [label for label in expected_s10_labels if label not in ops_labels]
    if missing:
        final_errors.append("missing Section 10 labels: " + ",".join(missing))

transit = by_label.get("Admin_Operations_ToTransit", [None])[0]
if not transit:
    final_errors.append("missing sealed Section 11 connection Admin_Operations_ToTransit")
else:
    if _collision_enabled(transit) == "NO_COLLISION":
        final_errors.append("Admin_Operations_ToTransit must block until Section 11 is built")

missing_stations = [label for label in STATION_LABELS if len(by_label.get(label, [])) != 1]
if missing_stations:
    final_errors.append("missing stations: " + ",".join(missing_stations))
if len(by_label.get("Admin_Operations_CentralDisplay", [])) != 1:
    final_errors.append("missing Admin_Operations_CentralDisplay")

if any(label.startswith("Admin_Transit") or label.startswith("Admin_S11_") for label in by_label):
    final_errors.append("Section 11 Transit Control was built; it must not be")

s6_after = _snapshot_group(actor_sub, S6_SNAPSHOT_PREFIXES, S6_SNAPSHOT_EXACT)
s7_after = _snapshot_group(actor_sub, S7_SNAPSHOT_PREFIXES, S7_SNAPSHOT_EXACT)
s8_after = _snapshot_group(actor_sub, (S8_PREFIX,), ())
s9_after = _snapshot_group(actor_sub, (S9_PREFIX,), ())
if s6_after != s6_before:
    final_errors.append("Section 6 actors changed during Section 10 build")
if s7_after != s7_before:
    final_errors.append("Section 7 actors changed during Section 10 build")
if s8_after != s8_before:
    final_errors.append("Section 8 actors changed during Section 10 build")
if s9_after != s9_before:
    final_errors.append("Section 9 actors changed during Section 10 build")

for spec in s10_specs:
    found = by_label.get(spec[0], [])
    if len(found) != 1:
        final_errors.append("%s count=%d after build" % (spec[0], len(found)))
        continue
    if _actor_owner_package(found[0]) != REQUIRED_PACKAGE:
        final_errors.append("%s owner=%s" % (spec[0], _actor_owner_package(found[0])))
    if _actor_owner_package(found[0]) == MENU_PACKAGE:
        final_errors.append("%s owned by Lvl_MainMenu" % spec[0])

if final_errors:
    _log("=== SECTION 10 VERIFICATION FAILED ===")
    _fail(" | ".join(final_errors))

_, world_pkg = _assert_admin_map("pre-save")
if world_pkg != REQUIRED_PACKAGE:
    _fail("Refusing to save: current package is %s" % world_pkg)
saved = level_sub.save_current_level()
_log("saved current level only; package=%s result=%s" % (world_pkg, _safe_str(saved)))
_log("Lvl_MainMenu was not saved")

_log("=== SECTION 10 OPERATIONS CONTROL VERIFIED IN SL_EPITOPE_ADMIN ===")
_log("Section 10 actor count=%d" % len(s10_specs))
_log("created=%d already-existing perimeter-split=%d already-existing s10=%d" % (
    len(created), len(repl_already_ok), len(s10_already_ok)
))
_log("perimeter wall deleted=%s replacements=%s" % (deleted_original, ",".join(PERIMETER_REPLACEMENT_LABELS)))
_log("interior=%.0f x %.0f ceiling=%.0f" % (ROOM_SIZE_X, ROOM_SIZE_Y, CEILING_Z))
_log("entrance door=%.0f x %.0f from hub east X=%.1f" % (DOOR_WIDTH, DOOR_HEIGHT, hub_east_x))
_log("sealed S11 connection=Admin_Operations_ToTransit at east wall")
_log("stations=%s" % ",".join(STATION_LABELS))
_log("central=Admin_Operations_CentralDisplay")
_log("Section 6/7/8/9 snapshots unchanged. Section 11 was not built. Lvl_MainMenu was not saved.")
_log("=== SECTION 10 OPERATIONS CONTROL COMPLETE ===")
