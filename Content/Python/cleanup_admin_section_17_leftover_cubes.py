# ProjectOrganoid — Section 17 leftover template cube cleanup.
# Deletes ONLY Cube / Cube2 / Cube3 on the Vestibule Y=0 centerline.
# Does not touch Admin_S1_* geometry, BP_AdminAccessDoor, PlayerStart,
# ProjectOrganoidCharacter, input/camera, or any other map.
# Does not Save All. Saves only SL_Epitope_Admin after a successful delete.
# Does not begin Section 18.
#
# Stop PIE first, then:
#   py "C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Content\Python\cleanup_admin_section_17_leftover_cubes.py"

import unreal

TAG = "[S17 CUBES] "
ADMIN_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
MENU_PACKAGE = "/Game/Maps/Lvl_MainMenu"
SPINE_PACKAGE = "/Game/Maps/Lvl_Epitope"
CUBE_MESH_PATHS = (
    "/Engine/BasicShapes/Cube",
    "/Engine/BasicShapes/Cube.Cube",
)
LOC_EPS = 0.51
SCALE_EPS = 0.01

TARGETS = (
    ("Cube", (0.0, 0.0, 0.0)),
    ("Cube2", (200.0, 0.0, 0.0)),
    ("Cube3", (400.0, 0.0, 0.0)),
)
PROTECTED_LABELS = (
    "Admin_S1_Vestibule_Floor",
    "Admin_S1_Reception_Wall_NegX_PosY",
    "Admin_S1_Reception_Wall_NegX_NegY",
    "PlayerStart_ReceptionAtrium",
)


def _log(msg):
    unreal.log(TAG + msg)


def _fail(msg):
    _log("=== SECTION 17 CUBE CLEANUP ABORTED ===")
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
    if pkg == ADMIN_PACKAGE or pkg.endswith("/SL_Epitope_Admin") or pkg == "SL_Epitope_Admin":
        return ADMIN_PACKAGE
    if pkg == MENU_PACKAGE or pkg.endswith("/Lvl_MainMenu") or pkg == "Lvl_MainMenu":
        return MENU_PACKAGE
    if pkg == SPINE_PACKAGE or pkg.endswith("/Lvl_Epitope") or pkg == "Lvl_Epitope":
        return SPINE_PACKAGE
    return pkg


def _get_editor_world():
    sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    if sub:
        world = _safe_call(sub, "get_editor_world")
        if world:
            return world
    return None


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


def _fmt_vec(vec):
    if vec is None:
        return "None"
    return "(%.2f, %.2f, %.2f)" % (vec.x, vec.y, vec.z)


def _near(vec, expected):
    if vec is None:
        return False
    return (
        abs(vec.x - expected[0]) <= LOC_EPS
        and abs(vec.y - expected[1]) <= LOC_EPS
        and abs(vec.z - expected[2]) <= LOC_EPS
    )


def _scale_ok(vec):
    if vec is None:
        return False
    return (
        abs(vec.x - 1.0) <= SCALE_EPS
        and abs(vec.y - 1.0) <= SCALE_EPS
        and abs(vec.z - 1.0) <= SCALE_EPS
    )


def _mesh_path(actor):
    # Same StaticMeshActor lookup used by Admin S9–S12 builders: the mesh
    # lives on the StaticMeshComponent as editor property "static_mesh".
    # get_static_mesh() / get_components_by_class are not the UE5 Python API.
    mesh_comp = _safe_call(actor, "get_component_by_class", unreal.StaticMeshComponent)
    if mesh_comp is None:
        mesh_comp = getattr(actor, "static_mesh_component", None)
    if not mesh_comp:
        return ""
    static_mesh = None
    try:
        static_mesh = mesh_comp.get_editor_property("static_mesh")
    except Exception:
        static_mesh = None
    if static_mesh is None:
        static_mesh = _safe_call(mesh_comp, "get_static_mesh")
    if not static_mesh:
        return ""
    return _safe_str(_safe_call(static_mesh, "get_path_name"), "")


def _is_engine_cube_mesh(path):
    cleaned = (path or "").replace("\\", "/")
    return (
        cleaned in CUBE_MESH_PATHS
        or "BasicShapes/Cube" in cleaned
        or cleaned.endswith("/Cube")
        or cleaned.endswith(".Cube")
    )


def _attached_parent_name(actor):
    root = _safe_call(actor, "get_root_component")
    parent = _safe_call(root, "get_attach_parent") if root else None
    if parent is None:
        return ""
    owner = _safe_call(parent, "get_owner")
    return _actor_label(owner) if owner else _safe_str(_safe_call(parent, "get_name"))


def _child_actor_labels(actor):
    labels = []
    attached = _safe_call(actor, "get_attached_actors")
    if attached:
        for child in attached:
            labels.append(_actor_label(child))
    return labels


def _collect_actors(actor_sub):
    try:
        return list(actor_sub.get_all_level_actors())
    except Exception as exc:
        _fail("EditorActorSubsystem.get_all_level_actors failed: %s" % exc)
        return []


# --- preflight ---
editor_sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
game_world = _safe_call(editor_sub, "get_game_world") if editor_sub else None
if game_world:
    _fail("PIE is running. Stop Play first. ZERO deletions.")

world = _get_editor_world()
if world is None:
    _fail("No editor world. ZERO deletions.")

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not actor_sub:
    _fail("EditorActorSubsystem unavailable. ZERO deletions.")

loaded = _collect_actors(actor_sub)
by_label = {}
for actor in loaded:
    label = _actor_label(actor)
    by_label.setdefault(label, []).append(actor)

for protected in PROTECTED_LABELS:
    if protected not in by_label:
        _log("note: protected actor not in loaded set (ok if Admin not current): %s" % protected)

sequence_hits = []
for actor in loaded:
    cls = _safe_call(actor, "get_class")
    cls_name = _safe_str(_safe_call(cls, "get_name") if cls else None)
    if "LevelSequence" in cls_name or "Sequencer" in cls_name:
        sequence_hits.append("%s/%s" % (_actor_label(actor), cls_name))
if sequence_hits:
    _log("loaded sequencer actors (not auto-bound as cube refs): %s" % ", ".join(sequence_hits))

selected = []
errors = []
for label, expected_loc in TARGETS:
    matches = by_label.get(label, [])
    if len(matches) != 1:
        errors.append("%s count=%d expected=1" % (label, len(matches)))
        continue
    actor = matches[0]
    loc = actor.get_actor_location()
    scale = actor.get_actor_scale3d()
    owner = _actor_owner_package(actor)
    cls = _safe_call(actor, "get_class")
    cls_name = _safe_str(_safe_call(cls, "get_name") if cls else None)
    mesh = _mesh_path(actor)
    parent = _attached_parent_name(actor)
    children = _child_actor_labels(actor)
    path = _safe_str(_safe_call(actor, "get_path_name"))
    _log(
        "CANDIDATE label=%s class=%s loc=%s scale=%s owner=%s mesh=%s parent=%s children=%s path=%s"
        % (label, cls_name, _fmt_vec(loc), _fmt_vec(scale), owner, mesh or "None", parent or "None", children, path)
    )
    if cls_name != "StaticMeshActor":
        errors.append("%s class=%s expected StaticMeshActor" % (label, cls_name))
    if not _near(loc, expected_loc):
        errors.append("%s loc=%s expected=%s" % (label, _fmt_vec(loc), expected_loc))
    if not _scale_ok(scale):
        errors.append("%s scale=%s expected=(1,1,1)" % (label, _fmt_vec(scale)))
    if owner != ADMIN_PACKAGE:
        errors.append("%s owner=%s expected=%s" % (label, owner, ADMIN_PACKAGE))
    if not _is_engine_cube_mesh(mesh):
        errors.append("%s mesh=%s expected Engine BasicShapes Cube" % (label, mesh))
    if label.startswith("Admin_"):
        errors.append("%s looks like authored Admin geometry" % label)
    if parent:
        errors.append("%s is attached to %s" % (label, parent))
    if children:
        errors.append("%s has attached children %s" % (label, children))
    selected.append((label, owner, actor, path))

# Disk/code reference audit is recorded in the Cursor report. In-editor extra
# copies of these exact labels on Admin would mean we might delete the wrong one.
for label, _expected_loc in TARGETS:
    extras = [a for a in by_label.get(label, []) if a not in [row[2] for row in selected]]
    if extras:
        errors.append("%s has extra loaded copies=%d" % (label, len(extras)))

if errors:
    _fail("Preflight failed; ZERO deletions. " + " | ".join(errors))

_log("=== PREFLIGHT PASSED — unreferenced leftover cubes on %s ===" % ADMIN_PACKAGE)

deleted = []
for label, owner, actor, path in selected:
    destroyed = actor_sub.destroy_actor(actor)
    if not destroyed:
        _fail("destroy_actor returned false for %s path=%s. Partial deletion possible; map not saved." % (label, path))
    deleted.append(label)
    _log("deleted label=%s owner=%s path=%s" % (label, owner, path))

saved = unreal.EditorAssetLibrary.save_asset(ADMIN_PACKAGE, only_if_is_dirty=False)
_log("saved %s ok=%s" % (ADMIN_PACKAGE, saved))
if not saved:
    _fail("Deleted %s but failed to save %s." % (deleted, ADMIN_PACKAGE))

remaining = _collect_actors(actor_sub)
remaining_labels = [_actor_label(a) for a in remaining]
verify_errors = []
for label, expected_loc in TARGETS:
    still = [a for a in remaining if _actor_label(a) == label]
    if still:
        verify_errors.append("%s still present count=%d" % (label, len(still)))
for protected in PROTECTED_LABELS:
    if protected in by_label and protected not in remaining_labels:
        verify_errors.append("protected actor missing after cleanup: %s" % protected)

if verify_errors:
    _fail(" | ".join(verify_errors))

_log("=== SECTION 17 LEFTOVER CUBE CLEANUP VERIFIED ===")
_log("deleted count=%d labels=%s" % (len(deleted), ",".join(deleted)))
_log("saved package=%s" % ADMIN_PACKAGE)
_log("Lvl_Epitope / Lvl_MainMenu / S1-S16 authored actors were not saved or modified.")
_log("Section 18 not started.")
