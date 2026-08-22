# ProjectOrganoid — Phase 3 data layer (UE Editor Python)
#
# Creates mission / item / dialogue DataAssets, assigns the host mesh, then
# re-runs the room blockout so pickups and the survivor conversation land in-world.
#
# Run:
#   Tools -> Execute Python Script... -> Content/Python/build_epitope_data.py
# Headless (forward slashes only):
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="<abs>/build_epitope_data.py"

import unreal


DATA_ROOT = "/Game/Data"
ITEM_DIR = f"{DATA_ROOT}/Items"
MISSION_DIR = f"{DATA_ROOT}/Missions"
DIALOGUE_DIR = f"{DATA_ROOT}/Dialogue"

HOST_BP_PATH = "/Game/Hosts/BP_OrganoidHost"
HOST_MESH_PATH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"

ITEM_KEYCARD = f"{ITEM_DIR}/DA_Item_AdminKeycard"
ITEM_SOT = f"{ITEM_DIR}/DA_Item_SOT"
MISSION_AUDIT = f"{MISSION_DIR}/DA_Mission_TheAudit"
MISSION_PRODUCTION = f"{MISSION_DIR}/DA_Mission_TheProduction"
MISSION_HANDOVER = f"{MISSION_DIR}/DA_Mission_TheHandover"
MISSION_CONCLUSION = f"{MISSION_DIR}/DA_Mission_TheConclusion"
DIALOGUE_SURVIVOR = f"{DIALOGUE_DIR}/DA_Dialogue_IncineratorSurvivor"


def report(message):
    unreal.log_warning(f"[DATA] {message}")


def ensure_dir(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def _snake(name):
    return "".join(f"_{c.lower()}" if c.isupper() else c for c in name).lstrip("_")


def set_prop(obj, name, value):
    candidates = [name, _snake(name)]
    if name.startswith("b") and len(name) > 1 and name[1].isupper():
        stripped = name[1:]
        candidates.extend([stripped, _snake(stripped)])

    for candidate in candidates:
        try:
            obj.set_editor_property(candidate, value)
            return True
        except Exception:
            pass
        try:
            setattr(obj, candidate, value)
            return True
        except Exception:
            pass

    report(f"could not set {name} (tried {candidates})")
    return False


def resolve_enum(enum_type_name, wanted):
    enum_type = getattr(unreal, enum_type_name, None)
    if not enum_type:
        report(f"enum missing: {enum_type_name}")
        return None
    target = wanted.replace("_", "").lower()
    for attr in dir(enum_type):
        if attr.startswith("_"):
            continue
        if attr.replace("_", "").lower() == target:
            return getattr(enum_type, attr)
    report(f"enum value missing: {enum_type_name}.{wanted}")
    return None


def text(value):
    try:
        return unreal.Text(value)
    except Exception:
        return value


def create_data_asset(asset_path, class_type):
    package_path, _, asset_name = asset_path.rpartition("/")
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        report(f"exists: {asset_path}")
        return unreal.EditorAssetLibrary.load_asset(asset_path)

    factory = unreal.DataAssetFactory()
    try:
        factory.set_editor_property("data_asset_class", class_type)
    except Exception:
        try:
            factory.set_editor_property("DataAssetClass", class_type)
        except Exception:
            pass

    created = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, package_path, class_type, factory
    )
    if not created:
        report(f"failed to create {asset_path}")
        return None
    report(f"created {asset_path}")
    return created


def make_objective(objective_id, title, description, notes, stage, target=1,
                   prereqs=None, obj_type="Main", auto_activate=True):
    objective = unreal.ProjectOrganoidObjective()
    set_prop(objective, "ObjectiveId", objective_id)
    set_prop(objective, "Title", text(title))
    set_prop(objective, "Description", text(description))
    set_prop(objective, "JournalNotes", text(notes))
    set_prop(objective, "StageIndex", stage)
    set_prop(objective, "TargetProgress", target)
    set_prop(objective, "CurrentProgress", 0)
    set_prop(objective, "bShowInJournal", True)
    set_prop(objective, "bAutoUnlockWhenPrerequisitesMet", True)
    if prereqs:
        set_prop(objective, "PrerequisiteObjectiveIds", prereqs)
    enum_val = resolve_enum("ProjectOrganoidObjectiveType", obj_type)
    if enum_val is not None:
        set_prop(objective, "Type", enum_val)
    inactive = resolve_enum("ProjectOrganoidObjectiveState", "Inactive")
    if inactive is not None:
        set_prop(objective, "State", inactive)
    return objective, auto_activate


def make_trigger(event_id, action="Complete", delta=1):
    trigger = unreal.ProjectOrganoidObjectiveEventTrigger()
    set_prop(trigger, "EventId", event_id)
    set_prop(trigger, "ProgressDelta", delta)
    action_val = resolve_enum("ProjectOrganoidObjectiveEventAction", action)
    if action_val is not None:
        set_prop(trigger, "Action", action_val)
    return trigger


def make_task(objective, auto_activate, triggers):
    task = unreal.ProjectOrganoidMissionTaskDefinition()
    set_prop(task, "Objective", objective)
    set_prop(task, "bAutoActivate", auto_activate)
    set_prop(task, "EventTriggers", triggers)
    return task


def save_asset(asset, path):
    if not asset:
        return
    unreal.EditorAssetLibrary.save_asset(path)


# ----------------------------------------------------------------------------------
# Items
# ----------------------------------------------------------------------------------

def build_items():
    keycard = create_data_asset(ITEM_KEYCARD, unreal.ProjectOrganoidItemData)
    if keycard:
        set_prop(keycard, "ItemName", text("Admin Keycard — Tier 1"))
        set_prop(keycard, "ItemType", resolve_enum("ProjectOrganoidItemType", "KeyItem"))
        set_prop(keycard, "SecurityTier", resolve_enum("ProjectOrganoidSecurityTier", "Level1_Admin"))
        set_prop(keycard, "GridWidth", 1)
        set_prop(keycard, "GridHeight", 1)
        set_prop(keycard, "ItemWeight", 0.2)
        set_prop(keycard, "bCanStack", False)
        save_asset(keycard, ITEM_KEYCARD)

    sot = create_data_asset(ITEM_SOT, unreal.ProjectOrganoidItemData)
    if sot:
        set_prop(sot, "ItemName", text("Synthetic Organoid Tissue"))
        set_prop(sot, "ItemType", resolve_enum("ProjectOrganoidItemType", "SOT"))
        set_prop(sot, "GridWidth", 1)
        set_prop(sot, "GridHeight", 1)
        set_prop(sot, "ItemWeight", 0.15)
        set_prop(sot, "bCanStack", True)
        set_prop(sot, "MaxStackCount", 99)
        save_asset(sot, ITEM_SOT)

    return keycard, sot


# ----------------------------------------------------------------------------------
# Missions
# ----------------------------------------------------------------------------------

def build_missions():
    conclusion = create_data_asset(MISSION_CONCLUSION, unreal.ProjectOrganoidObjectiveDataAsset)
    handover = create_data_asset(MISSION_HANDOVER, unreal.ProjectOrganoidObjectiveDataAsset)
    production = create_data_asset(MISSION_PRODUCTION, unreal.ProjectOrganoidObjectiveDataAsset)
    audit = create_data_asset(MISSION_AUDIT, unreal.ProjectOrganoidObjectiveDataAsset)

    if conclusion:
        set_prop(conclusion, "MissionId", "Mission_TheConclusion")
        set_prop(conclusion, "MissionTitle", text("The Conclusion"))
        set_prop(conclusion, "MissionDescription", text(
            "The incubator is awake. Avery files the report."))
        obj, auto = make_objective(
            "Main_ReachControlSpine",
            "Reach the Control Spine",
            "Use the reactor control terminal overlooking the primary incubator.",
            "Every earlier document leads here. The choice is the audit's conclusion.",
            stage=3,
        )
        set_prop(conclusion, "Tasks", [make_task(obj, auto, [make_trigger("Event_ReactorControlUsed")])])
        save_asset(conclusion, MISSION_CONCLUSION)

    if handover:
        set_prop(handover, "MissionId", "Mission_TheHandover")
        set_prop(handover, "MissionTitle", text("The Handover"))
        set_prop(handover, "MissionDescription", text(
            "The compute substrate has been running the lockdown. Talk to it."))
        hack, hack_auto = make_objective(
            "Main_HackComputeCore",
            "Wake the Interface Chamber",
            "Complete the three-terminal hack chain in Bio-Neural Compute.",
            "Each successful hack is a conversation with the facility.",
            stage=2,
            target=3,
        )
        confession, confession_auto = make_objective(
            "Main_ReadSterlingConfession",
            "Read Sterling's Confession",
            "Recover the unsent message addressed to Avery by name.",
            "He did not lose control. He handed it over.",
            stage=2,
            auto_activate=True,
        )
        set_prop(handover, "Tasks", [
            make_task(hack, hack_auto, [make_trigger("Event_TerminalHackSuccess", "Advance", 1)]),
            make_task(confession, confession_auto, [make_trigger("Event_SterlingConfessionRead")]),
        ])
        set_prop(handover, "NextMissionAsset", conclusion)
        save_asset(handover, MISSION_HANDOVER)

    if production:
        set_prop(production, "MissionId", "Mission_TheProduction")
        set_prop(production, "MissionTitle", text("The Production"))
        set_prop(production, "MissionDescription", text(
            "The specimens were staff. Confirm it in Neuro-Genetics and Cryo."))
        hosts, hosts_auto = make_objective(
            "Side_ClearNeuroHosts",
            "Neutralize Mutated Hosts",
            "Put down the organoid hosts in the Neuro-Genetics wing.",
            "They still wear pieces of the shift.",
            stage=1,
            target=3,
            obj_type="Side",
        )
        survivor, survivor_auto = make_objective(
            "Main_HearSurvivor",
            "Hear the Incinerator Survivor",
            "Talk to the person hiding in the incinerator bay.",
            "Their account will contradict the official Admin logs.",
            stage=1,
        )
        manifests, manifests_auto = make_objective(
            "Main_RecoverCryoEvidence",
            "Recover Cryo Evidence",
            "Read the remaining facility documents downstairs.",
            "Lot numbers, consent forms, Sterling's private note.",
            stage=1,
            target=3,
        )
        set_prop(production, "Tasks", [
            make_task(hosts, hosts_auto, [make_trigger("Event_HostNeutralized", "Advance", 1)]),
            make_task(survivor, survivor_auto, [make_trigger("Event_DialogueSurvivorHeard")]),
            make_task(manifests, manifests_auto, [make_trigger("Event_DataPadRead", "Advance", 1)]),
        ])
        set_prop(production, "NextMissionAsset", handover)
        save_asset(production, MISSION_PRODUCTION)

    if audit:
        set_prop(audit, "MissionId", "Mission_TheAudit")
        set_prop(audit, "MissionTitle", text("The Audit"))
        set_prop(audit, "MissionDescription", text(
            "Avery Vance enters Epitope on a scheduled bio-hazard audit. The lockdown was already filed."))
        keycard, key_auto = make_objective(
            "Main_ObtainAdminKeycard",
            "Recover the Admin Keycard",
            "Search the HEPA plenum for a Tier 1 keycard.",
            "The east airlock will take the card, or the security office can override it.",
            stage=0,
        )
        airlock, air_auto = make_objective(
            "Main_OverrideAdminAirlock",
            "Open the Atrium Airlock",
            "Unlock the vestibule door with the keycard or the security terminal.",
            "Admin decon — HEPA seals still engaged.",
            stage=0,
        )
        logs, logs_auto = make_objective(
            "Main_ReadFacilityLogs",
            "Recover Facility Logs",
            "Read the data pads left on Admin desks.",
            "Authorization slip, shift roster, visitor log.",
            stage=0,
            target=3,
        )
        sterling, sterling_auto = make_objective(
            "Main_ReachSterlingTerminal",
            "Locate Dr. Sterling's Terminal",
            "Find the operational upgrade terminal in his field office.",
            "Unlocks after the atrium airlock is open.",
            stage=0,
            prereqs=["Main_OverrideAdminAirlock"],
            auto_activate=False,
        )
        set_prop(audit, "Tasks", [
            make_task(keycard, key_auto, [make_trigger("Event_KeycardPickedUp")]),
            make_task(airlock, air_auto, [make_trigger("Event_DoorUnlocked")]),
            make_task(logs, logs_auto, [make_trigger("Event_DataPadRead", "Advance", 1)]),
            make_task(sterling, sterling_auto, [make_trigger("Event_SterlingTerminalUsed")]),
        ])
        set_prop(audit, "NextMissionAsset", production)
        save_asset(audit, MISSION_AUDIT)

    return audit


# ----------------------------------------------------------------------------------
# Dialogue
# ----------------------------------------------------------------------------------

def make_node(node_id, speaker, line, emotion, next_id=None, ends=False, choices=None, shot="OverShoulder"):
    node = unreal.ProjectOrganoidDialogueNode()
    set_prop(node, "NodeId", node_id)
    set_prop(node, "SpeakerName", text(speaker))
    set_prop(node, "LineText", text(line))
    set_prop(node, "bEndsConversation", ends)
    if next_id:
        set_prop(node, "NextNodeId", next_id)
    emotion_val = resolve_enum("ProjectOrganoidSpeakerEmotion", emotion)
    if emotion_val is not None:
        set_prop(node, "Emotion", emotion_val)
    shot_val = resolve_enum("ProjectOrganoidDialogueCameraShot", shot)
    if shot_val is not None:
        set_prop(node, "CameraShot", shot_val)
    if choices:
        set_prop(node, "Choices", choices)
    return node


def make_choice(label, next_id, event_id=None):
    choice = unreal.ProjectOrganoidDialogueChoice()
    set_prop(choice, "ChoiceText", text(label))
    set_prop(choice, "NextNodeId", next_id)
    if event_id:
        set_prop(choice, "GameplayEventId", event_id)
    return choice


def build_dialogue():
    conversation = create_data_asset(DIALOGUE_SURVIVOR, unreal.ProjectOrganoidDialogueDataAsset)
    if not conversation:
        return None

    set_prop(conversation, "ConversationId", "Conv_IncineratorSurvivor")
    set_prop(conversation, "ConversationTitle", text("Incinerator Bay"))
    set_prop(conversation, "EntryNodeId", "Node_Open")

    nodes = [
        make_node(
            "Node_Open", "Survivor",
            "Don't... don't go down to the matrices without reading the badges first. They weren't specimens.",
            "Injured",
            choices=[
                make_choice("What happened here?", "Node_What"),
                make_choice("The logs upstairs called this a clean lockdown.", "Node_Contradiction"),
            ],
        ),
        make_node(
            "Node_What", "Survivor",
            "They were the BSL-4 team. I watched them badge in this morning. Then I watched them get catalogued.",
            "Fearful",
            next_id="Node_Close",
            shot="CloseUp",
        ),
        make_node(
            "Node_Contradiction", "Survivor",
            "That authorization was filed three days before anyone died. Someone wanted us sealed in with the product.",
            "Angry",
            next_id="Node_Close",
            shot="CloseUp",
        ),
        make_node(
            "Node_Close", "Survivor",
            "Sterling knew. He tried to stop the cryo intake. They overruled him. Check the manifests downstairs if you don't believe me.",
            "Urgent",
            choices=[
                make_choice("I'll look.", "Node_End", "Event_DialogueSurvivorHeard"),
            ],
        ),
        make_node(
            "Node_End", "Survivor",
            "Don't file what they want you to file.",
            "Tense",
            ends=True,
            shot="Profile",
        ),
    ]
    set_prop(conversation, "Nodes", nodes)
    save_asset(conversation, DIALOGUE_SURVIVOR)
    return conversation


# ----------------------------------------------------------------------------------
# Host mesh
# ----------------------------------------------------------------------------------

def assign_host_mesh():
    mesh = unreal.EditorAssetLibrary.load_asset(HOST_MESH_PATH)
    if not mesh:
        report(f"host mesh missing: {HOST_MESH_PATH}")
        return False

    if not unreal.EditorAssetLibrary.does_asset_exist(HOST_BP_PATH):
        report(f"host blueprint missing: {HOST_BP_PATH}")
        return False

    bp = unreal.EditorAssetLibrary.load_asset(HOST_BP_PATH)
    generated = None
    try:
        generated = unreal.EditorAssetLibrary.load_blueprint_class(HOST_BP_PATH)
    except Exception as exc:
        report(f"could not load host generated class: {exc}")
        return False

    cdo = unreal.get_default_object(generated) if generated else None
    if not cdo:
        report("host CDO missing")
        return False

    skeletal = None
    for name in ("mesh", "Mesh"):
        try:
            skeletal = cdo.get_editor_property(name)
            if skeletal:
                break
        except Exception:
            continue

    if not skeletal:
        report("host has no mesh component")
        return False

    assigned = False
    for setter in ("set_skeletal_mesh_asset", "set_skeletal_mesh"):
        if hasattr(skeletal, setter):
            getattr(skeletal, setter)(mesh)
            assigned = True
            break

    if not assigned:
        try:
            skeletal.set_editor_property("skeletal_mesh_asset", mesh)
            assigned = True
        except Exception:
            try:
                skeletal.set_editor_property("skeletal_mesh", mesh)
                assigned = True
            except Exception as exc:
                report(f"could not assign host mesh: {exc}")
                return False

    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception as exc:
        report(f"host compile warning: {exc}")

    unreal.EditorAssetLibrary.save_asset(HOST_BP_PATH)
    report(f"assigned {HOST_MESH_PATH} to BP_OrganoidHost")
    return True


def build_epitope_data():
    report("=== Epitope data layer ===")
    ensure_dir(DATA_ROOT)
    ensure_dir(ITEM_DIR)
    ensure_dir(MISSION_DIR)
    ensure_dir(DIALOGUE_DIR)

    build_items()
    build_missions()
    build_dialogue()
    assign_host_mesh()

    unreal.EditorAssetLibrary.save_directory(DATA_ROOT, only_if_is_dirty=False, recursive=True)
    report("=== Data assets saved ===")


build_epitope_data()
