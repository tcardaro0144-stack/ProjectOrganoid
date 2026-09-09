# ProjectOrganoid — Section 17 placed-instance reconstruction ONLY.
# Reruns Construction Script on BP_AdminAccessDoor_C_0 in SL_Epitope_Admin.
# Does not save, start PIE, spawn, delete, or edit any other actor.
# Does not set AccessTrigger geometry (no instance override).
#
# py "C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Content\Python\rerun_admin_access_door_construction.py"

import unreal

TAG = "[S17 RECONSTRUCT] "
ADMIN_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
ACTOR_NAME = "BP_AdminAccessDoor_C_0"
ACTOR_LABEL = "BP_AdminAccessDoor"
CLASS_TOKEN = "BP_AdminAccessDoor_C"


def _log(msg):
    unreal.log(TAG + msg)


def _pkg(actor):
    path = ""
    try:
        path = str(actor.get_path_name())
    except Exception:
        path = ""
    if "SL_Epitope_Admin" in path:
        return ADMIN_PACKAGE
    try:
        outermost = actor.get_outermost()
        name = str(outermost.get_name()) if outermost else ""
        if name.endswith("SL_Epitope_Admin") or name == "SL_Epitope_Admin":
            return ADMIN_PACKAGE
        return name
    except Exception:
        return path


def _fmt_vec(vec):
    if vec is None:
        return "None"
    return "(%.2f, %.2f, %.2f)" % (vec.x, vec.y, vec.z)


def _comp(actor, name):
    try:
        comps = actor.get_components_by_class(unreal.ActorComponent)
    except Exception:
        comps = []
    for comp in comps or []:
        if comp is None:
            continue
        if str(comp.get_name()) == name:
            return comp
    return None


def _dump_comp(actor, name):
    comp = _comp(actor, name)
    if comp is None:
        _log("%s MISSING" % name)
        return
    rel = getattr(comp, "relative_location", None)
    extra = ""
    if isinstance(comp, unreal.BoxComponent):
        extra = " box_extent=%s" % _fmt_vec(comp.get_unscaled_box_extent())
    profile = ""
    pawn = ""
    try:
        if hasattr(comp, "get_collision_profile_name"):
            profile = str(comp.get_collision_profile_name())
        if hasattr(comp, "get_collision_response_to_channel"):
            pawn = str(comp.get_collision_response_to_channel(unreal.CollisionChannel.PAWN))
    except Exception:
        pass
    _log("%s rel=%s%s profile=%s pawn=%s" % (name, _fmt_vec(rel), extra, profile, pawn))


def _dump_door(actor, stage):
    loc = actor.get_actor_location()
    _log("%s actor=(%.2f, %.2f, %.2f)" % (stage, loc.x, loc.y, loc.z))
    for name in ("AccessTrigger", "Door_Left", "Door_Right", "DoorFrame"):
        _dump_comp(actor, name)


def _find_door():
    editor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    matches = []
    for actor in editor.get_all_level_actors():
        if actor is None:
            continue
        name = str(actor.get_name())
        label = str(actor.get_actor_label())
        cls = str(actor.get_class().get_name()) if actor.get_class() else ""
        if name != ACTOR_NAME and label != ACTOR_LABEL:
            continue
        if CLASS_TOKEN not in cls:
            continue
        matches.append(actor)
    return matches


def main():
    game = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if game:
        _log("ABORT PIE is running. ZERO writes.")
        return
    matches = _find_door()
    if len(matches) != 1:
        _log("ABORT unique door count=%d expected=1. ZERO writes." % len(matches))
        return
    actor = matches[0]
    owner = _pkg(actor)
    if owner != ADMIN_PACKAGE:
        _log("ABORT owning package '%s' is not Admin. ZERO writes." % owner)
        return
    _dump_door(actor, "BEFORE")
    actor.rerun_construction_scripts()
    _dump_door(actor, "AFTER")
    _log("RerunConstructionScripts completed. No save.")


if __name__ == "__main__":
    main()
