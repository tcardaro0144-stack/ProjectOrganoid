# ProjectOrganoid — READ-ONLY Section 7 / 8 ownership audit.
# Does not spawn, move, delete, rename, save, load/unload, or change visibility.

import unreal

S7_LABEL = "Admin_RecordsArchives_Block"
S8_PREFIX = "Admin_ConferenceRoom_"
ADMIN_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
MENU_PACKAGE = "/Game/Maps/Lvl_MainMenu"


def _log(msg):
    unreal.log(msg)


def _fmt_vec(v):
    return "(%.2f, %.2f, %.2f)" % (v.x, v.y, v.z)


def _safe_call(obj, method_name):
    if obj is None:
        return None
    method = getattr(obj, method_name, None)
    if not callable(method):
        return None
    try:
        return method()
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


def _object_name(obj):
    return _safe_str(_safe_call(obj, "get_name"))


def _object_path(obj):
    return _safe_str(_safe_call(obj, "get_path_name"))


def _outermost(obj):
    return _safe_call(obj, "get_outermost")


def _outermost_name(obj):
    return _object_name(_outermost(obj))


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


def _normalize_package(text):
    if not text or text == "None":
        return ""
    cleaned = text.replace("\\", "/")
    if "|" in cleaned:
        cleaned = cleaned.split("|")[0].strip()
    if cleaned.endswith("."):
        cleaned = cleaned[:-1]
    return cleaned


def _package_from_actor_path(actor_path):
    if not actor_path or actor_path == "None":
        return ""
    cleaned = actor_path.replace("\\", "/")
    if "." in cleaned:
        return cleaned.split(".", 1)[0]
    return cleaned


def _classify_ownership(package_candidates):
    admin_hit = False
    menu_hit = False
    other = []
    for raw in package_candidates:
        pkg = _normalize_package(raw)
        if not pkg:
            continue
        if pkg == ADMIN_PACKAGE or pkg.endswith("/SL_Epitope_Admin") or pkg == "SL_Epitope_Admin":
            admin_hit = True
            continue
        if pkg == MENU_PACKAGE or pkg.endswith("/Lvl_MainMenu") or pkg == "Lvl_MainMenu":
            menu_hit = True
            continue
        if pkg.startswith("/"):
            other.append(pkg)
        elif pkg not in ("None", "Transient", "World"):
            other.append(pkg)
    if admin_hit and not menu_hit and not other:
        return ADMIN_PACKAGE
    if menu_hit and not admin_hit and not other:
        return MENU_PACKAGE
    if admin_hit or menu_hit or other:
        mixed = []
        if admin_hit:
            mixed.append(ADMIN_PACKAGE)
        if menu_hit:
            mixed.append(MENU_PACKAGE)
        mixed.extend(other)
        unique = []
        for item in mixed:
            if item not in unique:
                unique.append(item)
        if len(unique) == 1:
            return unique[0]
        return "mixed:" + ",".join(unique)
    return "cannot be determined"


def _ownership_bucket(classification):
    if classification == ADMIN_PACKAGE:
        return "SL_Epitope_Admin"
    if classification == MENU_PACKAGE:
        return "Lvl_MainMenu"
    if classification == "cannot be determined":
        return "undetermined"
    return "elsewhere"


world = _get_editor_world()
actors = _collect_loaded_actors(world)

s7_rows = []
s8_rows = []

_log("=== SECTION 7/8 OWNERSHIP AUDIT (detail) ===")
_log("active_editor_world_name=%s" % _object_name(world))
_log("active_editor_world_path=%s" % _object_path(world))
_log("NOTE: active editor world is context only; ownership is taken from each actor Outer/Level/package")

for actor in actors:
    label = actor.get_actor_label()
    is_s7 = label == S7_LABEL
    is_s8 = label.startswith(S8_PREFIX)
    if not is_s7 and not is_s8:
        continue

    loc = actor.get_actor_location()
    scale = actor.get_actor_scale3d()
    outer = _safe_call(actor, "get_outer")
    owning_level = _get_actor_level(actor)
    actor_path = _object_path(actor)
    outer_path = _object_path(outer)
    level_name = _object_name(owning_level)
    level_path = _object_path(owning_level)
    level_pkg_name = _outermost_name(owning_level)
    level_pkg_path = _outermost_path(owning_level)
    actor_pkg_name = _outermost_name(actor)
    actor_pkg_path = _outermost_path(actor)
    outer_pkg_name = _outermost_name(outer)
    outer_pkg_path = _outermost_path(outer)
    actor_path_package = _package_from_actor_path(actor_path)

    classification = _classify_ownership([
        level_pkg_path,
        level_pkg_name,
        actor_pkg_path,
        actor_pkg_name,
        outer_pkg_path,
        outer_pkg_name,
        actor_path_package,
        _package_from_actor_path(outer_path),
        _package_from_actor_path(level_path),
    ])

    row = {
        "label": label,
        "loc": loc,
        "scale": scale,
        "outer_name": _object_name(outer),
        "outer_path": outer_path,
        "level_name": level_name,
        "level_path": level_path,
        "level_pkg_name": level_pkg_name,
        "level_pkg_path": level_pkg_path,
        "actor_path": actor_path,
        "classification": classification,
        "bucket": _ownership_bucket(classification),
    }

    _log(
        "ACTOR label=%s loc=%s scale=%s outer=%s outer_path=%s owning_level=%s owning_level_path=%s outermost_name=%s outermost_path=%s actor_path=%s ownership=%s"
        % (
            row["label"],
            _fmt_vec(row["loc"]),
            _fmt_vec(row["scale"]),
            row["outer_name"],
            row["outer_path"],
            row["level_name"],
            row["level_path"],
            row["level_pkg_name"],
            row["level_pkg_path"],
            row["actor_path"],
            row["classification"],
        )
    )

    if is_s7:
        s7_rows.append(row)
    if is_s8:
        s8_rows.append(row)

s8_rows.sort(key=lambda r: r["label"])

_log("=== SECTION 7/8 OWNERSHIP AUDIT ===")

_log("Section 7 actor:")
if not s7_rows:
    _log("  label=Admin_RecordsArchives_Block")
    _log("  owning level/package=NOT FOUND")
    _log("  ownership classification=cannot be determined")
else:
    for row in s7_rows:
        _log("  label=%s" % row["label"])
        _log("  owning level/package=%s | %s" % (row["level_name"], row["level_pkg_path"]))
        _log("  ownership classification=%s" % row["classification"])

_log("Section 8 actors:")
_log("  total found=%d" % len(s8_rows))
if not s8_rows:
    _log("  Admin_ConferenceRoom_* : NOT FOUND")
    _log("  count owned by SL_Epitope_Admin=0")
    _log("  count owned by Lvl_MainMenu=0")
    _log("  count owned elsewhere=0")
    _log("  count undetermined=0")
else:
    admin_count = 0
    menu_count = 0
    elsewhere_count = 0
    undetermined_count = 0
    owners = {}
    for row in s8_rows:
        _log(
            "  %s -> owning level/package=%s | %s | classification=%s"
            % (row["label"], row["level_name"], row["level_pkg_path"], row["classification"])
        )
        bucket = row["bucket"]
        if bucket == "SL_Epitope_Admin":
            admin_count += 1
        elif bucket == "Lvl_MainMenu":
            menu_count += 1
        elif bucket == "undetermined":
            undetermined_count += 1
        else:
            elsewhere_count += 1
        key = row["classification"]
        owners.setdefault(key, []).append(row["label"])

    _log("  count owned by SL_Epitope_Admin=%d" % admin_count)
    _log("  count owned by Lvl_MainMenu=%d" % menu_count)
    _log("  count owned elsewhere=%d" % elsewhere_count)
    _log("  count undetermined=%d" % undetermined_count)

    if len(owners) == 1:
        only = list(owners.keys())[0]
        _log("  ALL Section 8 actors share one owner: %s" % only)
    else:
        _log("  ownership is MIXED across Section 8 actors")
        for owner, labels in owners.items():
            _log("  owner=%s count=%d" % (owner, len(labels)))
            for label in labels:
                _log("    - %s" % label)
