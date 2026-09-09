# ProjectOrganoid — PHASE 1 READ-ONLY capture of mistaken Sections 6–8 actors.
# Intended to run while Lvl_MainMenu is the active editor world.
# Does not spawn, move, delete, rename, save, load/unload, or change visibility.

import unreal

S7_LABEL = "Admin_RecordsArchives_Block"
S8_PREFIX = "Admin_ConferenceRoom_"
S6_EXACT = ("Terminal_AdminSecurity",)
S6_PREFIXES = ("Admin_SecurityOffice_", "Admin_S6_")
MENU_PACKAGE = "/Game/Maps/Lvl_MainMenu"
ADMIN_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"


def _log(msg):
    unreal.log(msg)


def _fmt_vec(v, digits=6):
    fmt = "(%%.%df, %%.%df, %%.%df)" % (digits, digits, digits)
    return fmt % (v.x, v.y, v.z)


def _fmt_rot(r, digits=6):
    fmt = "(%%.%df, %%.%df, %%.%df)" % (digits, digits, digits)
    return fmt % (r.pitch, r.yaw, r.roll)


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


def _safe_prop(obj, names, fallback=None):
    if obj is None:
        return fallback
    for name in names:
        try:
            return obj.get_editor_property(name)
        except Exception:
            continue
    return fallback


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


def _object_name(obj):
    return _safe_str(_safe_call(obj, "get_name"))


def _object_path(obj):
    return _safe_str(_safe_call(obj, "get_path_name"))


def _outermost(obj):
    return _safe_call(obj, "get_outermost")


def _outermost_path(obj):
    return _object_path(_outermost(obj))


def _get_editor_world():
    sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    if sub:
        world = _safe_call(sub, "get_editor_world")
        if world:
            return world
    try:
        return unreal.EditorLevelLibrary.get_editor_world()
    except Exception:
        return None


def _get_actor_level(actor):
    level = _safe_call(actor, "get_level")
    if level is not None:
        return level
    return _safe_call(actor, "get_outer")


def _package_from_path(path):
    cleaned = _safe_str(path, "")
    cleaned = cleaned.replace("\\", "/")
    if not cleaned or cleaned == "None":
        return ""
    if "." in cleaned:
        cleaned = cleaned.split(".", 1)[0]
    return cleaned


def _classify_package(pkg):
    if pkg == ADMIN_PACKAGE or pkg.endswith("/SL_Epitope_Admin"):
        return ADMIN_PACKAGE
    if pkg == MENU_PACKAGE or pkg.endswith("/Lvl_MainMenu"):
        return MENU_PACKAGE
    if pkg:
        return pkg
    return "cannot be determined"


def _actor_owner_package(actor):
    owning_level = _get_actor_level(actor)
    candidates = [
        _package_from_path(_outermost_path(actor)),
        _package_from_path(_outermost_path(owning_level)),
        _package_from_path(_object_path(actor)),
        _package_from_path(_object_path(_safe_call(actor, "get_outer"))),
        _package_from_path(_object_path(owning_level)),
    ]
    for candidate in candidates:
        classified = _classify_package(candidate)
        if classified != "cannot be determined":
            return classified
    return "cannot be determined"


def _is_section6(label):
    if label in S6_EXACT:
        return True
    for prefix in S6_PREFIXES:
        if label.startswith(prefix):
            return True
    lowered = label.lower()
    if "securityoffice" in lowered or "security_office" in lowered:
        return True
    return False


def _is_section7(label):
    return label == S7_LABEL or label.startswith("Admin_RecordsArchives_") or label.startswith("Admin_S7_")


def _is_section8(label):
    return label.startswith(S8_PREFIX) or label.startswith("Admin_S8_")


def _section_id(label):
    if _is_section6(label):
        return 6
    if _is_section7(label):
        return 7
    if _is_section8(label):
        return 8
    return 0


def _collect_loaded_actors(world):
    found = {}
    actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if actor_sub:
        try:
            for actor in actor_sub.get_all_level_actors():
                if actor:
                    found[_object_path(actor)] = actor
        except Exception as exc:
            _log("EditorActorSubsystem.get_all_level_actors failed: %s" % exc)
    if world:
        try:
            for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
                if actor:
                    found[_object_path(actor)] = actor
        except Exception as exc:
            _log("GameplayStatics.get_all_actors_of_class failed: %s" % exc)
    return list(found.values())


def _asset_path(obj):
    if obj is None:
        return "None"
    path = _object_path(obj)
    if path.endswith("_C"):
        return path
    try:
        loaded = unreal.EditorAssetLibrary.get_path_name_for_loaded_asset(obj)
        if loaded:
            return loaded
    except Exception:
        pass
    return path


def _mesh_info(actor):
    mesh = actor.get_component_by_class(unreal.StaticMeshComponent)
    if not mesh:
        return {
            "mesh_path": "None",
            "collision_profile": "None",
            "collision_enabled": "None",
            "mobility": "None",
            "materials": "None",
            "rel_loc": "None",
            "rel_rot": "None",
            "rel_scale": "None",
            "cast_shadow": "None",
        }
    static_mesh = _safe_prop(mesh, ["static_mesh", "StaticMesh"])
    profile = _safe_call(mesh, "get_collision_profile_name")
    if profile is None:
        profile = _safe_prop(mesh, ["collision_profile_name", "CollisionProfileName"])
    collision_enabled = _safe_call(mesh, "get_collision_enabled")
    if collision_enabled is None:
        collision_enabled = _safe_prop(mesh, ["collision_enabled", "CollisionEnabled"])
    mobility = _safe_prop(mesh, ["mobility", "Mobility"])
    materials = []
    num_materials = _safe_call(mesh, "get_num_materials")
    if isinstance(num_materials, int):
        for index in range(num_materials):
            mat = _safe_call(mesh, "get_material", index)
            materials.append(_asset_path(mat) if mat else "None")
    rel_loc = _safe_call(mesh, "get_relative_location")
    rel_rot = _safe_call(mesh, "get_relative_rotation")
    rel_scale = _safe_call(mesh, "get_relative_scale3d")
    cast_shadow = _safe_prop(mesh, ["cast_shadow", "CastShadow"])
    return {
        "mesh_path": _asset_path(static_mesh) if static_mesh else "None",
        "collision_profile": _safe_str(profile),
        "collision_enabled": _safe_str(collision_enabled),
        "mobility": _safe_str(mobility),
        "materials": ",".join(materials) if materials else "None",
        "rel_loc": _fmt_vec(rel_loc) if rel_loc else "None",
        "rel_rot": _fmt_rot(rel_rot) if rel_rot else "None",
        "rel_scale": _fmt_vec(rel_scale) if rel_scale else "None",
        "cast_shadow": _safe_str(cast_shadow),
    }


def _folder_path(actor):
    folder = _safe_call(actor, "get_folder_path")
    return _safe_str(folder, "")


def _hidden(actor):
    hidden = _safe_call(actor, "is_hidden_ed")
    if hidden is None:
        hidden = _safe_prop(actor, ["hidden", "bHidden"])
    in_game = _safe_prop(actor, ["hidden", "bHidden"], None)
    return _safe_str(hidden), _safe_str(in_game)


def _tags(actor):
    tags = getattr(actor, "tags", None)
    if not tags:
        return "None"
    names = []
    for tag in tags:
        names.append(_safe_str(tag))
    return ",".join(names) if names else "None"


world = _get_editor_world()
world_pkg = _classify_package(_package_from_path(_object_path(world)) or _package_from_path(_outermost_path(world)))

_log("=== SECTION 6-8 PHASE 1 CAPTURE (detail) ===")
_log("active_editor_world_name=%s" % _object_name(world))
_log("active_editor_world_path=%s" % _object_path(world))
_log("active_editor_world_package=%s" % world_pkg)
_log("NOTE: capture is read-only; transforms come from the actors themselves")
if world_pkg != MENU_PACKAGE:
    _log("WARNING: active world package is not %s" % MENU_PACKAGE)

actors = _collect_loaded_actors(world)
matched = []
for actor in actors:
    label = actor.get_actor_label()
    section = _section_id(label)
    if section == 0:
        continue
    loc = actor.get_actor_location()
    rot = actor.get_actor_rotation()
    scale = actor.get_actor_scale3d()
    cls = actor.get_class()
    mesh_info = _mesh_info(actor)
    owner = _actor_owner_package(actor)
    hidden_ed, hidden_game = _hidden(actor)
    row = {
        "section": section,
        "label": label,
        "class_path": _object_path(cls),
        "loc": loc,
        "rot": rot,
        "scale": scale,
        "owner": owner,
        "actor_path": _object_path(actor),
        "folder": _folder_path(actor),
        "tags": _tags(actor),
        "hidden_ed": hidden_ed,
        "hidden_game": hidden_game,
    }
    row.update(mesh_info)
    matched.append(row)
    _log(
        "CAPTURE_DETAIL section=%d label=%s class=%s loc=%s rot_pyr=%s scale=%s mesh=%s collision_profile=%s collision_enabled=%s mobility=%s materials=%s rel_loc=%s rel_rot=%s rel_scale=%s folder=%s tags=%s hidden_ed=%s owner=%s actor_path=%s"
        % (
            row["section"],
            row["label"],
            row["class_path"],
            _fmt_vec(row["loc"]),
            _fmt_rot(row["rot"]),
            _fmt_vec(row["scale"]),
            row["mesh_path"],
            row["collision_profile"],
            row["collision_enabled"],
            row["mobility"],
            row["materials"],
            row["rel_loc"],
            row["rel_rot"],
            row["rel_scale"],
            row["folder"],
            row["tags"],
            row["hidden_ed"],
            row["owner"],
            row["actor_path"],
        )
    )

matched.sort(key=lambda r: (r["section"], r["label"]))
mainmenu_rows = [r for r in matched if r["owner"] == MENU_PACKAGE]
other_rows = [r for r in matched if r["owner"] != MENU_PACKAGE]

_log("=== COPYABLE RECOVERY SPEC (Lvl_MainMenu-owned only) ===")
_log("RECOVERY_SPEC = [")
if not mainmenu_rows:
    _log("]")
    _log("RECOVERY_SPEC empty: no matching Section 6-8 actors owned by Lvl_MainMenu")
else:
    last_index = len(mainmenu_rows) - 1
    for index, row in enumerate(mainmenu_rows):
        comma = "," if index != last_index else ""
        _log(
            "    (%d, '%s', '%s', %s, %s, %s, '%s', '%s', '%s')%s"
            % (
                row["section"],
                row["label"],
                row["class_path"],
                _fmt_vec(row["loc"]),
                _fmt_rot(row["rot"]),
                _fmt_vec(row["scale"]),
                row["mesh_path"],
                row["collision_profile"],
                row["collision_enabled"],
                comma,
            )
        )
    _log("]")
    _log("RECOVERY_SPEC tuple fields: section, label, class_path, location, rotation_pyr, scale, mesh_path, collision_profile, collision_enabled")

_log("=== MATCHES NOT OWNED BY Lvl_MainMenu ===")
if not other_rows:
    _log("none")
else:
    for row in other_rows:
        _log(
            "OTHER_OWNER section=%d label=%s owner=%s loc=%s scale=%s"
            % (row["section"], row["label"], row["owner"], _fmt_vec(row["loc"]), _fmt_vec(row["scale"]))
        )


def _count(rows, section):
    return len([r for r in rows if r["section"] == section])


s6_menu = _count(mainmenu_rows, 6)
s7_menu = _count(mainmenu_rows, 7)
s8_menu = _count(mainmenu_rows, 8)
s6_all = _count(matched, 6)
s7_all = _count(matched, 7)
s8_all = _count(matched, 8)

_log("=== SECTION 6-8 PHASE 1 CAPTURE ===")
_log("active_world_package=%s" % world_pkg)
_log("matched_total=%d mainmenu_owned=%d other_owned=%d" % (len(matched), len(mainmenu_rows), len(other_rows)))
_log("Section 6 captured on Lvl_MainMenu=%d (all loaded matches=%d)" % (s6_menu, s6_all))
_log("Section 7 captured on Lvl_MainMenu=%d (all loaded matches=%d)" % (s7_menu, s7_all))
_log("Section 8 captured on Lvl_MainMenu=%d (all loaded matches=%d)" % (s8_menu, s8_all))
if s7_menu == 0:
    _log("Admin_RecordsArchives_Block owned by Lvl_MainMenu: NOT FOUND")
if s8_menu == 0:
    _log("Admin_ConferenceRoom_* owned by Lvl_MainMenu: NOT FOUND")
if s6_menu == 0:
    _log("Section 6 actors owned by Lvl_MainMenu: NOT FOUND")
_log("Phase 1 complete. No actors were modified or saved.")
