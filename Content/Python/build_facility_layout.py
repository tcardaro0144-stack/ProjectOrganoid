# ProjectOrganoid — Epitope facility layout generator (UE Editor Python)
#
# Run in Unreal:
#   Tools → Execute Python Script… → Content/Python/build_facility_layout.py
#   or: py "Content/Python/build_facility_layout.py" in the Output Log Python console
#
# Requires the ProjectOrganoid C++ module compiled (UpgradeTerminal, DoorLock,
# HazardZone, LevelTransitionZone, HostBase / BP child).

import unreal


def build_facility_layout():
    """
    Generate a multi-room facility scaffold for ProjectOrganoid:
    Admin (Sub-Level 1) → transition corridor → Biolab / Neuro-Genetics (Sub-Level 2).
    """
    editor_actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------
    def load_native_class(class_path: str):
        """Load a C++ or Blueprint class. Prefer /Script/Module.Class for natives."""
        actor_class = unreal.load_class(None, class_path)
        if actor_class:
            return actor_class
        # Blueprint soft path fallback (e.g. /Game/.../BP_Host_Test.BP_Host_Test_C)
        try:
            actor_class = unreal.EditorAssetLibrary.load_blueprint_class(class_path)
        except Exception:
            actor_class = None
        return actor_class

    def spawn_static_mesh(mesh_path, location, rotation=None, scale=None, name="MeshActor"):
        rotation = rotation or unreal.Rotator(0.0, 0.0, 0.0)
        scale = scale or unreal.Vector(1.0, 1.0, 1.0)
        mesh_asset = unreal.EditorAssetLibrary.load_asset(mesh_path)
        if not mesh_asset:
            unreal.log_warning(f"Could not load asset: {mesh_path}")
            return None

        spawned_actor = editor_actor_subsystem.spawn_actor_from_class(
            unreal.StaticMeshActor, location, rotation
        )
        if spawned_actor:
            spawned_actor.set_actor_label(name)
            mesh_component = spawned_actor.static_mesh_component
            mesh_component.set_static_mesh(mesh_asset)
            spawned_actor.set_actor_scale3d(scale)
        return spawned_actor

    def spawn_custom_actor(class_path, location, rotation=None, name="CustomActor"):
        rotation = rotation or unreal.Rotator(0.0, 0.0, 0.0)
        actor_class = load_native_class(class_path)
        if not actor_class:
            unreal.log_warning(f"Could not load class: {class_path}")
            return None

        try:
            spawned_actor = editor_actor_subsystem.spawn_actor_from_class(
                actor_class, location, rotation
            )
        except Exception as exc:
            unreal.log_warning(f"Failed to spawn {class_path}: {exc}")
            return None

        if spawned_actor:
            spawned_actor.set_actor_label(name)
        return spawned_actor

    def spawn_light(location, intensity=5000.0, color=None, name="FacilityLight"):
        color = color or unreal.LinearColor(1.0, 1.0, 1.0, 1.0)
        light_class = getattr(unreal, "PointLightActor", None) or unreal.PointLight
        light_actor = editor_actor_subsystem.spawn_actor_from_class(light_class, location)
        if not light_actor:
            return None

        light_actor.set_actor_label(name)
        light_component = light_actor.get_component_by_class(unreal.LightComponent)
        if light_component:
            light_component.set_intensity(intensity)
            light_component.set_light_color(color)
        return light_actor

    def set_enum_property(actor, property_name, enum_type_name, enum_value_names):
        """
        Set a reflected enum property using the first matching Python enum member name.
        enum_value_names: list of candidate names (UE Python naming varies by version).
        """
        if not actor:
            return False
        enum_type = getattr(unreal, enum_type_name, None)
        if not enum_type:
            unreal.log_warning(f"Enum type not exposed to Python: {enum_type_name}")
            return False

        value = None
        for candidate in enum_value_names:
            value = getattr(enum_type, candidate, None)
            if value is not None:
                break

        if value is None:
            unreal.log_warning(
                f"Could not resolve enum value on {enum_type_name} from {enum_value_names}"
            )
            return False

        try:
            actor.set_editor_property(property_name, value)
            return True
        except Exception as exc:
            unreal.log_warning(f"set_editor_property({property_name}) failed: {exc}")
            return False

    cube_mesh = "/Engine/BasicShapes/Cube.Cube"
    unreal.log("--- Starting ProjectOrganoid Facility Generation ---")

    # ------------------------------------------------------------------
    # Sub-Level 1: Admin & Decontamination (origin)
    # ------------------------------------------------------------------
    unreal.log("Building Sub-Level 1: Admin Area...")

    spawn_static_mesh(
        cube_mesh,
        unreal.Vector(0.0, 0.0, -10.0),
        scale=unreal.Vector(20.0, 20.0, 0.2),
        name="Admin_Floor",
    )
    spawn_static_mesh(
        cube_mesh,
        unreal.Vector(0.0, 0.0, 500.0),
        scale=unreal.Vector(20.0, 20.0, 0.2),
        name="Admin_Ceiling",
    )
    # Simple perimeter walls (north/south/west; east opens to corridor)
    wall_h = unreal.Vector(20.0, 0.4, 5.0)
    spawn_static_mesh(cube_mesh, unreal.Vector(0.0, -1000.0, 240.0), scale=wall_h, name="Admin_Wall_South")
    spawn_static_mesh(cube_mesh, unreal.Vector(0.0, 1000.0, 240.0), scale=wall_h, name="Admin_Wall_North")
    spawn_static_mesh(
        cube_mesh,
        unreal.Vector(-1000.0, 0.0, 240.0),
        scale=unreal.Vector(0.4, 20.0, 5.0),
        name="Admin_Wall_West",
    )

    spawn_light(
        unreal.Vector(0.0, 0.0, 400.0),
        intensity=8000.0,
        color=unreal.LinearColor(0.8, 0.9, 1.0, 1.0),
        name="Admin_MainLight",
    )

    spawn_custom_actor(
        "/Script/ProjectOrganoid.ProjectOrganoidUpgradeTerminal",
        unreal.Vector(-500.0, 0.0, 50.0),
        name="Sterling_UpgradeTerminal",
    )

    tier1_door = spawn_custom_actor(
        "/Script/ProjectOrganoid.ProjectOrganoidDoorLock",
        unreal.Vector(1000.0, 0.0, 50.0),
        name="DoorLock_Tier1_AdminExit",
    )
    # EProjectOrganoidSecurityTier::Level1_Admin
    set_enum_property(
        tier1_door,
        "required_security_tier",
        "ProjectOrganoidSecurityTier",
        ["LEVEL1_ADMIN", "Level1_Admin", "LEVEL_1_ADMIN"],
    )
    # Some UE Python builds expose the property in PascalCase:
    if tier1_door:
        try:
            set_enum_property(
                tier1_door,
                "RequiredSecurityTier",
                "ProjectOrganoidSecurityTier",
                ["LEVEL1_ADMIN", "Level1_Admin", "LEVEL_1_ADMIN"],
            )
        except Exception:
            pass

    # ------------------------------------------------------------------
    # Transition corridor (Admin → Biolab)
    # ------------------------------------------------------------------
    unreal.log("Building Transition Corridor...")

    spawn_static_mesh(
        cube_mesh,
        unreal.Vector(1500.0, 0.0, -10.0),
        scale=unreal.Vector(10.0, 4.0, 0.2),
        name="Corridor_Floor",
    )
    spawn_static_mesh(
        cube_mesh,
        unreal.Vector(1500.0, -200.0, 240.0),
        scale=unreal.Vector(10.0, 0.4, 5.0),
        name="Corridor_Wall_South",
    )
    spawn_static_mesh(
        cube_mesh,
        unreal.Vector(1500.0, 200.0, 240.0),
        scale=unreal.Vector(10.0, 0.4, 5.0),
        name="Corridor_Wall_North",
    )

    transition_zone = spawn_custom_actor(
        "/Script/ProjectOrganoid.ProjectOrganoidLevelTransitionZone",
        unreal.Vector(1500.0, 0.0, 100.0),
        name="TransitionZone_AdminToBiolab",
    )
    if transition_zone:
        # EProjectOrganoidSubLevelTag::SubLevel2_NeuroGenetics
        set_enum_property(
            transition_zone,
            "TargetSubLevelTag",
            "ProjectOrganoidSubLevelTag",
            [
                "SUB_LEVEL2_NEURO_GENETICS",
                "SubLevel2_NeuroGenetics",
                "SUBLEVEL2_NEUROGENETICS",
            ],
        )
        try:
            transition_zone.set_editor_property("TargetStreamingLevelName", "SL_Epitope_NeuroGenetics")
        except Exception:
            try:
                transition_zone.set_editor_property("target_streaming_level_name", "SL_Epitope_NeuroGenetics")
            except Exception as exc:
                unreal.log_warning(f"Could not set TargetStreamingLevelName: {exc}")

        try:
            transition_zone.set_editor_property(
                "StreamingLevelsToUnload",
                ["SL_Epitope_Admin"],
            )
        except Exception:
            pass

    # ------------------------------------------------------------------
    # Sub-Level 2: Biolab / Neuro-Genetics
    # ------------------------------------------------------------------
    unreal.log("Building Sub-Level 2: Biolabs Area...")

    spawn_static_mesh(
        cube_mesh,
        unreal.Vector(3500.0, 0.0, -10.0),
        scale=unreal.Vector(30.0, 30.0, 0.2),
        name="Biolab_Floor",
    )
    spawn_light(
        unreal.Vector(3500.0, 0.0, 400.0),
        intensity=10000.0,
        color=unreal.LinearColor(0.4, 1.0, 0.3, 1.0),
        name="Biolab_HazardLight",
    )

    toxic_hazard = spawn_custom_actor(
        "/Script/ProjectOrganoid.ProjectOrganoidHazardZone",
        unreal.Vector(3500.0, 500.0, 100.0),
        name="HazardVolume_ToxicGas",
    )
    if toxic_hazard:
        # EProjectOrganoidHazardType::ToxicGas (None=0, UVC=1, LN2=2, ToxicGas=3)
        set_enum_property(
            toxic_hazard,
            "HazardType",
            "ProjectOrganoidHazardType",
            ["TOXIC_GAS", "ToxicGas", "TOXICGAS"],
        )
        try:
            toxic_hazard.set_editor_property("bIsActive", True)
        except Exception:
            toxic_hazard.set_editor_property("is_active", True)

        set_enum_property(
            toxic_hazard,
            "AssociatedSubLevelTag",
            "ProjectOrganoidSubLevelTag",
            [
                "SUB_LEVEL2_NEURO_GENETICS",
                "SubLevel2_NeuroGenetics",
                "SUBLEVEL2_NEUROGENETICS",
            ],
        )

    # HostBase is Abstract — prefer a Blueprint child if present
    host_class_candidates = [
        "/Game/ProjectOrganoid/Characters/BP_Host_Test.BP_Host_Test_C",
        "/Game/Blueprints/BP_Host_Test.BP_Host_Test_C",
        "/Script/ProjectOrganoid.ProjectOrganoidHostBase",
    ]

    def spawn_host(location, rotation, name):
        for class_path in host_class_candidates:
            host = spawn_custom_actor(class_path, location, rotation=rotation, name=name)
            if host:
                return host
        unreal.log_warning(
            f"{name}: create BP_Host_Test (parent ProjectOrganoidHostBase) and re-run, "
            "or temporarily remove Abstract from AProjectOrganoidHostBase."
        )
        return None

    spawn_host(
        unreal.Vector(3200.0, -500.0, 50.0),
        unreal.Rotator(0.0, 180.0, 0.0),
        "HostEnemy_SpecimenA",
    )
    spawn_host(
        unreal.Vector(4000.0, 200.0, 50.0),
        unreal.Rotator(0.0, 220.0, 0.0),
        "HostEnemy_SpecimenB",
    )

    # Player start in Admin
    try:
        player_start = editor_actor_subsystem.spawn_actor_from_class(
            unreal.PlayerStart,
            unreal.Vector(0.0, 0.0, 100.0),
            unreal.Rotator(0.0, 0.0, 0.0),
        )
        if player_start:
            player_start.set_actor_label("PlayerStart_Admin")
    except Exception as exc:
        unreal.log_warning(f"Could not spawn PlayerStart: {exc}")

    unreal.log("--- ProjectOrganoid Facility Layout Generation Complete! ---")
    unreal.log(
        "Next: assign streaming level names in World Settings / Levels panel to match "
        "SL_Epitope_Admin and SL_Epitope_NeuroGenetics if you split into true sub-levels."
    )


if __name__ == "__main__":
    build_facility_layout()
