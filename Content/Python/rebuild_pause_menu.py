# ProjectOrganoid — inspect and rebuild WBP_PauseMenu so it cannot block the title screen.
#
# Headless (forward slashes only):
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript
#       -script="<abs>/rebuild_pause_menu.py"

import unreal


MENU_DIR = "/Game/UI/Menus"
PAUSE_PATH = f"{MENU_DIR}/WBP_PauseMenu"
MAIN_PATH = f"{MENU_DIR}/WBP_MainMenu"
PAUSE_PARENT = "/Script/ProjectOrganoid.ProjectOrganoidPauseWidget"


def report(message):
    unreal.log_warning(f"[PAUSE MENU] {message}")


def describe(value):
    if value is None:
        return "none"
    for attr in ("get_name", "get_path_name"):
        getter = getattr(value, attr, None)
        if callable(getter):
            try:
                return getter()
            except Exception:
                continue
    return str(value)


def widget_tree_of(asset):
    for candidate in ("widget_tree", "WidgetTree"):
        try:
            tree = asset.get_editor_property(candidate)
            if tree:
                return tree
        except Exception:
            pass
        tree = getattr(asset, candidate, None)
        if tree and not callable(tree):
            return tree
    return None


def inspect_widget(path):
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        report(f"{path}: missing")
        return None

    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        report(f"{path}: failed to load")
        return None

    parent = None
    generated = None
    try:
        parent = asset.get_editor_property("parent_class")
    except Exception:
        parent = getattr(asset, "parent_class", None)
    try:
        generated = asset.get_editor_property("generated_class")
    except Exception:
        generated = getattr(asset, "generated_class", None)

    tree = widget_tree_of(asset)
    root = None
    if tree:
        root = getattr(tree, "root_widget", None)
        if root is None:
            try:
                root = tree.get_editor_property("root_widget")
            except Exception:
                pass
    else:
        report(f"{path}: widget_tree unreadable")

    report(
        f"{path}: class={asset.get_class().get_name()} "
        f"parent={describe(parent)} generated={describe(generated)} root={describe(root)}"
    )
    return asset


def set_button_label(button, label):
    try:
        text = unreal.TextBlock()
        text.set_text(unreal.Text(label))
        if hasattr(button, "set_content"):
            button.set_content(text)
        elif hasattr(button, "add_child"):
            button.add_child(text)
    except Exception as exc:
        report(f"could not label {label}: {exc}")


def add_slider(widget_tree, parent_box, slider_name, label):
    row = widget_tree.construct_widget(unreal.HorizontalBox, f"{slider_name}Row")
    label_widget = widget_tree.construct_widget(unreal.TextBlock, f"{slider_name}Label")
    label_widget.set_text(unreal.Text(label))
    slider = widget_tree.construct_widget(unreal.Slider, slider_name)
    slider.set_value(1.0)
    parent_box.add_child_to_vertical_box(row)
    row.add_child_to_horizontal_box(label_widget)
    row.add_child_to_horizontal_box(slider)
    return slider


def build_layout(widget_bp):
    widget_tree = widget_tree_of(widget_bp)
    if not widget_tree:
        raise RuntimeError("new WBP_PauseMenu has no widget tree")
    root = widget_tree.construct_widget(unreal.CanvasPanel, "RootCanvas")
    widget_tree.root_widget = root

    dimmer = widget_tree.construct_widget(unreal.Image, "Dimmer")
    dimmer_slot = root.add_child_to_canvas(dimmer)
    dimmer_slot.set_anchors(unreal.Anchors(0.0, 0.0, 1.0, 1.0))
    dimmer_slot.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 0.0))
    try:
        dimmer.set_color_and_opacity(unreal.LinearColor(0.0, 0.0, 0.0, 0.65))
    except Exception:
        pass

    menu_box = widget_tree.construct_widget(unreal.VerticalBox, "PauseBox")
    slot = root.add_child_to_canvas(menu_box)
    slot.set_anchors(unreal.Anchors(0.5, 0.5, 0.5, 0.5))
    slot.set_alignment(unreal.Vector2D(0.5, 0.5))
    slot.set_auto_size(True)

    title = widget_tree.construct_widget(unreal.TextBlock, "TitleText")
    title.set_text(unreal.Text("PAUSED"))
    menu_box.add_child_to_vertical_box(title)

    for name, label in (
        ("ResumeButton", "RESUME"),
        ("ReturnToMainMenuButton", "MAIN MENU"),
        ("QuitButton", "QUIT"),
    ):
        button = widget_tree.construct_widget(unreal.Button, name)
        set_button_label(button, label)
        menu_box.add_child_to_vertical_box(button)

    header = widget_tree.construct_widget(unreal.TextBlock, "SettingsHeader")
    header.set_text(unreal.Text("SETTINGS"))
    menu_box.add_child_to_vertical_box(header)

    add_slider(widget_tree, menu_box, "MasterVolumeSlider", "Master")
    add_slider(widget_tree, menu_box, "SFXVolumeSlider", "SFX")
    add_slider(widget_tree, menu_box, "MusicVolumeSlider", "Music")
    add_slider(widget_tree, menu_box, "ResolutionScaleSlider", "Resolution Scale")

    graphics_row = widget_tree.construct_widget(unreal.HorizontalBox, "GraphicsRow")
    graphics_label = widget_tree.construct_widget(unreal.TextBlock, "GraphicsLabel")
    graphics_label.set_text(unreal.Text("Graphics"))
    graphics_combo = widget_tree.construct_widget(unreal.ComboBoxString, "GraphicsQualityCombo")
    menu_box.add_child_to_vertical_box(graphics_row)
    graphics_row.add_child_to_horizontal_box(graphics_label)
    graphics_row.add_child_to_horizontal_box(graphics_combo)

    window_row = widget_tree.construct_widget(unreal.HorizontalBox, "WindowModeRow")
    window_label = widget_tree.construct_widget(unreal.TextBlock, "WindowModeLabel")
    window_label.set_text(unreal.Text("Window Mode"))
    window_combo = widget_tree.construct_widget(unreal.ComboBoxString, "WindowModeCombo")
    menu_box.add_child_to_vertical_box(window_row)
    window_row.add_child_to_horizontal_box(window_label)
    window_row.add_child_to_horizontal_box(window_combo)

    report("rebuilt pause layout (Resume / Main Menu / Quit + settings)")


def rebuild_pause_menu():
    report("=== Inspect and rebuild WBP_PauseMenu ===")
    inspect_widget(PAUSE_PATH)
    inspect_widget(MAIN_PATH)

    if unreal.EditorAssetLibrary.does_asset_exist(PAUSE_PATH):
        if not unreal.EditorAssetLibrary.delete_asset(PAUSE_PATH):
            report("delete failed; will overwrite in place")
        else:
            report("deleted stale WBP_PauseMenu")

    parent = unreal.load_class(None, PAUSE_PARENT)
    if not parent:
        report(f"missing parent {PAUSE_PARENT}")
        return

    if not unreal.EditorAssetLibrary.does_directory_exist(MENU_DIR):
        unreal.EditorAssetLibrary.make_directory(MENU_DIR)

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    widget_bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "WBP_PauseMenu", MENU_DIR, unreal.WidgetBlueprint, factory
    )
    if not widget_bp:
        report("failed to create WBP_PauseMenu")
        return

    build_layout(widget_bp)
    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(widget_bp)
    except Exception as exc:
        report(f"compile warning: {exc}")
    unreal.EditorAssetLibrary.save_asset(PAUSE_PATH)
    inspect_widget(PAUSE_PATH)

    if unreal.EditorAssetLibrary.does_asset_exist(MAIN_PATH):
        main = unreal.EditorAssetLibrary.load_asset(MAIN_PATH)
        if main:
            for candidate in ("gameplay_level_name", "GameplayLevelName"):
                try:
                    main.set_editor_property(candidate, "/Game/Maps/Lvl_Epitope")
                    unreal.EditorAssetLibrary.save_asset(MAIN_PATH)
                    report("WBP_MainMenu GameplayLevelName -> /Game/Maps/Lvl_Epitope")
                    break
                except Exception:
                    continue

    report("=== Pause menu rebuild complete ===")


rebuild_pause_menu()
