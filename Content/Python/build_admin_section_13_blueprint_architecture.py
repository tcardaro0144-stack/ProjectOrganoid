# ProjectOrganoid — Section 13 Blueprint Architecture (Content stubs only).
# Does not spawn map actors. Does not save any umap.
# Does not touch S1–S12 geometry. Does not create Admin_S13_* room actors.
# Does not modify BP_AdminAccessDoor / BP_AdminSecurityGate.
# Does not create BP_AdminDoor, BPI_AdminInteractable, or E_AdminAccessLevel.
# Does not import or run build_epitope_rooms.py or Section 12 recovery.
# Does not assign, recook, or edit audio uassets.
#
# Run manually:
#   File -> Execute Python Script...
#   Content/Python/build_admin_section_13_blueprint_architecture.py

import unreal

ADMIN_ROOT = "/Game/ProjectOrganoid/Environment/Admin"
BLUEPRINTS_DIR = ADMIN_ROOT + "/Blueprints"
REQUIRED_EXISTING = (
    BLUEPRINTS_DIR + "/BP_AdminAccessDoor",
    BLUEPRINTS_DIR + "/BP_AdminSecurityGate",
)
FORBIDDEN_NAMES = (
    "BP_AdminDoor",
    "BPI_AdminInteractable",
    "E_AdminAccessLevel",
    "E_AdminFacilityState",
    "BPI_AdminStateListener",
)
FOLDERS = (
    BLUEPRINTS_DIR,
    ADMIN_ROOT + "/Materials",
    ADMIN_ROOT + "/Meshes",
    ADMIN_ROOT + "/Audio",
    ADMIN_ROOT + "/FX",
    ADMIN_ROOT + "/UI",
)

# name, parent class path, created vs skipped if present
NEW_BLUEPRINTS = (
    ("BP_AdminSectorController", "/Script/Engine.Actor"),
    ("BP_AdminRoomTrigger", "/Script/Engine.Actor"),
    ("BP_AdminTerminal", "/Script/ProjectOrganoid.ProjectOrganoidTerminal"),
    ("BP_AdminFacilityHologram", "/Script/Engine.Actor"),
    ("BP_AdminLightController", "/Script/Engine.Actor"),
    ("BP_AdminAudioZone", "/Script/ProjectOrganoid.ProjectOrganoidAmbienceZone"),
    ("BP_AdminAccessPanel", "/Script/ProjectOrganoid.ProjectOrganoidInteractable"),
    ("BP_AdminSectorDisplay", "/Script/Engine.Actor"),
)


def _log(msg):
    unreal.log("[S13] " + msg)


def _fail(msg):
    _log("=== SECTION 13 ABORTED ===")
    _log(msg)
    raise RuntimeError(msg)


def _asset_path(name):
    return BLUEPRINTS_DIR + "/" + name


def _ensure_folder(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        if not unreal.EditorAssetLibrary.make_directory(path):
            _fail("Failed to create folder: %s" % path)
        _log("created folder %s" % path)
        return "created"
    _log("folder exists %s" % path)
    return "existed"


def _load_parent(class_path):
    parent = unreal.load_class(None, class_path)
    if parent:
        return parent
    _fail("Missing parent class: %s" % class_path)


def _compile_blueprint(bp):
    compiled = False
    errors = []
    for lib_name, method_name in (
        ("KismetEditorLibrary", "compile_blueprint"),
        ("BlueprintEditorLibrary", "compile_blueprint"),
    ):
        lib = getattr(unreal, lib_name, None)
        method = getattr(lib, method_name, None) if lib else None
        if callable(method):
            try:
                method(bp)
                compiled = True
                break
            except Exception as ex:
                errors.append("%s.%s: %s" % (lib_name, method_name, ex))
    status = "unknown"
    try:
        status = str(bp.get_editor_property("status"))
    except Exception:
        pass
    return compiled, status, errors


def _create_or_reuse_blueprint(name, parent_path):
    path = _asset_path(name)
    action = "reused"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        _log("blueprint already exists (leave as-is, compile only): %s" % path)
        bp = unreal.EditorAssetLibrary.load_asset(path)
        if not bp:
            _fail("Failed to load existing blueprint: %s" % path)
        return bp, action

    parent = _load_parent(parent_path)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, BLUEPRINTS_DIR, unreal.Blueprint, factory
    )
    if not bp:
        _fail("Failed to create blueprint: %s parent=%s" % (name, parent_path))
    action = "created"
    _log("created %s parent=%s" % (path, parent_path))
    return bp, action


def _status_ok(status):
    text = status.lower()
    return (
        "error" not in text
        and "unknown" not in text
    ) or "uptodate" in text.replace("_", "").replace(" ", "")


_log("=== SECTION 13 BLUEPRINT ARCHITECTURE BEGIN ===")
_log("ZERO map actor spawns. ZERO umap saves. ZERO S1-S12 geometry edits.")
_log("ZERO Admin_S13_* room actors. ZERO audio uasset edits.")

for forbidden in FORBIDDEN_NAMES:
    if unreal.EditorAssetLibrary.does_asset_exist(_asset_path(forbidden)):
        _fail("Forbidden asset already exists; refusing to continue: %s" % _asset_path(forbidden))

for required in REQUIRED_EXISTING:
    if not unreal.EditorAssetLibrary.does_asset_exist(required):
        _fail("Required existing asset missing (must preserve, not recreate): %s" % required)
    _log("preserved existing %s" % required)

folder_results = []
for folder in FOLDERS:
    folder_results.append((folder, _ensure_folder(folder)))

created = []
reused = []
compile_rows = []
compile_failures = []

for name, parent_path in NEW_BLUEPRINTS:
    bp, action = _create_or_reuse_blueprint(name, parent_path)
    if action == "created":
        created.append(name)
    else:
        reused.append(name)

    compiled, status, errors = _compile_blueprint(bp)
    saved = unreal.EditorAssetLibrary.save_asset(_asset_path(name), only_if_is_dirty=False)
    row = "%s parent=%s compile_called=%s status=%s saved=%s" % (
        name, parent_path, compiled, status, saved
    )
    if errors:
        row += " notes=%s" % "; ".join(errors)
    compile_rows.append(row)
    _log(row)

    status_l = status.lower().replace(" ", "").replace("_", "")
    if "error" in status_l or (not compiled and "uptodate" not in status_l):
        compile_failures.append(row)

if compile_failures:
    _fail("Blueprint compile/status failure(s):\n  " + "\n  ".join(compile_failures))

# Map safety: never save the current level from this script.
_log("map save skipped (Content assets only)")
_log("created=%s" % (", ".join(created) if created else "(none)"))
_log("reused=%s" % (", ".join(reused) if reused else "(none)"))
_log("folders=%s" % ", ".join("%s:%s" % (p, s) for p, s in folder_results))
_log("=== SECTION 13 BLUEPRINT ARCHITECTURE COMPLETE ===")
_log("Section 14 was not started. No map actors were placed.")
