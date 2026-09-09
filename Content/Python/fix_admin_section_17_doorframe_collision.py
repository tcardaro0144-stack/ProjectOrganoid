# ProjectOrganoid — Section 17 DoorFrame collision only.
# Sets BP_AdminAccessDoor DoorFrame to NoCollision / NO_COLLISION.
# Does not touch Door_Left, Door_Right, AccessTrigger, Timeline, variables,
# EventGraph, map actors, or SL_Epitope_Admin.
# Does not call compile_blueprint (use the Blueprint editor Compile button).
#
# Run:
#   py "C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\Content\Python\fix_admin_section_17_doorframe_collision.py"

import unreal

BP_PATH = "/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor"
TARGET_NAME = "DoorFrame"
LEAVE_NAMES = ("Door_Left", "Door_Right")


def _log(msg):
    unreal.log("[S17] " + msg)


def _fail(msg):
    _log("=== SECTION 17 DOORFRAME COLLISION ABORTED ===")
    _log(msg)
    raise RuntimeError(msg)


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


def _safe_str(value, fallback=""):
    if value is None:
        return fallback
    try:
        text = str(value)
        if text and text != "None":
            return text
    except Exception:
        pass
    return fallback


def _node_name(node):
    name = _safe_str(_safe_call(node, "get_variable_name"))
    if name:
        return name
    for prop in ("variable_name", "VariableName"):
        try:
            name = _safe_str(node.get_editor_property(prop))
        except Exception:
            name = ""
        if name:
            return name
    return ""


def _template(node):
    template = getattr(node, "component_template", None)
    if template:
        return template
    for prop in ("component_template", "ComponentTemplate"):
        try:
            template = node.get_editor_property(prop)
        except Exception:
            template = None
        if template:
            return template
    return None


def _profile(comp):
    return _safe_str(_safe_call(comp, "get_collision_profile_name"), "unknown")


def _enabled(comp):
    return _safe_str(_safe_call(comp, "get_collision_enabled"), "unknown")


_log("=== SECTION 17 DOORFRAME COLLISION BEGIN ===")
_log("DoorFrame only. NO compile_blueprint. NO map save. NO leaf/trigger edits.")

if not unreal.EditorAssetLibrary.does_asset_exist(BP_PATH):
    _fail("Missing %s" % BP_PATH)
bp = unreal.EditorAssetLibrary.load_asset(BP_PATH)
if not bp:
    _fail("Failed to load %s" % BP_PATH)

scs = None
for prop in ("simple_construction_script", "SimpleConstructionScript"):
    try:
        scs = bp.get_editor_property(prop)
    except Exception:
        scs = getattr(bp, prop, None)
    if scs:
        break
if not scs:
    _fail("BP_AdminAccessDoor has no SimpleConstructionScript")

nodes = _safe_call(scs, "get_all_nodes")
if not nodes:
    _fail("SCS get_all_nodes returned no nodes")

named = {}
for node in nodes:
    name = _node_name(node)
    if name:
        named[name] = node
        _log("scs node=%s" % name)

if TARGET_NAME not in named:
    _fail("DoorFrame SCS node not found. Nodes: %s" % ", ".join(sorted(named)))

leave_before = {}
for leave in LEAVE_NAMES:
    if leave not in named:
        _fail("%s SCS node missing; aborting so leaves are not implied changed" % leave)
    template = _template(named[leave])
    if not template:
        _fail("%s has no component template" % leave)
    leave_before[leave] = (_profile(template), _enabled(template))
    _log("leave %s before profile=%s enabled=%s" % (leave, leave_before[leave][0], leave_before[leave][1]))

frame_node = named[TARGET_NAME]
frame = _template(frame_node)
if not frame:
    _fail("DoorFrame has no component template")
if not isinstance(frame, unreal.StaticMeshComponent):
    _fail("DoorFrame template is %s, expected StaticMeshComponent" % type(frame).__name__)

before_profile = _profile(frame)
before_enabled = _enabled(frame)
_log("DoorFrame before profile=%s enabled=%s" % (before_profile, before_enabled))

frame.set_collision_profile_name("NoCollision")
frame.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)

after_profile = _profile(frame)
after_enabled = _enabled(frame)
_log("DoorFrame after profile=%s enabled=%s" % (after_profile, after_enabled))
if after_profile != "NoCollision":
    _fail("DoorFrame profile is %s after write, expected NoCollision" % after_profile)
if "NO_COLLISION" not in after_enabled.upper():
    _fail("DoorFrame enabled is %s after write, expected NO_COLLISION" % after_enabled)

for leave in LEAVE_NAMES:
    template = _template(named[leave])
    now = (_profile(template), _enabled(template))
    if now != leave_before[leave]:
        _fail("%s collision changed from %s to %s" % (leave, leave_before[leave], now))
    _log("leave %s unchanged profile=%s enabled=%s" % (leave, now[0], now[1]))

saved = unreal.EditorAssetLibrary.save_asset(BP_PATH, only_if_is_dirty=False)
_log("saved %s ok=%s" % (BP_PATH, saved))
if not saved:
    _fail("Failed to save BP_AdminAccessDoor")

_log("=== SECTION 17 DOORFRAME COLLISION COMPLETE ===")
_log("Next: Compile BP_AdminAccessDoor with the editor Compile button only.")
_log("If Unreal asks to save SL_Epitope_Admin, Don't Save.")
