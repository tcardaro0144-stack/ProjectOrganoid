# ProjectOrganoid — READ-ONLY Admin actor ownership / level-context audit.
# Does not spawn, move, delete, rename, save, or change streaming/visibility.

import unreal

PREFIX = "Admin_"
S7_BLOCK_LABEL = "Admin_RecordsArchives_Block"
S8_PREFIX = "Admin_ConferenceRoom_"


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
    name = _safe_call(obj, "get_name")
    return _safe_str(name)


def _object_path(obj):
    path = _safe_call(obj, "get_path_name")
    return _safe_str(path)


def _outermost_path(obj):
    outermost = _safe_call(obj, "get_outermost")
    if outermost is None:
        return "None"
    name = _safe_call(outermost, "get_name")
    path = _safe_call(outermost, "get_path_name")
    if name and path and name != path:
        return "%s | %s" % (name, path)
    return _safe_str(name or path)


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


def _get_current_level():
    sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not sub:
        return None
    return _safe_call(sub, "get_current_level")


def _get_actor_level(actor):
    level = _safe_call(actor, "get_level")
    if level is not None:
        return level
    return _safe_call(actor, "get_outer")


def _is_section6(label):
    if label.startswith("Admin_SecurityOffice_"):
        return True
    if label.startswith("Admin_S6_"):
        return True
    lowered = label.lower()
    if "securityoffice" in lowered:
        return True
    if "security_office" in lowered:
        return True
    return False


def _is_section7(label):
    if label == S7_BLOCK_LABEL:
        return True
    if label.startswith("Admin_RecordsArchives_"):
        return True
    if label.startswith("Admin_S7_"):
        return True
    return False


def _is_section8(label):
    if label.startswith(S8_PREFIX):
        return True
    if label.startswith("Admin_S8_"):
        return True
    return False


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


def _log_loaded_levels(world):
    _log("--- loaded levels in current editor world (read-only) ---")
    if not world:
        _log("no editor world; cannot enumerate levels")
        return
    try:
        levels = unreal.EditorLevelUtils.get_levels(world)
    except Exception as exc:
        _log("EditorLevelUtils.get_levels failed: %s" % exc)
        return
    _log("loaded_level_count=%d" % len(levels))
    for level in levels:
        outer = _safe_call(level, "get_outer")
        _log(
            "LOADED_LEVEL name=%s path=%s outer=%s outermost=%s"
            % (
                _object_name(level),
                _object_path(level),
                _object_name(outer),
                _outermost_path(level),
            )
        )


world = _get_editor_world()
current_level = _get_current_level()

_log("=== ADMIN ACTOR OWNERSHIP AUDIT (detail) ===")
_log("editor_world_name=%s" % _object_name(world))
_log("editor_world_path=%s" % _object_path(world))
_log("editor_world_outermost=%s" % _outermost_path(world))
_log("current_level_name=%s" % _object_name(current_level))
_log("current_level_path=%s" % _object_path(current_level))
_log("current_level_outermost=%s" % _outermost_path(current_level))
_log_loaded_levels(world)

actors = _collect_loaded_actors(world)
admin_actors = []
for actor in actors:
    label = actor.get_actor_label()
    if label.startswith(PREFIX):
        admin_actors.append(actor)

admin_actors.sort(key=lambda a: a.get_actor_label())
_log("loaded_actor_count=%d" % len(actors))
_log("admin_actor_count=%d" % len(admin_actors))

section6 = []
section7 = []
section8 = []
s7_block_found = False

for actor in admin_actors:
    label = actor.get_actor_label()
    loc = actor.get_actor_location()
    owning_level = _get_actor_level(actor)
    outer = _safe_call(actor, "get_outer")
    row = {
        "label": label,
        "loc": loc,
        "level_name": _object_name(owning_level),
        "level_path": _object_path(owning_level),
        "level_package": _outermost_path(owning_level),
        "outer_name": _object_name(outer),
        "outer_path": _object_path(outer),
        "actor_path": _object_path(actor),
    }
    _log(
        "ACTOR label=%s loc=%s owning_level=%s owning_level_path=%s owning_package=%s outer=%s outer_path=%s actor_path=%s"
        % (
            row["label"],
            _fmt_vec(row["loc"]),
            row["level_name"],
            row["level_path"],
            row["level_package"],
            row["outer_name"],
            row["outer_path"],
            row["actor_path"],
        )
    )
    if _is_section6(label):
        section6.append(row)
    if _is_section7(label):
        section7.append(row)
    if label == S7_BLOCK_LABEL:
        s7_block_found = True
    if _is_section8(label):
        section8.append(row)


def _print_section(title, rows):
    _log("--- %s ---" % title)
    if not rows:
        _log("NOT FOUND")
        return
    for row in rows:
        _log(
            "OWNED_BY label=%s level=%s package=%s"
            % (row["label"], row["level_name"], row["level_package"])
        )


_log("=== ADMIN ACTOR OWNERSHIP AUDIT ===")
_log("editor_world=%s" % _object_name(world))
_log("editor_world_path=%s" % _object_path(world))
_log("editor_world_package=%s" % _outermost_path(world))
_print_section("Section 6 / Security Office actors", section6)
_print_section("Section 7 / Admin_RecordsArchives_Block and RecordsArchives actors", section7)
if not s7_block_found:
    _log("Admin_RecordsArchives_Block: NOT FOUND")
_print_section("Section 8 / Admin_ConferenceRoom_* actors", section8)
if not section6:
    _log("Section 6 actors: NOT FOUND")
if not section7:
    _log("Section 7 actors: NOT FOUND")
if not section8:
    _log("Section 8 actors: NOT FOUND")
_log("admin_actor_count=%d s6_count=%d s7_count=%d s8_count=%d" % (
    len(admin_actors), len(section6), len(section7), len(section8)
))
