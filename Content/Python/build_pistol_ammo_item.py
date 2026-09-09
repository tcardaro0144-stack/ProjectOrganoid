# Creates /Game/Data/Items/DA_Item_PistolAmmo. Does not place world pickups.
# Run after C++ AmmoType exists on UProjectOrganoidItemData.

import unreal

ITEM_PATH = "/Game/Data/Items/DA_Item_PistolAmmo"


def report(message):
    unreal.log_warning(f"[PISTOL AMMO] {message}")


def set_prop(obj, name, value):
    candidates = [name]
    snake = "".join(f"_{c.lower()}" if c.isupper() else c for c in name).lstrip("_")
    candidates.append(snake)
    if name.startswith("b") and len(name) > 1 and name[1].isupper():
        candidates.extend([name[1:], snake[1:] if snake.startswith("b_") else snake])
    for candidate in candidates:
        try:
            obj.set_editor_property(candidate, value)
            return True
        except Exception:
            pass
    report(f"could not set {name}")
    return False


def resolve_enum(enum_type_name, wanted):
    enum_type = getattr(unreal, enum_type_name, None)
    if not enum_type:
        report(f"enum missing: {enum_type_name}")
        return None
    target = wanted.replace("_", "").lower()
    for attr in dir(enum_type):
        if attr.startswith("_"):
            continue
        if attr.replace("_", "").lower() == target:
            return getattr(enum_type, attr)
    report(f"enum value missing: {enum_type_name}.{wanted}")
    return None


def create_or_load():
    package_path, _, asset_name = ITEM_PATH.rpartition("/")
    if unreal.EditorAssetLibrary.does_asset_exist(ITEM_PATH):
        report(f"exists: {ITEM_PATH}")
        return unreal.EditorAssetLibrary.load_asset(ITEM_PATH)

    factory = unreal.DataAssetFactory()
    try:
        factory.set_editor_property("data_asset_class", unreal.ProjectOrganoidItemData)
    except Exception:
        try:
            factory.set_editor_property("DataAssetClass", unreal.ProjectOrganoidItemData)
        except Exception:
            pass

    created = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, package_path, unreal.ProjectOrganoidItemData, factory
    )
    if not created:
        report(f"failed to create {ITEM_PATH}")
        return None
    report(f"created {ITEM_PATH}")
    return created


def build():
    if not unreal.EditorAssetLibrary.does_directory_exist("/Game/Data/Items"):
        unreal.EditorAssetLibrary.make_directory("/Game/Data/Items")

    asset = create_or_load()
    if not asset:
        return False

    set_prop(asset, "ItemName", unreal.Text("Pistol Ammunition"))
    set_prop(asset, "ItemType", resolve_enum("ProjectOrganoidItemType", "Ammo"))
    set_prop(asset, "AmmoType", resolve_enum("ProjectOrganoidAmmoType", "Pistol"))
    set_prop(asset, "GridWidth", 1)
    set_prop(asset, "GridHeight", 1)
    set_prop(asset, "ItemWeight", 0.05)
    set_prop(asset, "bCanStack", True)
    set_prop(asset, "MaxStackCount", 30)
    unreal.EditorAssetLibrary.save_asset(ITEM_PATH)
    report("saved DA_Item_PistolAmmo")
    return True


if __name__ == "__main__":
    build()
