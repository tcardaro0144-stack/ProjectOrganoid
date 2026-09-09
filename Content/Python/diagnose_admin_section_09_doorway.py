# ProjectOrganoid — READ-ONLY diagnosis of why Section 9 doorway verify
# flagged Admin_FloorPlate. Performs ZERO world writes.
# Do not import build_admin_section_09_director_suite.py (that module executes).

import unreal

REQUIRED_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
MENU_PACKAGE = "/Game/Maps/Lvl_MainMenu"
CUBE_HALF = 50.0
FLOOR_LABEL = "Admin_FloorPlate"
THRESHOLD_LABEL = "Admin_DirectorSuite_Threshold"
S9_PREFIX = "Admin_DirectorSuite_"

# Copied exactly from build_admin_section_09_director_suite.py
VERIFIED_WALL_LOC = (2487.5, -1850.0, 225.0)
VERIFIED_WALL_SCALE = (0.25, 12.0, 4.5)
DOOR_WIDTH = 140.0
DOOR_HEIGHT = 240.0
OVERLAP_EPS = 0.5

# Ignore list copied from the build script's final doorway check.
WALK_IGNORE = set([
    "Admin_ConferenceRoom_Wall_NegX_South",
    "Admin_ConferenceRoom_Wall_NegX_North",
    "Admin_ConferenceRoom_Wall_NegX_Header",
    "Admin_DirectorSuite_Threshold",
    "Admin_DirectorSuite_Floor",
    "Admin_ConferenceRoom_Floor",
    "Admin_DirectorSuite_Ceiling",
    "Admin_DirectorSuite_Wall_West",
    "Admin_DirectorSuite_Wall_North",
    "Admin_DirectorSuite_Wall_South",
    "Admin_DirectorSuite_Wall_East_North",
    "Admin_DirectorSuite_Wall_East_South",
    "Admin_DirectorSuite_Wall_East_Header",
    "Admin_DirectorSuite_Backdrop",
    "Admin_DirectorSuite_Desk",
    "Admin_DirectorSuite_DirectorChair",
    "Admin_DirectorSuite_VisitorChair_01",
    "Admin_DirectorSuite_VisitorChair_02",
    "Admin_DirectorSuite_Cabinet_01",
    "Admin_DirectorSuite_Cabinet_02",
    "Admin_DirectorSuite_Glass_North",
])


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


def _intersection(a, b):
    mn = (max(a[0][0], b[0][0]), max(a[0][1], b[0][1]), max(a[0][2], b[0][2]))
    mx = (min(a[1][0], b[1][0]), min(a[1][1], b[1][1]), min(a[1][2], b[1][2]))
    if mn[0] >= mx[0] or mn[1] >= mx[1] or mn[2] >= mx[2]:
        return None
    return (mn, mx)


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


def _component_aabb(actor):
    mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
    if not mesh:
        return None
    try:
        origin, extent = mesh.get_local_bounds()
    except Exception:
        origin = None
        extent = None
    if origin is None:
        return None
    loc = actor.get_actor_location()
    scale = actor.get_actor_scale3d()
    hx = abs(extent.x * scale.x)
    hy = abs(extent.y * scale.y)
    hz = abs(extent.z * scale.z)
    world_origin = unreal.Vector(
        loc.x + origin.x * scale.x,
        loc.y + origin.y * scale.y,
        loc.z + origin.z * scale.z,
    )
    return (
        (world_origin.x - hx, world_origin.y - hy, world_origin.z - hz),
        (world_origin.x + hx, world_origin.y + hy, world_origin.z + hz),
    )


def _build_door_aabb_exact():
    orig_aabb = _aabb_from_loc_scale(VERIFIED_WALL_LOC, VERIFIED_WALL_SCALE)
    center_y = VERIFIED_WALL_LOC[1]
    door_y0 = center_y - DOOR_WIDTH * 0.5
    door_y1 = center_y + DOOR_WIDTH * 0.5
    door_aabb = (
        (orig_aabb[0][0], door_y0, orig_aabb[0][2]),
        (orig_aabb[1][0], door_y1, DOOR_HEIGHT),
    )
    return orig_aabb, door_aabb


world = _get_editor_world()
pkg = _world_package(world)
if pkg != REQUIRED_PACKAGE:
    _log("=== SECTION 9 DOORWAY DIAGNOSIS ABORTED — WRONG MAP ===")
    _log("Expected: %s" % REQUIRED_PACKAGE)
    _log("Actual: %s" % pkg)
    raise RuntimeError("diagnostic aborted: wrong map %s" % pkg)

_log("=== SECTION 9 DOORWAY DIAGNOSIS (READ-ONLY) ===")
_log("editor_world_package=%s" % pkg)
_log("zero writes: no spawn/destroy/move/save")

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not actor_sub:
    raise RuntimeError("EditorActorSubsystem unavailable")

orig_aabb, door_aabb = _build_door_aabb_exact()
_log("verified original wall AABB: %s" % _aabb_str(orig_aabb))
_log("EXACT doorway check volume from build script: %s" % _aabb_str(door_aabb))
_log("overlap test: get_actor_bounds(False, False) vs doorway AABB, eps=%.2f" % OVERLAP_EPS)
_log("walk_ignore does NOT include Admin_FloorPlate")

floor_actors = []
threshold_actors = []
s9_actors = []
for actor in actor_sub.get_all_level_actors():
    label = actor.get_actor_label()
    if label == FLOOR_LABEL:
        floor_actors.append(actor)
    if label == THRESHOLD_LABEL:
        threshold_actors.append(actor)
    if label.startswith(S9_PREFIX):
        s9_actors.append(actor)

_log("Admin_FloorPlate count=%d" % len(floor_actors))
_log("Admin_DirectorSuite_Threshold count=%d" % len(threshold_actors))
_log("Admin_DirectorSuite_* count=%d" % len(s9_actors))

if not floor_actors:
    _log("Admin_FloorPlate: NOT FOUND")
    _log("=== DIAGNOSIS: INCONCLUSIVE ===")
    _log("Admin_FloorPlate is not present in the current editor world, so this run cannot reproduce the blockage.")
else:
    conclusions = []
    for actor in floor_actors:
        loc = actor.get_actor_location()
        rot = actor.get_actor_rotation()
        scale = actor.get_actor_scale3d()
        bounds_aabb = _aabb_from_actor(actor)
        loc_scale_aabb = _aabb_from_loc_scale((loc.x, loc.y, loc.z), (scale.x, scale.y, scale.z))
        comp_aabb = _component_aabb(actor)
        owner = _actor_owner_package(actor)
        profile = _collision_profile(actor)
        enabled = _collision_enabled(actor)
        ignored = actor.get_actor_label() in WALK_IGNORE
        skipped_nocol = enabled == "NO_COLLISION" or profile == "NoCollision"
        intersects = _aabb_overlaps(bounds_aabb, door_aabb, eps=OVERLAP_EPS)
        inter = _intersection(bounds_aabb, door_aabb) if intersects else None

        _log("FLOOR label=%s" % actor.get_actor_label())
        _log("FLOOR class=%s" % _safe_str(_safe_call(actor.get_class(), "get_path_name")))
        _log("FLOOR owner=%s" % owner)
        _log("FLOOR loc=%s" % _fmt_vec(loc))
        _log("FLOOR rot=%s" % _fmt_rot(rot))
        _log("FLOOR scale=%s" % _fmt_vec(scale))
        _log("FLOOR actor_bounds AABB=%s" % _aabb_str(bounds_aabb))
        _log("FLOOR loc*scale AABB=%s" % _aabb_str(loc_scale_aabb))
        _log("FLOOR staticmesh_component AABB=%s" % _aabb_str(comp_aabb))
        _log("FLOOR mesh=%s" % _mesh_path(actor))
        _log("FLOOR collision_profile=%s" % profile)
        _log("FLOOR collision_enabled=%s" % enabled)
        _log("FLOOR in walk_ignore=%s" % ignored)
        _log("FLOOR skipped as NoCollision=%s" % skipped_nocol)
        _log("FLOOR intersects doorway check volume=%s" % intersects)
        if inter:
            ix = inter[1][0] - inter[0][0]
            iy = inter[1][1] - inter[0][1]
            iz = inter[1][2] - inter[0][2]
            _log("FLOOR overlap min=(%.6f, %.6f, %.6f)" % (inter[0][0], inter[0][1], inter[0][2]))
            _log("FLOOR overlap max=(%.6f, %.6f, %.6f)" % (inter[1][0], inter[1][1], inter[1][2]))
            _log("FLOOR overlap size=(%.6f, %.6f, %.6f)" % (ix, iy, iz))
            floor_top_z = bounds_aabb[1][2]
            floor_bot_z = bounds_aabb[0][2]
            overlap_top_z = inter[1][2]
            overlap_bot_z = inter[0][2]
            walk_clearance_z0 = 20.0
            overlap_into_walk = max(0.0, overlap_top_z - walk_clearance_z0)
            only_sill = overlap_top_z <= walk_clearance_z0 + 0.51
            _log("FLOOR own Z range=%.6f to %.6f" % (floor_bot_z, floor_top_z))
            _log("FLOOR overlap Z range=%.6f to %.6f" % (overlap_bot_z, overlap_top_z))
            _log("FLOOR overlap extending above Z=20 (walk clearance)=%.6f" % overlap_into_walk)
            _log("FLOOR overlap confined to sill/floor plane (Z<=20)=%s" % only_sill)
            if ignored or skipped_nocol:
                conclusions.append("inconclusive")
            elif only_sill and overlap_into_walk <= 0.51:
                conclusions.append("false_positive")
            elif overlap_into_walk > 0.51:
                conclusions.append("legitimate")
            else:
                conclusions.append("inconclusive")
        else:
            _log("FLOOR does not intersect the exact doorway AABB with eps=0.5")
            conclusions.append("inconclusive")

    unique = list(set(conclusions))
    if unique == ["false_positive"]:
        _log("=== DIAGNOSIS: DOORWAY CHECK IS FALSE-POSITIVE ON FLOOR ===")
        _log("Admin_FloorPlate AABB intersects the doorway volume only in the floor-slab Z band. The build script's walk_ignore list skipped Conference/Director floors but not Admin_FloorPlate.")
    elif unique == ["legitimate"]:
        _log("=== DIAGNOSIS: FLOOR IS LEGITIMATELY BLOCKING DOORWAY ===")
        _log("Admin_FloorPlate overlap extends vertically into the walkable doorway opening above the sill/floor plane.")
    else:
        _log("=== DIAGNOSIS: INCONCLUSIVE ===")
        _log("Could not uniquely classify the FloorPlate overlap from current bounds. See overlap numbers above.")

if threshold_actors:
    for actor in threshold_actors:
        loc = actor.get_actor_location()
        scale = actor.get_actor_scale3d()
        _log("THRESHOLD label=%s" % actor.get_actor_label())
        _log("THRESHOLD loc=%s" % _fmt_vec(loc))
        _log("THRESHOLD scale=%s" % _fmt_vec(scale))
        _log("THRESHOLD bounds=%s" % _aabb_str(_aabb_from_actor(actor)))
        _log("THRESHOLD owner=%s" % _actor_owner_package(actor))
        _log("THRESHOLD collision_profile=%s" % _collision_profile(actor))
        _log("THRESHOLD collision_enabled=%s" % _collision_enabled(actor))
        _log("THRESHOLD intersects doorway=%s" % _aabb_overlaps(_aabb_from_actor(actor), door_aabb, eps=OVERLAP_EPS))
else:
    _log("THRESHOLD Admin_DirectorSuite_Threshold: NOT FOUND")

_log("--- existing Admin_DirectorSuite_* actors ---")
if not s9_actors:
    _log("none")
else:
    s9_actors.sort(key=lambda a: a.get_actor_label())
    for actor in s9_actors:
        loc = actor.get_actor_location()
        scale = actor.get_actor_scale3d()
        _log(
            "S9 label=%s loc=%s scale=%s owner=%s"
            % (actor.get_actor_label(), _fmt_vec(loc), _fmt_vec(scale), _actor_owner_package(actor))
        )

_log("diagnostic complete. no actors were spawned, moved, deleted, or saved.")
