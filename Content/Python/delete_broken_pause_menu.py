# Remove a tree-less WBP_PauseMenu so editor compile-on-load cannot block the title map.
import unreal

PATH = "/Game/UI/Menus/WBP_PauseMenu"
if unreal.EditorAssetLibrary.does_asset_exist(PATH):
    unreal.EditorAssetLibrary.delete_asset(PATH)
    unreal.log_warning("[PAUSE MENU] deleted broken WBP_PauseMenu (C++ pause layout is the runtime fallback)")
else:
    unreal.log_warning("[PAUSE MENU] WBP_PauseMenu already absent")
