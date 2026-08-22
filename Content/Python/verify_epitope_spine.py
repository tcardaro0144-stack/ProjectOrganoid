# ProjectOrganoid — verify the Epitope spine wiring (UE Editor Python)
#
# Opens /Game/Maps/Lvl_Epitope and reports which partitions are registered and how
# every streaming volume is configured. Read-only; safe to run any time.

import unreal


SPINE_MAP = "/Game/Maps/Lvl_Epitope"


def report(line):
    unreal.log_warning(f"[SPINE CHECK] {line}")


def verify():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    level_subsystem.load_level(SPINE_MAP)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if not world:
        report("FAIL: no editor world")
        return

    try:
        levels = unreal.EditorLevelUtils.get_levels(world)
        report(f"Levels loaded (persistent + partitions): {len(levels)}")
        for level in levels:
            outer = level.get_outer()
            report(f"  level: {outer.get_name() if outer else level.get_name()}")
    except Exception as exc:
        report(f"Could not enumerate levels: {exc}")

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    report(f"Total actors across loaded levels: {len(actors)}")

    volumes = [a for a in actors if "StreamingVolume" in a.get_class().get_name()]
    report(f"Streaming volumes found: {len(volumes)}")

    for volume in sorted(volumes, key=lambda a: a.get_actor_label()):
        label = volume.get_actor_label()
        try:
            partitions = list(volume.get_editor_property("requested_streaming_levels"))
        except Exception:
            partitions = "<unreadable>"
        try:
            region = volume.get_editor_property("region_context_tag")
        except Exception:
            region = "<unreadable>"
        try:
            extent = volume.trigger_volume.get_editor_property("box_extent")
        except Exception:
            extent = "<unreadable>"

        report(f"  {label}: partitions={partitions} region={region} extent={extent}")

    starts = [a for a in actors if isinstance(a, unreal.PlayerStart)]
    report(f"PlayerStart count: {len(starts)}")
    for start in starts:
        report(f"  {start.get_actor_label()} at {start.get_actor_location()}")

    meshes = [a for a in actors if isinstance(a, unreal.StaticMeshActor)]
    report(f"StaticMeshActor count: {len(meshes)}")

    game_mode = world.get_world_settings().get_editor_property("default_game_mode")
    report(f"Default game mode: {game_mode}")


verify()
