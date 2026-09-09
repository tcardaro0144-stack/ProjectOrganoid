# ProjectOrganoid — READ-ONLY PIE pawn dump (Section 17).
# Prints the possessed player pawn via PlayerController/GetPawn.
# Does not spawn, move, save, compile, or touch maps/Blueprints/S1–S16/door.
#
# Run WHILE PIE is playing, standing at the invisible stop:
#   py "C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Content\Python\diagnose_runtime_possessed_pawn.py"

import unreal


def _log(msg):
    unreal.log("[S17 PAWN] " + msg)


def _enum_name(value):
    text = str(value)
    if "." in text:
        return text.rsplit(".", 1)[-1]
    return text


world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if world is None:
    _log("NO PIE WORLD — start Play, walk to the stop, run this again.")
else:
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    if pawn is None and pc is not None:
        pawn = pc.get_pawn()

    if pawn is None:
        _log("NO POSSESSED PAWN — controller=%s" % (pc.get_name() if pc else "None"))
    else:
        loc = pawn.get_actor_location()
        cls = pawn.get_class()
        cls_name = cls.get_name() if cls else "None"
        cls_path = cls.get_path_name() if cls else "None"

        mode = "NO_MOVEMENT_COMPONENT"
        grounded = "unknown"
        move = pawn.get_movement_component()
        if move is None and isinstance(pawn, unreal.Character):
            move = pawn.get_character_movement()
        if move is not None:
            mode = _enum_name(getattr(move, "movement_mode", "unknown"))
            if hasattr(move, "is_moving_on_ground"):
                grounded = str(bool(move.is_moving_on_ground()))
            elif hasattr(move, "is_walking"):
                grounded = "is_walking=%s" % bool(move.is_walking())

        _log("instance=%s" % pawn.get_name())
        _log("class=%s" % cls_name)
        _log("class_path=%s" % cls_path)
        _log("location_xyz=(%.2f, %.2f, %.2f)" % (loc.x, loc.y, loc.z))
        _log("location_z=%.2f" % loc.z)
        _log("movement_mode=%s" % mode)
        _log("is_moving_on_ground=%s" % grounded)
        _log("DONE")
