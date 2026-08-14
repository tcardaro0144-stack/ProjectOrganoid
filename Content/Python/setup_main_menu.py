# ProjectOrganoid — Main Menu / Pause Menu / Title Map setup (UE Editor Python)
#
# Run in Unreal Editor (project must be compiled so C++ parents exist):
#   Tools → Execute Python Script… → Content/Python/setup_main_menu.py
#   or Output Log: py "Content/Python/setup_main_menu.py"
#
# Creates:
#   /Game/UI/Menus/WBP_MainMenu
#   /Game/UI/Menus/WBP_PauseMenu
#   /Game/Maps/Lvl_MainMenu  (GameMode override → AProjectOrganoidMainMenuGameMode)
# Also writes GameDefaultMap / EditorStartupMap in Config/DefaultEngine.ini

import unreal
import os


MENU_DIR = "/Game/UI/Menus"
MAPS_DIR = "/Game/Maps"
MAIN_MENU_WBP = f"{MENU_DIR}/WBP_MainMenu"
PAUSE_MENU_WBP = f"{MENU_DIR}/WBP_PauseMenu"
TITLE_MAP = f"{MAPS_DIR}/Lvl_MainMenu"

MAIN_MENU_PARENT = "/Script/ProjectOrganoid.ProjectOrganoidMainMenuWidget"
PAUSE_MENU_PARENT = "/Script/ProjectOrganoid.ProjectOrganoidPauseWidget"
MAIN_MENU_GM = "/Script/ProjectOrganoid.ProjectOrganoidMainMenuGameMode"


def ensure_directory(content_path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(content_path):
        unreal.EditorAssetLibrary.make_directory(content_path)


def load_parent_class(class_path: str):
    parent = unreal.load_class(None, class_path)
    if not parent:
        unreal.log_error(f"Could not load parent class: {class_path} (is the C++ module compiled?)")
    return parent


def create_or_load_widget_bp(asset_name: str, package_path: str, parent_class_path: str):
    asset_path = f"{package_path}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log(f"Widget already exists: {asset_path}")
        return unreal.EditorAssetLibrary.load_asset(asset_path)

    parent = load_parent_class(parent_class_path)
    if not parent:
        return None

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    widget_bp = asset_tools.create_asset(asset_name, package_path, unreal.WidgetBlueprint, factory)
    if not widget_bp:
        unreal.log_error(f"Failed to create widget blueprint: {asset_path}")
    return widget_bp


def _set_button_label(button, label: str) -> None:
    try:
        text = unreal.TextBlock()
        text.set_text(unreal.Text(label))
        # ContentWidget API
        if hasattr(button, "set_content"):
            button.set_content(text)
        elif hasattr(button, "add_child"):
            button.add_child(text)
    except Exception as exc:
        unreal.log_warning(f"Could not set button label '{label}': {exc}")


def _add_labeled_slider(widget_tree, parent_box, slider_name: str, label: str):
    row = widget_tree.construct_widget(unreal.HorizontalBox, f"{slider_name}Row")
    label_widget = widget_tree.construct_widget(unreal.TextBlock, f"{slider_name}Label")
    label_widget.set_text(unreal.Text(label))
    slider = widget_tree.construct_widget(unreal.Slider, slider_name)
    slider.set_value(1.0)
    try:
        parent_box.add_child_to_vertical_box(row)
        row.add_child_to_horizontal_box(label_widget)
        row.add_child_to_horizontal_box(slider)
    except Exception:
        # Fallback if box APIs differ
        parent_box.add_child(row)
    return slider


def build_main_menu_layout(widget_bp) -> bool:
    """Build a centered vertical menu with BindWidget-compatible names."""
    if not widget_bp:
        return False

    try:
        widget_tree = widget_bp.widget_tree
    except Exception as exc:
        unreal.log_warning(f"WBP_MainMenu has no widget_tree yet: {exc}")
        return False

    try:
        root = widget_tree.construct_widget(unreal.CanvasPanel, "RootCanvas")
        widget_tree.root_widget = root

        menu_box = widget_tree.construct_widget(unreal.VerticalBox, "MenuBox")
        slot = root.add_child_to_canvas(menu_box)
        slot.set_anchors(unreal.Anchors(0.5, 0.5, 0.5, 0.5))
        slot.set_alignment(unreal.Vector2D(0.5, 0.5))
        slot.set_auto_size(True)
        slot.set_position(unreal.Vector2D(0.0, 0.0))

        title = widget_tree.construct_widget(unreal.TextBlock, "TitleText")
        title.set_text(unreal.Text("PROJECT ORGANOID"))
        try:
            title.set_editor_property("font", unreal.SlateFontInfo())
        except Exception:
            pass
        menu_box.add_child_to_vertical_box(title)

        subtitle = widget_tree.construct_widget(unreal.TextBlock, "SubtitleText")
        subtitle.set_text(unreal.Text("Epitope Subterranean Complex"))
        menu_box.add_child_to_vertical_box(subtitle)

        buttons = [
            ("NewGameButton", "NEW GAME"),
            ("LoadSlot0Button", "LOAD SLOT 0"),
            ("LoadSlot1Button", "LOAD SLOT 1"),
            ("LoadSlot2Button", "LOAD SLOT 2"),
            ("QuitButton", "QUIT"),
        ]
        for name, label in buttons:
            button = widget_tree.construct_widget(unreal.Button, name)
            _set_button_label(button, label)
            menu_box.add_child_to_vertical_box(button)

        settings_header = widget_tree.construct_widget(unreal.TextBlock, "SettingsHeader")
        settings_header.set_text(unreal.Text("SETTINGS"))
        menu_box.add_child_to_vertical_box(settings_header)

        _add_labeled_slider(widget_tree, menu_box, "MasterVolumeSlider", "Master")
        _add_labeled_slider(widget_tree, menu_box, "SFXVolumeSlider", "SFX")
        _add_labeled_slider(widget_tree, menu_box, "MusicVolumeSlider", "Music")

        graphics_row = widget_tree.construct_widget(unreal.HorizontalBox, "GraphicsRow")
        graphics_label = widget_tree.construct_widget(unreal.TextBlock, "GraphicsLabel")
        graphics_label.set_text(unreal.Text("Graphics"))
        graphics_combo = widget_tree.construct_widget(unreal.ComboBoxString, "GraphicsQualityCombo")
        menu_box.add_child_to_vertical_box(graphics_row)
        graphics_row.add_child_to_horizontal_box(graphics_label)
        graphics_row.add_child_to_horizontal_box(graphics_combo)

        unreal.log("Built WBP_MainMenu layout")
        return True
    except Exception as exc:
        unreal.log_warning(f"Could not fully build WBP_MainMenu layout (open in UMG to polish): {exc}")
        return False


def build_pause_menu_layout(widget_bp) -> bool:
    if not widget_bp:
        return False

    try:
        widget_tree = widget_bp.widget_tree
    except Exception as exc:
        unreal.log_warning(f"WBP_PauseMenu has no widget_tree yet: {exc}")
        return False

    try:
        root = widget_tree.construct_widget(unreal.CanvasPanel, "RootCanvas")
        widget_tree.root_widget = root

        menu_box = widget_tree.construct_widget(unreal.VerticalBox, "PauseBox")
        slot = root.add_child_to_canvas(menu_box)
        slot.set_anchors(unreal.Anchors(0.5, 0.5, 0.5, 0.5))
        slot.set_alignment(unreal.Vector2D(0.5, 0.5))
        slot.set_auto_size(True)

        title = widget_tree.construct_widget(unreal.TextBlock, "TitleText")
        title.set_text(unreal.Text("PAUSED"))
        menu_box.add_child_to_vertical_box(title)

        for name, label in [
            ("ResumeButton", "RESUME"),
            ("ReturnToMainMenuButton", "MAIN MENU"),
            ("QuitButton", "QUIT"),
        ]:
            button = widget_tree.construct_widget(unreal.Button, name)
            _set_button_label(button, label)
            menu_box.add_child_to_vertical_box(button)

        settings_header = widget_tree.construct_widget(unreal.TextBlock, "SettingsHeader")
        settings_header.set_text(unreal.Text("SETTINGS"))
        menu_box.add_child_to_vertical_box(settings_header)

        _add_labeled_slider(widget_tree, menu_box, "MasterVolumeSlider", "Master")
        _add_labeled_slider(widget_tree, menu_box, "SFXVolumeSlider", "SFX")
        _add_labeled_slider(widget_tree, menu_box, "MusicVolumeSlider", "Music")
        _add_labeled_slider(widget_tree, menu_box, "ResolutionScaleSlider", "Resolution Scale")

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

        unreal.log("Built WBP_PauseMenu layout")
        return True
    except Exception as exc:
        unreal.log_warning(f"Could not fully build WBP_PauseMenu layout (open in UMG to polish): {exc}")
        return False


def compile_and_save_widget(widget_bp, asset_path: str) -> None:
    if not widget_bp:
        return
    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(widget_bp)
    except Exception as exc:
        unreal.log_warning(f"Compile warning for {asset_path}: {exc}")
    unreal.EditorAssetLibrary.save_asset(asset_path)


def create_title_map() -> bool:
    ensure_directory(MAPS_DIR)

    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_subsystem:
        unreal.log_error("LevelEditorSubsystem unavailable")
        return False

    # Create or open the title map
    if unreal.EditorAssetLibrary.does_asset_exist(TITLE_MAP):
        unreal.log(f"Title map exists, opening: {TITLE_MAP}")
        level_subsystem.load_level(TITLE_MAP)
    else:
        # new_level path creates and opens an empty persistent level
        try:
            level_subsystem.new_level(TITLE_MAP)
        except Exception:
            # Fallback for older EditorLevelLibrary API
            unreal.EditorLevelLibrary.new_level(TITLE_MAP)

    world = None
    try:
        world = unreal.EditorLevelLibrary.get_editor_world()
    except Exception:
        pass
    if not world:
        try:
            world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        except Exception:
            unreal.log_error("Could not get editor world for title map")
            return False

    gm_class = load_parent_class(MAIN_MENU_GM)
    if not gm_class:
        return False

    world_settings = world.get_world_settings()
    world_settings.set_editor_property("default_game_mode", gm_class)

    # Dark title-screen mood: nudge sky/exposure via a basic directional light if empty
    try:
        editor_actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
        existing = editor_actor_subsystem.get_all_level_actors()
        has_light = any(isinstance(a, unreal.DirectionalLight) or a.get_class().get_name().startswith("DirectionalLight") for a in existing)
        if not has_light:
            light = editor_actor_subsystem.spawn_actor_from_class(
                unreal.DirectionalLight, unreal.Vector(0.0, 0.0, 300.0)
            )
            if light:
                light.set_actor_label("TitleKeyLight")
                light.set_actor_rotation(unreal.Rotator(-40.0, 30.0, 0.0))
    except Exception as exc:
        unreal.log_warning(f"Optional title light skipped: {exc}")

    try:
        level_subsystem.save_current_level()
    except Exception:
        unreal.EditorLevelLibrary.save_current_level()

    unreal.log(f"Title map ready: {TITLE_MAP} → GameMode {MAIN_MENU_GM}")
    return True


def update_default_engine_ini() -> None:
    """Point GameDefaultMap / EditorStartupMap at Lvl_MainMenu."""
    project_dir = unreal.Paths.project_dir()
    ini_path = os.path.join(project_dir, "Config", "DefaultEngine.ini")
    if not os.path.isfile(ini_path):
        unreal.log_warning(f"DefaultEngine.ini not found: {ini_path}")
        return

    with open(ini_path, "r", encoding="utf-8") as handle:
        lines = handle.readlines()

    map_value = "/Game/Maps/Lvl_MainMenu.Lvl_MainMenu"
    replacements = {
        "GameDefaultMap=": f"GameDefaultMap={map_value}\n",
        "EditorStartupMap=": f"EditorStartupMap={map_value}\n",
    }

    out_lines = []
    seen = {key: False for key in replacements}
    for line in lines:
        replaced = False
        for prefix, new_line in replacements.items():
            if line.startswith(prefix):
                out_lines.append(new_line)
                seen[prefix] = True
                replaced = True
                break
        if not replaced:
            out_lines.append(line)

    # Ensure GameModeMapPrefixes backup for Lvl_MainMenu
    prefix_line = '+GameModeMapPrefixes=(Prefix="Lvl_MainMenu",GameMode="/Script/ProjectOrganoid.ProjectOrganoidMainMenuGameMode")\n'
    has_prefix = any("Lvl_MainMenu" in line and "GameModeMapPrefixes" in line for line in out_lines)
    if not has_prefix:
        inserted = False
        for i, line in enumerate(out_lines):
            if line.startswith("[/Script/EngineSettings.GameMapsSettings]"):
                # Insert after section header block keys
                insert_at = i + 1
                while insert_at < len(out_lines) and out_lines[insert_at].strip() and not out_lines[insert_at].startswith("["):
                    insert_at += 1
                out_lines.insert(insert_at, prefix_line)
                inserted = True
                break
        if not inserted:
            out_lines.insert(0, "[/Script/EngineSettings.GameMapsSettings]\n")
            out_lines.insert(1, prefix_line)

    with open(ini_path, "w", encoding="utf-8", newline="\n") as handle:
        handle.writelines(out_lines)

    unreal.log(f"Updated DefaultEngine.ini → GameDefaultMap={map_value}")


def setup_main_menu():
    unreal.log("=== ProjectOrganoid main menu setup ===")
    ensure_directory(MENU_DIR)
    ensure_directory(MAPS_DIR)

    main_wbp = create_or_load_widget_bp("WBP_MainMenu", MENU_DIR, MAIN_MENU_PARENT)
    if main_wbp and (not getattr(main_wbp.widget_tree, "root_widget", None)):
        build_main_menu_layout(main_wbp)
    elif main_wbp:
        # Rebuild layout if root is missing/empty
        try:
            if main_wbp.widget_tree.root_widget is None:
                build_main_menu_layout(main_wbp)
        except Exception:
            build_main_menu_layout(main_wbp)
    compile_and_save_widget(main_wbp, MAIN_MENU_WBP)

    pause_wbp = create_or_load_widget_bp("WBP_PauseMenu", MENU_DIR, PAUSE_MENU_PARENT)
    if pause_wbp:
        try:
            if pause_wbp.widget_tree.root_widget is None:
                build_pause_menu_layout(pause_wbp)
        except Exception:
            build_pause_menu_layout(pause_wbp)
    compile_and_save_widget(pause_wbp, PAUSE_MENU_WBP)

    create_title_map()
    update_default_engine_ini()

    unreal.log("=== Setup complete ===")
    unreal.log("PIE from Lvl_MainMenu (or restart editor so GameDefaultMap refreshes).")
    unreal.log("C++ loads WBP_MainMenu / WBP_PauseMenu automatically when present.")


# Allow Execute Python Script and `py` console both
setup_main_menu()
