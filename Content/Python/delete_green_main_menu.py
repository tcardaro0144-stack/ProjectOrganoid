# Deletes the retired green New Game widget so it cannot compile-on-load.

import unreal

PATH = "/Game/UI/Menus/WBP_MainMenu"


def main():
    if unreal.EditorAssetLibrary.does_asset_exist(PATH):
        if unreal.EditorAssetLibrary.delete_asset(PATH):
            unreal.log(f"[delete_green_main_menu] deleted {PATH}")
        else:
            unreal.log_error(f"[delete_green_main_menu] failed to delete {PATH}")
    else:
        unreal.log(f"[delete_green_main_menu] already gone: {PATH}")


main()
