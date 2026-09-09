# ProjectOrganoid — Section 18 READ-ONLY SCS lookup self-test.
# Zero mutations. No compile. No save. No PIE. No add_new_subobject.
#
# Run:
#   File -> Execute Python Script...
#   Content/Python/diagnose_admin_section_18_scs_lookup.py

import unreal

BP_PATH = "/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminTerminal"


def _log(msg):
    unreal.log("[S18-SCS] " + msg)


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
        if text and text not in ("None", "none", "NAME_None"):
            return text
    except Exception:
        pass
    return fallback


def _component_var_name(value):
    text = _safe_str(value, "")
    return text.split(".")[-1] if text else ""


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
    except Exception:
        pass
    data = unreal.SubobjectData()
    try:
        lib.get_data(handle, data)
        return data
    except Exception:
        return None


def _subobject_template(lib, data, bp):
    obj = _safe_call(lib, "get_object_for_blueprint", data, bp)
    if obj is None:
        obj = _safe_call(lib, "get_associated_object", data)
    return obj


def inventory_via_subobjects(bp):
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
        if _safe_call(lib, "is_actor", data) or not _safe_call(lib, "is_component", data):
            continue
        name = _component_var_name(_safe_call(lib, "get_variable_name", data))
        native = bool(_safe_call(lib, "is_native_component", data))
        inherited = bool(_safe_call(lib, "is_inherited_component", data))
        local = bool(name) and (not native) and (not inherited)
        template = _subobject_template(lib, data, bp)
        inventory.append({
            "name": name,
            "native": native,
            "inherited": inherited,
            "local": local,
        })
        if local and template is not None:
            named[name] = template
    return named, inventory, "subobject_data"


def inventory_via_properties(bp):
    named = {}
    inventory = []
    scs = _read_prop(bp, ("simple_construction_script", "SimpleConstructionScript"))
    if not scs:
        return named, inventory, "scs_property_unavailable"
    nodes = _read_prop(scs, ("all_nodes", "AllNodes")) or _read_prop(scs, ("root_nodes", "RootNodes"))
    for node in nodes or []:
        name = _component_var_name(_read_prop(node, ("internal_variable_name", "InternalVariableName")))
        template = _read_prop(node, ("component_template", "ComponentTemplate"))
        local = bool(name)
        inventory.append({
            "name": name,
            "native": False,
            "inherited": False,
            "local": local,
        })
        if local:
            named[name] = template if template is not None else node
    return named, inventory, "scs_uproperty"


_log("=== SECTION 18 SCS LOOKUP SELF-TEST (READ ONLY) ===")
if not unreal.EditorAssetLibrary.does_asset_exist(BP_PATH):
    _log("MISSING %s" % BP_PATH)
    raise RuntimeError("BP_AdminTerminal missing")

bp = unreal.EditorAssetLibrary.load_asset(BP_PATH)
named, inventory, path = inventory_via_subobjects(bp)
if "TerminalMesh" not in named or "StatusLight" not in named:
    named, inventory, path = inventory_via_properties(bp)
_log("lookup path=%s" % path)
for row in inventory:
    _log(
        "name=%s native=%s inherited=%s local=%s"
        % (row.get("name") or "(unnamed)", row.get("native"), row.get("inherited"), row.get("local"))
    )

mesh = "TerminalMesh" in named
light = "StatusLight" in named
rows = {row.get("name"): row for row in inventory}
mesh_row = rows.get("TerminalMesh") or {}
light_row = rows.get("StatusLight") or {}
both_local = bool(
    mesh and light
    and mesh_row.get("local") and light_row.get("local")
    and (not mesh_row.get("native")) and (not light_row.get("native"))
    and (not mesh_row.get("inherited")) and (not light_row.get("inherited"))
)
_log("TerminalMesh found = %s" % ("true" if mesh else "false"))
_log("StatusLight found = %s" % ("true" if light else "false"))
if both_local:
    _log("both local, non-native, non-inherited")
else:
    _log("NOT both local, non-native, non-inherited")
_log("NO mutations. NO compile. NO save.")
_log("=== SECTION 18 SCS LOOKUP SELF-TEST COMPLETE ===")
if not both_local:
    raise RuntimeError("SCS lookup self-test failed")
