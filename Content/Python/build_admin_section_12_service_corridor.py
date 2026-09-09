# ProjectOrganoid — Section 12 Service Corridor for SL_Epitope_Admin.
# Operations-south service leg only. Does not open Transit east.
# Does not build Section 13 Blueprint Architecture.
# Does not touch Lvl_MainMenu.
# Do not import this module from another script; execution starts on load.

import unreal

REQUIRED_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
MENU_PACKAGE = "/Game/Maps/Lvl_MainMenu"
CUBE_PATH = "/Engine/BasicShapes/Cube.Cube"
CUBE_HALF = 50.0
TRANSFORM_EPS = 0.05
S12_PREFIX = "Admin_ServiceCorridor_"
S11_PREFIX = "Admin_Transit_"
S10_PREFIX = "Admin_Operations_"
S9_PREFIX = "Admin_DirectorSuite_"
S8_PREFIX = "Admin_ConferenceRoom_"
ORIGINAL_SOUTH = "Admin_Operations_Wall_South"
SOUTH_WEST = "Admin_Operations_Wall_South_West"
SOUTH_EAST = "Admin_Operations_Wall_South_East"
SOUTH_HEADER = "Admin_Operations_Wall_South_Header"
S10_EAST_NORTH = "Admin_Operations_Wall_East_North"
S10_EAST_SOUTH = "Admin_Operations_Wall_East_South"
S10_EAST_HEADER = "Admin_Operations_Wall_East_Header"
S11_EAST = "Admin_Transit_Wall_East"
S11_SOUTH = "Admin_Transit_Wall_South"
S11_NORTH = "Admin_Transit_Wall_North"

EXPECTED_ORIGINAL_LOC = (3705.0, -612.5, 225.0)
EXPECTED_ORIGINAL_SCALE = (16.0, 0.25, 4.5)
EXPECTED_ORIGINAL_AABB = ((2905.0, -625.0, 0.0), (4505.0, -600.0, 450.0))
DOOR_X0 = 3870.0
DOOR_X1 = 4130.0
DOOR_Y0 = -625.0
DOOR_Y1 = -600.0
DOOR_WIDTH = 260.0
DOOR_HEIGHT = 300.0
DOOR_CENTER = (4000.0, -612.5, 150.0)
WALKABLE_OPENING = ((3870.0, -650.0, 25.0), (4130.0, -575.0, 300.0))
S12_ENVELOPE = ((2920.0, -1225.0, 0.0), (4505.0, -625.0, 480.0))
FLOOR_Z = 10.0
CEILING_Z = 470.0
WALKABLE_Z0 = 25.0
EXPECTED_FLOOR_TOP_Z = 20.0

S6_SNAPSHOT_PREFIXES = ("Admin_SecurityOffice_", "Admin_S6_")
S6_SNAPSHOT_EXACT = ("Terminal_AdminSecurity",)
S7_SNAPSHOT_PREFIXES = ("Admin_RecordsArchives_", "Admin_S7_")
S7_SNAPSHOT_EXACT = ("Admin_RecordsArchives_Block",)

S12_SPECS = (
    ("Admin_ServiceCorridor_Floor", (3712.5, -912.5, 10.0), (15.35, 5.75, 0.20)),
    ("Admin_ServiceCorridor_Ceiling", (3712.5, -912.5, 470.0), (15.35, 5.75, 0.20)),
    ("Admin_ServiceCorridor_Wall_South", (3712.5, -1212.5, 225.0), (15.85, 0.25, 4.50)),
    ("Admin_ServiceCorridor_Wall_West", (2932.5, -925.0, 225.0), (0.25, 6.00, 4.50)),
    ("Admin_ServiceCorridor_Wall_East", (4492.5, -925.0, 225.0), (0.25, 6.00, 4.50)),
    ("Admin_ServiceCorridor_Threshold", (4000.0, -612.5, 10.0), (2.60, 0.50, 0.20)),
    ("Admin_ServiceCorridor_Cabinet_West", (3200.0, -1175.0, 80.0), (1.40, 0.50, 1.60)),
    ("Admin_ServiceCorridor_Cabinet_East", (4300.0, -1175.0, 80.0), (1.40, 0.50, 1.60)),
)
SPLIT_SPECS = (
    (SOUTH_WEST, (3387.5, -612.5, 225.0), (9.65, 0.25, 4.50)),
    (SOUTH_EAST, (4317.5, -612.5, 225.0), (3.75, 0.25, 4.50)),
    (SOUTH_HEADER, (4000.0, -612.5, 375.0), (2.60, 0.25, 1.50)),
)
TRANSIT_SEAL_LABELS = (S11_EAST, S11_SOUTH, S11_NORTH)
S10_EAST_KEEP = (S10_EAST_NORTH, S10_EAST_SOUTH, S10_EAST_HEADER)


def _log(msg):
    unreal.log(msg)


def _fmt_vec(v):
    return "(%.6f, %.6f, %.6f)" % (v.x, v.y, v.z)


def _fmt_rot(r):
    return "(%.6f, %.6f, %.6f)" % (r.pitch, r.yaw, r.roll)


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
        _log("=== SECTION 12 EXECUTION ABORTED — REVIEW REQUIRED ===")
        _log("=== SECTION 12 ABORTED — WRONG MAP ===")
        _log("Expected: %s" % REQUIRED_PACKAGE)
        _log("Actual: %s" % pkg)
        _log("stage=%s ZERO actor writes, deletes, or saves." % stage)
        raise RuntimeError(
            "SECTION 12 aborted at %s: current package '%s', expected '%s'"
            % (stage, pkg, REQUIRED_PACKAGE)
        )
    return world, pkg


def _fail(message):
    _log("=== SECTION 12 EXECUTION ABORTED — REVIEW REQUIRED ===")
    _log("=== SECTION 12 ABORTED ===")
    _log(message)
    raise RuntimeError(message)


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
    return (
        (loc_xyz[0] - hx, loc_xyz[1] - hy, loc_xyz[2] - hz),
        (loc_xyz[0] + hx, loc_xyz[1] + hy, loc_xyz[2] + hz),
    )


def _aabb_from_actor(actor):
    origin, extent = actor.get_actor_bounds(False, False)
    return (
        (origin.x - extent.x, origin.y - extent.y, origin.z - extent.z),
        (origin.x + extent.x, origin.y + extent.y, origin.z + extent.z),
    )


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


def _aabb_close(actual, expected, eps=1.0):
    return (
        abs(actual[0][0] - expected[0][0]) <= eps
        and abs(actual[0][1] - expected[0][1]) <= eps
        and abs(actual[0][2] - expected[0][2]) <= eps
        and abs(actual[1][0] - expected[1][0]) <= eps
        and abs(actual[1][1] - expected[1][1]) <= eps
        and abs(actual[1][2] - expected[1][2]) <= eps
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


def _solid(label, loc, scale):
    return (label, loc, (0.0, 0.0, 0.0), scale, "BlockAll", "QUERY_AND_PHYSICS")


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


def _is_write_set_label(label):
    if label == ORIGINAL_SOUTH:
        return True
    if label in (SOUTH_WEST, SOUTH_EAST, SOUTH_HEADER):
        return True
    if label.startswith(S12_PREFIX) or label.startswith("Admin_S12_"):
        return True
    return False


def _snapshot_protected(actor_sub):
    rows = []
    for actor in actor_sub.get_all_level_actors():
        if not actor:
            continue
        if _actor_owner_package(actor) != REQUIRED_PACKAGE:
            continue
        label = actor.get_actor_label()
        if _is_write_set_label(label):
            continue
        rows.append(_snapshot_actor(actor))
    rows.sort()
    return rows


def _snapshot_group(actor_sub, prefixes, exact_labels=()):
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


def _admin_found(by_label, label):
    return [a for a in by_label.get(label, []) if _actor_owner_package(a) == REQUIRED_PACKAGE]


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
            _fail("Failed to spawn " + spec[0] + ". Original %s was not deleted." % ORIGINAL_SOUTH)
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


def _blocking_walkable_opening(actor_sub, walk_aabb, extra_ignore=()):
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
    return blockers


def _walkable_ignore_labels():
    return set([
        "Admin_FloorPlate",
        "Admin_Hub_Floor",
        "Admin_S5_Hub_Floor",
        "Admin_Operations_Floor",
        "Admin_ServiceCorridor_Floor",
        "Admin_ServiceCorridor_Threshold",
    ])


def _is_s6_label(label):
    if label in S6_SNAPSHOT_EXACT:
        return True
    return any(label.startswith(prefix) for prefix in S6_SNAPSHOT_PREFIXES)


def _is_s7_label(label):
    if label in S7_SNAPSHOT_EXACT:
        return True
    return any(label.startswith(prefix) for prefix in S7_SNAPSHOT_PREFIXES)


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


s12_specs = [_solid(label, loc, scale) for label, loc, scale in S12_SPECS]
split_specs = [_solid(label, loc, scale) for label, loc, scale in SPLIT_SPECS]
all_new_specs = s12_specs + split_specs
expected_s12_labels = [spec[0] for spec in s12_specs]
expected_split_labels = [spec[0] for spec in split_specs]

# ---------------------------------------------------------------------------
# 1. Map guard
# ---------------------------------------------------------------------------
world, world_pkg = _assert_admin_map("precondition-package")
_log("SECTION 12 map guard passed: %s" % world_pkg)

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
s11_before = _snapshot_group(actor_sub, (S11_PREFIX,), ())
s10_keep_before = [
    row for row in _snapshot_group(actor_sub, (S10_PREFIX,), ())
    if row[0] not in (ORIGINAL_SOUTH, SOUTH_WEST, SOUTH_EAST, SOUTH_HEADER)
]

_log("=== SECTION 12 PRE-BUILD SPATIAL AUDIT ===")
s8_aabb, s8_count = _group_aabb(actor_sub, (S8_PREFIX,))
s9_aabb, s9_count = _group_aabb(actor_sub, (S9_PREFIX,))
s10_aabb, s10_count = _group_aabb(actor_sub, (S10_PREFIX,))
s11_aabb, s11_count = _group_aabb(actor_sub, (S11_PREFIX,))
_log("Section 8 count=%d bounds=%s" % (s8_count, _aabb_str(s8_aabb)))
_log("Section 9 count=%d bounds=%s" % (s9_count, _aabb_str(s9_aabb)))
_log("Section 10 count=%d bounds=%s" % (s10_count, _aabb_str(s10_aabb)))
_log("Section 11 count=%d bounds=%s" % (s11_count, _aabb_str(s11_aabb)))
_log("protected Admin actors (excluding S12 write-set)=%d" % len(protected_before))
_log("proposed S12 envelope %s" % _aabb_str(S12_ENVELOPE))
_log("proposed opening X=%.1f to %.1f Y=%.1f to %.1f Z=%.1f to %.1f" % (
    WALKABLE_OPENING[0][0], WALKABLE_OPENING[1][0],
    WALKABLE_OPENING[0][1], WALKABLE_OPENING[1][1],
    WALKABLE_OPENING[0][2], WALKABLE_OPENING[1][2],
))

if s8_count < 1 or s9_count < 1 or s10_count < 1 or s11_count != 14:
    _fail("Precondition failed: verified Sections 8/9/10/11 missing or S11 count!=14. ZERO modifications.")
if any(label.startswith("Admin_S13_") for label in by_label):
    _fail("Precondition failed: Admin_S13_* already exists. ZERO modifications.")
if any(label.startswith("Admin_S12_") for label in by_label):
    _fail("Precondition failed: unexpected Admin_S12_ labels exist. ZERO modifications.")

unexpected_s12 = [
    label for label in by_label
    if label.startswith(S12_PREFIX) and label not in expected_s12_labels
]
if unexpected_s12:
    _fail(
        "Precondition failed: unexpected Service Corridor labels. ZERO modifications. labels=%s"
        % ",".join(sorted(set(unexpected_s12)))
    )

# ---------------------------------------------------------------------------
# 2. Live original south wall / rerun replacements
# ---------------------------------------------------------------------------
original_found = _admin_found(by_label, ORIGINAL_SOUTH)
if len(original_found) > 1:
    _fail("Precondition failed: %s count=%d expected 0 or 1. ZERO modifications." % (
        ORIGINAL_SOUTH, len(original_found)
    ))
original_wall = original_found[0] if original_found else None

if original_wall:
    loc = original_wall.get_actor_location()
    scale = original_wall.get_actor_scale3d()
    rot = original_wall.get_actor_rotation()
    live_aabb = _aabb_from_actor_loc_scale(original_wall)
    _log("INSPECT %s loc=%s rot=%s scale=%s aabb=%s owner=%s collision=%s/%s" % (
        ORIGINAL_SOUTH,
        _fmt_vec(loc),
        _fmt_rot(rot),
        _fmt_vec(scale),
        _aabb_str(live_aabb),
        _actor_owner_package(original_wall),
        _collision_profile(original_wall),
        _collision_enabled(original_wall),
    ))
    if not _vec_close(loc, EXPECTED_ORIGINAL_LOC):
        _fail(
            "Precondition failed: live %s loc=%s expected=%s. ZERO modifications."
            % (ORIGINAL_SOUTH, _fmt_vec(loc), str(EXPECTED_ORIGINAL_LOC))
        )
    if not _vec_close(scale, EXPECTED_ORIGINAL_SCALE):
        _fail(
            "Precondition failed: live %s scale=%s expected=%s. ZERO modifications."
            % (ORIGINAL_SOUTH, _fmt_vec(scale), str(EXPECTED_ORIGINAL_SCALE))
        )
    if abs(rot.pitch) > TRANSFORM_EPS or abs(rot.yaw) > TRANSFORM_EPS or abs(rot.roll) > TRANSFORM_EPS:
        _fail("Precondition failed: live %s rotation is not zero. ZERO modifications." % ORIGINAL_SOUTH)
    if not _aabb_close(live_aabb, EXPECTED_ORIGINAL_AABB):
        _fail(
            "Precondition failed: live %s AABB=%s expected=%s. ZERO modifications."
            % (ORIGINAL_SOUTH, _aabb_str(live_aabb), _aabb_str(EXPECTED_ORIGINAL_AABB))
        )
else:
    _log("%s already absent; treating as rerun after split." % ORIGINAL_SOUTH)

transit_keep_specs = []
for label in TRANSIT_SEAL_LABELS + S10_EAST_KEEP:
    found = _admin_found(by_label, label)
    if len(found) != 1:
        _fail("Precondition failed: %s count=%d expected=1. ZERO modifications." % (label, len(found)))
    transit_keep_specs.append(_capture_live_spec(found[0]))
    _log("KEEP %s loc=%s scale=%s aabb=%s" % (
        label,
        _fmt_vec(found[0].get_actor_location()),
        _fmt_vec(found[0].get_actor_scale3d()),
        _aabb_str(_aabb_from_actor_loc_scale(found[0])),
    ))

east_aabb = _aabb_from_actor_loc_scale(_admin_found(by_label, S11_EAST)[0])
if not _aabb_close(east_aabb, ((5455.0, -350.0, 0.0), (5480.0, 350.0, 450.0))):
    _fail("Precondition failed: Transit east AABB drifted from surveyed sealed wall. ZERO modifications.")

s12_to_create, s12_already_ok, s12_mismatch = _plan_creates(by_label, s12_specs)
split_to_create, split_already_ok, split_mismatch = _plan_creates(by_label, split_specs)
if s12_mismatch or split_mismatch:
    _fail(
        "Precondition failed: existing labels conflict with intended spec. ZERO modifications. "
        + " | ".join(s12_mismatch + split_mismatch)
    )

# ---------------------------------------------------------------------------
# 3. Envelope overlap vs S6–S11 / perimeter (kisses allowed via eps=1)
# ---------------------------------------------------------------------------
allowed_overlap = set([
    "Admin_FloorPlate",
    ORIGINAL_SOUTH,
    SOUTH_WEST,
    SOUTH_EAST,
    SOUTH_HEADER,
])
allowed_overlap.update(expected_s12_labels)
conflict = []
for spec in s12_specs:
    aabb = _aabb_from_loc_scale(spec[1], spec[3])
    for actor in actor_sub.get_all_level_actors():
        if not actor:
            continue
        if _actor_owner_package(actor) != REQUIRED_PACKAGE:
            continue
        label = actor.get_actor_label()
        if label in allowed_overlap:
            continue
        other = _aabb_from_actor(actor)
        if _aabb_overlaps(aabb, other, eps=1.0):
            if spec[0] == "Admin_ServiceCorridor_Threshold" and label.startswith(S10_PREFIX):
                continue
            conflict.append("%s overlaps %s" % (spec[0], label))
if conflict:
    _fail("Precondition failed: spatial conflict. ZERO modifications. " + " | ".join(conflict[:20]))

_log("=== SECTION 12 PRECONDITIONS PASSED ===")
_log(
    "will create s12=%d split=%d; already-ok s12=%d split=%d; original_wall_present=%s"
    % (
        len(s12_to_create),
        len(split_to_create),
        len(s12_already_ok),
        len(split_already_ok),
        bool(original_wall),
    )
)

cube = unreal.EditorAssetLibrary.load_asset(CUBE_PATH)
if not cube:
    raise RuntimeError("Missing mesh: " + CUBE_PATH)

# ---------------------------------------------------------------------------
# EXECUTION — create/reuse S12 + replacements, delete original last
# ---------------------------------------------------------------------------
created = []
created.extend(_spawn_specs(actor_sub, cube, split_to_create))
created.extend(_spawn_specs(actor_sub, cube, s12_to_create))
for label in split_already_ok + s12_already_ok:
    _log("exists, skip spawn: %s" % label)

split_errors, _, _, by_label = _verify_specs_present(actor_sub, split_specs)
s12_errors, _, _, by_label = _verify_specs_present(actor_sub, s12_specs)
keep_errors, _, _, by_label = _verify_specs_present(actor_sub, transit_keep_specs)
if split_errors or s12_errors or keep_errors:
    _fail(
        "Replacement/S12/sealed-face validation failed. Original %s NOT deleted. "
        % ORIGINAL_SOUTH
        + " | ".join(split_errors + s12_errors + keep_errors)
    )

pre_ignore = _walkable_ignore_labels() | set([ORIGINAL_SOUTH])
pre_blockers = _blocking_walkable_opening(actor_sub, WALKABLE_OPENING, pre_ignore)
_log("pre-delete walkable opening AABB: %s" % _aabb_str(WALKABLE_OPENING))
if pre_blockers:
    _fail(
        "Opening still blocked by replacement/other geometry. Original %s NOT deleted. blockers=%s"
        % (ORIGINAL_SOUTH, ",".join(pre_blockers))
    )

deleted_original = False
if original_wall is not None:
    _assert_admin_map("pre-delete-south-wall")
    still = _admin_found(_collect_by_label(actor_sub), ORIGINAL_SOUTH)
    if len(still) != 1:
        _fail("Refusing to delete: %s count changed to %d before delete." % (ORIGINAL_SOUTH, len(still)))
    destroyed = actor_sub.destroy_actor(still[0])
    if not destroyed:
        _fail("destroy_actor returned false for %s." % ORIGINAL_SOUTH)
    deleted_original = True
    _log("deleted original %s after S12 and replacement validation" % ORIGINAL_SOUTH)
else:
    _log("no %s delete performed" % ORIGINAL_SOUTH)

# ---------------------------------------------------------------------------
# Final verification
# ---------------------------------------------------------------------------
_assert_admin_map("pre-final-verify")
by_label = _collect_by_label(actor_sub)
final_errors = []

if by_label.get(ORIGINAL_SOUTH):
    final_errors.append("original %s still exists after split" % ORIGINAL_SOUTH)

keep_errors, _, _, by_label = _verify_specs_present(actor_sub, transit_keep_specs)
final_errors.extend(keep_errors)
split_errors, _, _, by_label = _verify_specs_present(actor_sub, split_specs)
final_errors.extend(split_errors)
s12_errors, _, _, by_label = _verify_specs_present(actor_sub, s12_specs)
final_errors.extend(s12_errors)

blockers = _blocking_walkable_opening(actor_sub, WALKABLE_OPENING, _walkable_ignore_labels())
_log("walkable opening AABB: %s" % _aabb_str(WALKABLE_OPENING))
_log("walkable Z starts at %.3f (expected floor top %.3f + clearance 5.000)" % (
    WALKABLE_Z0, EXPECTED_FLOOR_TOP_Z
))
_log("door center=%s width=%.1f height=%.1f" % (str(DOOR_CENTER), DOOR_WIDTH, DOOR_HEIGHT))
if blockers:
    final_errors.append("walkable Operations-Service opening still blocked by: " + ",".join(blockers))

s12_labels = [label for label in by_label if label.startswith(S12_PREFIX)]
dupes = [label for label in s12_labels if len(by_label.get(label, [])) > 1]
dupes.extend([label for label in expected_split_labels if len(by_label.get(label, [])) > 1])
if dupes:
    final_errors.append("duplicate labels: " + ",".join(sorted(set(dupes))))
extra = [label for label in s12_labels if label not in expected_s12_labels]
if extra:
    final_errors.append("unexpected Admin_ServiceCorridor_* labels: " + ",".join(sorted(set(extra))))
missing = [label for label in expected_s12_labels if label not in s12_labels]
if missing:
    final_errors.append("missing Section 12 labels: " + ",".join(missing))
for label in expected_split_labels:
    if len(by_label.get(label, [])) != 1:
        final_errors.append("%s count=%d after build" % (label, len(by_label.get(label, []))))

if any(label.startswith("Admin_S13_") for label in by_label):
    final_errors.append("Admin_S13_* actors were created")
if any(label.startswith("Admin_S12_") for label in by_label):
    final_errors.append("unexpected Admin_S12_ labels exist")
if any(label.startswith("Admin_S11_") for label in by_label):
    final_errors.append("unexpected Admin_S11_ labels exist")

east_after = _admin_found(by_label, S11_EAST)
if len(east_after) != 1:
    final_errors.append("Transit east count=%d" % len(east_after))
elif not _aabb_close(_aabb_from_actor_loc_scale(east_after[0]), ((5455.0, -350.0, 0.0), (5480.0, 350.0, 450.0))):
    final_errors.append("Transit east AABB changed")

protected_after = _snapshot_protected(actor_sub)
s6_after = _snapshot_group(actor_sub, S6_SNAPSHOT_PREFIXES, S6_SNAPSHOT_EXACT)
s7_after = _snapshot_group(actor_sub, S7_SNAPSHOT_PREFIXES, S7_SNAPSHOT_EXACT)
s8_after = _snapshot_group(actor_sub, (S8_PREFIX,), ())
s9_after = _snapshot_group(actor_sub, (S9_PREFIX,), ())
s11_after = _snapshot_group(actor_sub, (S11_PREFIX,), ())
s10_keep_after = [
    row for row in _snapshot_group(actor_sub, (S10_PREFIX,), ())
    if row[0] not in (ORIGINAL_SOUTH, SOUTH_WEST, SOUTH_EAST, SOUTH_HEADER)
]
if protected_after != protected_before:
    final_errors.append("protected Admin actors changed during Section 12 build")
if s6_after != s6_before:
    final_errors.append("Section 6 actors changed during Section 12 build")
if s7_after != s7_before:
    final_errors.append("Section 7 actors changed during Section 12 build")
if s8_after != s8_before:
    final_errors.append("Section 8 actors changed during Section 12 build")
if s9_after != s9_before:
    final_errors.append("Section 9 actors changed during Section 12 build")
if s11_after != s11_before:
    final_errors.append("Section 11 actors changed during Section 12 build")
if s10_keep_after != s10_keep_before:
    final_errors.append("Section 10 actors other than the approved south-wall split changed")

for spec in all_new_specs:
    found = by_label.get(spec[0], [])
    if len(found) != 1:
        continue
    if _actor_owner_package(found[0]) != REQUIRED_PACKAGE:
        final_errors.append("%s owned by %s" % (spec[0], _actor_owner_package(found[0])))
    if _collision_enabled(found[0]) != "QUERY_AND_PHYSICS":
        final_errors.append("%s collision_enabled=%s" % (spec[0], _collision_enabled(found[0])))

if final_errors:
    _log("=== SECTION 12 VERIFICATION FAILED ===")
    _fail(" | ".join(final_errors))

_, world_pkg = _assert_admin_map("pre-save")
if world_pkg != REQUIRED_PACKAGE:
    _fail("Refusing to save: current package is %s" % world_pkg)
saved = level_sub.save_current_level()
_log("saved current level only; package=%s result=%s" % (world_pkg, _safe_str(saved)))
_log("Lvl_MainMenu was not saved")

_log("=== SECTION 12 SERVICE CORRIDOR VERIFIED IN SL_EPITOPE_ADMIN ===")
_log("active package=%s" % world_pkg)
_log("Section 12 actor count=%d" % len(expected_s12_labels))
_log("Admin_ServiceCorridor labels=%s" % ",".join(expected_s12_labels))
_log("south replacements=%s" % ",".join(expected_split_labels))
_log("original %s deleted=%s remaining=%s" % (
    ORIGINAL_SOUTH, deleted_original, bool(by_label.get(ORIGINAL_SOUTH))
))
_log("opening width=%.1f height=%.1f center=%s walkable=%s" % (
    DOOR_WIDTH, DOOR_HEIGHT, str(DOOR_CENTER), _aabb_str(WALKABLE_OPENING)
))
_log("duplicate count=0")
_log("ownership=SL_Epitope_Admin collision=BlockAll/QUERY_AND_PHYSICS")
_log("S1-S11 protected snapshots unchanged except approved south-wall split")
_log("Transit east remains sealed and unchanged")
_log("Admin_S13_* count=0")
_log("created=%d already-existing s12=%d already-existing split=%d" % (
    len(created), len(s12_already_ok), len(split_already_ok)
))
_log("interior=1535 x 575 ceiling=%.0f" % CEILING_Z)
_log("Section 13 was not built. Lvl_MainMenu was not saved.")
_log("=== SECTION 12 SERVICE CORRIDOR COMPLETE ===")
