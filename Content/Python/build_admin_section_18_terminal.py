# ProjectOrganoid — Section 18 Phase 1 BP_AdminTerminal.
# Resume from verified Content state. These are prerequisites and must be
# reused, never duplicated or recreated:
#   parent AProjectOrganoidInteractable
#   SCS TerminalMesh, StatusLight
#   members TerminalID, TerminalType, Title, bInitiallyPowered, bIsPowered,
#           bOneShot, bHasActivated
#   EventGraph custom events Activate / PlayErrorFX / ActivateScreen / On*Activated
#   existing K2Node_SwitchEnum_0 (Switch on E_AdminTerminalType)
# Switch case pins use the OrganoidAIBridge 0.2.3 native mapping
# (internal_name / PinName), never display-name pin lookup and never Python
# UEnum enumeration.
# Do not construct E_AdminTerminalType pin types in Python.
# Does not create a second BP_AdminTerminal.
# Does not place Security/Records/Executive/Operations/Transit.
# Does not modify BP_AdminAccessDoor, Terminal_AdminSecurity, C++ terminal,
# character, InteractionComponent, Enhanced Input, Level Blueprint,
# Admin_SectorController flags, or the power subsystem.
# Saves BP_AdminTerminal only. Does not save SL_Epitope_Admin / Lvl_Epitope.
# Does not PIE. Aborts if Lvl_MainMenu is current.
#
# Run:
#   File -> Execute Python Script...
#   Content/Python/build_admin_section_18_terminal.py

import unreal

REQUIRED_PACKAGE = "/Game/Maps/Epitope/SL_Epitope_Admin"
MENU_PACKAGE = "/Game/Maps/Lvl_MainMenu"
PERSISTENT_PACKAGE = "/Game/Maps/Lvl_Epitope"
BP_PATH = "/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminTerminal"
ENUM_DIR = "/Game/ProjectOrganoid/Environment/Admin/Blueprints"
ENUM_NAME = "E_AdminTerminalType"
ENUM_PATH = ENUM_DIR + "/" + ENUM_NAME
OLD_PARENT = "/Script/ProjectOrganoid.ProjectOrganoidTerminal"
NEW_PARENT = "/Script/ProjectOrganoid.ProjectOrganoidInteractable"
CUBE_MESH = "/Engine/BasicShapes/Cube.Cube"
INSTANCE_LABEL = "Admin_Terminal_Reception"
DESK_LABEL = "Admin_S1_Reception_Desk"
DOOR_LABEL = "BP_AdminAccessDoor"
GRAPH_MARKER = "S18_PHASE1 BP_AdminTerminal"
# Established native OrganoidAIBridge 0.2.3 mapping from
# UUserDefinedEnum GetNameStringByIndex + GetDisplayNameTextByIndex +
# GetValueByIndex, independently confirmed on K2Node_SwitchEnum_0
# PinName / PinFriendlyName. Do not rediscover this via Python UEnum APIs.
SWITCH_CASES = (
    ("Reception", "NewEnumerator0", "OnReceptionActivated"),
    ("Security", "NewEnumerator1", "OnSecurityActivated"),
    ("Records", "NewEnumerator2", "OnRecordsActivated"),
    ("Executive", "NewEnumerator3", "OnExecutiveActivated"),
    ("Operations", "NewEnumerator4", "OnOperationsActivated"),
    ("Transit", "NewEnumerator5", "OnTransitActivated"),
)
TYPE_NAMES = tuple(case[0] for case in SWITCH_CASES)
SWITCH_PIN_NAMES = tuple(case[1] for case in SWITCH_CASES)
TYPE_EVENTS = tuple(case[2] for case in SWITCH_CASES)
CUSTOM_EVENTS = ("Activate", "PlayErrorFX", "ActivateScreen") + TYPE_EVENTS
MESH_REL = (0.0, 0.0, 40.0)
MESH_SCALE = (0.35, 0.35, 0.80)
LIGHT_REL = (0.0, 0.0, 90.0)
INTERACTION_RANGE = 150.0
PROMPT = "Use Terminal"


def _log(msg):
    unreal.log("[S18] " + msg)


def _fail(msg):
    _log("=== SECTION 18 PHASE 1 ABORTED ===")
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
    if pkg == PERSISTENT_PACKAGE or pkg.endswith("/Lvl_Epitope"):
        return PERSISTENT_PACKAGE
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
        if classified in (REQUIRED_PACKAGE, MENU_PACKAGE, PERSISTENT_PACKAGE):
            return classified
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
    ]
    hits = []
    for candidate in candidates:
        classified = _classify_package(candidate)
        if classified == "cannot be determined":
            continue
        if classified not in hits:
            hits.append(classified)
    if REQUIRED_PACKAGE in hits and PERSISTENT_PACKAGE in hits:
        return REQUIRED_PACKAGE
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
        _safe_str(_safe_call(cls, "get_name") if cls else None),
        _safe_str(_safe_call(cls, "get_path_name") if cls else None),
        _safe_str(_safe_call(actor, "get_path_name")),
    ))


def _compile_blueprint(bp, reason):
    compiled = False
    for lib_name in ("BlueprintEditorLibrary", "KismetEditorLibrary"):
        lib = getattr(unreal, lib_name, None)
        method = getattr(lib, "compile_blueprint", None) if lib else None
        if callable(method):
            method(bp)
            compiled = True
            break
    status = "unknown"
    try:
        status = _safe_str(bp.get_editor_property("status"))
    except Exception:
        pass
    _log("compile (%s) called=%s status=%s" % (reason, compiled, status))
    status_l = status.lower().replace(" ", "").replace("_", "")
    if "error" in status_l:
        _fail("Blueprint compile error after %s: %s" % (reason, status))
    return compiled, status


def _parent_name(bp):
    parent = None
    try:
        parent = unreal.BlueprintEditorLibrary.get_blueprint_parent_class(bp)
    except Exception:
        parent = None
    if parent is None:
        try:
            parent = bp.get_editor_property("parent_class")
        except Exception:
            parent = None
    return _safe_str(_safe_call(parent, "get_path_name") if parent else None)


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


def _self_pin(node):
    if node is None:
        return None
    pin = unreal.BlueprintEditorLibrary.find_self_pin(node)
    if _valid_pin(pin):
        return pin
    for name in ("self", "Self", "Target", "target"):
        pin = _pin(node, name, "in")
        if _valid_pin(pin):
            return pin
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


def _node_title(node):
    try:
        return _safe_str(unreal.BlueprintEditorLibrary.get_node_title(node), "")
    except Exception:
        return type(node).__name__ if node else ""


def _list_nodes(editor):
    try:
        result = editor.list_all_nodes()
    except TypeError:
        result = []
    try:
        return list(result or [])
    except TypeError:
        return []


def _find_node_by_title(editor, needle):
    needle_l = needle.lower().replace(" ", "")
    for node in _list_nodes(editor):
        compact = _node_title(node).lower().replace(" ", "")
        if needle_l in compact:
            return node
    return None


def _member_names(bp):
    names = []
    try:
        listed = unreal.BlueprintEditorLibrary.list_member_variable_names(bp, False)
        names.extend([str(item) for item in listed or []])
    except Exception:
        pass
    try:
        for desc in bp.new_variables:
            raw = None
            try:
                raw = desc.get_editor_property("var_name")
            except Exception:
                raw = getattr(desc, "var_name", None)
            if raw and str(raw) not in names:
                names.append(str(raw))
    except Exception:
        pass
    return names


def _has_member(bp, name):
    listed = _member_names(bp)
    short = [item.split(".")[-1] for item in listed]
    return name in listed or name in short


def _set_any(obj, names, value):
    for name in names:
        try:
            obj.set_editor_property(name, value)
            return name
        except Exception:
            continue
    return None


def _try_set_pin_value(pin, value):
    if not _valid_pin(pin):
        return False
    try:
        unreal.BlueprintGraphPinLibrary.set_pin_value(pin, value)
        return True
    except Exception:
        return False


def _sds():
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    if not sds:
        _fail("SubobjectDataSubsystem unavailable")
    return sds


def _subobject_handles(sds, actor, blueprint):
    lib = unreal.SubobjectDataBlueprintFunctionLibrary
    if blueprint:
        return list(sds.k2_gather_subobject_data_for_blueprint(blueprint) or [])
    return list(sds.k2_gather_subobject_data_for_instance(actor) or [])


def _add_scs_component(bp, component_class, name):
    sds = _sds()
    lib = unreal.SubobjectDataBlueprintFunctionLibrary
    actor = unreal.get_default_object(unreal.BlueprintEditorLibrary.generated_class(bp))
    actor_handle = None
    parent_handle = None
    for handle in _subobject_handles(sds, actor, bp):
        data = lib.get_data(handle)
        if lib.is_root_component(data):
            parent_handle = handle
        elif lib.is_actor(data):
            actor_handle = handle
        if parent_handle and actor_handle:
            break
    if not parent_handle:
        parent_handle = actor_handle
    if not parent_handle:
        _fail("No SCS parent handle for %s" % name)
    new_handle, failure = sds.add_new_subobject(
        unreal.AddNewSubobjectParams(parent_handle, component_class, bp)
    )
    if failure and str(failure):
        _fail("Failed to add %s: %s" % (name, failure))
    sds.rename_subobject(handle=new_handle, new_name=unreal.Text(name))
    data = lib.get_data(new_handle)
    component = lib.get_associated_object(data)
    _log("added SCS %s class=%s" % (name, type(component).__name__))
    return component


def _component_var_name(value):
    text = _safe_str(value, "")
    if not text or text in ("None", "none", "NAME_None"):
        return ""
    return text.split(".")[-1]


def _read_prop(obj, names):
    if obj is None:
        return None
    for name in names:
        try:
            value = obj.get_editor_property(name)
        except Exception:
            value = getattr(obj, name, None)
        if value is not None:
            return value
    return None


def _subobject_data(lib, handle):
    try:
        data = lib.get_data(handle)
        if data is not None:
            return data
    except TypeError:
        pass
    except Exception:
        pass
    data = unreal.SubobjectData()
    try:
        lib.get_data(handle, data)
        return data
    except Exception:
        return None


def _subobject_template(lib, data, bp):
    obj = None
    try:
        obj = lib.get_object_for_blueprint(data, bp)
    except Exception:
        obj = None
    if obj is None:
        try:
            obj = lib.get_associated_object(data)
        except Exception:
            obj = None
    return obj


def _scs_named_via_subobjects(bp):
    """Enumerate components through exported Subobject Data APIs. No add_new_subobject."""
    named = {}
    inventory = []
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    lib = getattr(unreal, "SubobjectDataBlueprintFunctionLibrary", None)
    if not sds or not lib:
        return named, inventory, "subobject_subsystem_unavailable"
    try:
        handles = list(sds.k2_gather_subobject_data_for_blueprint(bp) or [])
    except Exception as exc:
        return named, inventory, "k2_gather_subobject_data_for_blueprint failed: %s" % exc
    for handle in handles:
        data = _subobject_data(lib, handle)
        if data is None:
            continue
        is_actor = bool(_safe_call(lib, "is_actor", data))
        is_component = bool(_safe_call(lib, "is_component", data))
        if is_actor or not is_component:
            continue
        name = _component_var_name(_safe_call(lib, "get_variable_name", data))
        if not name:
            name = _component_var_name(_safe_call(lib, "get_display_name", data))
        native = bool(_safe_call(lib, "is_native_component", data))
        inherited = bool(_safe_call(lib, "is_inherited_component", data))
        local = bool(name) and (not native) and (not inherited)
        template = _subobject_template(lib, data, bp)
        inventory.append({
            "name": name,
            "native": native,
            "inherited": inherited,
            "local": local,
            "template": _safe_str(type(template).__name__ if template else None),
        })
        if local and template is not None:
            named[name] = template
    return named, inventory, "subobject_data"


def _scs_named_via_properties(bp):
    """Fallback: UPROPERTY all_nodes/root_nodes + internal_variable_name only."""
    named = {}
    inventory = []
    scs = _read_prop(bp, ("simple_construction_script", "SimpleConstructionScript"))
    if not scs:
        return named, inventory, "scs_property_unavailable"
    nodes = _read_prop(scs, ("all_nodes", "AllNodes"))
    if not nodes:
        nodes = _read_prop(scs, ("root_nodes", "RootNodes"))
    for node in nodes or []:
        name = _component_var_name(_read_prop(node, ("internal_variable_name", "InternalVariableName")))
        template = _read_prop(node, ("component_template", "ComponentTemplate"))
        local = bool(name)
        inventory.append({
            "name": name,
            "native": False,
            "inherited": False,
            "local": local,
            "template": _safe_str(type(template).__name__ if template else None),
        })
        if local:
            named[name] = template if template is not None else node
    return named, inventory, "scs_uproperty"


def _scs_named(bp):
    named, inventory, path = _scs_named_via_subobjects(bp)
    if "TerminalMesh" not in named or "StatusLight" not in named:
        fallback_named, fallback_inventory, fallback_path = _scs_named_via_properties(bp)
        if fallback_named:
            named = fallback_named
            inventory = fallback_inventory
            path = fallback_path
    _log("SCS lookup path=%s count=%d names=%s" % (
        path,
        len(named),
        ", ".join(sorted(named.keys())) or "(none)",
    ))
    for row in inventory:
        _log(
            "SCS item name=%s native=%s inherited=%s local=%s template=%s"
            % (row.get("name") or "(unnamed)", row.get("native"), row.get("inherited"), row.get("local"), row.get("template"))
        )
    return named, inventory, path


def _scs_lookup_self_test(named, inventory):
    def _row(name):
        for item in inventory or []:
            if item.get("name") == name:
                return item
        return None

    mesh = "TerminalMesh" in named
    light = "StatusLight" in named
    mesh_row = _row("TerminalMesh")
    light_row = _row("StatusLight")
    both_local = bool(
        mesh and light
        and mesh_row and light_row
        and mesh_row.get("local") and light_row.get("local")
        and (not mesh_row.get("native")) and (not light_row.get("native"))
        and (not mesh_row.get("inherited")) and (not light_row.get("inherited"))
    )
    _log("TerminalMesh found = %s" % ("true" if mesh else "false"))
    _log("StatusLight found = %s" % ("true" if light else "false"))
    _log("both local, non-native, non-inherited" if both_local else "NOT both local, non-native, non-inherited")
    return mesh, light, both_local


def _template_from_node(node):
    if node is None:
        return None
    try:
        if isinstance(node, unreal.ActorComponent):
            return node
    except Exception:
        pass
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


def _load_manual_enum():
    """E_AdminTerminalType is a manual prerequisite. Do not read or rewrite entries.

    UE 5.8 Python does not export UserDefinedEnum.set_enums, and num_enums /
    get_name_string_by_index report empty even when the Content asset is valid.
    Existence + load is the only Python check. Values were authored in the editor.
    """
    if not unreal.EditorAssetLibrary.does_asset_exist(ENUM_PATH):
        _fail(
            "Manual prerequisite missing: %s. "
            "E_AdminTerminalType must already exist in Content. Python will not create or fill it."
            % ENUM_PATH
        )
    enum_asset = unreal.EditorAssetLibrary.load_asset(ENUM_PATH)
    if not enum_asset:
        _fail("Failed to load existing %s" % ENUM_PATH)
    _log("using manual prerequisite enum %s (no Python name read, no rewrite, no enum save)" % ENUM_PATH)
    return enum_asset


def _reuse_prerequisite_member(bp, name):
    if not _has_member(bp, name):
        _fail(
            "Prerequisite variable %s is missing on BP_AdminTerminal. "
            "Refusing to recreate it. TerminalType must not be built through Python pin construction."
            % name
        )
    _log("reusing prerequisite variable %s" % name)
    return name


def _reuse_prerequisite_scs(named, name):
    if name not in named:
        _fail(
            "Prerequisite SCS component %s is missing on BP_AdminTerminal. "
            "Refusing to recreate it."
            % name
        )
    _log("reusing prerequisite SCS %s" % name)
    return named[name]


def _add_typed_variable(bp, name, pin_type, instance_editable, category):
    if name == "TerminalType":
        _fail(
            "TerminalType is a prerequisite. Python must not add or retype it. "
            "Use the existing E_AdminTerminalType member."
        )
    if _has_member(bp, name):
        _log("member exists %s" % name)
    else:
        if not pin_type:
            _fail("Failed to resolve pin type for variable %s" % name)
        if not unreal.BlueprintEditorLibrary.add_member_variable(bp, name, pin_type):
            _fail("Failed to add variable %s" % name)
        _log("added variable %s" % name)
    unreal.BlueprintEditorLibrary.set_blueprint_variable_instance_editable(bp, name, instance_editable)
    unreal.BlueprintEditorLibrary.set_blueprint_variable_category(bp, name, unreal.Text(category))


def _add_basic_variable(bp, name, type_name, instance_editable, category):
    pin_type = unreal.BlueprintEditorLibrary.get_basic_type_by_name(type_name)
    _add_typed_variable(bp, name, pin_type, instance_editable, category)


def _has_marker(editor):
    for node in _list_nodes(editor):
        try:
            text = _safe_str(unreal.BlueprintEditorLibrary.get_comment_text(node), "")
        except Exception:
            text = ""
        if GRAPH_MARKER in text:
            return True
        if GRAPH_MARKER in _node_title(node):
            return True
    return False


def _compact_name(value):
    return _safe_str(value, "").lower().replace(" ", "").replace("_", "")


def _node_type_name(node):
    return type(node).__name__ if node else ""


def _pin_names(node):
    names = []
    try:
        pins = unreal.BlueprintEditorLibrary.list_all_pins(node) or []
    except Exception:
        pins = []
    for pin in pins:
        try:
            names.append(str(unreal.BlueprintGraphPinLibrary.get_pin_name(pin)))
        except Exception:
            continue
    return names


def _pin_has_links(pin):
    if not _valid_pin(pin):
        return False
    try:
        connected = unreal.BlueprintGraphPinLibrary.list_connected_pins(pin)
    except Exception:
        connected = []
    return bool(connected)


def _log_graph_inventory(editor, label):
    rows = []
    for node in _list_nodes(editor):
        rows.append("%s/%s" % (_node_title(node), _node_type_name(node)))
    _log("%s nodes=%d %s" % (label, len(rows), "; ".join(rows[:60])))


def _find_switch_enum(editor):
    """Reuse the live Switch on E_AdminTerminalType. Identified by PinName set, not display names."""
    for node in _list_nodes(editor):
        title = _compact_name(_node_title(node))
        type_name = _compact_name(_node_type_name(node))
        pins = _pin_names(node)
        has_cases = ("NewEnumerator0" in pins and "NewEnumerator5" in pins and "Selection" in pins)
        looks_like = (
            "switchenum" in type_name
            or "switchone_adminterminaltype" in title
            or "switchoneadminterminaltype" in title
            or ("switch" in title and "eadminterminaltype" in title)
        )
        if has_cases or looks_like:
            if _pin(node, "NewEnumerator0", "out"):
                _log("reusing existing Switch node title=%s type=%s pins=%s" % (
                    _node_title(node), _node_type_name(node), ", ".join(pins)))
                return node
    return None


def _ensure_marker(editor):
    if _has_marker(editor):
        _log("graph marker already present")
        return
    editor.add_comment_node(GRAPH_MARKER, unreal.Vector2D(-80.0, -80.0), unreal.Vector2D(520.0, 120.0))
    _log("wrote graph marker %s" % GRAPH_MARKER)


def _ensure_custom_event(editor, name, x, y):
    node = editor.find_event_node(name)
    if node:
        _log("reusing custom event %s" % name)
        return node
    compact = _compact_name(name)
    for existing in _list_nodes(editor):
        if compact != _compact_name(_node_title(existing)):
            continue
        if "callevent" in _compact_name(_node_type_name(existing)):
            continue
        if "customevent" in _compact_name(_node_type_name(existing)) or not _exec_pin(existing):
            _log("reusing custom event %s" % name)
            return existing
    node = editor.add_custom_event_node(name)
    if not node:
        _fail("Failed to add custom event %s" % name)
    _set_pos(node, x, y)
    _log("added custom event %s" % name)
    return node


def _spawn_call(editor, name, x, y):
    node = editor.create_node_from_name(name, unreal.Vector2D(float(x), float(y)), [], None)
    if not node:
        node = editor.create_node_from_name(
            "AddEvent|Custom|" + name, unreal.Vector2D(float(x), float(y)), [], None
        )
    if not node:
        node = editor.add_call_function_node(name)
    if not node:
        _fail("Could not spawn call to %s" % name)
    _set_pos(node, x, y)
    return node


def _ensure_call(editor, name, x, y):
    event_node = editor.find_event_node(name)
    compact = _compact_name(name)
    for node in _list_nodes(editor):
        if node is event_node:
            continue
        if compact not in _compact_name(_node_title(node)):
            continue
        if "customevent" in _compact_name(_node_type_name(node)):
            continue
        if _exec_pin(node):
            _log("reusing call node %s" % name)
            return node
    _log("spawning call node %s" % name)
    return _spawn_call(editor, name, x, y)


def _ensure_type_event_print(editor, event_node, event_name, x, y):
    then_pin = _then_pin(event_node) if event_node else None
    if not then_pin:
        _log("no then pin on %s; skipping print" % event_name)
        return
    if _pin_has_links(then_pin):
        _log("already connected %s then" % event_name)
        return
    type_print = _print_node(editor, "AdminTerminal %s" % event_name, x, y)
    _connect(then_pin, _exec_pin(type_print), "%s -> print" % event_name)


def _wire_verified_switch_cases(editor, switch_node, type_event_nodes):
    _log("wiring Switch cases from native mapping Reception=NewEnumerator0 ... Transit=NewEnumerator5")
    for index, (display_name, internal_name, event_name) in enumerate(SWITCH_CASES):
        case_pin = _pin(switch_node, internal_name, "out")
        if not case_pin:
            _log("switch pins=%s" % ", ".join(_pin_names(switch_node)))
            _fail("Switch missing verified case pin %s (%s)" % (internal_name, display_name))
        event_node = None
        if type_event_nodes and index < len(type_event_nodes):
            event_node = type_event_nodes[index]
        if event_node is None:
            event_node = editor.find_event_node(event_name)
        call_type = _ensure_call(editor, event_name, 1600, 1320 + index * 80)
        _connect(
            case_pin,
            _exec_pin(call_type),
            "switch %s (%s) -> %s" % (internal_name, display_name, event_name),
        )
        _ensure_type_event_print(editor, event_node, event_name, 280, 1500 + index * 160)


def _switch_cases_unwired(switch_node):
    missing = []
    for display_name, internal_name, _event_name in SWITCH_CASES:
        pin = _pin(switch_node, internal_name, "out")
        if not pin or not _pin_has_links(pin):
            missing.append("%s/%s" % (internal_name, display_name))
    return missing


def _print_node(editor, message, x, y):
    node = editor.add_call_function_node("/Script/Engine.KismetSystemLibrary.PrintString")
    if not node:
        _fail("Failed to spawn PrintString")
    _set_pos(node, x, y)
    in_string = _pin(node, "InString") or _pin(node, "in_string")
    _try_set_pin_value(in_string, message)
    duration = _pin(node, "Duration")
    _try_set_pin_value(duration, "3.0")
    return node


def _set_light(editor, color_text, x, y):
    get_light = editor.add_get_member_variable_node("StatusLight")
    set_color = editor.add_call_function_node("/Script/Engine.LightComponent.SetLightColor")
    _set_pos(get_light, x, y - 80)
    _set_pos(set_color, x + 240, y)
    _connect(_result_pin(get_light, "StatusLight"), _self_pin(set_color), "StatusLight -> SetLightColor")
    color_pin = _pin(set_color, "NewLightColor", "in") or _pin(set_color, "Color", "in")
    _try_set_pin_value(color_pin, color_text)
    return set_color


def _wire_event_graph(bp, editor):
    _log_graph_inventory(editor, "EventGraph before wire")
    _log("native Switch mapping (do not Python-enumerate enum): %s" % ", ".join(
        "%s=%s" % (display, internal) for display, internal, _event in SWITCH_CASES
    ))
    switch_node = _find_switch_enum(editor)
    activate = editor.find_event_node("Activate")
    type_event_flags = []
    for event_name in TYPE_EVENTS:
        present = bool(editor.find_event_node(event_name))
        type_event_flags.append("%s=%s" % (event_name, present))
    _log("resume detect switch=%s activate=%s marker=%s %s" % (
        bool(switch_node),
        bool(activate),
        _has_marker(editor),
        ", ".join(type_event_flags),
    ))

    type_nodes = []
    for index, event_name in enumerate(TYPE_EVENTS):
        type_nodes.append(_ensure_custom_event(editor, event_name, 0, 1500 + index * 160))
    _ensure_custom_event(editor, "Activate", 0, 360)
    _ensure_custom_event(editor, "PlayErrorFX", 0, 860)
    _ensure_custom_event(editor, "ActivateScreen", 0, 1120)

    if switch_node:
        missing = _switch_cases_unwired(switch_node)
        _log("existing Switch unwired cases=%s" % (", ".join(missing) if missing else "(none)"))
        if missing:
            _wire_verified_switch_cases(editor, switch_node, type_nodes)
        else:
            _log("all verified Switch cases already connected; not spawning calls")
        _ensure_marker(editor)
        _log("partial EventGraph resume complete (Switch reused, Activate body not rebuilt)")
        return

    if _has_marker(editor) and activate:
        _log("EventGraph already has S18 marker + Activate but no Switch; continuing to author Switch")

    begin_play = editor.find_event_node("ReceiveBeginPlay")
    if not begin_play:
        begin_play = unreal.BlueprintEditorLibrary.add_event_override(
            bp, "ReceiveBeginPlay", unreal.IntPoint(0, 0)
        )
    if not begin_play:
        _fail("ReceiveBeginPlay missing")
    _set_pos(begin_play, 0, 0)
    get_init = editor.add_get_member_variable_node("bInitiallyPowered")
    set_powered = editor.add_set_member_variable_node("bIsPowered")
    _set_pos(get_init, 280, -80)
    _set_pos(set_powered, 520, 0)
    if _then_pin(begin_play) and _exec_pin(set_powered):
        then_pin = _then_pin(begin_play)
        try:
            unreal.BlueprintGraphPinLibrary.break_pin_links(then_pin)
        except Exception:
            pass
        _connect(then_pin, _exec_pin(set_powered), "BeginPlay -> bIsPowered")
    _connect(_pin(get_init, "bInitiallyPowered"), _pin(set_powered, "bIsPowered"), "init -> runtime power")

    activate = _ensure_custom_event(editor, "Activate", 0, 360)
    play_error = _ensure_custom_event(editor, "PlayErrorFX", 0, 860)
    play_screen = _ensure_custom_event(editor, "ActivateScreen", 0, 1120)
    type_nodes = []
    for index, event_name in enumerate(TYPE_EVENTS):
        type_nodes.append(_ensure_custom_event(editor, event_name, 0, 1500 + index * 160))
    _compile_blueprint(bp, "after custom event nodes")
    editor = unreal.BlueprintGraphEditor.get_graph_editor_by_name(bp, "EventGraph")
    if not editor:
        _fail("EventGraph editor lost after custom-event compile")
    activate = _ensure_custom_event(editor, "Activate", 0, 360)
    play_error = _ensure_custom_event(editor, "PlayErrorFX", 0, 860)
    play_screen = _ensure_custom_event(editor, "ActivateScreen", 0, 1120)
    type_nodes = []
    for index, event_name in enumerate(TYPE_EVENTS):
        type_nodes.append(_ensure_custom_event(editor, event_name, 0, 1500 + index * 160))

    oneshot_and = editor.add_call_function_node("/Script/Engine.KismetMathLibrary.BooleanAND")
    get_oneshot = editor.add_get_member_variable_node("bOneShot")
    get_has = editor.add_get_member_variable_node("bHasActivated")
    skip_branch = editor.add_branch_node()
    power_branch = editor.add_branch_node()
    get_power = editor.add_get_member_variable_node("bIsPowered")
    _set_pos(get_oneshot, 280, 260)
    _set_pos(get_has, 280, 320)
    _set_pos(oneshot_and, 520, 280)
    _set_pos(skip_branch, 760, 360)
    _set_pos(get_power, 760, 520)
    _set_pos(power_branch, 1000, 360)
    _connect(_then_pin(activate), _exec_pin(skip_branch), "Activate -> one-shot skip")
    _connect(_pin(get_oneshot, "bOneShot"), _pin(oneshot_and, "A"), "oneshot A")
    _connect(_pin(get_has, "bHasActivated"), _pin(oneshot_and, "B"), "oneshot B")
    _connect(_result_pin(oneshot_and), unreal.BlueprintEditorLibrary.find_condition_pin(skip_branch), "skip cond")
    _connect(
        unreal.BlueprintEditorLibrary.find_else_pin(skip_branch),
        _exec_pin(power_branch),
        "not consumed -> power?",
    )
    _connect(_pin(get_power, "bIsPowered"), unreal.BlueprintEditorLibrary.find_condition_pin(power_branch), "power cond")

    call_error = _ensure_call(editor, "PlayErrorFX", 1240, 520)
    call_screen = _ensure_call(editor, "ActivateScreen", 1240, 280)
    _connect(unreal.BlueprintEditorLibrary.find_else_pin(power_branch), _exec_pin(call_error), "unpowered -> Error")
    _connect(unreal.BlueprintEditorLibrary.find_then_pin(power_branch), _exec_pin(call_screen), "powered -> Screen")

    error_print = _print_node(editor, "AdminTerminal Error: unpowered", 280, 860)
    error_light = _set_light(editor, "(R=1.000000,G=0.120000,B=0.080000,A=1.000000)", 280, 980)
    _connect(_then_pin(play_error), _exec_pin(error_light), "PlayErrorFX -> light")
    _connect(_then_pin(error_light), _exec_pin(error_print), "error light -> print")

    screen_print = _print_node(editor, "AdminTerminal Screen", 280, 1120)
    screen_light = _set_light(editor, "(R=0.200000,G=0.850000,B=1.000000,A=1.000000)", 280, 1240)
    set_has = editor.add_set_member_variable_node("bHasActivated")
    oneshot_disable = editor.add_branch_node()
    get_oneshot2 = editor.add_get_member_variable_node("bOneShot")
    set_interactable = editor.add_set_member_variable_node("bIsInteractable")
    get_type = editor.add_get_member_variable_node("TerminalType")
    _set_pos(set_has, 760, 1120)
    _set_pos(get_oneshot2, 1000, 1040)
    _set_pos(oneshot_disable, 1000, 1120)
    _set_pos(set_interactable, 1240, 1180)
    _set_pos(get_type, 760, 1320)
    _connect(_then_pin(play_screen), _exec_pin(screen_light), "ActivateScreen -> light")
    _connect(_then_pin(screen_light), _exec_pin(screen_print), "screen light -> print")
    _connect(_then_pin(screen_print), _exec_pin(set_has), "print -> bHasActivated")
    _try_set_pin_value(_pin(set_has, "bHasActivated"), "true")
    _connect(_then_pin(set_has), _exec_pin(oneshot_disable), "has -> oneshot?")
    _connect(_pin(get_oneshot2, "bOneShot"), unreal.BlueprintEditorLibrary.find_condition_pin(oneshot_disable), "oneshot disable cond")
    _connect(
        unreal.BlueprintEditorLibrary.find_then_pin(oneshot_disable),
        _exec_pin(set_interactable),
        "oneshot -> disable interact",
    )
    _try_set_pin_value(_pin(set_interactable, "bIsInteractable"), "false")

    type_pin = _result_pin(get_type, "TerminalType")
    switch_node = _find_switch_enum(editor)
    if switch_node:
        _log("reusing existing Switch while authoring Activate body")
        if type_pin and _pin(switch_node, "Selection", "in"):
            _connect(type_pin, _pin(switch_node, "Selection", "in"), "TerminalType -> Switch Selection")
    elif type_pin:
        names = []
        try:
            names = [str(item) for item in editor.list_available_nodes([type_pin]) or []]
        except Exception:
            names = []
        hits = [name for name in names if "switch" in name.lower().replace(" ", "")]
        _log("switch palette hits=%s" % (", ".join(hits[:8]) if hits else "(none)"))
        if hits:
            switch_node = editor.create_node_from_name(
                hits[0], unreal.Vector2D(1240.0, 1320.0), [type_pin], None
            )
    if not switch_node:
        _fail("Could not spawn Switch on TerminalType")
    _set_pos(switch_node, 1240, 1320)
    else_pin = unreal.BlueprintEditorLibrary.find_else_pin(oneshot_disable)
    if else_pin:
        _connect(else_pin, _exec_pin(switch_node), "not oneshot -> switch")
    if _then_pin(set_interactable):
        _connect(_then_pin(set_interactable), _exec_pin(switch_node), "oneshot disable -> switch")

    _wire_verified_switch_cases(editor, switch_node, type_nodes)
    _ensure_marker(editor)


def _wire_interact_override(bp):
    existing_editor = unreal.BlueprintGraphEditor.get_graph_editor_by_name(bp, "Interact")
    if existing_editor and _find_node_by_title(existing_editor, "Activate"):
        _log("Interact override already calls Activate; leaving as-is")
        return
    infos = []
    try:
        infos = list(unreal.BlueprintEditorLibrary.list_functions(bp) or [])
    except Exception:
        infos = []
    interact_info = None
    for info in infos:
        name = ""
        try:
            name = _safe_str(info.get_editor_property("name"))
        except Exception:
            name = _safe_str(getattr(info, "name", ""))
        if name == "Interact":
            interact_info = info
            break
    if interact_info is None:
        _fail("Interact is not an inherited function on BP_AdminTerminal after reparent")
    graph = unreal.BlueprintEditorLibrary.add_function_override(bp, "Interact")
    if not graph:
        _fail("add_function_override(Interact) failed")
    editor = unreal.BlueprintGraphEditor.get_graph_editor_by_name(bp, "Interact")
    if not editor:
        _fail("Interact function graph editor unavailable")
    parent_node = None
    for node in _list_nodes(editor):
        type_name = type(node).__name__
        title = _node_title(node)
        if "CallParent" in type_name or "Parent" in title:
            parent_node = node
            break
    if not parent_node:
        _fail("Interact override is missing the emitted Call Parent node")
    if _find_node_by_title(editor, "Activate"):
        _log("Interact override already calls Activate; leaving as-is")
        return
    get_oneshot = editor.add_get_member_variable_node("bOneShot")
    get_has = editor.add_get_member_variable_node("bHasActivated")
    oneshot_and = editor.add_call_function_node("/Script/Engine.KismetMathLibrary.BooleanAND")
    skip_branch = editor.add_branch_node()
    power_ok = editor.add_branch_node()
    call_activate = _ensure_call(editor, "Activate", 1480, 40)
    entry = None
    for node in _list_nodes(editor):
        if "FunctionEntry" in type(node).__name__ or _node_title(node).lower().startswith("interact"):
            if _then_pin(node) and _pin(node, "Interactor"):
                entry = node
                break
    if entry is None:
        for node in _list_nodes(editor):
            if "FunctionEntry" in type(node).__name__:
                entry = node
                break
    if entry is None:
        _fail("Interact function entry node missing")
    entry_then = _then_pin(entry)
    parent_exec = _exec_pin(parent_node)
    if entry_then and parent_exec:
        try:
            unreal.BlueprintGraphPinLibrary.break_pin_links(entry_then)
        except Exception:
            pass
        _connect(entry_then, _exec_pin(skip_branch), "Interact entry -> one-shot skip")
    _set_pos(get_oneshot, 280, -40)
    _set_pos(get_has, 280, 40)
    _set_pos(oneshot_and, 520, 0)
    _set_pos(skip_branch, 760, 80)
    _set_pos(parent_node, 1000, 80)
    _set_pos(power_ok, 1240, 80)
    _set_pos(call_activate, 1480, 40)
    _connect(_pin(get_oneshot, "bOneShot"), _pin(oneshot_and, "A"), "override oneshot A")
    _connect(_pin(get_has, "bHasActivated"), _pin(oneshot_and, "B"), "override oneshot B")
    _connect(_result_pin(oneshot_and), unreal.BlueprintEditorLibrary.find_condition_pin(skip_branch), "override skip cond")
    _connect(unreal.BlueprintEditorLibrary.find_else_pin(skip_branch), parent_exec, "not consumed -> Parent Interact")
    parent_then = _then_pin(parent_node)
    parent_ret = _result_pin(parent_node, "ReturnValue")
    if parent_then:
        try:
            unreal.BlueprintGraphPinLibrary.break_pin_links(parent_then)
        except Exception:
            pass
        _connect(parent_then, _exec_pin(power_ok), "Parent then -> CanInteract result")
    if parent_ret:
        _connect(parent_ret, unreal.BlueprintEditorLibrary.find_condition_pin(power_ok), "Parent ReturnValue cond")
    _connect(unreal.BlueprintEditorLibrary.find_then_pin(power_ok), _exec_pin(call_activate), "Parent true -> Activate")
    _log("Interact override: Call Parent (CanInteract + OnInteracted) then Activate")


def _mesh_asset_path(component):
    mesh = _read_prop(component, ("static_mesh", "StaticMesh"))
    if mesh is None:
        return None
    path = _safe_call(mesh, "get_path_name")
    if path:
        return _safe_str(path)
    return _safe_str(mesh)


def _configure_mesh(component):
    loc = _safe_call(component, "get_relative_location")
    scale = _safe_call(component, "get_relative_scale3d")
    mesh_path = _mesh_asset_path(component)
    _log("TerminalMesh inspect StaticMesh=%s rel=%s scale=%s" % (
        mesh_path or "(none)",
        _safe_str(loc),
        _safe_str(scale),
    ))
    if mesh_path:
        _log("TerminalMesh already has a StaticMesh; not recreating and not rewriting transform/bounds")
        return
    _log("TerminalMesh has no StaticMesh assigned; applying Cube placeholder + intended transform")
    mesh = unreal.EditorAssetLibrary.load_asset(CUBE_MESH)
    if mesh:
        _set_any(component, ("static_mesh", "StaticMesh"), mesh)
    component.set_relative_location(unreal.Vector(*MESH_REL), False, False)
    component.set_relative_scale3d(unreal.Vector(*MESH_SCALE))
    component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    component.set_collision_profile_name("BlockAllDynamic")


def _configure_light(component):
    component.set_relative_location(unreal.Vector(*LIGHT_REL), False, False)
    _set_any(component, ("intensity", "Intensity"), 800.0)
    _set_any(component, ("attenuation_radius", "AttenuationRadius"), 180.0)
    _set_any(component, ("light_color", "LightColor"), unreal.Color(51, 217, 255, 255))


def _desk_terminal_location(desk):
    origin = None
    extent = None
    try:
        origin, extent = desk.get_actor_bounds(True)
    except Exception:
        origin = desk.get_actor_location()
        extent = unreal.Vector(50.0, 200.0, 55.0)
    top_z = origin.z + extent.z
    loc = unreal.Vector(origin.x, origin.y, top_z)
    _log(
        "desk bounds origin=(%.1f, %.1f, %.1f) extent=(%.1f, %.1f, %.1f) place=(%.1f, %.1f, %.1f)"
        % (origin.x, origin.y, origin.z, extent.x, extent.y, extent.z, loc.x, loc.y, loc.z)
    )
    return loc


def _find_admin_streaming(world):
    for streaming in world.get_streaming_levels() or []:
        name = _safe_str(_safe_call(streaming, "get_world_asset_package_f_name"))
        if "SL_Epitope_Admin" in name:
            return streaming
    return None


def _move_to_admin(actor, world):
    owner = _actor_owner_package(actor)
    if owner == REQUIRED_PACKAGE:
        return owner
    streaming = _find_admin_streaming(world)
    if not streaming:
        return owner
    moved = False
    try:
        moved = bool(unreal.EditorLevelUtils.move_actors_to_level([actor], streaming, False))
    except Exception as exc:
        _log("move_actors_to_level failed: %s" % exc)
    _log("move to Admin streaming moved=%s" % moved)
    return _actor_owner_package(actor)


def _dirty_packages():
    rows = []
    utils = getattr(unreal, "EditorLoadingAndSavingUtils", None)
    if not utils:
        return ["EditorLoadingAndSavingUtils unavailable"]
    for method_name in ("get_dirty_content_packages", "get_dirty_map_packages"):
        method = getattr(utils, method_name, None)
        if not callable(method):
            continue
        try:
            packages = method()
        except Exception as exc:
            rows.append("%s error=%s" % (method_name, exc))
            continue
        if not packages:
            rows.append("%s=(none)" % method_name)
            continue
        for package in packages:
            rows.append("%s %s" % (
                method_name,
                _safe_str(_safe_call(package, "get_name") or _safe_call(package, "get_path_name")),
            ))
    return rows


_log("=== SECTION 18 PHASE 1 BEGIN ===")
_log("NO second BP_AdminTerminal. NO five extra types. NO AccessDoor edits. NO PIE. NO Save All.")
_log("Prerequisites reused: parent Interactable, TerminalMesh, StatusLight, TerminalID, TerminalType.")
_log("Switch case pins use native NewEnumerator0-5 mapping. Python will not enumerate E_AdminTerminalType.")
_log("Interact is a function-shape NativeEvent. Override will Call Parent then Activate.")

world = _get_editor_world()
pkg = _world_package(world)
_log("active package=%s" % pkg)
if pkg == MENU_PACKAGE:
    _fail("Lvl_MainMenu is current. Open SL_Epitope_Admin or Lvl_Epitope with Admin streamed. ZERO writes.")
if pkg not in (REQUIRED_PACKAGE, PERSISTENT_PACKAGE):
    _fail("Unexpected current package %s. ZERO writes." % pkg)

if not unreal.EditorAssetLibrary.does_asset_exist(BP_PATH):
    _fail("Missing S13 stub %s" % BP_PATH)

actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
all_actors = actor_sub.get_all_level_actors() if actor_sub else []
door_actors = [a for a in all_actors if DOOR_LABEL in _actor_label(a) and "AccessDoor" in _actor_class_blob(a)]
if len(door_actors) != 1:
    _log("WARNING: BP_AdminAccessDoor count=%d (expected 1); continuing without moving it" % len(door_actors))
door_before = None
if door_actors:
    door_before = (
        door_actors[0].get_actor_location(),
        door_actors[0].get_actor_rotation(),
        door_actors[0].get_actor_scale3d(),
    )
    _log("door snapshot loc=%s" % door_before[0])

extra_terminals = [
    a for a in all_actors
    if _actor_label(a).startswith("Admin_Terminal_") and _actor_label(a) != INSTANCE_LABEL
]
if extra_terminals:
    _fail("Other Admin terminals already exist; Phase 1 places Reception only: %s" % [
        _actor_label(a) for a in extra_terminals
    ])

enum_asset = _load_manual_enum()
bp = unreal.EditorAssetLibrary.load_asset(BP_PATH)
if not bp:
    _fail("Failed to load %s" % BP_PATH)
current_parent = _parent_name(bp)
_log("current parent=%s" % current_parent)
_log("members at resume=%s" % ", ".join(_member_names(bp)))
if OLD_PARENT.split(".")[-1] in current_parent or "ProjectOrganoidTerminal" in current_parent:
    parent = unreal.load_class(None, NEW_PARENT)
    if not parent:
        _fail("Missing parent class %s" % NEW_PARENT)
    unreal.BlueprintEditorLibrary.reparent_blueprint(bp, parent)
    _log("reparented BP_AdminTerminal -> ProjectOrganoidInteractable")
    _compile_blueprint(bp, "after reparent")
elif "ProjectOrganoidInteractable" in current_parent:
    _log("parent already ProjectOrganoidInteractable; not reparenting")
else:
    _fail("Unexpected parent %s; refusing to continue" % current_parent)

named, scs_inventory, scs_path = _scs_named(bp)
mesh_ok, light_ok, both_local = _scs_lookup_self_test(named, scs_inventory)
if not (mesh_ok and light_ok and both_local):
    _fail(
        "SCS lookup self-test failed (path=%s). "
        "TerminalMesh/StatusLight exist as local SCS but Python still cannot identify them as local non-native non-inherited. ZERO component writes."
        % scs_path
    )
mesh_node = _reuse_prerequisite_scs(named, "TerminalMesh")
light_node = _reuse_prerequisite_scs(named, "StatusLight")
mesh_template = _template_from_node(mesh_node)
if mesh_template:
    _configure_mesh(mesh_template)
light_template = _template_from_node(light_node)
if light_template:
    _configure_light(light_template)

_reuse_prerequisite_member(bp, "TerminalID")
unreal.BlueprintEditorLibrary.set_blueprint_variable_instance_editable(bp, "TerminalID", True)
unreal.BlueprintEditorLibrary.set_blueprint_variable_category(bp, "TerminalID", unreal.Text("Admin|Terminal"))
_reuse_prerequisite_member(bp, "TerminalType")
unreal.BlueprintEditorLibrary.set_blueprint_variable_instance_editable(bp, "TerminalType", True)
unreal.BlueprintEditorLibrary.set_blueprint_variable_category(bp, "TerminalType", unreal.Text("Admin|Terminal"))
_log("TerminalType reused; Python enum pin construction not used")

_add_basic_variable(bp, "Title", "text", True, "Admin|Terminal")
_add_basic_variable(bp, "bInitiallyPowered", "bool", True, "Admin|Terminal")
_add_basic_variable(bp, "bIsPowered", "bool", False, "Admin|Terminal")
_add_basic_variable(bp, "bOneShot", "bool", True, "Admin|Terminal")
_add_basic_variable(bp, "bHasActivated", "bool", False, "Admin|Terminal")
_log("members=%s" % ", ".join(_member_names(bp)))

cdo = unreal.get_default_object(unreal.BlueprintEditorLibrary.generated_class(bp))
_set_any(cdo, ("interaction_prompt", "InteractionPrompt"), unreal.Text(PROMPT))
_set_any(cdo, ("interaction_range", "InteractionRange"), INTERACTION_RANGE)
_set_any(cdo, ("b_initially_powered", "bInitiallyPowered"), True)
_set_any(cdo, ("b_one_shot", "bOneShot"), False)
_set_any(cdo, ("terminal_id", "TerminalID"), "Terminal_Unnamed")
sphere = None
try:
    sphere = cdo.get_editor_property("interaction_sphere")
except Exception:
    sphere = cdo.get_component_by_class(unreal.SphereComponent)
if sphere:
    _safe_call(sphere, "set_sphere_radius", INTERACTION_RANGE)
    _log("InteractionSphere radius set to %.1f" % INTERACTION_RANGE)

event_editor = unreal.BlueprintGraphEditor.get_graph_editor_by_name(bp, "EventGraph")
if not event_editor:
    _fail("EventGraph editor unavailable")
_wire_event_graph(bp, event_editor)
_compile_blueprint(bp, "after EventGraph")
_wire_interact_override(bp)
compiled, status = _compile_blueprint(bp, "after Interact override")
saved_bp = unreal.EditorAssetLibrary.save_asset(BP_PATH, only_if_is_dirty=False)
_log("saved blueprint ok=%s" % saved_bp)
if not saved_bp:
    _fail("Failed to save BP_AdminTerminal")

bp_class = unreal.EditorAssetLibrary.load_blueprint_class(BP_PATH)
if not bp_class:
    _fail("Generated class missing after compile/save")

desks = [a for a in actor_sub.get_all_level_actors() if _actor_label(a) == DESK_LABEL]
if len(desks) != 1:
    _fail("Admin_S1_Reception_Desk count=%d (need 1)" % len(desks))
place_loc = _desk_terminal_location(desks[0])

existing = [a for a in actor_sub.get_all_level_actors() if _actor_label(a) == INSTANCE_LABEL]
if len(existing) > 1:
    _fail("Multiple %s actors already exist" % INSTANCE_LABEL)
if existing:
    actor = existing[0]
    actor.set_actor_location(place_loc, False, False)
    _log("reconfigured existing %s" % INSTANCE_LABEL)
else:
    actor = actor_sub.spawn_actor_from_class(bp_class, place_loc, unreal.Rotator(0.0, 0.0, 0.0))
    if not actor:
        _fail("Failed to spawn Reception terminal")
    actor.set_actor_label(INSTANCE_LABEL)
    _log("placed %s" % INSTANCE_LABEL)

owner = _move_to_admin(actor, world)
if owner != REQUIRED_PACKAGE:
    _safe_call(actor, "destroy_actor")
    _fail("Reception terminal owner package is %s, expected %s. Actor destroyed." % (owner, REQUIRED_PACKAGE))

_set_any(actor, ("terminal_id", "TerminalID"), "Terminal_AdminReception")
_set_any(actor, ("title", "Title"), unreal.Text("Reception Terminal"))
_set_any(actor, ("b_initially_powered", "bInitiallyPowered"), True)
_set_any(actor, ("b_one_shot", "bOneShot"), False)
_set_any(actor, ("terminal_type", "TerminalType"), 0)
_set_any(actor, ("interaction_prompt", "InteractionPrompt"), unreal.Text(PROMPT))
_set_any(actor, ("interaction_range", "InteractionRange"), INTERACTION_RANGE)

if door_actors and door_before:
    loc_now = door_actors[0].get_actor_location()
    if abs(loc_now.x - door_before[0].x) > 0.05 or abs(loc_now.y - door_before[0].y) > 0.05:
        _fail("BP_AdminAccessDoor moved; Phase 1 must not touch Section 17")

final_loc = actor.get_actor_location()
_log("Reception terminal loc=(%.3f, %.3f, %.3f) owner=%s" % (final_loc.x, final_loc.y, final_loc.z, owner))
_log("parent now=%s" % _parent_name(bp))
_log("compile status=%s" % status)
_log("dirty packages:")
for row in _dirty_packages():
    _log("  " + row)
_log("map NOT saved. Do not PIE from this script. Do not Save All.")
_log("=== SECTION 18 PHASE 1 SCRIPT COMPLETE ===")
