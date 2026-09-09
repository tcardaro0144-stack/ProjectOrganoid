# Headless: verify Lvl_Epitope uses the gameplay GameMode and has a PlayerStart.

import unreal

SPINE_MAP = "/Game/Maps/Lvl_Epitope"
GAMEPLAY_GM = "/Script/ProjectOrganoid.ProjectOrganoidGameMode"


def report(msg):
    unreal.log(f"[fix_epitope_transition] {msg}")


def main():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not levels or not actors:
        unreal.log_error("Missing editor subsystems")
        return

    levels.load_level(SPINE_MAP)
    world = unreal.EditorLevelLibrary.get_editor_world()
    settings = world.get_world_settings() if world else None
    current_gm = None
    if settings:
        try:
            current_gm = settings.get_editor_property("default_game_mode")
        except Exception:
            current_gm = None
    report(f"Lvl_Epitope World Settings GameMode: {current_gm}")

    wanted = unreal.load_class(None, GAMEPLAY_GM)
    if wanted and current_gm != wanted:
        settings.set_editor_property("default_game_mode", wanted)
        levels.save_current_level()
        report(f"Set default_game_mode -> {GAMEPLAY_GM}")
    elif wanted:
        report("GameMode already ProjectOrganoidGameMode")
    else:
        unreal.log_warning(f"Could not load {GAMEPLAY_GM}")

    starts = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.PlayerStart)]
    report(f"PlayerStart count: {len(starts)}")
    for start in starts:
        report(f"  {start.get_actor_label()} @ {start.get_actor_location()}")


main()
