# ProjectOrganoid — Section 9 Director Suite + guarded S8 west-wall doorway migration.
# Does not touch Lvl_MainMenu. Does not build Section 10.
# Original Admin_ConferenceRoom_Wall_NegX is removed ONLY after replacement pieces
# and Section 9 geometry are created and validated.

import unreal

REQUIRED_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
MENU_PACKAGE = "/Game/Maps/Lvl_MainMenu"
CUBE_PATH = "/Engine/BasicShapes/Cube.Cube"
CUBE_HALF = 50.0
TRANSFORM_EPS = 0.05
SPAN_EPS = 0.05
S9_PREFIX = "Admin_DirectorSuite_"

ORIGINAL_WALL_LABEL = "Admin_ConferenceRoom_Wall_NegX"
WALL_SOUTH_LABEL = "Admin_ConferenceRoom_Wall_NegX_South"
WALL_NORTH_LABEL = "Admin_ConferenceRoom_Wall_NegX_North"
WALL_HEADER_LABEL = "Admin_ConferenceRoom_Wall_NegX_Header"
REPLACEMENT_LABELS = (WALL_SOUTH_LABEL, WALL_NORTH_LABEL, WALL_HEADER_LABEL)

# Verified recovered Section 8 west wall.
VERIFIED_WALL_LOC = (2487.5, -1850.0, 225.0)
VERIFIED_WALL_ROT = (0.0, 0.0, 0.0)
VERIFIED_WALL_SCALE = (0.25, 12.0, 4.5)

DOOR_WIDTH = 140.0
DOOR_HEIGHT = 240.0
ORIGINAL_SPAN_Y = 1200.0
ORIGINAL_HEIGHT = 450.0
WALL_T = 25.0

ROOM_SIZE_X = 1000.0
ROOM_SIZE_Y = 800.0
FLOOR_Z = 10.0
CEILING_Z = 460.0
WALL_Z = 225.0
WALL_H = 4.5
FLOOR_SLAB_SCALE_Z = 0.20
WALKABLE_CLEARANCE_Z = 5.0
EXPECTED_FLOOR_TOP_Z = FLOOR_Z + FLOOR_SLAB_SCALE_Z * CUBE_HALF
WALKABLE_Z0 = EXPECTED_FLOOR_TOP_Z + WALKABLE_CLEARANCE_Z

S6_SNAPSHOT_PREFIXES = ("Admin_SecurityOffice_", "Admin_S6_")
S6_SNAPSHOT_EXACT = ("Terminal_AdminSecurity",)
S7_SNAPSHOT_EXACT = ("Admin_RecordsArchives_Block",)


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
        _log("=== SECTION 9 ABORTED — WRONG MAP ===")
        _log("Expected: %s" % REQUIRED_PACKAGE)
        _log("Actual: %s" % pkg)
        _log("stage=%s ZERO actor writes, deletes, or saves." % stage)
        raise RuntimeError(
            "SECTION 9 aborted at %s: current package '%s', expected '%s'"
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


def _xyz_close(actual_xyz, expected_xyz):
    return (
        abs(actual_xyz[0] - expected_xyz[0]) <= TRANSFORM_EPS
        and abs(actual_xyz[1] - expected_xyz[1]) <= TRANSFORM_EPS
        and abs(actual_xyz[2] - expected_xyz[2]) <= TRANSFORM_EPS
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
    _log("=== SECTION 9 ABORTED ===")
    _log(message)
    raise RuntimeError(message)


def _solid(label, loc, scale):
    return (label, loc, (0.0, 0.0, 0.0), scale, "BlockAll", "QUERY_AND_PHYSICS")


def _visual(label, loc, scale):
    return (label, loc, (0.0, 0.0, 0.0), scale, "NoCollision", "NO_COLLISION")


def _build_replacement_specs():
    loc = VERIFIED_WALL_LOC
    scale = VERIFIED_WALL_SCALE
    orig_aabb = _aabb_from_loc_scale(loc, scale)
    center_y = loc[1]
    door_y0 = center_y - DOOR_WIDTH * 0.5
    door_y1 = center_y + DOOR_WIDTH * 0.5
    south_y0 = orig_aabb[0][1]
    south_y1 = door_y0
    north_y0 = door_y1
    north_y1 = orig_aabb[1][1]
    south_len = south_y1 - south_y0
    north_len = north_y1 - north_y0
    header_z0 = DOOR_HEIGHT
    header_z1 = orig_aabb[1][2]
    header_h = header_z1 - header_z0
    specs = [
        _solid(WALL_SOUTH_LABEL, (loc[0], (south_y0 + south_y1) * 0.5, loc[2]), (scale[0], south_len / 100.0, scale[2])),
        _solid(WALL_NORTH_LABEL, (loc[0], (north_y0 + north_y1) * 0.5, loc[2]), (scale[0], north_len / 100.0, scale[2])),
        _solid(WALL_HEADER_LABEL, (loc[0], center_y, (header_z0 + header_z1) * 0.5), (scale[0], DOOR_WIDTH / 100.0, header_h / 100.0)),
    ]
    door_aabb = (
        (orig_aabb[0][0], door_y0, orig_aabb[0][2]),
        (orig_aabb[1][0], door_y1, DOOR_HEIGHT),
    )
    return specs, orig_aabb, door_aabb, south_len, north_len


def _verify_replacement_footprint(repl_specs, orig_aabb, door_aabb, south_len, north_len):
    errors = []
    rebuilt = south_len + DOOR_WIDTH + north_len
    if abs(rebuilt - ORIGINAL_SPAN_Y) > SPAN_EPS:
        errors.append("south+door+north=%.6f expected=%.6f" % (rebuilt, ORIGINAL_SPAN_Y))
    if abs(south_len - north_len) > SPAN_EPS:
        errors.append("south/north jambs are unequal: %.6f vs %.6f" % (south_len, north_len))
    header = None
    south = None
    north = None
    for spec in repl_specs:
        if spec[0] == WALL_HEADER_LABEL:
            header = spec
        elif spec[0] == WALL_SOUTH_LABEL:
            south = spec
        elif spec[0] == WALL_NORTH_LABEL:
            north = spec
    header_aabb = _aabb_from_loc_scale(header[1], header[3])
    south_aabb = _aabb_from_loc_scale(south[1], south[3])
    north_aabb = _aabb_from_loc_scale(north[1], north[3])
    if abs(header_aabb[0][1] - door_aabb[0][1]) > SPAN_EPS or abs(header_aabb[1][1] - door_aabb[1][1]) > SPAN_EPS:
        errors.append("header Y does not match doorway Y")
    if abs(header_aabb[0][2] - DOOR_HEIGHT) > SPAN_EPS:
        errors.append("header does not start at doorway top Z=%.1f" % DOOR_HEIGHT)
    if abs(header_aabb[1][2] - orig_aabb[1][2]) > SPAN_EPS:
        errors.append("header does not reach original wall top")
    if _aabb_overlaps(south_aabb, door_aabb, eps=0.5):
        errors.append("south jamb overlaps doorway")
    if _aabb_overlaps(north_aabb, door_aabb, eps=0.5):
        errors.append("north jamb overlaps doorway")
    if _aabb_overlaps(header_aabb, door_aabb, eps=0.5):
        errors.append("header overlaps doorway walk volume")
    union = _aabb_union(_aabb_union(south_aabb, north_aabb), header_aabb)
    if abs(union[0][0] - orig_aabb[0][0]) > SPAN_EPS or abs(union[1][0] - orig_aabb[1][0]) > SPAN_EPS:
        errors.append("replacement X footprint != original wall")
    if abs(union[0][1] - orig_aabb[0][1]) > SPAN_EPS or abs(union[1][1] - orig_aabb[1][1]) > SPAN_EPS:
        errors.append("replacement Y footprint != original wall")
    if abs(union[1][2] - orig_aabb[1][2]) > SPAN_EPS:
        errors.append("replacement top Z != original wall")
    return errors


def _build_s9_specs(west_face_x, center_y):
    floor_max_x = west_face_x - WALL_T
    floor_min_x = floor_max_x - ROOM_SIZE_X
    floor_cx = (floor_min_x + floor_max_x) * 0.5
    floor_cy = center_y
    floor_min_y = center_y - ROOM_SIZE_Y * 0.5
    floor_max_y = center_y + ROOM_SIZE_Y * 0.5
    door_half = DOOR_WIDTH * 0.5
    door_y0 = center_y - door_half
    door_y1 = center_y + door_half
    east_north_len = floor_max_y - door_y1
    east_south_len = door_y0 - floor_min_y
    east_north_cy = (door_y1 + floor_max_y) * 0.5
    east_south_cy = (floor_min_y + door_y0) * 0.5
    east_x = west_face_x - WALL_T * 0.5
    west_x = floor_min_x - WALL_T * 0.5
    north_y = floor_max_y + WALL_T * 0.5
    south_y = floor_min_y - WALL_T * 0.5
    header_z = (DOOR_HEIGHT + (WALL_Z + WALL_H * CUBE_HALF)) * 0.5
    header_h = (WALL_Z + WALL_H * CUBE_HALF) - DOOR_HEIGHT
    specs = [
        _solid("Admin_DirectorSuite_Floor", (floor_cx, floor_cy, FLOOR_Z), (ROOM_SIZE_X / 100.0, ROOM_SIZE_Y / 100.0, 0.20)),
        _solid("Admin_DirectorSuite_Ceiling", (floor_cx, floor_cy, CEILING_Z), (ROOM_SIZE_X / 100.0, ROOM_SIZE_Y / 100.0, 0.20)),
        _solid("Admin_DirectorSuite_Wall_West", (west_x, floor_cy, WALL_Z), (WALL_T / 100.0, ROOM_SIZE_Y / 100.0, WALL_H)),
        _solid("Admin_DirectorSuite_Wall_North", (floor_cx, north_y, WALL_Z), (ROOM_SIZE_X / 100.0, WALL_T / 100.0, WALL_H)),
        _solid("Admin_DirectorSuite_Wall_South", (floor_cx, south_y, WALL_Z), (ROOM_SIZE_X / 100.0, WALL_T / 100.0, WALL_H)),
        _solid("Admin_DirectorSuite_Wall_East_North", (east_x, east_north_cy, WALL_Z), (WALL_T / 100.0, east_north_len / 100.0, WALL_H)),
        _solid("Admin_DirectorSuite_Wall_East_South", (east_x, east_south_cy, WALL_Z), (WALL_T / 100.0, east_south_len / 100.0, WALL_H)),
        _solid("Admin_DirectorSuite_Wall_East_Header", (east_x, center_y, header_z), (WALL_T / 100.0, DOOR_WIDTH / 100.0, header_h / 100.0)),
        _solid("Admin_DirectorSuite_Threshold", (west_face_x, center_y, FLOOR_Z), (0.50, DOOR_WIDTH / 100.0, 0.20)),
        _solid("Admin_DirectorSuite_Backdrop", (floor_min_x + 25.0, floor_cy, 180.0), (0.20, 5.00, 3.20)),
        _solid("Admin_DirectorSuite_Desk", (floor_cx - 200.0, floor_cy, 55.0), (2.40, 1.20, 1.10)),
        _solid("Admin_DirectorSuite_DirectorChair", (floor_cx - 350.0, floor_cy, 45.0), (0.50, 0.50, 0.90)),
        _solid("Admin_DirectorSuite_VisitorChair_01", (floor_cx - 50.0, floor_cy + 110.0, 45.0), (0.45, 0.45, 0.90)),
        _solid("Admin_DirectorSuite_VisitorChair_02", (floor_cx - 50.0, floor_cy - 110.0, 45.0), (0.45, 0.45, 0.90)),
        _solid("Admin_DirectorSuite_Cabinet_01", (floor_cx + 150.0, floor_min_y + 50.0, 80.0), (1.40, 0.50, 1.60)),
        _solid("Admin_DirectorSuite_Cabinet_02", (floor_cx + 350.0, floor_min_y + 50.0, 80.0), (1.40, 0.50, 1.60)),
        _visual("Admin_DirectorSuite_Glass_North", (floor_cx, floor_max_y - 15.0, 220.0), (5.00, 0.08, 2.40)),
    ]
    room_aabb = (
        (floor_min_x - WALL_T, floor_min_y - WALL_T, 0.0),
        (west_face_x + WALL_T, floor_max_y + WALL_T, CEILING_Z + 10.0),
    )
    s9_door_aabb = (
        (floor_max_x, door_y0, 0.0),
        (west_face_x, door_y1, DOOR_HEIGHT),
    )
    return specs, room_aabb, s9_door_aabb


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
        scale = actor.get_actor_scale3d()
        rows.append((
            label,
            _safe_str(_safe_call(actor, "get_path_name")),
            (loc.x, loc.y, loc.z),
            (scale.x, scale.y, scale.z),
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
            _fail("Failed to spawn " + spec[0] + ". Original Wall_NegX was not deleted.")
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
    # Same X/Y as the wall cut. Z starts above the expected floor/sill so a
    # normal floor slab at the walking surface is not treated as an obstruction.
    return (
        (door_aabb[0][0], door_aabb[0][1], WALKABLE_Z0),
        (door_aabb[1][0], door_aabb[1][1], door_aabb[1][2]),
    )


def _blocking_walkable_opening(actor_sub, door_aabb):
    walk_aabb = _walkable_opening_aabb(door_aabb)
    blockers = []
    for actor in actor_sub.get_all_level_actors():
        if _collision_enabled(actor) == "NO_COLLISION":
            continue
        profile = _collision_profile(actor)
        if profile == "NoCollision":
            continue
        actor_aabb = _aabb_from_actor(actor)
        if _aabb_overlaps(actor_aabb, walk_aabb, eps=0.5):
            blockers.append("%s topZ=%.3f" % (actor.get_actor_label(), actor_aabb[1][2]))
    return blockers, walk_aabb


# ---------------------------------------------------------------------------
# 1. Map guard — zero writes if not exactly SL_Epitope_Admin
# ---------------------------------------------------------------------------
world, world_pkg = _assert_admin_map("precondition-package")
_log("SECTION 9 map guard passed: %s" % world_pkg)

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not actor_sub or not level_sub:
    raise RuntimeError("EditorActorSubsystem / LevelEditorSubsystem unavailable")

by_label = _collect_by_label(actor_sub)
s6_before = _snapshot_group(actor_sub, S6_SNAPSHOT_PREFIXES, S6_SNAPSHOT_EXACT)
s7_before = _snapshot_group(actor_sub, (), S7_SNAPSHOT_EXACT)

_log("=== SECTION 9 PRE-BUILD SPATIAL AUDIT ===")
s6_aabb, s6_count = _group_aabb(actor_sub, S6_SNAPSHOT_PREFIXES, S6_SNAPSHOT_EXACT)
s7_aabb, s7_count = _group_aabb(actor_sub, (), S7_SNAPSHOT_EXACT)
s8_aabb, s8_count = _group_aabb(actor_sub, ("Admin_ConferenceRoom_",))
hub_aabb, hub_count = _group_aabb(actor_sub, ("Admin_Hub_", "Admin_S5_"), ("Admin_Hub_Floor", "Admin_Hub_Opening_Cut"))
_log("Section 6 count=%d bounds=%s" % (s6_count, _aabb_str(s6_aabb)))
_log("Section 7 count=%d bounds=%s" % (s7_count, _aabb_str(s7_aabb)))
_log("Section 8 count=%d bounds=%s" % (s8_count, _aabb_str(s8_aabb)))
_log("Hub-related count=%d bounds=%s" % (hub_count, _aabb_str(hub_aabb)))

# ---------------------------------------------------------------------------
# 2–3. Original west wall exists once and matches verified S8 spec
# ---------------------------------------------------------------------------
originals = by_label.get(ORIGINAL_WALL_LABEL, [])
if len(originals) != 1:
    # Rerun: replacements already in place and original already removed.
    have_replacements = all(len(by_label.get(label, [])) == 1 for label in REPLACEMENT_LABELS)
    if len(originals) == 0 and have_replacements:
        _log("Original %s already removed; replacement pieces present. Skipping wall delete." % ORIGINAL_WALL_LABEL)
        original_wall = None
        original_already_migrated = True
    else:
        _fail(
            "Precondition failed: %s count=%d expected=1. ZERO modifications."
            % (ORIGINAL_WALL_LABEL, len(originals))
        )
else:
    original_already_migrated = False
    original_wall = originals[0]

verified_wall_spec = _solid(ORIGINAL_WALL_LABEL, VERIFIED_WALL_LOC, VERIFIED_WALL_SCALE)
if original_wall is not None:
    wall_reasons = _matches_spec(original_wall, verified_wall_spec)
    if wall_reasons:
        _fail(
            "Precondition failed: %s does not match verified Section 8 spec. ZERO modifications. %s"
            % (ORIGINAL_WALL_LABEL, "; ".join(wall_reasons))
        )
    _log(
        "precondition wall match loc=%s scale=%s owner=%s mesh=%s collision=%s/%s"
        % (
            _fmt_vec(original_wall.get_actor_location()),
            _fmt_vec(original_wall.get_actor_scale3d()),
            _actor_owner_package(original_wall),
            _mesh_path(original_wall),
            _collision_profile(original_wall),
            _collision_enabled(original_wall),
        )
    )

repl_specs, orig_aabb, door_aabb, south_len, north_len = _build_replacement_specs()
footprint_errors = _verify_replacement_footprint(repl_specs, orig_aabb, door_aabb, south_len, north_len)
if footprint_errors:
    _fail("Precondition failed: replacement pieces do not reconstruct original footprint. ZERO modifications. " + " | ".join(footprint_errors))
_log("precondition footprint south=%.2f door=%.2f north=%.2f total=%.2f header_above_z=%.1f" % (
    south_len, DOOR_WIDTH, north_len, south_len + DOOR_WIDTH + north_len, DOOR_HEIGHT
))

west_face_x = VERIFIED_WALL_LOC[0] - abs(VERIFIED_WALL_SCALE[0]) * CUBE_HALF
center_y = VERIFIED_WALL_LOC[1]
s9_specs, room_aabb, s9_door_aabb = _build_s9_specs(west_face_x, center_y)
expected_s9_labels = [spec[0] for spec in s9_specs]
s8_connection_labels = set(REPLACEMENT_LABELS + (ORIGINAL_WALL_LABEL,))
s9_connection_labels = set([
    "Admin_DirectorSuite_Wall_East_North",
    "Admin_DirectorSuite_Wall_East_South",
    "Admin_DirectorSuite_Wall_East_Header",
    "Admin_DirectorSuite_Threshold",
])

_log("proposed Section 9 bounds: %s" % _aabb_str(room_aabb))
_log("proposed Section 9 actor count=%d" % len(s9_specs))
_log("doorway walk AABB S8: %s" % _aabb_str(door_aabb))
_log("NOCOLLISION visual: Admin_DirectorSuite_Glass_North only")

# ---------------------------------------------------------------------------
# 5. S9 clear of S6/S7; no unintended S8 overlap
# ---------------------------------------------------------------------------
conflict = []
for spec in s9_specs:
    label, loc_xyz, rot_pyr, scale_xyz, profile, enabled = spec
    aabb = _aabb_from_loc_scale(loc_xyz, scale_xyz)
    if s6_aabb and _aabb_overlaps(aabb, s6_aabb):
        conflict.append("%s overlaps Section 6" % label)
    if s7_aabb and _aabb_overlaps(aabb, s7_aabb):
        conflict.append("%s overlaps Section 7" % label)
    if s8_aabb and _aabb_overlaps(aabb, s8_aabb) and label not in s9_connection_labels:
        conflict.append("%s overlaps Section 8 (not connection geometry)" % label)
for spec in repl_specs:
    aabb = _aabb_from_loc_scale(spec[1], spec[3])
    if s6_aabb and _aabb_overlaps(aabb, s6_aabb):
        conflict.append("%s overlaps Section 6" % spec[0])
    if s7_aabb and _aabb_overlaps(aabb, s7_aabb):
        conflict.append("%s overlaps Section 7" % spec[0])
if conflict:
    _fail("Precondition failed: spatial conflict. ZERO modifications. " + " | ".join(conflict))

# ---------------------------------------------------------------------------
# 6. No conflicting existing replacement / DirectorSuite labels
# ---------------------------------------------------------------------------
unexpected_s9 = []
for label, actors in by_label.items():
    if label.startswith(S9_PREFIX) and label not in expected_s9_labels:
        unexpected_s9.append(label)
if unexpected_s9:
    _fail(
        "Precondition failed: unexpected Admin_DirectorSuite_* labels already exist. ZERO modifications. labels=%s"
        % ",".join(sorted(set(unexpected_s9)))
    )

repl_to_create, repl_already_ok, repl_mismatch = _plan_creates(by_label, repl_specs)
s9_to_create, s9_already_ok, s9_mismatch = _plan_creates(by_label, s9_specs)
if repl_mismatch or s9_mismatch:
    _fail(
        "Precondition failed: existing labels conflict with intended spec. ZERO modifications. "
        + " | ".join(repl_mismatch + s9_mismatch)
    )

_log("=== CLEANUP PREFLIGHT / SECTION 9 PRECONDITIONS PASSED ===")
_log("will create replacements=%d s9=%d; already-ok replacements=%d s9=%d" % (
    len(repl_to_create), len(s9_to_create), len(repl_already_ok), len(s9_already_ok)
))
_log("original wall delete deferred until replacement+S9 validation")

cube = unreal.EditorAssetLibrary.load_asset(CUBE_PATH)
if not cube:
    raise RuntimeError("Missing mesh: " + CUBE_PATH)

# ---------------------------------------------------------------------------
# EXECUTION: create replacements + S9 FIRST, then delete original wall
# ---------------------------------------------------------------------------
created = []
created.extend(_spawn_specs(actor_sub, cube, repl_to_create))
for label in repl_already_ok:
    _log("exists, skip spawn: %s" % label)

repl_errors, repl_rows, repl_union, by_label = _verify_specs_present(actor_sub, repl_specs)
if repl_errors:
    _fail("Replacement wall pieces failed validation. Original Wall_NegX NOT deleted. " + " | ".join(repl_errors))
_log("replacement pieces validated; original %s still present=%s" % (
    ORIGINAL_WALL_LABEL, original_wall is not None and not original_already_migrated
))

created.extend(_spawn_specs(actor_sub, cube, s9_to_create))
for label in s9_already_ok:
    _log("exists, skip spawn: %s" % label)

s9_errors, s9_rows, s9_union, by_label = _verify_specs_present(actor_sub, s9_specs)
if s9_errors:
    _fail("Section 9 failed validation. Original Wall_NegX NOT deleted. " + " | ".join(s9_errors))
if s6_aabb and s9_union and _aabb_overlaps(s9_union, s6_aabb):
    _fail("Section 9 overlaps Section 6 after spawn. Original Wall_NegX NOT deleted.")
if s7_aabb and s9_union and _aabb_overlaps(s9_union, s7_aabb):
    _fail("Section 9 overlaps Section 7 after spawn. Original Wall_NegX NOT deleted.")
_log("Section 9 geometry validated; proceeding to original-wall removal")

# ---------------------------------------------------------------------------
# Remove original wall only after replacement + S9 are validated
# ---------------------------------------------------------------------------
deleted_original = False
if original_wall is not None and not original_already_migrated:
    _assert_admin_map("pre-delete-original-wall")
    still = [a for a in actor_sub.get_all_level_actors() if a.get_actor_label() == ORIGINAL_WALL_LABEL]
    if len(still) != 1:
        _fail("Refusing to delete: %s count changed to %d before delete." % (ORIGINAL_WALL_LABEL, len(still)))
    destroyed = actor_sub.destroy_actor(still[0])
    if not destroyed:
        _fail("destroy_actor returned false for %s. Replacement/S9 actors were created; original wall may still exist." % ORIGINAL_WALL_LABEL)
    deleted_original = True
    _log("deleted original %s after replacement validation" % ORIGINAL_WALL_LABEL)
else:
    _log("original wall already absent; no delete performed")

# ---------------------------------------------------------------------------
# Final verification
# ---------------------------------------------------------------------------
_assert_admin_map("pre-final-verify")
by_label = _collect_by_label(actor_sub)
final_errors = []

if by_label.get(ORIGINAL_WALL_LABEL):
    final_errors.append("original %s still exists after migration" % ORIGINAL_WALL_LABEL)

blockers, walk_aabb = _blocking_walkable_opening(actor_sub, door_aabb)
_log("walkable opening AABB: %s" % _aabb_str(walk_aabb))
_log("walkable Z starts at %.3f (expected floor top %.3f + clearance %.3f)" % (
    WALKABLE_Z0, EXPECTED_FLOOR_TOP_Z, WALKABLE_CLEARANCE_Z
))
if blockers:
    final_errors.append("walkable doorway still blocked by: " + ",".join(blockers))

south_actor = by_label.get(WALL_SOUTH_LABEL, [None])[0]
north_actor = by_label.get(WALL_NORTH_LABEL, [None])[0]
header_actor = by_label.get(WALL_HEADER_LABEL, [None])[0]
if not south_actor or not north_actor or not header_actor:
    final_errors.append("missing replacement wall piece after migration")
else:
    south_aabb = _aabb_from_actor(south_actor)
    north_aabb = _aabb_from_actor(north_actor)
    header_aabb = _aabb_from_actor(header_actor)
    span = _span_y(south_aabb) + DOOR_WIDTH + _span_y(north_aabb)
    if abs(span - ORIGINAL_SPAN_Y) > SPAN_EPS:
        final_errors.append("south+doorway+north=%.6f expected=%.6f" % (span, ORIGINAL_SPAN_Y))
    if _aabb_overlaps(south_aabb, door_aabb, eps=0.5) or _aabb_overlaps(north_aabb, door_aabb, eps=0.5):
        final_errors.append("jamb still occupies doorway")
    if _aabb_overlaps(header_aabb, door_aabb, eps=0.5):
        final_errors.append("header occupies doorway walk volume")
    if abs(header_aabb[0][2] - DOOR_HEIGHT) > 1.0:
        final_errors.append("header does not sit only above Z=%.1f" % DOOR_HEIGHT)

threshold = by_label.get("Admin_DirectorSuite_Threshold", [None])[0]
east_gap = by_label.get("Admin_DirectorSuite_Wall_East_North", [None])[0]
if not threshold:
    final_errors.append("missing traversable threshold in doorway")
if not east_gap:
    final_errors.append("missing Section 9 east opening")

s6_after = _snapshot_group(actor_sub, S6_SNAPSHOT_PREFIXES, S6_SNAPSHOT_EXACT)
s7_after = _snapshot_group(actor_sub, (), S7_SNAPSHOT_EXACT)
if s6_after != s6_before:
    final_errors.append("Section 6 actors changed during Section 9 build")
if s7_after != s7_before:
    final_errors.append("Section 7 actors changed during Section 9 build")

for spec in s9_specs:
    found = by_label.get(spec[0], [])
    if len(found) != 1:
        final_errors.append("%s count=%d after migration" % (spec[0], len(found)))
        continue
    if _actor_owner_package(found[0]) != REQUIRED_PACKAGE:
        final_errors.append("%s owner=%s" % (spec[0], _actor_owner_package(found[0])))
    if _actor_owner_package(found[0]) == MENU_PACKAGE:
        final_errors.append("%s owned by Lvl_MainMenu" % spec[0])

if s6_aabb and s9_union and _aabb_overlaps(s9_union, s6_aabb):
    final_errors.append("final Section 9 overlaps Section 6")
if s7_aabb and s9_union and _aabb_overlaps(s9_union, s7_aabb):
    final_errors.append("final Section 9 overlaps Section 7")

if final_errors:
    _log("=== SECTION 9 VERIFICATION FAILED ===")
    _fail(" | ".join(final_errors))

_assert_admin_map("pre-save")
saved = level_sub.save_current_level()
_log("saved SL_Epitope_Admin=%s" % _safe_str(saved))

_log("=== SECTION 9 DIRECTOR SUITE VERIFIED IN SL_EPITOPE_ADMIN ===")
_log("Section 9 actor count=%d" % len(s9_specs))
_log("created=%d already-existing replacements=%d already-existing s9=%d" % (
    len(created), len(repl_already_ok), len(s9_already_ok)
))
_log("original wall deleted=%s" % deleted_original)
_log("doorway width=%.1f height=%.1f center=%s" % (DOOR_WIDTH, DOOR_HEIGHT, _fmt_xyz((VERIFIED_WALL_LOC[0], center_y, DOOR_HEIGHT * 0.5))))
_log("south+door+north=%.2f" % (south_len + DOOR_WIDTH + north_len))
_log("Section 9 bounds: %s" % _aabb_str(s9_union))
_log("Glass_North is NoCollision / NO_COLLISION; doorway has no solid door cube")
_log("Section 6 and Section 7 snapshots unchanged. Lvl_MainMenu was not saved.")
