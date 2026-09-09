# ProjectOrganoid — Section 16 BP_AdminAccessDoor auto-open / Denied FX.
# Extends the existing Section 2 Actor Blueprint. Does not reparent.
# Does not call compile_blueprint. Does not create BP_AdminDoor.
# Does not place BP_AdminSecurityGate. Does not touch S1–S12 geometry.
# Does not move/spawn room triggers or Admin_SectorController.
# Does not add door audio assets. Does not begin Section 17.
# Does not save SL_Epitope_Admin.
# Does not add a Blueprint AccessLevel enum member. Per-door security tier
# already lives on AProjectOrganoidDoorLock::RequiredSecurityTier. S16 lock
# state is the existing bool bLocked.
#
# UE 5.8 Timeline note: UBlueprint.Timelines and UK2Node_Timeline.TimelineName
# are UPROPERTY() without CPF_Edit, so Python get_editor_property cannot read
# them. FloatTracks is also not CPF_Edit. This script locates a template only
# via BlueprintEditorLibrary.get_node_title + generated_class + find_object,
# and will not invent a FloatTracks write. If Alpha is missing after a real
# UK2Node_Timeline exists, stop for a manual Timeline-editor Float Track named
# Alpha.
#
# Palette pitfall: list_available_nodes strips spaces, so "Add Timeline..."
# becomes "AddTimeline...". A substring match on "timeline" can spawn
# Audio.SetCustomSourceTimelineStart instead. Only the Add Timeline palette
# action and nodes with PlayFromStart+Update+Finished count as a Timeline.
#
# Current editor map must be /Game/Maps/Epitope/SL_Epitope_Admin.
#
# Run:
#   File -> Execute Python Script...
#   Content/Python/build_admin_section_16_access_door.py
# On COMPLETE: compile BP_AdminAccessDoor with the Blueprint editor Compile
# button only (not Python compile_blueprint). Do not save the map. Do not PIE.

import unreal

REQUIRED_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
MENU_PACKAGE = "/Game/Maps/Lvl_MainMenu"
BP_PATH = "/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor"
FORBIDDEN_DOOR_BP = "/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminDoor"
TRIGGER_BP = "/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminRoomTrigger"
CONTROLLER_LABEL = "Admin_SectorController"
OPEN_DISTANCE = 130.0
OPEN_TIME = 1.0
TIMELINE_NAME = "OpenDoor"


def _log(msg):
    unreal.log("[S16] " + msg)


def _fail(msg):
    _log("=== SECTION 16 ABORTED ===")
    _log(msg)
    raise RuntimeError(msg)


def _save_access_door_bp(reason):
    saved = unreal.EditorAssetLibrary.save_asset(BP_PATH, only_if_is_dirty=False)
    _log("saved %s (%s) ok=%s" % (BP_PATH, reason, saved))
    if not saved:
        _fail("Failed to save BP_AdminAccessDoor after %s" % reason)
    return saved


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
    if pkg == REQUIRED_PACKAGE or pkg.endswith("/SL_Epitope_Admin"):
        return REQUIRED_PACKAGE
    if pkg == MENU_PACKAGE or pkg.endswith("/Lvl_MainMenu"):
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
        if classified == REQUIRED_PACKAGE:
            return REQUIRED_PACKAGE
        if classified == MENU_PACKAGE:
            return MENU_PACKAGE
        if classified != "cannot be determined" and classified.startswith("/Game/"):
            classified_any = classified
    return classified_any


def _assert_admin_map(stage):
    world = _get_editor_world()
    pkg = _world_package(world)
    if pkg != REQUIRED_PACKAGE:
        _fail("WRONG MAP at %s. Expected %s, actual %s. ZERO writes." % (
            stage, REQUIRED_PACKAGE, pkg
        ))
    return world, pkg


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


def _actor_label(actor):
    label = _safe_call(actor, "get_actor_label")
    if label:
        return _safe_str(label)
    return _safe_str(_safe_call(actor, "get_name"))


def _actor_class_blob(actor):
    cls = _safe_call(actor, "get_class")
    return " ".join((
        _safe_str(cls),
        _safe_str(_safe_call(cls, "get_name") if cls else None),
        _safe_str(_safe_call(cls, "get_path_name") if cls else None),
    ))


def _collect_admin_actors():
    actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not actor_sub:
        _fail("EditorActorSubsystem unavailable")
    found = []
    for actor in actor_sub.get_all_level_actors():
        if _actor_owner_package(actor) != REQUIRED_PACKAGE:
            continue
        found.append(actor)
    return actor_sub, found


def _geom_key(actor):
    loc = actor.get_actor_location()
    rot = actor.get_actor_rotation()
    scale = actor.get_actor_scale3d()
    return (
        _actor_label(actor),
        round(loc.x, 2), round(loc.y, 2), round(loc.z, 2),
        round(rot.pitch, 2), round(rot.yaw, 2), round(rot.roll, 2),
        round(scale.x, 4), round(scale.y, 4), round(scale.z, 4),
    )


def _is_room_trigger(actor):
    return "AdminRoomTrigger" in _actor_class_blob(actor) and "StaticMeshActor" not in _actor_class_blob(actor)


def _is_controller(actor):
    return "AdminSectorController" in _actor_class_blob(actor) and "StaticMeshActor" not in _actor_class_blob(actor)


def _is_access_door(actor):
    blob = _actor_class_blob(actor)
    return "BP_AdminAccessDoor" in blob and "StaticMeshActor" not in blob


def _pin(node, name, direction="any"):
    lib = unreal.BlueprintEditorLibrary
    if not node:
        return None
    if direction in ("any", "out"):
        pin = lib.find_output_pin(node, name)
        if pin and unreal.BlueprintGraphPinLibrary.is_valid(pin):
            return pin
    if direction in ("any", "in"):
        pin = lib.find_input_pin(node, name)
        if pin and unreal.BlueprintGraphPinLibrary.is_valid(pin):
            return pin
    return None


def _valid_pin(pin):
    return bool(pin) and unreal.BlueprintGraphPinLibrary.is_valid(pin)


def _log_node_pins(node, label):
    if node is None:
        _log("%s node is None" % label)
        return
    try:
        pins = unreal.BlueprintEditorLibrary.list_all_pins(node)
    except Exception:
        pins = []
    names = []
    for pin in pins or []:
        try:
            names.append(str(unreal.BlueprintGraphPinLibrary.get_pin_name(pin)))
        except Exception:
            names.append("?")
    _log("%s pins=%s" % (label, ", ".join(names) if names else "(none)"))


def _self_pin(node):
    """UE 5.8 object/target pin. Internal name is PN_Self ('self'); UI label is Target.
    Use FindSelfPin rather than looking up the friendly name. Also scan list_all_pins
    because FindInputPin('Target') does not match the internal name 'self'.
    """
    if node is None:
        return None
    pin = unreal.BlueprintEditorLibrary.find_self_pin(node)
    if _valid_pin(pin):
        return pin
    for name in ("self", "Self", "Target", "target"):
        pin = _pin(node, name, "in")
        if _valid_pin(pin):
            return pin
    try:
        pins = unreal.BlueprintEditorLibrary.list_all_pins(node)
    except Exception:
        pins = []
    for pin in pins or []:
        if not _valid_pin(pin):
            continue
        try:
            name = str(unreal.BlueprintGraphPinLibrary.get_pin_name(pin)).lower()
        except Exception:
            continue
        try:
            direction = str(unreal.BlueprintGraphPinLibrary.get_pin_direction(pin)).lower()
        except Exception:
            direction = "input"
        if name in ("self", "target") and "output" not in direction:
            return pin
    _log_node_pins(node, "self-pin search")
    return None


def _result_pin(node, *fallback_names):
    if node is None:
        return None
    pin = unreal.BlueprintEditorLibrary.find_result_pin(node)
    if _valid_pin(pin):
        return pin
    for name in fallback_names + ("ReturnValue",):
        pin = _pin(node, name, "out")
        if _valid_pin(pin):
            return pin
    return None


def _exec_pin(node):
    pin = unreal.BlueprintEditorLibrary.find_execute_pin(node)
    if pin and unreal.BlueprintGraphPinLibrary.is_valid(pin):
        return pin
    return _pin(node, "execute")


def _then_pin(node):
    pin = unreal.BlueprintEditorLibrary.find_then_pin(node)
    if pin and unreal.BlueprintGraphPinLibrary.is_valid(pin):
        return pin
    return _pin(node, "then")


def _pins_already_linked(a, b):
    lib = unreal.BlueprintGraphPinLibrary
    try:
        connected = lib.list_connected_pins(a)
    except Exception:
        connected = []
    for pin in connected or []:
        try:
            if lib.is_same_native_pin(pin, b):
                return True
        except Exception:
            continue
    return False


def _connect(a, b, context):
    if not a or not b:
        _fail("Missing pin while connecting %s" % context)
    if _pins_already_linked(a, b):
        _log("already connected %s" % context)
        return
    if not unreal.BlueprintGraphPinLibrary.try_create_connection(a, b):
        _fail("Failed to connect %s" % context)


def _set_pos(node, x, y):
    unreal.BlueprintEditorLibrary.set_node_pos(node, unreal.IntPoint(int(x), int(y)))


def _member_names(bp):
    names = []
    try:
        unreal.BlueprintEditorLibrary.list_member_variable_names(bp, names, False)
    except Exception:
        pass
    return names


def _new_variable_names(bp):
    names = []
    try:
        for desc in bp.new_variables:
            raw = None
            try:
                raw = desc.get_editor_property("var_name")
            except Exception:
                raw = getattr(desc, "var_name", None)
            if raw:
                names.append(str(raw))
    except Exception:
        pass
    return names


def _has_member(bp, name):
    listed = _member_names(bp)
    short = [item.split(".")[-1] for item in listed]
    if name in listed or name in short:
        return True
    return name in _new_variable_names(bp)


def _add_bool(editor, bp, name, default):
    if _has_member(bp, name):
        _log("member exists %s" % name)
        return
    pin_type = unreal.BlueprintEditorLibrary.get_basic_type_by_name("bool")
    if not editor.add_member_variable(name, pin_type, "true" if default else "false"):
        _fail("Failed to add bool %s" % name)
    unreal.BlueprintEditorLibrary.set_blueprint_variable_instance_editable(bp, name, True)
    unreal.BlueprintEditorLibrary.set_blueprint_variable_category(
        bp, name, unreal.Text("Admin|Door")
    )
    _log("added bool %s default=%s" % (name, default))


def _add_float(editor, bp, name, default):
    if _has_member(bp, name):
        _log("member exists %s" % name)
        return
    pin_type = unreal.BlueprintEditorLibrary.get_basic_type_by_name("real")
    if not editor.add_member_variable(name, pin_type, str(default)):
        _fail("Failed to add float %s" % name)
    unreal.BlueprintEditorLibrary.set_blueprint_variable_instance_editable(bp, name, True)
    unreal.BlueprintEditorLibrary.set_blueprint_variable_category(
        bp, name, unreal.Text("Admin|Door")
    )
    _log("added float %s default=%s" % (name, default))


def _add_vector(editor, bp, name):
    if _has_member(bp, name):
        _log("member exists %s" % name)
        return
    pin_type = unreal.BlueprintEditorLibrary.get_struct_type(unreal.Vector.static_struct())
    if not editor.add_member_variable(name, pin_type, ""):
        _fail("Failed to add vector %s" % name)
    unreal.BlueprintEditorLibrary.set_blueprint_variable_instance_editable(bp, name, False)
    unreal.BlueprintEditorLibrary.set_blueprint_variable_category(
        bp, name, unreal.Text("Admin|Door")
    )
    _log("added vector %s" % name)


def _skip_access_level_member(bp):
    """S16 does not manufacture an EProjectOrganoidSecurityTier Blueprint variable.

    BP_AdminAccessDoor parents Actor, so it does not inherit DoorLock/SecurityGate
    RequiredSecurityTier. UE 5.8 Python cannot create a typed enum member:
    FEdGraphPinType.pin_category is not writable, and
    BlueprintEditorLibrary.add_member_variable_with_value is a CustomThunk that
    is not exported to Python. Keycard/tier gating already lives on
    AProjectOrganoidDoorLock::RequiredSecurityTier. This door's lock flag is bLocked.
    """
    if _has_member(bp, "AccessLevel"):
        _log("member exists AccessLevel (left in place; S16 graph uses bLocked)")
        return
    _log("skipping AccessLevel Blueprint member")
    _log("security tier remains AProjectOrganoidDoorLock.RequiredSecurityTier")
    _log("S16 Denied/open path uses existing bool bLocked")


def _available_node_names(editor):
    """UE 5.8 Python: list_available_nodes(context_pins) — exactly one argument.
    Result TArray is the return value. Pass an empty pin list when unfiltered.
    """
    result = editor.list_available_nodes([])
    if result is None:
        return []
    try:
        return [str(item) for item in result]
    except TypeError:
        return []
    if result is None:
        return []
    try:
        return [str(item) for item in result]
    except TypeError:
        return []


def _palette_compact(name):
    return (name or "").replace(" ", "").replace(".", "").lower()


def _palette_menu_key(name):
    compact = _palette_compact(name)
    if "|" in compact:
        return compact.rsplit("|", 1)[-1]
    return compact


def _is_add_timeline_palette_action(name):
    compact = _palette_compact(name)
    menu = _palette_menu_key(name)
    if "setcustomsourcetimeline" in compact or "sourcetimeline" in compact:
        return False
    if "addtimeline" in menu:
        return True
    return menu == "timeline"


def _find_timeline_action(editor):
    names = _available_node_names(editor)
    hits = [name for name in names if "timeline" in _palette_compact(name)]
    _log("timeline-ish palette entries=%s" % (", ".join(hits) if hits else "(none)"))
    accepted = [name for name in hits if _is_add_timeline_palette_action(name)]
    rejected = [name for name in hits if name not in accepted]
    if rejected:
        _log("rejected non-Add-Timeline palette entries=%s" % ", ".join(rejected))
    if accepted:
        _log("using Add Timeline palette action=%s" % accepted[0])
        return accepted[0]
    return None


def _find_cast_action(editor):
    names = _available_node_names(editor)
    hits = [name for name in names if "projectorganoidcharacter" in name.lower().replace(" ", "")]
    _log("character-cast palette entries=%s" % (", ".join(hits[:8]) if hits else "(none)"))
    for name in hits:
        if "cast" in name.lower():
            return name
    return None


def _list_nodes_of_class(editor, node_class):
    if not node_class:
        return []
    try:
        result = editor.list_nodes_of_class(node_class)
    except TypeError:
        result = None
    if result is None:
        return []
    try:
        return list(result)
    except TypeError:
        return []


def _find_existing_timeline_node(editor):
    cls = getattr(unreal, "K2Node_Timeline", None)
    for node in _list_nodes_of_class(editor, cls):
        if _is_real_timeline_node(node):
            return node
    try:
        all_nodes = editor.list_all_nodes()
    except TypeError:
        all_nodes = []
    for node in all_nodes or []:
        if type(node).__name__ == "K2Node_Timeline" and _is_real_timeline_node(node):
            return node
    return None


def _is_real_timeline_node(node):
    if node is None:
        return False
    type_name = type(node).__name__
    timeline_cls = getattr(unreal, "K2Node_Timeline", None)
    is_class = False
    if timeline_cls:
        try:
            is_class = isinstance(node, timeline_cls)
        except TypeError:
            is_class = False
    if type_name != "K2Node_Timeline" and not is_class:
        return False
    play = _pin(node, "PlayFromStart") or _pin(node, "Play")
    update = _pin(node, "Update")
    finished = _pin(node, "Finished")
    if not (play and update and finished):
        _log(
            "rejecting %s title=%s; missing PlayFromStart/Update/Finished"
            % (type_name, _timeline_node_title(node))
        )
        return False
    return True


def _log_timeline_search_audit(editor):
    try:
        all_nodes = editor.list_all_nodes()
    except TypeError:
        all_nodes = []
    hits = 0
    for node in all_nodes or []:
        type_name = type(node).__name__
        title = ""
        try:
            title = unreal.BlueprintEditorLibrary.get_node_title(node)
        except Exception:
            title = ""
        title_str = _safe_str(title, "")
        blob = (type_name + " " + title_str).lower()
        if "timeline" not in blob:
            continue
        hits += 1
        _log(
            "timeline-ish graph node type=%s title=%s real_uk2node_timeline=%s"
            % (type_name, title_str, _is_real_timeline_node(node))
        )
    _log("timeline-ish graph nodes=%d valid_uk2node_timeline=%s" % (
        hits,
        bool(_find_existing_timeline_node(editor)),
    ))


def _try_editor_property(obj, prop):
    if obj is None:
        return False, None, "object is None"
    try:
        return True, obj.get_editor_property(prop), None
    except Exception as exc:
        return False, None, "%s: %s" % (type(exc).__name__, exc)


def _try_getattr(obj, prop):
    if obj is None:
        return False, None, "object is None"
    try:
        return True, getattr(obj, prop), None
    except Exception as exc:
        return False, None, "%s: %s" % (type(exc).__name__, exc)


def _log_property_probe(obj, label, prop):
    ok, value, err = _try_editor_property(obj, prop)
    if ok:
        _log("%s get_editor_property(%s)=%s" % (label, prop, value))
    else:
        _log("%s get_editor_property(%s) not exported (%s)" % (label, prop, err))
    ok, value, err = _try_getattr(obj, prop)
    if ok:
        _log("%s getattr(%s)=%s" % (label, prop, value))
    else:
        _log("%s getattr(%s) missing (%s)" % (label, prop, err))


def _log_interesting_dir(obj, label):
    try:
        names = dir(obj)
    except Exception as exc:
        _log("%s dir() failed: %s" % (label, exc))
        return
    keys = ("timeline", "template", "track", "alpha", "guid", "jump", "float", "length")
    hits = [name for name in names if any(key in name.lower() for key in keys)]
    _log("%s dir hits=%s" % (label, ", ".join(hits) if hits else "(none)"))


def _generated_class(bp):
    try:
        generated = unreal.BlueprintEditorLibrary.generated_class(bp)
    except Exception as exc:
        _log("BlueprintEditorLibrary.generated_class failed: %s" % exc)
        return None
    _log(
        "generated_class=%s path=%s"
        % (
            type(generated).__name__ if generated else "None",
            _safe_str(_safe_call(generated, "get_path_name") if generated else None),
        )
    )
    return generated


def _timeline_node_title(timeline_node):
    try:
        title = unreal.BlueprintEditorLibrary.get_node_title(timeline_node)
    except Exception as exc:
        _log("BlueprintEditorLibrary.get_node_title failed: %s" % exc)
        return ""
    return _safe_str(title, "")


def _timeline_templates(bp):
    """Locate UTimelineTemplate inners without UBlueprint.Timelines (not CPF_Edit)."""
    templates = []
    iterator_cls = getattr(unreal, "ObjectIterator", None)
    template_cls = getattr(unreal, "TimelineTemplate", None)
    if iterator_cls and template_cls:
        try:
            for obj in iterator_cls(template_cls):
                path = _safe_str(_safe_call(obj, "get_path_name"))
                if "BP_AdminAccessDoor" in path:
                    templates.append(obj)
        except Exception as exc:
            _log("ObjectIterator(TimelineTemplate) failed: %s" % exc)
    elif not iterator_cls:
        _log("unreal.ObjectIterator not present in this Python runtime")
    elif not template_cls:
        _log("unreal.TimelineTemplate type not present in this Python runtime")
    ok, value, err = _try_editor_property(bp, "timelines")
    _log(
        "UBlueprint.Timelines probe: exported=%s value=%s err=%s"
        % (ok, value, err)
    )
    return templates


def _find_object_template(outer, object_name):
    if not outer or not object_name:
        return None
    template_cls = getattr(unreal, "TimelineTemplate", None)
    attempts = []
    if template_cls:
        attempts.append(("typed", template_cls))
    attempts.append(("untyped", None))
    for label, type_cls in attempts:
        try:
            if type_cls is not None:
                found = unreal.find_object(outer, object_name, type_cls)
            else:
                found = unreal.find_object(outer, object_name)
        except Exception as exc:
            _log("find_object(%s, %s) %s failed: %s" % (
                _safe_str(_safe_call(outer, "get_name")),
                object_name,
                label,
                exc,
            ))
            continue
        _log(
            "find_object(%s, %s) %s -> %s"
            % (
                _safe_str(_safe_call(outer, "get_name")),
                object_name,
                label,
                found,
            )
        )
        if found:
            return found
    return None


def _template_from_timeline_node(bp, timeline_node):
    title = _timeline_node_title(timeline_node)
    _log("timeline node title (GetNodeTitle ListView)=%s" % title)
    generated = _generated_class(bp)
    if title and title != "Add Timeline...":
        found = _find_object_template(generated, title + "_Template")
        if found:
            return found
        found = _find_object_template(bp, title + "_Template")
        if found:
            return found
    for template in _timeline_templates(bp):
        name = _safe_str(_safe_call(template, "get_name"))
        _log("ObjectIterator candidate=%s path=%s" % (
            name,
            _safe_str(_safe_call(template, "get_path_name")),
        ))
        if title and title != "Add Timeline..." and (name == title + "_Template" or title in name):
            return template
    if title == "Add Timeline...":
        _log("GetNodeTitle is 'Add Timeline...': FindTimelineTemplateByVariableName returned null")
    return None


def _audit_timeline_surface(bp, timeline_node):
    _log("=== UE 5.8 TIMELINE TEMPLATE API AUDIT ===")
    _log("timeline node type=%s" % type(timeline_node).__name__)
    _log("timeline node path=%s" % _safe_str(_safe_call(timeline_node, "get_path_name")))
    _log_interesting_dir(timeline_node, "UK2Node_Timeline")
    for prop in (
        "timeline_name",
        "timeline_guid",
        "b_auto_play",
        "b_loop",
        "b_replicated",
        "b_ignore_time_dilation",
        "timelines",
    ):
        _log_property_probe(timeline_node, "UK2Node_Timeline", prop)
    jump = _safe_call(timeline_node, "get_jump_target_for_double_click")
    _log("get_jump_target_for_double_click callable=%s result=%s" % (
        bool(getattr(timeline_node, "get_jump_target_for_double_click", None)),
        jump,
    ))
    _log_node_pins(timeline_node, "UK2Node_Timeline")
    _log_property_probe(bp, "UBlueprint", "timelines")
    _log_interesting_dir(bp, "UBlueprint")
    find_fn = getattr(bp, "find_timeline_template_by_variable_name", None)
    _log("UBlueprint.find_timeline_template_by_variable_name present=%s" % bool(find_fn))
    library_names = [name for name in dir(unreal.BlueprintEditorLibrary) if "timeline" in name.lower()]
    _log(
        "BlueprintEditorLibrary timeline methods=%s"
        % (", ".join(library_names) if library_names else "(none)")
    )
    _log("unreal.find_object present=%s" % bool(getattr(unreal, "find_object", None)))
    _log("unreal.ObjectIterator present=%s" % bool(getattr(unreal, "ObjectIterator", None)))
    _log("unreal.TimelineTemplate present=%s" % bool(getattr(unreal, "TimelineTemplate", None)))


def _configure_timeline_template(bp, timeline_node, length):
    _audit_timeline_surface(bp, timeline_node)
    template = _template_from_timeline_node(bp, timeline_node)
    if not template:
        _save_access_door_bp("real Timeline node present but template unresolved")
        _fail(
            "Could not resolve UTimelineTemplate through a supported UE 5.8 Python API. "
            "UK2Node_Timeline.TimelineName and UBlueprint.Timelines are UPROPERTY() without "
            "CPF_Edit, so get_editor_property cannot read them. FindTimelineTemplateByVariableName "
            "and GetJumpTargetForDoubleClick are not UFUNCTIONs. Attempted "
            "BlueprintEditorLibrary.get_node_title + generated_class + find_object(outer, "
            "title+'_Template') and ObjectIterator(TimelineTemplate). If the node title is "
            "'Add Timeline...', the template was never created. Open BP_AdminAccessDoor, "
            "double-click the existing Timeline node (do not add a second Timeline), add a "
            "Float Track named Alpha with keys 0.0=0.0 and 1.0=1.0, length 1.0, AutoPlay off, "
            "Loop off, then rerun this script."
        )
    _log("resolved template type=%s path=%s" % (
        type(template).__name__,
        _safe_str(_safe_call(template, "get_path_name")),
    ))
    _log_interesting_dir(template, "UTimelineTemplate")
    for prop in (
        "timeline_length",
        "length_mode",
        "b_auto_play",
        "b_loop",
        "b_replicated",
        "float_tracks",
        "event_tracks",
        "track_display_order",
    ):
        _log_property_probe(template, "UTimelineTemplate", prop)
    try:
        template.set_editor_property("timeline_length", float(length))
        _log("set timeline_length=%.2f (EditAnywhere)" % length)
    except Exception as exc:
        _log("set timeline_length failed: %s" % exc)
    try:
        template.set_editor_property("length_mode", unreal.TimelineLengthMode.TL_TIMELINELENGTH)
    except Exception as exc:
        _log("set length_mode skipped: %s" % exc)
    try:
        template.set_editor_property("b_auto_play", False)
        template.set_editor_property("b_loop", False)
    except Exception as exc:
        _log("set autoplay/loop skipped: %s" % exc)
    ok, tracks, err = _try_editor_property(template, "float_tracks")
    _log("float_tracks exported=%s value=%s err=%s" % (ok, tracks, err))
    _safe_call(timeline_node, "reconstruct_node")
    _log_node_pins(timeline_node, "UK2Node_Timeline after length set")
    alpha_pin = _pin(timeline_node, "Alpha")
    if alpha_pin:
        _log("Alpha pin already present; skipping any FloatTracks write")
        return template
    _save_access_door_bp("real Timeline node present; Alpha track must be added in the Timeline editor")
    _fail(
        "MANUAL_TIMELINE_ALPHA_REQUIRED: UE 5.8 Python can set EditAnywhere template fields "
        "(timeline_length, length_mode, b_auto_play, b_loop) but UTimelineTemplate.FloatTracks "
        "has no CPF_Edit, so Python cannot add the Alpha 0->1 float track or call "
        "AddDisplayTrack/ReconstructNode. Open BP_AdminAccessDoor EventGraph, double-click the "
        "existing Timeline node (reuse it; do not add another), add a Float Track named Alpha "
        "with keys 0.0=0.0 and 1.0=1.0, length 1.0, AutoPlay off, Loop off. Do not Compile yet. "
        "Then rerun this script so it can wire PlayFromStart / Update / Finished / VLerp."
    )


def _remove_tick(editor):
    tick = editor.find_event_node("ReceiveTick")
    if tick:
        editor.remove_nodes([tick])
        _log("removed ReceiveTick")
    else:
        _log("ReceiveTick already absent")


GRAPH_MARKER = "S16_ACCESS_DOOR_LOGIC"


def _event_then_connected(editor, event_name):
    node = editor.find_event_node(event_name)
    if not node:
        return False
    then_pin = _then_pin(node)
    if not then_pin:
        return False
    try:
        connected = unreal.BlueprintGraphPinLibrary.list_connected_pins(then_pin)
    except Exception:
        connected = []
    return bool(connected)


def _has_s16_marker(editor):
    try:
        comments = editor.list_comment_nodes()
    except Exception:
        comments = []
    for comment in comments or []:
        try:
            text = unreal.BlueprintEditorLibrary.get_comment_text(comment)
        except Exception:
            text = ""
        if GRAPH_MARKER in (text or ""):
            return True
    return False


def _s16_graph_already_built(editor, bp):
    if _has_s16_marker(editor):
        return True
    if _find_existing_timeline_node(editor) and _event_then_connected(editor, "ReceiveActorBeginOverlap"):
        return True
    return False


def _member_input_wired(editor, var_name):
    cls = getattr(unreal, "K2Node_VariableSet", None)
    for node in _list_nodes_of_class(editor, cls):
        pin = _pin(node, var_name, "in")
        if not pin:
            continue
        try:
            connected = unreal.BlueprintGraphPinLibrary.list_connected_pins(pin)
        except Exception:
            connected = []
        if connected:
            return True
    return False


def _should_skip_begin_play(editor):
    if _member_input_wired(editor, "ClosedLeft"):
        return True
    if _event_then_connected(editor, "ReceiveBeginPlay") and _find_existing_timeline_node(editor):
        return True
    return False


def _should_skip_overlap(editor):
    return _event_then_connected(editor, "ReceiveActorBeginOverlap")


def _node_title(node):
    if node is None:
        return ""
    try:
        return _safe_str(unreal.BlueprintEditorLibrary.get_node_title(node), "")
    except Exception:
        return ""


def _title_compact(node):
    return _node_title(node).replace(" ", "").replace("_", "").lower()


def _pin_links(pin):
    if not _valid_pin(pin):
        return []
    try:
        connected = unreal.BlueprintGraphPinLibrary.list_connected_pins(pin)
    except Exception:
        connected = []
    return list(connected or [])


def _pin_has_links(pin):
    return bool(_pin_links(pin))


def _connection_blob(node):
    parts = [_title_compact(node)]
    try:
        pins = unreal.BlueprintEditorLibrary.list_all_pins(node)
    except Exception:
        pins = []
    for pin in pins or []:
        try:
            parts.append(str(unreal.BlueprintGraphPinLibrary.get_pin_name(pin)).lower())
        except Exception:
            pass
        for other in _pin_links(pin):
            try:
                parts.append(str(unreal.BlueprintGraphPinLibrary.get_pin_name(other)).lower())
            except Exception:
                pass
            other_node = getattr(other, "node", None)
            if other_node:
                parts.append(_title_compact(other_node))
    return " ".join(parts)


def _infer_door_member(node):
    blob = _connection_blob(node)
    if "closedright" in blob or "doorright" in blob or "door_right" in blob:
        return "Door_Right"
    if "closedleft" in blob or "doorleft" in blob or "door_left" in blob:
        return "Door_Left"
    try:
        pos = unreal.BlueprintEditorLibrary.get_node_pos(node)
        if getattr(pos, "y", 0) >= 400:
            return "Door_Right"
    except Exception:
        pass
    return "Door_Left"


def _find_var_get(editor, member_name):
    cls = getattr(unreal, "K2Node_VariableGet", None)
    want = member_name.replace("_", "").lower()
    for node in _list_nodes_of_class(editor, cls):
        if want in _title_compact(node) or _pin(node, member_name, "out"):
            return node
    node = editor.add_get_member_variable_node(member_name)
    if not node:
        _fail("Could not spawn Get %s" % member_name)
    _log("spawned Get %s for component-target repair" % member_name)
    return node


def _is_relative_location_get(node):
    return type(node).__name__ == "K2Node_VariableGet" and "relativelocation" in _title_compact(node)


def _is_set_relative_location(node):
    compact = _title_compact(node)
    return "setrelativelocation" in compact


def _is_set_light_color(node):
    return "setlightcolor" in _title_compact(node)


def _find_rel_loc_palette(editor, context_pin):
    try:
        if context_pin:
            names = editor.list_available_nodes([context_pin])
        else:
            names = editor.list_available_nodes([])
    except TypeError:
        names = []
    try:
        names = [str(item) for item in (names or [])]
    except TypeError:
        names = []
    for name in names:
        compact = _palette_compact(name)
        if "setrelativelocation" in compact:
            continue
        if compact.endswith("relativelocation") or "getrelativelocation" in compact:
            return name
    return None


def _rewire_result(old_node, new_node, pin_name, context):
    old_out = _result_pin(old_node, pin_name)
    new_out = _result_pin(new_node, pin_name)
    if not old_out or not new_out:
        return
    dests = _pin_links(old_out)
    for dest in dests:
        _connect(new_out, dest, "%s rewire" % context)
    try:
        unreal.BlueprintGraphPinLibrary.break_pin_links(old_out)
    except Exception:
        pass


def _replace_hidden_rel_loc(editor, old_node, door_get, member_name):
    source = _result_pin(door_get, member_name)
    if not _valid_pin(source):
        _fail("Get %s has no result pin for RelativeLocation repair" % member_name)
    action = _find_rel_loc_palette(editor, source)
    pos = unreal.Vector2D(520.0, -40.0)
    try:
        node_pos = unreal.BlueprintEditorLibrary.get_node_pos(old_node)
        pos = unreal.Vector2D(float(node_pos.x), float(node_pos.y))
    except Exception:
        pass
    scene_class = unreal.load_class(None, "/Script/Engine.SceneComponent")
    spawned = None
    if action:
        spawned = editor.create_node_from_name(action, pos, [source], scene_class)
        _log("replaced hidden RelativeLocation via palette %s" % action)
    if not _is_relative_location_get(spawned) and not (spawned and _result_pin(spawned, "RelativeLocation")):
        spawned = editor.add_get_member_variable_node(
            "RelativeLocation", "/Script/Engine.SceneComponent"
        )
        if spawned:
            _set_pos(spawned, int(pos.x), int(pos.y))
            target = _self_pin(spawned)
            if _valid_pin(target):
                _connect(source, target, "%s -> replacement RelativeLocation target" % member_name)
    if not spawned:
        _fail("Could not replace Get RelativeLocation whose Target pin was hidden")
    _rewire_result(old_node, spawned, "RelativeLocation", member_name)
    try:
        editor.remove_nodes([old_node])
        _log("removed broken Get RelativeLocation after target repair")
    except Exception as exc:
        _log("left old RelativeLocation in place after replace: %s" % exc)
    return spawned


def _ensure_scene_target(editor, node, member_name):
    target = _self_pin(node)
    if _valid_pin(target) and _pin_has_links(target):
        _log("target already wired %s (%s)" % (_node_title(node), member_name))
        return node
    door_get = _find_var_get(editor, member_name)
    source = _result_pin(door_get, member_name)
    if not _valid_pin(source):
        _fail("Get %s missing result pin" % member_name)
    if _valid_pin(target):
        _connect(source, target, "%s -> %s target" % (member_name, _node_title(node)))
        _log("wired %s onto %s Target" % (member_name, _node_title(node)))
        return node
    if _is_relative_location_get(node):
        return _replace_hidden_rel_loc(editor, node, door_get, member_name)
    _fail(
        "No visible Target pin on %s (%s). Cannot wire %s."
        % (_node_title(node), type(node).__name__, member_name)
    )


def _repair_component_targets(editor):
    """Fix 'this blueprint (self) is not a SceneComponent' compile errors.

    Get RelativeLocation / K2_SetRelativeLocation Target pins must be Door_Left or
    Door_Right, never Actor Self. Runs even when BeginPlay spawn is skipped.
    """
    _log("repairing SceneComponent / LightComponent Target pins")
    cls_get = getattr(unreal, "K2Node_VariableGet", None)
    cls_call = getattr(unreal, "K2Node_CallFunction", None)
    rel_gets = [node for node in _list_nodes_of_class(editor, cls_get) if _is_relative_location_get(node)]
    set_locs = [node for node in _list_nodes_of_class(editor, cls_call) if _is_set_relative_location(node)]
    lights = [node for node in _list_nodes_of_class(editor, cls_call) if _is_set_light_color(node)]
    _log(
        "target-repair candidates rel_gets=%d set_rel_loc=%d set_light=%d"
        % (len(rel_gets), len(set_locs), len(lights))
    )
    for node in rel_gets:
        _ensure_scene_target(editor, node, _infer_door_member(node))
    for node in set_locs:
        _ensure_scene_target(editor, node, _infer_door_member(node))
    for node in lights:
        light_get = _find_var_get(editor, "StatusLight")
        source = _result_pin(light_get, "StatusLight")
        target = _self_pin(node)
        if _valid_pin(target) and not _pin_has_links(target):
            _connect(source, target, "StatusLight -> SetLightColor target")
            _log("wired StatusLight onto %s Target" % _node_title(node))
        elif not _valid_pin(target):
            _log("WARNING: SetLightColor has no visible Target pin: %s" % _node_title(node))
    _log("component-target repair complete")


def _build_access_door_graph(editor, bp):
    _log_timeline_search_audit(editor)
    existing_timeline = _find_existing_timeline_node(editor)
    timeline_action = _find_timeline_action(editor)
    if not timeline_action and not existing_timeline:
        _fail(
            "No Add Timeline palette action. Refusing Tick-based fallback and refusing "
            "SetCustomSourceTimelineStart / other Timeline-named CallFunction nodes."
        )
    char_class = unreal.load_class(None, "/Script/ProjectOrganoid.ProjectOrganoidCharacter")
    if not char_class:
        _fail("ProjectOrganoidCharacter missing")
    cast_action = _find_cast_action(editor)

    begin_play = editor.find_event_node("ReceiveBeginPlay")
    if not begin_play:
        begin_play = unreal.BlueprintEditorLibrary.add_event_override(
            bp, "ReceiveBeginPlay", unreal.IntPoint(0, 0)
        )
    overlap = editor.find_event_node("ReceiveActorBeginOverlap")
    if not overlap:
        overlap = unreal.BlueprintEditorLibrary.add_event_override(
            bp, "ReceiveActorBeginOverlap", unreal.IntPoint(0, 400)
        )
    if not begin_play or not overlap:
        _fail("Required Actor events missing")

    skip_begin_play = _should_skip_begin_play(editor)
    if skip_begin_play:
        _log("BeginPlay store-closed chain already complete; skipping rebuild")
    else:
        if _event_then_connected(editor, "ReceiveBeginPlay"):
            then_pin = _then_pin(begin_play)
            if then_pin:
                unreal.BlueprintGraphPinLibrary.break_pin_links(then_pin)
                _log("cleared incomplete BeginPlay exec from prior abort")

        get_left = editor.add_get_member_variable_node("Door_Left")
        get_right = editor.add_get_member_variable_node("Door_Right")
        get_rel_left = editor.add_get_member_variable_node(
            "RelativeLocation", "/Script/Engine.SceneComponent"
        )
        get_rel_right = editor.add_get_member_variable_node(
            "RelativeLocation", "/Script/Engine.SceneComponent"
        )
        if not get_rel_left or not get_rel_right:
            _fail("Could not spawn SceneComponent.RelativeLocation getters")
        set_closed_left = editor.add_set_member_variable_node("ClosedLeft")
        set_closed_right = editor.add_set_member_variable_node("ClosedRight")
        set_tick = editor.add_call_function_node("/Script/Engine.Actor.SetActorTickEnabled")
        get_light = editor.add_get_member_variable_node("StatusLight")
        set_ok_color = editor.add_call_function_node("/Script/Engine.LightComponent.SetLightColor")

        _set_pos(begin_play, 0, 0)
        _set_pos(get_left, 280, -40)
        _set_pos(get_rel_left, 520, -40)
        _set_pos(set_closed_left, 780, -40)
        _set_pos(get_right, 280, 80)
        _set_pos(get_rel_right, 520, 80)
        _set_pos(set_closed_right, 780, 80)
        _set_pos(set_tick, 780, 220)
        _set_pos(get_light, 280, 280)
        _set_pos(set_ok_color, 520, 280)

        _connect(_then_pin(begin_play), _exec_pin(set_closed_left), "BeginPlay -> store left")
        _connect(_then_pin(set_closed_left), _exec_pin(set_closed_right), "store left -> store right")
        _connect(_then_pin(set_closed_right), _exec_pin(set_tick), "store right -> disable tick")
        _connect(_then_pin(set_tick), _exec_pin(set_ok_color), "disable tick -> ok light")
        get_rel_left = _ensure_scene_target(editor, get_rel_left, "Door_Left")
        get_rel_right = _ensure_scene_target(editor, get_rel_right, "Door_Right")
        _connect(
            _result_pin(get_rel_left, "RelativeLocation"),
            _pin(set_closed_left, "ClosedLeft", "in"),
            "left loc",
        )
        _connect(
            _result_pin(get_rel_right, "RelativeLocation"),
            _pin(set_closed_right, "ClosedRight", "in"),
            "right loc",
        )
        _connect(
            _result_pin(get_light, "StatusLight"),
            _self_pin(set_ok_color),
            "StatusLight -> SetLightColor target",
        )
        ok_color = _pin(set_ok_color, "NewLightColor", "in") or _pin(set_ok_color, "Color", "in")
        if ok_color:
            unreal.BlueprintGraphPinLibrary.set_pin_value(ok_color, "(R=0.200000,G=0.850000,B=1.000000,A=1.000000)")
        tick_enabled = _pin(set_tick, "bEnabled", "in") or _pin(set_tick, "Enabled", "in")
        if tick_enabled:
            unreal.BlueprintGraphPinLibrary.set_pin_value(tick_enabled, "false")

    timeline_node = _find_existing_timeline_node(editor)
    if timeline_node:
        _log("reusing existing UK2Node_Timeline title=%s" % _timeline_node_title(timeline_node))
    else:
        if not timeline_action:
            _fail("No valid UK2Node_Timeline in the graph and no Add Timeline palette action.")
        _log("no valid UK2Node_Timeline present; spawning from %s" % timeline_action)
        spawned = editor.create_node_from_name(
            timeline_action, unreal.Vector2D(1400.0, 420.0), [], None
        )
        if not _is_real_timeline_node(spawned):
            title = _timeline_node_title(spawned) if spawned else "None"
            type_name = type(spawned).__name__ if spawned else "None"
            _fail(
                "Palette action %s spawned %s (%s), not UK2Node_Timeline. "
                "Prior aborted runs likely spawned SetCustomSourceTimelineStart CallFunction "
                "nodes; those leftovers stay unwired. Refusing to treat them as a Timeline."
                % (timeline_action, type_name, title)
            )
        timeline_node = spawned
        _log("spawned UK2Node_Timeline title=%s" % _timeline_node_title(timeline_node))
    _set_pos(timeline_node, 1400, 400)
    _configure_timeline_template(bp, timeline_node, OPEN_TIME)

    skip_overlap = _should_skip_overlap(editor)
    if skip_overlap:
        _log("Overlap exec already wired; skipping overlap/lerp rebuild so leftover nodes stay unused")
        if not _has_s16_marker(editor):
            editor.add_comment_node(GRAPH_MARKER, unreal.Vector2D(-80.0, -80.0), unreal.Vector2D(480.0, 120.0))
            _log("wrote graph marker %s" % GRAPH_MARKER)
        return

    if cast_action:
        cast_node = editor.create_node_from_name(
            cast_action, unreal.Vector2D(280.0, 420.0), [], char_class
        )
    else:
        cast_node = editor.create_node_from_name(
            "Cast To ProjectOrganoidCharacter",
            unreal.Vector2D(280.0, 420.0),
            [],
            char_class,
        )
    if not cast_node:
        _fail("Could not spawn Cast To ProjectOrganoidCharacter")
    auto_branch = editor.add_branch_node()
    lock_branch = editor.add_branch_node()
    get_auto = editor.add_get_member_variable_node("bAutomatic")
    get_locked = editor.add_get_member_variable_node("bLocked")
    get_open = editor.add_get_member_variable_node("bIsOpen")
    open_skip = editor.add_branch_node()
    denied_light_get = editor.add_get_member_variable_node("StatusLight")
    denied_color = editor.add_call_function_node("/Script/Engine.LightComponent.SetLightColor")

    _set_pos(overlap, 0, 420)
    _set_pos(cast_node, 280, 420)
    _set_pos(auto_branch, 620, 420)
    _set_pos(get_auto, 620, 300)
    _set_pos(open_skip, 860, 420)
    _set_pos(get_open, 860, 300)
    _set_pos(lock_branch, 1100, 420)
    _set_pos(get_locked, 1100, 300)
    _set_pos(denied_light_get, 1100, 620)
    _set_pos(denied_color, 1340, 620)
    _set_pos(timeline_node, 1400, 400)

    _connect(_then_pin(overlap), _exec_pin(cast_node), "overlap -> cast")
    other_pin = _pin(overlap, "OtherActor")
    cast_obj = _pin(cast_node, "Object") or _pin(cast_node, "self")
    if other_pin and cast_obj:
        _connect(other_pin, cast_obj, "OtherActor -> cast")
    cast_then = _then_pin(cast_node) or _pin(cast_node, "then")
    _connect(cast_then, _exec_pin(auto_branch), "cast success -> automatic?")
    _connect(_pin(get_auto, "bAutomatic"), unreal.BlueprintEditorLibrary.find_condition_pin(auto_branch), "bAutomatic cond")
    _connect(unreal.BlueprintEditorLibrary.find_then_pin(auto_branch), _exec_pin(open_skip), "automatic true -> open skip")
    _connect(_pin(get_open, "bIsOpen"), unreal.BlueprintEditorLibrary.find_condition_pin(open_skip), "already open cond")
    _connect(
        unreal.BlueprintEditorLibrary.find_else_pin(open_skip),
        _exec_pin(lock_branch),
        "not open -> locked?",
    )
    _connect(_pin(get_locked, "bLocked"), unreal.BlueprintEditorLibrary.find_condition_pin(lock_branch), "bLocked cond")
    _connect(
        unreal.BlueprintEditorLibrary.find_then_pin(lock_branch),
        _exec_pin(denied_color),
        "locked -> denied light",
    )
    play_pin = _pin(timeline_node, "PlayFromStart") or _pin(timeline_node, "Play")
    _connect(unreal.BlueprintEditorLibrary.find_else_pin(lock_branch), play_pin, "unlocked -> PlayFromStart")
    _connect(
        _result_pin(denied_light_get, "StatusLight") or _result_pin(denied_light_get, "StatusLight"),
        _self_pin(denied_color),
        "denied light self",
    )
    denied_color_pin = _pin(denied_color, "NewLightColor", "in") or _pin(denied_color, "Color", "in")
    if denied_color_pin:
        unreal.BlueprintGraphPinLibrary.set_pin_value(
            denied_color_pin, "(R=1.000000,G=0.120000,B=0.080000,A=1.000000)"
        )

    alpha_pin = _pin(timeline_node, "Alpha")
    update_pin = _pin(timeline_node, "Update")
    finished_pin = _pin(timeline_node, "Finished")
    if not update_pin:
        _fail("Timeline node has no Update pin")
    if not alpha_pin:
        _fail(
            "Timeline node still has no Alpha pin after template resolve. "
            "Add a Float Track named Alpha in the Blueprint Timeline editor, then rerun."
        )

    lerp_left = editor.add_call_function_node("/Script/Engine.KismetMathLibrary.VLerp")
    lerp_right = editor.add_call_function_node("/Script/Engine.KismetMathLibrary.VLerp")
    make_left_open = editor.add_call_function_node("/Script/Engine.KismetMathLibrary.Add_VectorVector")
    make_right_open = editor.add_call_function_node("/Script/Engine.KismetMathLibrary.Add_VectorVector")
    make_left_delta = editor.add_call_function_node("/Script/Engine.KismetMathLibrary.MakeVector")
    make_right_delta = editor.add_call_function_node("/Script/Engine.KismetMathLibrary.MakeVector")
    get_closed_left = editor.add_get_member_variable_node("ClosedLeft")
    get_closed_right = editor.add_get_member_variable_node("ClosedRight")
    get_dist = editor.add_get_member_variable_node("OpenDistance")
    neg_dist = editor.add_call_function_node("/Script/Engine.KismetMathLibrary.Multiply_DoubleDouble")
    set_left_loc = editor.add_call_function_node("/Script/Engine.SceneComponent.K2_SetRelativeLocation")
    set_right_loc = editor.add_call_function_node("/Script/Engine.SceneComponent.K2_SetRelativeLocation")
    get_left2 = editor.add_get_member_variable_node("Door_Left")
    get_right2 = editor.add_get_member_variable_node("Door_Right")
    set_is_open = editor.add_set_member_variable_node("bIsOpen")

    _set_pos(get_closed_left, 1680, 200)
    _set_pos(get_closed_right, 1680, 520)
    _set_pos(get_dist, 1680, 360)
    _set_pos(neg_dist, 1880, 300)
    _set_pos(make_left_delta, 2080, 260)
    _set_pos(make_right_delta, 2080, 480)
    _set_pos(make_left_open, 2280, 220)
    _set_pos(make_right_open, 2280, 500)
    _set_pos(lerp_left, 2520, 220)
    _set_pos(lerp_right, 2520, 500)
    _set_pos(get_left2, 2520, 80)
    _set_pos(get_right2, 2520, 640)
    _set_pos(set_left_loc, 2760, 180)
    _set_pos(set_right_loc, 2760, 480)
    _set_pos(set_is_open, 2760, 700)

    _connect(update_pin, _exec_pin(set_left_loc), "timeline update -> set left")
    _connect(_then_pin(set_left_loc), _exec_pin(set_right_loc), "set left -> set right")
    if finished_pin:
        _connect(finished_pin, _exec_pin(set_is_open), "timeline finished -> bIsOpen")
    open_val = _pin(set_is_open, "bIsOpen")
    if open_val:
        unreal.BlueprintGraphPinLibrary.set_pin_value(open_val, "true")

    set_left_loc = _ensure_scene_target(editor, set_left_loc, "Door_Left")
    set_right_loc = _ensure_scene_target(editor, set_right_loc, "Door_Right")
    _connect(_pin(get_closed_left, "ClosedLeft"), _pin(lerp_left, "A"), "lerp left A")
    _connect(_pin(get_closed_right, "ClosedRight"), _pin(lerp_right, "A"), "lerp right A")
    _connect(_pin(get_closed_left, "ClosedLeft"), _pin(make_left_open, "A"), "left open base")
    _connect(_pin(get_closed_right, "ClosedRight"), _pin(make_right_open, "A"), "right open base")
    _connect(_pin(get_dist, "OpenDistance"), _pin(neg_dist, "A"), "neg dist A")
    b_pin = _pin(neg_dist, "B")
    if b_pin:
        unreal.BlueprintGraphPinLibrary.set_pin_value(b_pin, "-1.0")
    _connect(_pin(neg_dist, "ReturnValue"), _pin(make_left_delta, "Y"), "left delta Y")
    _connect(_pin(get_dist, "OpenDistance"), _pin(make_right_delta, "Y"), "right delta Y")
    for zero_name in ("X", "Z"):
        zp = _pin(make_left_delta, zero_name)
        if zp:
            unreal.BlueprintGraphPinLibrary.set_pin_value(zp, "0.0")
        zp = _pin(make_right_delta, zero_name)
        if zp:
            unreal.BlueprintGraphPinLibrary.set_pin_value(zp, "0.0")
    _connect(_pin(make_left_delta, "ReturnValue"), _pin(make_left_open, "B"), "left delta vec")
    _connect(_pin(make_right_delta, "ReturnValue"), _pin(make_right_open, "B"), "right delta vec")
    _connect(_pin(make_left_open, "ReturnValue"), _pin(lerp_left, "B"), "lerp left B")
    _connect(_pin(make_right_open, "ReturnValue"), _pin(lerp_right, "B"), "lerp right B")
    if alpha_pin:
        _connect(alpha_pin, _pin(lerp_left, "Alpha"), "alpha left")
        _connect(alpha_pin, _pin(lerp_right, "Alpha"), "alpha right")
    _connect(_pin(lerp_left, "ReturnValue"), _pin(set_left_loc, "NewLocation"), "left loc in")
    _connect(_pin(lerp_right, "ReturnValue"), _pin(set_right_loc, "NewLocation"), "right loc in")
    sweep_left = _pin(set_left_loc, "bSweep")
    sweep_right = _pin(set_right_loc, "bSweep")
    if sweep_left:
        unreal.BlueprintGraphPinLibrary.set_pin_value(sweep_left, "false")
    if sweep_right:
        unreal.BlueprintGraphPinLibrary.set_pin_value(sweep_right, "false")

    if not _has_s16_marker(editor):
        editor.add_comment_node(GRAPH_MARKER, unreal.Vector2D(-80.0, -80.0), unreal.Vector2D(480.0, 120.0))
        _log('wrote graph marker %s' % GRAPH_MARKER)


_log("=== SECTION 16 ACCESS DOOR BEGIN ===")
_log("NO reparent. NO compile_blueprint. NO BP_AdminDoor. NO SecurityGate place.")
_log("ZERO S1-S12 geometry edits. ZERO S15 trigger edits. ZERO Section 17.")
if unreal.EditorAssetLibrary.does_asset_exist(FORBIDDEN_DOOR_BP):
    _fail("BP_AdminDoor already exists; Section 16 must extend BP_AdminAccessDoor only.")
world, pkg = _assert_admin_map("start")
_log("active package=%s" % pkg)

if not unreal.EditorAssetLibrary.does_asset_exist(BP_PATH):
    _fail("Missing %s" % BP_PATH)
bp = unreal.EditorAssetLibrary.load_asset(BP_PATH)
if not bp:
    _fail("Failed to load %s" % BP_PATH)
parent = unreal.BlueprintEditorLibrary.get_blueprint_parent_class(bp)
parent_name = _safe_str(_safe_call(parent, "get_name") if parent else None)
if "DoorLock" in parent_name or "ProjectOrganoidDoorLock" in parent_name:
    _fail("BP_AdminAccessDoor parent is %s; refusing reparent-class work." % parent_name)
_log("door parent=%s" % parent_name)

actor_sub, admin_actors = _collect_admin_actors()
geom_before = [_geom_key(actor) for actor in admin_actors]
doors = [actor for actor in admin_actors if _is_access_door(actor)]
triggers = [actor for actor in admin_actors if _is_room_trigger(actor)]
controllers = [actor for actor in admin_actors if _is_controller(actor)]
gates = [actor for actor in admin_actors if "BP_AdminSecurityGate" in _actor_class_blob(actor)]
s13 = [_actor_label(actor) for actor in admin_actors if _actor_label(actor).startswith("Admin_S13_")]
if len(doors) != 1:
    _fail("Expected exactly 1 BP_AdminAccessDoor instance, found %d" % len(doors))
if len(triggers) != 10:
    _fail("Expected 10 room-trigger class instances, found %d" % len(triggers))
if len(controllers) != 1:
    _fail("Expected 1 Admin_SectorController class instance, found %d" % len(controllers))
if gates:
    _fail("BP_AdminSecurityGate is placed; Section 16 must not touch it.")
if s13:
    _fail("Admin_S13_* present: %s" % ", ".join(s13))
door_actor = doors[0]
door_xform = (
    door_actor.get_actor_location(),
    door_actor.get_actor_rotation(),
    door_actor.get_actor_scale3d(),
)
_log("existing door label=%s loc=%s" % (_actor_label(door_actor), door_xform[0]))

editor = unreal.BlueprintGraphEditor.get_graph_editor_by_name(bp, "EventGraph")
if not editor:
    _fail("EventGraph editor unavailable")

existing_members = _member_names(bp)
for extra in _new_variable_names(bp):
    if extra not in existing_members:
        existing_members.append(extra)
_log("partial-state members=%s" % (", ".join(existing_members) if existing_members else "(none)"))
_log(
    "partial-state marker=%s overlap_wired=%s tick_present=%s"
    % (
        _has_s16_marker(editor),
        _event_then_connected(editor, "ReceiveActorBeginOverlap"),
        bool(editor.find_event_node("ReceiveTick")),
    )
)
_log_timeline_search_audit(editor)

_add_bool(editor, bp, "bLocked", False)
_add_bool(editor, bp, "bAutomatic", True)
_add_bool(editor, bp, "bIsOpen", False)
_add_float(editor, bp, "OpenDistance", OPEN_DISTANCE)
_add_float(editor, bp, "OpenTime", OPEN_TIME)
_add_vector(editor, bp, "ClosedLeft")
_add_vector(editor, bp, "ClosedRight")
_skip_access_level_member(bp)

_remove_tick(editor)
_repair_component_targets(editor)

if _s16_graph_already_built(editor, bp):
    _log('S16 EventGraph already present; skipping node spawn')
else:
    _build_access_door_graph(editor, bp)
_repair_component_targets(editor)

_save_access_door_bp("section 16 graph pass")

_, after_actors = _collect_admin_actors()
geom_after = [_geom_key(actor) for actor in after_actors]
if geom_before != geom_after:
    _fail("Map actor transforms changed during Blueprint edit. ZERO map save.")
after_doors = [actor for actor in after_actors if _is_access_door(actor)]
after_triggers = [actor for actor in after_actors if _is_room_trigger(actor)]
after_controllers = [actor for actor in after_actors if _is_controller(actor)]
if len(after_doors) != 1 or after_doors[0] is not door_actor:
    _fail("Door instance count/identity changed")
if len(after_triggers) != 10 or len(after_controllers) != 1:
    _fail("Trigger/controller counts changed")
after_loc = after_doors[0].get_actor_location()
if abs(after_loc.x - door_xform[0].x) > 0.05 or abs(after_loc.y - door_xform[0].y) > 0.05:
    _fail("Door instance was moved")

_log("map actor snapshot unchanged. door instance unmoved.")
_log("map NOT saved. Do not PIE from this script.")
_log("Next manual step: compile BP_AdminAccessDoor with the editor Compile button only.")
_log("=== SECTION 16 ACCESS DOOR SCRIPT COMPLETE ===")
_log("Section 17 was not started.")
