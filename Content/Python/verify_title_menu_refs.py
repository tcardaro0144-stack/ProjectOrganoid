# ProjectOrganoid — prove nothing still points at the deleted WBP_MainMenu.
# Output Log: py "Content/Python/verify_title_menu_refs.py"

import os
import unreal


RETIRED = "/Game/UI/Menus/WBP_MainMenu"
TITLE_MAP = "/Game/Maps/Lvl_MainMenu"
TITLE_GM = "/Script/ProjectOrganoid.ProjectOrganoidMainMenuGameMode"


def report(ok, message):
    if ok:
        unreal.log(f"[title-refs] OK  {message}")
    else:
        unreal.log_error(f"[title-refs] BAD {message}")


def check_asset():
    exists = unreal.EditorAssetLibrary.does_asset_exist(RETIRED)
    report(not exists, f"asset {RETIRED} exists={exists} (must be False)")


def check_world_settings():
    if not unreal.EditorAssetLibrary.does_asset_exist(TITLE_MAP):
        report(False, f"missing map {TITLE_MAP}")
        return
    unreal.EditorLoadingAndSavingUtils.load_map(TITLE_MAP)
    world = unreal.EditorLevelLibrary.get_editor_world()
    settings = world.get_world_settings() if world else None
    gm = None
    if settings:
        try:
            gm = settings.get_editor_property("default_game_mode")
        except Exception:
            gm = getattr(settings, "default_game_mode", None)
    gm_path = gm.get_path_name() if gm else "<None>"
    report(TITLE_GM in gm_path, f"Lvl_MainMenu World Settings GameMode = {gm_path}")


def check_ini():
    project_dir = unreal.Paths.project_dir()
    for name in ("DefaultEngine.ini", "DefaultGame.ini"):
        path = os.path.join(project_dir, "Config", name)
        if not os.path.isfile(path):
            report(False, f"missing {path}")
            continue
        text = open(path, encoding="utf-8").read()
        report("WBP_MainMenu" not in text, f"{name} contains WBP_MainMenu = {'WBP_MainMenu' in text}")
        if name == "DefaultEngine.ini":
            report("ProjectOrganoidMainMenuGameMode" in text, f"{name} title GameMode prefix present")


def main():
    unreal.log("=== title menu reference audit ===")
    check_asset()
    check_ini()
    check_world_settings()
    unreal.log("=== done — title spawn is C++ only after a full editor restart ===")


main()
