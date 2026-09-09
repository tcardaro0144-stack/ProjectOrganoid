# ProjectOrganoid — Lvl_MainMenu cleanup of mistaken Section 7/8 copies.
# Deletes only MainMenu-owned Admin_RecordsArchives_Block and Admin_ConferenceRoom_*.
# Does not touch Section 6, SL_Epitope_Admin, or Section 9.

import unreal

MENU_PACKAGE = "/Game/Maps/Lvl_MainMenu"
ADMIN_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
S7_LABEL = "Admin_RecordsArchives_Block"
S8_PREFIX = "Admin_ConferenceRoom_"
EXPECTED_S7 = 1
EXPECTED_S8 = 20


def _log(msg):
    unreal.log(msg)


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
        if classified == MENU_PACKAGE:
            return MENU_PACKAGE
        if classified == ADMIN_PACKAGE:
            return ADMIN_PACKAGE
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
        _package_from_path(_safe_call(_safe_call(actor, "get_outer"), "get_path_name")),
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


def _is_s7(label):
    return label == S7_LABEL


def _is_s8(label):
    return label.startswith(S8_PREFIX)


def _fail(message):
    _log("=== CLEANUP VERIFICATION FAILED ===")
    _log(message)
    raise RuntimeError(message)


def _collect_loaded_actors(world, actor_sub):
    found = {}
    if actor_sub:
        try:
            for actor in actor_sub.get_all_level_actors():
                if actor:
                    found[_safe_str(_safe_call(actor, "get_path_name"))] = actor
        except Exception as exc:
            _log("EditorActorSubsystem.get_all_level_actors failed: %s" % exc)
    if world:
        try:
            for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
                if actor:
                    found[_safe_str(_safe_call(actor, "get_path_name"))] = actor
        except Exception as exc:
            _log("GameplayStatics.get_all_actors_of_class failed: %s" % exc)
    return list(found.values())


def _scan_targets(actors):
    s7_menu = []
    s8_menu = []
    unclear = []
    admin_owned_matches = []
    other_owned_matches = []
    for actor in actors:
        label = actor.get_actor_label()
        if not _is_s7(label) and not _is_s8(label):
            continue
        owner = _actor_owner_package(actor)
        row = (label, owner, actor)
        if owner == ADMIN_PACKAGE:
            admin_owned_matches.append(row)
            continue
        if owner == "cannot be determined" or owner.startswith("mixed:"):
            unclear.append(row)
            continue
        if owner != MENU_PACKAGE:
            other_owned_matches.append(row)
            continue
        if _is_s7(label):
            s7_menu.append(row)
        else:
            s8_menu.append(row)
    return s7_menu, s8_menu, unclear, admin_owned_matches, other_owned_matches


world = _get_editor_world()
world_pkg = _world_package(world)
if world_pkg != MENU_PACKAGE:
    raise RuntimeError(
        "CLEANUP aborted: current editor world/package is '%s', expected '%s'. ZERO writes performed."
        % (world_pkg, MENU_PACKAGE)
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
if current_level_pkg == ADMIN_PACKAGE:
    raise RuntimeError(
        "CLEANUP aborted: current level package is '%s'. ZERO writes performed."
        % current_level_pkg
    )
if current_level is not None and current_level_pkg not in (MENU_PACKAGE, "cannot be determined"):
    raise RuntimeError(
        "CLEANUP aborted: current level package is '%s', expected '%s'. ZERO writes performed."
        % (current_level_pkg, MENU_PACKAGE)
    )

_log("CLEANUP map guard passed: %s" % world_pkg)

loaded_actors = _collect_loaded_actors(world, actor_sub)
s7_menu, s8_menu, unclear, admin_owned_matches, other_owned_matches = _scan_targets(loaded_actors)

_log("preflight MainMenu Section 7 count=%d" % len(s7_menu))
_log("preflight MainMenu Section 8 count=%d" % len(s8_menu))
for label, owner, actor in s7_menu + s8_menu:
    _log(
        "SELECTED label=%s owner=%s path=%s"
        % (label, owner, _safe_str(_safe_call(actor, "get_path_name")))
    )
for label, owner, actor in admin_owned_matches:
    _log("NOT SELECTED admin-owned label=%s owner=%s" % (label, owner))
for label, owner, actor in other_owned_matches:
    _log("NOT SELECTED other-owned label=%s owner=%s" % (label, owner))

preflight_errors = []
if unclear:
    for label, owner, actor in unclear:
        preflight_errors.append("unclear ownership label=%s owner=%s" % (label, owner))
if len(s7_menu) != EXPECTED_S7:
    preflight_errors.append("Section 7 MainMenu count=%d expected=%d" % (len(s7_menu), EXPECTED_S7))
if len(s8_menu) != EXPECTED_S8:
    preflight_errors.append("Section 8 MainMenu count=%d expected=%d" % (len(s8_menu), EXPECTED_S8))

selected = s7_menu + s8_menu
selected_labels = [label for label, owner, actor in selected]
if S7_LABEL not in selected_labels:
    preflight_errors.append("%s not found as Lvl_MainMenu-owned" % S7_LABEL)
for label, owner, actor in selected:
    if owner == ADMIN_PACKAGE:
        preflight_errors.append("selected actor is Admin-owned: %s" % label)
    if owner != MENU_PACKAGE:
        preflight_errors.append("selected actor is not Lvl_MainMenu-owned: %s owner=%s" % (label, owner))
    if label != S7_LABEL and not label.startswith(S8_PREFIX):
        preflight_errors.append("selected unrelated label: %s" % label)
    if label == "Terminal_AdminSecurity":
        preflight_errors.append("refusing to delete Terminal_AdminSecurity")

if len(selected) != EXPECTED_S7 + EXPECTED_S8:
    preflight_errors.append("selected count=%d expected=21" % len(selected))

if preflight_errors:
    _fail("Preflight failed; ZERO deletions. " + " | ".join(preflight_errors))

_log("=== CLEANUP PREFLIGHT PASSED ===")
_log("will delete %d Lvl_MainMenu-owned actors (S7=%d S8=%d)" % (len(selected), len(s7_menu), len(s8_menu)))

deleted_s7 = []
deleted_s8 = []
for label, owner, actor in selected:
    path = _safe_str(_safe_call(actor, "get_path_name"))
    destroyed = actor_sub.destroy_actor(actor)
    if not destroyed:
        _fail("destroy_actor returned false for %s path=%s. Partial deletion possible; Admin map not saved." % (label, path))
    if _is_s7(label):
        deleted_s7.append(label)
    else:
        deleted_s8.append(label)
    _log("deleted label=%s owner=%s path=%s" % (label, owner, path))

saved = level_sub.save_current_level()
_log("saved Lvl_MainMenu=%s" % _safe_str(saved))

remaining_actors = _collect_loaded_actors(_get_editor_world(), actor_sub)
s7_left, s8_left, unclear_left, admin_left, other_left = _scan_targets(remaining_actors)

verify_errors = []
if len(s7_left) != 0:
    verify_errors.append("remaining MainMenu Section 7=%d labels=%s" % (len(s7_left), ",".join([r[0] for r in s7_left])))
if len(s8_left) != 0:
    verify_errors.append("remaining MainMenu Section 8=%d labels=%s" % (len(s8_left), ",".join([r[0] for r in s8_left])))
if unclear_left:
    verify_errors.append("remaining unclear ownership: " + ",".join(["%s/%s" % (r[0], r[1]) for r in unclear_left]))
if len(deleted_s7) != EXPECTED_S7:
    verify_errors.append("deleted Section 7 count=%d expected=%d" % (len(deleted_s7), EXPECTED_S7))
if len(deleted_s8) != EXPECTED_S8:
    verify_errors.append("deleted Section 8 count=%d expected=%d" % (len(deleted_s8), EXPECTED_S8))

if verify_errors:
    _fail(" | ".join(verify_errors))

_log("=== LVL_MAINMENU SECTION 7–8 CLEANUP VERIFIED ===")
_log("Section 7 deleted count=%d" % len(deleted_s7))
_log("Section 8 deleted count=%d" % len(deleted_s8))
_log("remaining Section 7 count=%d" % len(s7_left))
_log("remaining Section 8 count=%d" % len(s8_left))
_log("total actors deleted=%d" % (len(deleted_s7) + len(deleted_s8)))
_log("current world/package=%s" % _world_package(_get_editor_world()))
_log("Admin-owned matches left untouched=%d" % len(admin_left))
_log("Section 6 and SL_Epitope_Admin were not modified.")
