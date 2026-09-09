# ProjectOrganoid — PHASE 2 recreate Sections 7–8 into SL_Epitope_Admin.
# Uses Phase 1 transforms plus original per-actor collision (Glass = NoCollision).
# Does not delete Main Menu copies. Does not touch Section 6. Does not build Section 9.

import unreal

REQUIRED_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
CUBE_PATH = "/Engine/BasicShapes/Cube.Cube"
TRANSFORM_EPS = 0.05

# Phase 1 transforms + original Section 8 per-actor collision.
# section, label, loc, rot_pyr, scale, collision_profile, collision_enabled
RECOVERY_SPEC = [
    (7, "Admin_RecordsArchives_Block", (2680.0, 400.0, 140.0), (0.0, 0.0, 0.0), (4.0, 1.0, 2.8), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Ceiling", (3200.0, -1850.0, 460.0), (0.0, 0.0, 0.0), (14.0, 12.0, 0.2), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Floor", (3200.0, -1850.0, 10.0), (0.0, 0.0, 0.0), (14.0, 12.0, 0.2), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Glass", (3200.0, -1265.0, 200.0), (0.0, 0.0, 0.0), (5.0, 0.08, 2.4), "NoCollision", "NO_COLLISION"),
    (8, "Admin_ConferenceRoom_Projector", (3200.0, -1850.0, 90.0), (0.0, 0.0, 0.0), (0.6, 0.6, 0.4), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Screen", (3200.0, -2437.5, 225.0), (0.0, 0.0, 0.0), (4.0, 0.1, 2.0), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Seat_01", (3000.0, -1720.0, 45.0), (0.0, 0.0, 0.0), (0.4, 0.4, 0.9), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Seat_02", (3100.0, -1720.0, 45.0), (0.0, 0.0, 0.0), (0.4, 0.4, 0.9), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Seat_03", (3200.0, -1720.0, 45.0), (0.0, 0.0, 0.0), (0.4, 0.4, 0.9), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Seat_04", (3300.0, -1720.0, 45.0), (0.0, 0.0, 0.0), (0.4, 0.4, 0.9), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Seat_05", (3400.0, -1720.0, 45.0), (0.0, 0.0, 0.0), (0.4, 0.4, 0.9), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Seat_06", (3000.0, -1980.0, 45.0), (0.0, 0.0, 0.0), (0.4, 0.4, 0.9), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Seat_07", (3100.0, -1980.0, 45.0), (0.0, 0.0, 0.0), (0.4, 0.4, 0.9), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Seat_08", (3200.0, -1980.0, 45.0), (0.0, 0.0, 0.0), (0.4, 0.4, 0.9), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Seat_09", (3300.0, -1980.0, 45.0), (0.0, 0.0, 0.0), (0.4, 0.4, 0.9), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Seat_10", (3400.0, -1980.0, 45.0), (0.0, 0.0, 0.0), (0.4, 0.4, 0.9), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Table", (3200.0, -1850.0, 37.5), (0.0, 0.0, 0.0), (5.0, 1.6, 0.75), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Wall_Far", (3200.0, -2462.5, 225.0), (0.0, 0.0, 0.0), (14.0, 0.25, 4.5), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Wall_Hub", (3200.0, -1237.5, 225.0), (0.0, 0.0, 0.0), (14.0, 0.25, 4.5), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Wall_NegX", (2487.5, -1850.0, 225.0), (0.0, 0.0, 0.0), (0.25, 12.0, 4.5), "BlockAll", "QUERY_AND_PHYSICS"),
    (8, "Admin_ConferenceRoom_Wall_PosX", (3912.5, -1850.0, 225.0), (0.0, 0.0, 0.0), (0.25, 12.0, 4.5), "BlockAll", "QUERY_AND_PHYSICS"),
]


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
    for candidate in candidates:
        classified = _classify_package(candidate)
        if classified == REQUIRED_PACKAGE:
            return REQUIRED_PACKAGE
        if classified != "cannot be determined" and classified.startswith("/Game/"):
            return classified
    return "cannot be determined"


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
    for candidate in candidates:
        classified = _classify_package(candidate)
        if classified != "cannot be determined":
            return classified
    return "cannot be determined"


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
    static_mesh = None
    try:
        static_mesh = mesh.get_editor_property("static_mesh")
    except Exception:
        static_mesh = None
    if not static_mesh:
        return "None"
    path = _safe_call(static_mesh, "get_path_name")
    return _safe_str(path)


def _collision_profile(actor):
    mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
    if not mesh:
        return "None"
    profile = _safe_call(mesh, "get_collision_profile_name")
    return _safe_str(profile)


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


def _collision_enabled_enum(name):
    return getattr(unreal.CollisionEnabled, name)


def _row_to_spec(row):
    section, label, loc_xyz, rot_pyr, scale_xyz, profile, collision_enabled = row
    return (label, loc_xyz, rot_pyr, scale_xyz, profile, collision_enabled)


def _apply_blockout(actor, spec, cube):
    label, loc_xyz, rot_pyr, scale_xyz, profile, collision_enabled = spec
    actor.set_actor_label(label)
    actor.set_actor_location(unreal.Vector(*loc_xyz), False, False)
    actor.set_actor_rotation(unreal.Rotator(rot_pyr[0], rot_pyr[1], rot_pyr[2]), False)
    actor.set_actor_scale3d(unreal.Vector(*scale_xyz))
    mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
    if not mesh:
        raise RuntimeError(label + " has no StaticMeshComponent")
    mesh.set_static_mesh(cube)
    mesh.set_collision_profile_name(profile)
    mesh.set_collision_enabled(_collision_enabled_enum(collision_enabled))


def _matches_spec(actor, spec):
    label, loc_xyz, rot_pyr, scale_xyz, profile, collision_enabled = spec
    reasons = []
    owner = _actor_owner_package(actor)
    if owner != REQUIRED_PACKAGE:
        reasons.append("owner=%s" % owner)
    if actor.get_actor_label() != label:
        reasons.append("label=%s" % actor.get_actor_label())
    if not _vec_close(actor.get_actor_location(), loc_xyz):
        reasons.append("loc=%s expected=%s" % (_fmt_vec(actor.get_actor_location()), str(loc_xyz)))
    if not _rot_close(actor.get_actor_rotation(), rot_pyr):
        reasons.append("rot=%s expected=%s" % (_fmt_rot(actor.get_actor_rotation()), str(rot_pyr)))
    if not _vec_close(actor.get_actor_scale3d(), scale_xyz):
        reasons.append("scale=%s expected=%s" % (_fmt_vec(actor.get_actor_scale3d()), str(scale_xyz)))
    mesh_path = _mesh_path(actor)
    if mesh_path not in (CUBE_PATH, CUBE_PATH + "." + "Cube") and not mesh_path.endswith("Cube.Cube"):
        reasons.append("mesh=%s" % mesh_path)
    actual_profile = _collision_profile(actor)
    if actual_profile != profile and actual_profile != "None":
        reasons.append("collision_profile=%s expected=%s" % (actual_profile, profile))
    actual_enabled = _collision_enabled(actor)
    if actual_enabled != collision_enabled:
        reasons.append("collision_enabled=%s expected=%s" % (actual_enabled, collision_enabled))
    return reasons


def _collect_admin_actors(actor_sub):
    by_label = {}
    for actor in actor_sub.get_all_level_actors():
        if not actor:
            continue
        if _actor_owner_package(actor) != REQUIRED_PACKAGE:
            continue
        label = actor.get_actor_label()
        by_label.setdefault(label, []).append(actor)
    return by_label


def _fail(message):
    _log("=== RECOVERY VERIFICATION FAILED ===")
    _log(message)
    raise RuntimeError(message)


# --- hard map guard: zero writes unless this is exactly SL_Epitope_Admin ---
world = _get_editor_world()
world_pkg = _world_package(world)
if world_pkg != REQUIRED_PACKAGE:
    raise RuntimeError(
        "PHASE 2 aborted: current editor world/package is '%s', expected '%s'. ZERO writes performed."
        % (world_pkg, REQUIRED_PACKAGE)
    )

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not actor_sub or not level_sub:
    raise RuntimeError("EditorActorSubsystem / LevelEditorSubsystem unavailable")

current_level = _safe_call(level_sub, "get_current_level")
current_level_pkg = _classify_package(
    _package_from_path(_safe_call(_safe_call(current_level, "get_outermost"), "get_name"))
    or _package_from_path(_safe_call(current_level, "get_path_name"))
)
if current_level is not None and current_level_pkg not in (REQUIRED_PACKAGE, "cannot be determined"):
    if current_level_pkg != REQUIRED_PACKAGE:
        raise RuntimeError(
            "PHASE 2 aborted: current level package is '%s', expected '%s'. ZERO writes performed."
            % (current_level_pkg, REQUIRED_PACKAGE)
        )

_log("PHASE 2 map guard passed: %s" % world_pkg)

cube = unreal.EditorAssetLibrary.load_asset(CUBE_PATH)
if not cube:
    raise RuntimeError("Missing mesh: " + CUBE_PATH)

admin_by_label = _collect_admin_actors(actor_sub)
to_create = []
already_ok = []
mismatched = []

for row in RECOVERY_SPEC:
    section, label, loc_xyz, rot_pyr, scale_xyz, profile, collision_enabled = row
    spec = _row_to_spec(row)
    found = admin_by_label.get(label, [])
    if len(found) > 1:
        mismatched.append("%s has %d copies already in SL_Epitope_Admin" % (label, len(found)))
        continue
    if len(found) == 1:
        reasons = _matches_spec(found[0], spec)
        if reasons:
            mismatched.append("%s already exists in SL_Epitope_Admin but does not match Phase 1: %s" % (label, "; ".join(reasons)))
        else:
            already_ok.append(label)
        continue
    to_create.append(spec)

if mismatched:
    _fail("Preflight mismatch; no actors were spawned. " + " | ".join(mismatched))

created = []
for spec in to_create:
    label, loc_xyz, rot_pyr, scale_xyz, profile, collision_enabled = spec
    actor = actor_sub.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(*loc_xyz),
        unreal.Rotator(rot_pyr[0], rot_pyr[1], rot_pyr[2]),
    )
    if not actor:
        _fail("Failed to spawn " + label)
    _apply_blockout(actor, spec, cube)
    created.append(label)
    _log("created %s" % label)

for label in already_ok:
    _log("exists, skip spawn: %s" % label)

saved = level_sub.save_current_level()
_log("saved SL_Epitope_Admin=%s" % _safe_str(saved))

admin_by_label = _collect_admin_actors(actor_sub)
s7_found = []
s8_found = []
verify_failures = []
recovered_rows = []

expected_s7 = [row[1] for row in RECOVERY_SPEC if row[0] == 7]
expected_s8 = [row[1] for row in RECOVERY_SPEC if row[0] == 8]

for row in RECOVERY_SPEC:
    section, label, loc_xyz, rot_pyr, scale_xyz, profile, collision_enabled = row
    spec = _row_to_spec(row)
    found = admin_by_label.get(label, [])
    if not found:
        verify_failures.append("%s missing after save" % label)
        continue
    if len(found) != 1:
        verify_failures.append("%s duplicate count=%d after save" % (label, len(found)))
        continue
    actor = found[0]
    reasons = _matches_spec(actor, spec)
    owner = _actor_owner_package(actor)
    recovered_rows.append((label, actor.get_actor_location(), actor.get_actor_rotation(), actor.get_actor_scale3d(), owner))
    if reasons:
        verify_failures.append("%s verification failed: %s" % (label, "; ".join(reasons)))
    if section == 7:
        s7_found.append(label)
    else:
        s8_found.append(label)

extra_s8 = []
for label, actors in admin_by_label.items():
    if label.startswith("Admin_ConferenceRoom_") and label not in expected_s8:
        extra_s8.append(label)
    if label.startswith("Admin_ConferenceRoom_") and len(actors) > 1:
        verify_failures.append("%s has extra copies in SL_Epitope_Admin" % label)

if extra_s8:
    verify_failures.append("unexpected Admin_ConferenceRoom_* labels: " + ",".join(extra_s8))

if len(s7_found) != 1:
    verify_failures.append("Section 7 expected=1 found=%d" % len(s7_found))
if len(s8_found) != 20:
    verify_failures.append("Section 8 expected=20 found=%d" % len(s8_found))

if verify_failures:
    _log("Section 7 expected=1 found=%d" % len(s7_found))
    _log("Section 8 expected=20 found=%d" % len(s8_found))
    _log("created count=%d" % len(created))
    _log("already-existing count=%d" % len(already_ok))
    _log("mismatched count=%d" % len(verify_failures))
    _fail(" | ".join(verify_failures))

_log("=== SECTIONS 7–8 RECOVERY VERIFIED IN SL_EPITOPE_ADMIN ===")
_log("Section 7 expected/found count=1/%d" % len(s7_found))
_log("Section 8 expected/found count=20/%d" % len(s8_found))
_log("created count=%d" % len(created))
_log("already-existing count=%d" % len(already_ok))
_log("mismatched count=0")
for label, loc, rot, scale, owner in recovered_rows:
    _log(
        "RECOVERED label=%s loc=%s rot=%s scale=%s owner=%s"
        % (label, _fmt_vec(loc), _fmt_rot(rot), _fmt_vec(scale), owner)
    )
_log("Section 6 untouched. Lvl_MainMenu copies not deleted.")
