# ProjectOrganoid — Section 15 Admin room-identity triggers (safe placement).
# Reuses already-parented Section 13 BP_AdminRoomTrigger and Section 14
# Admin_SectorController. Does not reparent, compile, or save the Blueprint.
# Does not mutate collision. Does not touch S1–S12 geometry.
# Does not create Admin_S13_*. Does not begin Section 16.
# Does not save SL_Epitope_Admin; save manually after verification.
#
# Existing room triggers are identified by BP/C++ class
# (BP_AdminRoomTrigger / ProjectOrganoidAdminRoomTrigger), never by actor
# label alone. A StaticMeshActor (or any other class) sharing
# Admin_RoomTrigger_* is an impostor and aborts the run.
#
# Current editor map must be /Game/Maps/Epitope/SL_Epitope_Admin.
#
# Run:
#   File -> Execute Python Script...
#   Content/Python/build_admin_section_15_room_triggers.py

import unreal

REQUIRED_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
MENU_PACKAGE = "/Game/Maps/Lvl_MainMenu"
BP_PATH = "/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminRoomTrigger"
PARENT_CLASS = "/Script/ProjectOrganoid.ProjectOrganoidAdminRoomTrigger"
CONTROLLER_BP_PATH = "/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminSectorController"
CONTROLLER_CLASS = "/Script/ProjectOrganoid.ProjectOrganoidAdminSectorController"
CONTROLLER_LABEL = "Admin_SectorController"
CUBE_HALF = 50.0
CENTER_EPS = 50.0
TRIGGER_Z = 150.0
EXTENT_Z = 130.0
SECURITY_APPROVED_XY = (2680.0, -400.0)
SECURITY_APPROVED_LOC = (2680.0, -400.0, 150.0)
SECURITY_EXTENT = (140.0, 70.0, 130.0)
S6_NEAR_HUB_EPS = 400.0

# label, RoomID, expected live floor/prop label, expected XY center, seam mode
SPECS = (
    ("Admin_RoomTrigger_Vestibule", "Vestibule", "Admin_S1_Vestibule_Floor", (400.0, 0.0), "vestibule"),
    ("Admin_RoomTrigger_Reception", "Reception", "Admin_S1_Reception_Floor", (1500.0, 0.0), "reception"),
    ("Admin_RoomTrigger_Hub", "Hub", "Admin_Hub_Floor", (2280.0, 0.0), "hub"),
    ("Admin_RoomTrigger_Security", "Security", "east_hub_security", (2680.0, -400.0), "security"),
    ("Admin_RoomTrigger_Records", "Records", "Admin_RecordsArchives_Block", (2680.0, 400.0), "records"),
    ("Admin_RoomTrigger_Conference", "Conference", "Admin_ConferenceRoom_Floor", (3200.0, -1850.0), "conference"),
    ("Admin_RoomTrigger_DirectorSuite", "DirectorSuite", "Admin_DirectorSuite_Floor", (1950.0, -1850.0), "director"),
    ("Admin_RoomTrigger_Operations", "Operations", "Admin_Operations_Floor", (3705.0, 0.0), "operations"),
    ("Admin_RoomTrigger_Transit", "Transit", "Admin_Transit_Floor", (4980.0, 0.0), "transit"),
    ("Admin_RoomTrigger_ServiceCorridor", "ServiceCorridor", "Admin_ServiceCorridor_Floor", (3712.5, -912.5), "service"),
)


def _log(msg):
    unreal.log("[S15] " + msg)


def _fail(msg):
    _log("=== SECTION 15 ABORTED ===")
    _log(msg)
    raise RuntimeError(msg)


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
        _fail("WRONG MAP at %s. Expected %s, actual %s. ZERO writes." % (
            stage, REQUIRED_PACKAGE, pkg
        ))
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


def _actor_label(actor):
    label = _safe_call(actor, "get_actor_label")
    if label:
        return _safe_str(label)
    return _safe_str(_safe_call(actor, "get_name"))


def _aabb_from_actor(actor):
    loc = actor.get_actor_location()
    scale = actor.get_actor_scale3d()
    hx = abs(scale.x) * CUBE_HALF
    hy = abs(scale.y) * CUBE_HALF
    hz = abs(scale.z) * CUBE_HALF
    return (
        (loc.x - hx, loc.y - hy, loc.z - hz),
        (loc.x + hx, loc.y + hy, loc.z + hz),
        (loc.x, loc.y, loc.z),
    )


def _xy_dist(ax, ay, bx, by):
    dx = ax - bx
    dy = ay - by
    return (dx * dx + dy * dy) ** 0.5


def _apply_seams(mode, mn, mx):
    x0, y0, z0 = mn
    x1, y1, z1 = mx
    if mode == "vestibule":
        x0 += 40.0
        x1 -= 40.0
        y0 += 40.0
        y1 -= 40.0
    elif mode == "reception":
        x0 += 40.0
        x1 = min(x1 - 40.0, 1760.0)
        y0 += 50.0
        y1 -= 50.0
    elif mode == "hub":
        x0 = max(x0 + 50.0, 1830.0)
        x1 = min(x1 - 50.0, 2460.0)
        y0 = max(y0 + 50.0, -280.0)
        y1 = min(y1 - 50.0, 280.0)
    elif mode == "security":
        x0 += 10.0
        x1 -= 10.0
        y0 += 10.0
        y1 -= 10.0
    elif mode == "records":
        x0 += 10.0
        x1 -= 10.0
        y0 += 10.0
        y1 -= 10.0
    elif mode == "conference":
        x0 = max(x0 + 50.0, 2550.0)
        x1 -= 50.0
        y0 += 50.0
        y1 -= 50.0
    elif mode == "director":
        x0 += 50.0
        x1 = min(x1 - 50.0, 2400.0)
        y0 += 50.0
        y1 -= 50.0
    elif mode == "operations":
        x0 += 50.0
        x1 = min(x1 - 50.0, 4450.0)
        y0 = max(y0 + 50.0, -520.0)
        y1 = min(y1 - 50.0, 550.0)
    elif mode == "transit":
        x0 += 50.0
        x1 -= 50.0
        y0 += 50.0
        y1 -= 50.0
    elif mode == "service":
        x0 += 50.0
        x1 -= 50.0
        y0 += 50.0
        y1 = min(y1 - 50.0, -675.0)
    if x1 - x0 < 80.0 or y1 - y0 < 80.0:
        _fail("Seam inset collapsed volume for mode=%s aabb x=%.1f..%.1f y=%.1f..%.1f" % (
            mode, x0, x1, y0, y1
        ))
    return (x0, y0, z0), (x1, y1, z1)


def _collect_by_label(actor_sub):
    by_label = {}
    for actor in actor_sub.get_all_level_actors():
        if _actor_owner_package(actor) != REQUIRED_PACKAGE:
            continue
        by_label.setdefault(_actor_label(actor), []).append(actor)
    return by_label


def _actor_class_blob(actor):
    cls = _safe_call(actor, "get_class")
    return " ".join((
        _safe_str(cls),
        _safe_str(_safe_call(cls, "get_name") if cls else None),
        _safe_str(_safe_call(cls, "get_path_name") if cls else None),
    ))


def _class_is_a(actor, parent_cls):
    if not actor or not parent_cls:
        return False
    child = _safe_call(actor, "get_class")
    if child is None:
        return False
    if child == parent_cls:
        return True
    math = getattr(unreal, "MathLibrary", None)
    method = getattr(math, "class_is_child_of", None) if math else None
    if callable(method):
        try:
            if bool(method(child, parent_cls)):
                return True
        except Exception:
            pass
    parent_name = _safe_str(_safe_call(parent_cls, "get_name"))
    parent_path = _safe_str(_safe_call(parent_cls, "get_path_name"))
    blob = _actor_class_blob(actor)
    return (parent_name and parent_name in blob) or (parent_path and parent_path in blob)


def _matches_any_class(actor, classes, token):
    for parent_cls in classes:
        if _class_is_a(actor, parent_cls):
            return True
    blob = _actor_class_blob(actor)
    return token in blob and "StaticMeshActor" not in blob


def _is_room_trigger_actor(actor, trigger_classes):
    return _matches_any_class(actor, trigger_classes, "AdminRoomTrigger")


def _is_controller_actor(actor, controller_classes):
    return _matches_any_class(actor, controller_classes, "AdminSectorController")


def _split_by_class(actors, predicate):
    matched = [actor for actor in actors if predicate(actor)]
    others = [actor for actor in actors if actor not in matched]
    return matched, others


def _require_existing_trigger(by_label, label, trigger_classes):
    matched, others = _split_by_class(
        by_label.get(label, []),
        lambda actor: _is_room_trigger_actor(actor, trigger_classes),
    )
    if others:
        _fail(
            "%s label is shared with non-trigger class(es): %s. "
            "Refusing to treat an impostor as a room trigger."
            % (label, ", ".join(_actor_class_blob(a) for a in others))
        )
    if len(matched) > 1:
        _fail("%s already has %d BP/C++ trigger copies" % (label, len(matched)))
    return matched[0] if matched else None


def _load_optional_class(asset_path, cpp_path):
    loaded = []
    if asset_path and unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        bp_class = unreal.EditorAssetLibrary.load_blueprint_class(asset_path)
        if bp_class:
            loaded.append(bp_class)
    cpp_class = unreal.load_class(None, cpp_path) if cpp_path else None
    if cpp_class and cpp_class not in loaded:
        loaded.append(cpp_class)
    return tuple(loaded)


def _plan_security(by_label):
    """East-Hub Security only. Never follow the legacy SW-room Terminal_AdminSecurity."""
    terminals = by_label.get("Terminal_AdminSecurity", [])
    if len(terminals) > 1:
        _fail("Multiple Terminal_AdminSecurity actors; refusing Security origin.")
    if len(terminals) == 1:
        unused_min, unused_max, center = _aabb_from_actor(terminals[0])
        dist = _xy_dist(center[0], center[1], SECURITY_APPROVED_XY[0], SECURITY_APPROVED_XY[1])
        if dist > CENTER_EPS:
            _fail(
                "Live Terminal_AdminSecurity at (%.1f, %.1f) is %.1f uu from approved east-Hub Security %s. "
                "This is the legacy SW-room terminal. STOP rather than follow it. "
                "Admin_RoomTrigger_Security must use (2680, -400, 150)."
                % (center[0], center[1], dist, SECURITY_APPROVED_XY)
            )
        return (center[0], center[1], TRIGGER_Z), SECURITY_EXTENT, dist, "Terminal_AdminSecurity"

    nearest = None
    for label, actors in by_label.items():
        if not (label.startswith("Admin_SecurityOffice_") or label.startswith("Admin_S6_")):
            continue
        for actor in actors:
            unused_min, unused_max, center = _aabb_from_actor(actor)
            dist = _xy_dist(center[0], center[1], SECURITY_APPROVED_XY[0], SECURITY_APPROVED_XY[1])
            if dist <= S6_NEAR_HUB_EPS and (nearest is None or dist < nearest[0]):
                nearest = (dist, label)
    if nearest:
        _log("Security using approved east-Hub; nearest S6 actor %s delta=%.1f" % (
            nearest[1], nearest[0]
        ))
        return SECURITY_APPROVED_LOC, SECURITY_EXTENT, nearest[0], nearest[1]

    _log("Security using approved east-Hub (2680, -400, 150)")
    return SECURITY_APPROVED_LOC, SECURITY_EXTENT, 0.0, "approved_east_hub"


def _try_set_prop(obj, names, value):
    for name in names:
        try:
            obj.set_editor_property(name, value)
            return True
        except Exception:
            continue
    return False


def _set_prop(obj, names, value):
    if _try_set_prop(obj, names, value):
        return names[0]
    _fail("Failed to set %s = %s" % (names, value))


def _get_prop(obj, names, default=None):
    for name in names:
        try:
            return obj.get_editor_property(name)
        except Exception:
            continue
    return default


def _trigger_xy_aabb(loc, extent):
    return (
        loc[0] - extent[0],
        loc[1] - extent[1],
        loc[0] + extent[0],
        loc[1] + extent[1],
    )


def _xy_overlap(a, b):
    return not (a[2] <= b[0] or b[2] <= a[0] or a[3] <= b[1] or b[3] <= a[1])


def _close3(a, b, eps=0.05):
    return (
        abs(a[0] - b[0]) <= eps
        and abs(a[1] - b[1]) <= eps
        and abs(a[2] - b[2]) <= eps
    )


def _geom_snapshot(by_label, skip_predicate):
    snap = {}
    for label, actors in by_label.items():
        kept = [actor for actor in actors if not skip_predicate(actor)]
        for index, actor in enumerate(kept):
            loc = actor.get_actor_location()
            rot = actor.get_actor_rotation()
            scale = actor.get_actor_scale3d()
            snap[(label, index)] = (
                (loc.x, loc.y, loc.z),
                (rot.pitch, rot.yaw, rot.roll),
                (scale.x, scale.y, scale.z),
            )
    return snap


def _geom_changed(before, after):
    changed = []
    for key, old in before.items():
        new = after.get(key)
        if new is None:
            changed.append("missing:%s" % key[0])
            continue
        if not (_close3(old[0], new[0]) and _close3(old[1], new[1]) and _close3(old[2], new[2])):
            changed.append("%s%s" % (key[0], "" if key[1] == 0 else ("#%d" % key[1])))
    for key in after:
        if key not in before:
            changed.append("new:%s" % key[0])
    return changed


def _resolve_spawn_class():
    if not unreal.EditorAssetLibrary.does_asset_exist(BP_PATH):
        _fail("Section 13 asset missing: %s" % BP_PATH)
    bp_class = unreal.EditorAssetLibrary.load_blueprint_class(BP_PATH)
    if bp_class:
        _log("using existing generated class for %s" % BP_PATH)
        return bp_class
    cpp_class = unreal.load_class(None, PARENT_CLASS)
    if cpp_class:
        _log("BP generated class unavailable; using C++ %s" % PARENT_CLASS)
        return cpp_class
    _fail("Neither BP_AdminRoomTrigger generated class nor %s could be loaded." % PARENT_CLASS)


def _trigger_box(actor):
    box = _get_prop(actor, ("trigger_box", "TriggerBox"))
    if not box:
        box = actor.get_component_by_class(unreal.BoxComponent)
    if not box:
        _fail("%s missing TriggerBox" % _actor_label(actor))
    return box


_log("=== SECTION 15 ROOM TRIGGERS BEGIN ===")
_log("NO reparent. NO compile. NO Blueprint save. NO collision mutation. NO map save.")
_log("ZERO S1-S12 geometry edits. ZERO Admin_S13_*. ZERO Section 16.")
world, pkg = _assert_admin_map("start")
_log("active package=%s" % pkg)

spawn_class = _resolve_spawn_class()
trigger_classes = _load_optional_class(BP_PATH, PARENT_CLASS)
if spawn_class and spawn_class not in trigger_classes:
    trigger_classes = (spawn_class,) + trigger_classes
if not trigger_classes:
    _fail("No BP_AdminRoomTrigger / ProjectOrganoidAdminRoomTrigger class loaded")
controller_classes = _load_optional_class(CONTROLLER_BP_PATH, CONTROLLER_CLASS)
if not controller_classes:
    _fail("No BP_AdminSectorController / ProjectOrganoidAdminSectorController class loaded")
skip_geom = lambda actor: _is_room_trigger_actor(actor, trigger_classes)

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not actor_sub:
    _fail("EditorActorSubsystem unavailable")
by_label = _collect_by_label(actor_sub)
geom_before = _geom_snapshot(by_label, skip_geom)

labeled_controllers, controller_impostors = _split_by_class(
    by_label.get(CONTROLLER_LABEL, []),
    lambda actor: _is_controller_actor(actor, controller_classes),
)
if controller_impostors:
    _fail(
        "%s label is shared with non-controller class(es): %s"
        % (CONTROLLER_LABEL, ", ".join(_actor_class_blob(a) for a in controller_impostors))
    )
class_controllers = []
for label, actors in by_label.items():
    for actor in actors:
        if _is_controller_actor(actor, controller_classes):
            class_controllers.append((label, actor))
if len(class_controllers) != 1:
    _fail("Expected exactly 1 AdminSectorController class instance, found %s" % [
        item[0] for item in class_controllers
    ])
if len(labeled_controllers) != 1 or labeled_controllers[0] is not class_controllers[0][1]:
    _fail("Admin_SectorController label/class mismatch")
controller = labeled_controllers[0]
_log("controller present loc=%s class_count=1" % controller.get_actor_location())

s13 = [label for label in by_label if label.startswith("Admin_S13_")]
if s13:
    _fail("Admin_S13_* present: %s" % ", ".join(s13))

placements = []
for label, room_id, source_label, expected_xy, mode in SPECS:
    if mode == "security":
        loc, extent, dist, source_used = _plan_security(by_label)
        existing_actor = _require_existing_trigger(by_label, label, trigger_classes)
        placements.append((label, room_id, loc, extent, source_used, dist, mode, existing_actor))
        _log("plan %s from %s loc=(%.1f, %.1f, %.1f) extent=(%.1f, %.1f, %.1f) center_delta=%.1f existing=%s" % (
            label, source_used, loc[0], loc[1], loc[2], extent[0], extent[1], extent[2], dist,
            "yes" if existing_actor else "no",
        ))
        continue
    found = by_label.get(source_label, [])
    if len(found) != 1:
        _fail("Live source '%s' count=%d (need 1). STOP." % (source_label, len(found)))
    source = found[0]
    mn, mx, center = _aabb_from_actor(source)
    dist = _xy_dist(center[0], center[1], expected_xy[0], expected_xy[1])
    skip_center = mode == "records"
    if (not skip_center) and dist > CENTER_EPS:
        _fail("Live %s center XY=(%.1f, %.1f) is %.1f uu from expected %s. STOP." % (
            source_label, center[0], center[1], dist, expected_xy
        ))
    if mode == "records":
        loc = (center[0], center[1], TRIGGER_Z)
        extent = (140.0, 70.0, EXTENT_Z)
    else:
        smn, smx = _apply_seams(mode, mn, mx)
        loc = ((smn[0] + smx[0]) * 0.5, (smn[1] + smx[1]) * 0.5, TRIGGER_Z)
        extent = ((smx[0] - smn[0]) * 0.5, (smx[1] - smn[1]) * 0.5, EXTENT_Z)
    existing_actor = _require_existing_trigger(by_label, label, trigger_classes)
    placements.append((label, room_id, loc, extent, source_label, dist, mode, existing_actor))
    _log("plan %s from %s loc=(%.1f, %.1f, %.1f) extent=(%.1f, %.1f, %.1f) center_delta=%.1f existing=%s" % (
        label, source_label, loc[0], loc[1], loc[2], extent[0], extent[1], extent[2], dist,
        "yes" if existing_actor else "no",
    ))

overlap_pairs = []
for i, left in enumerate(placements):
    left_aabb = _trigger_xy_aabb(left[2], left[3])
    for right in placements[i + 1:]:
        right_aabb = _trigger_xy_aabb(right[2], right[3])
        if _xy_overlap(left_aabb, right_aabb):
            overlap_pairs.append("%s x %s" % (left[0], right[0]))
if overlap_pairs:
    _fail("Identity volumes would overlap (dead bands must stay empty): %s" % ", ".join(overlap_pairs))

by_mode = {item[6]: item for item in placements}
reception = by_mode.get("reception")
hub = by_mode.get("hub")
if reception and hub:
    rec_max = reception[2][0] + reception[3][0]
    hub_min = hub[2][0] - hub[3][0]
    if rec_max >= hub_min:
        _fail("Reception/Hub dead band missing: reception_max_x=%.1f hub_min_x=%.1f" % (
            rec_max, hub_min
        ))
    _log("Reception/Hub dead band X %.1f .. %.1f" % (rec_max, hub_min))

ops = by_mode.get("operations")
transit = by_mode.get("transit")
service = by_mode.get("service")
if ops and transit and transit[2][0] <= ops[2][0]:
    _fail("Transit is not east of Operations")
if ops and service and service[2][1] >= (ops[2][1] - ops[3][1]):
    _fail("Service Corridor is not south of Operations")

_log("preflight passed; beginning placement/update of 10 triggers")

created = []
updated = []
for label, room_id, loc, extent, source_label, dist, mode, existing_actor in placements:
    _assert_admin_map("pre-spawn " + label)
    if existing_actor:
        actor = existing_actor
        updated.append(label)
    else:
        actor = actor_sub.spawn_actor_from_class(
            spawn_class,
            unreal.Vector(*loc),
            unreal.Rotator(0.0, 0.0, 0.0),
        )
        if not actor:
            _fail("Failed to spawn %s" % label)
        created.append(label)
    actor.set_actor_label(label)
    actor.set_actor_location(unreal.Vector(*loc), False, False)
    actor.set_actor_rotation(unreal.Rotator(0.0, 0.0, 0.0), False)
    actor.set_actor_scale3d(unreal.Vector(1.0, 1.0, 1.0))
    _set_prop(actor, ("room_id", "RoomID"), unreal.Name(room_id))
    _trigger_box(actor).set_box_extent(unreal.Vector(*extent), True)
    if _actor_owner_package(actor) != REQUIRED_PACKAGE:
        _fail("%s owner is not SL_Epitope_Admin" % label)
    _log("%s %s RoomID=%s" % ("updated" if label in updated else "created", label, room_id))

by_label = _collect_by_label(actor_sub)
geom_after = _geom_snapshot(by_label, skip_geom)
changed = _geom_changed(geom_before, geom_after)
if changed:
    _fail("S1-S12/other existing actors changed: %s" % ", ".join(changed[:20]))

trigger_labels = [spec[0] for spec in SPECS]
class_triggers = []
for label, actors in by_label.items():
    for actor in actors:
        if _is_room_trigger_actor(actor, trigger_classes):
            class_triggers.append((label, actor))

missing = []
extra_trigger_labels = []
impostor_labels = []
counts = []
for name in trigger_labels:
    matched, others = _split_by_class(
        by_label.get(name, []),
        lambda actor: _is_room_trigger_actor(actor, trigger_classes),
    )
    if others:
        impostor_labels.append(name)
    if not matched:
        missing.append(name)
    counts.append(len(matched))
extra_trigger_labels = [
    label for label, unused_actor in class_triggers
    if label not in trigger_labels
]
post_controllers, post_controller_impostors = _split_by_class(
    by_label.get(CONTROLLER_LABEL, []),
    lambda actor: _is_controller_actor(actor, controller_classes),
)
class_controller_count = 0
for actors in by_label.values():
    for actor in actors:
        if _is_controller_actor(actor, controller_classes):
            class_controller_count += 1
if (
    missing
    or extra_trigger_labels
    or impostor_labels
    or any(c != 1 for c in counts)
    or len(class_triggers) != 10
    or class_controller_count != 1
    or len(post_controllers) != 1
    or post_controller_impostors
):
    _fail("Post-place verify failed missing=%s extra=%s impostors=%s counts=%s class_triggers=%d controllers=%d" % (
        missing, extra_trigger_labels, impostor_labels, counts, len(class_triggers), class_controller_count
    ))
if [name for name in by_label if name.startswith("Admin_S13_")]:
    _fail("Admin_S13_* appeared")

room_ids = []
for label, room_id, loc, extent, source_label, dist, mode, existing_actor in placements:
    actor = _require_existing_trigger(by_label, label, trigger_classes)
    actual = _safe_str(_get_prop(actor, ("room_id", "RoomID")))
    if actual != room_id:
        _fail("%s RoomID=%s expected %s" % (label, actual, room_id))
    room_ids.append(actual)

_log("trigger count=10 controller count=1 Admin_S13_*=0 extra=0")
_log("RoomIDs=%s" % ",".join(room_ids))
_log("created=%s" % (",".join(created) if created else "(none)"))
_log("updated=%s" % (",".join(updated) if updated else "(none)"))
_log("map NOT saved. Save SL_Epitope_Admin manually after verification.")
_log("=== SECTION 15 ROOM TRIGGERS COMPLETE ===")
_log("Section 16 was not started.")
