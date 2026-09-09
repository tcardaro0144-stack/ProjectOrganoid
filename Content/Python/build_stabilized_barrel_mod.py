# Creates /Game/Data/Weapons/Mods/DA_Mod_StabilizedBarrel.
# Does not place world actors. Campaign-compatible pistol Barrel mod.

import unreal

MOD_PATH = "/Game/Data/Weapons/Mods/DA_Mod_StabilizedBarrel"


def report(message):
    unreal.log_warning(f"[STABILIZED BARREL] {message}")


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
    package_path, _, asset_name = MOD_PATH.rpartition("/")
    if unreal.EditorAssetLibrary.does_asset_exist(MOD_PATH):
        report(f"exists: {MOD_PATH}")
        return unreal.EditorAssetLibrary.load_asset(MOD_PATH)

    factory = unreal.DataAssetFactory()
    cls = getattr(unreal, "ProjectOrganoidWeaponMod_StabilizedBarrel", None)
    if not cls:
        cls = getattr(unreal, "ProjectOrganoidWeaponModData", None)
    if not cls:
        report("mod class missing")
        return None
    try:
        factory.set_editor_property("data_asset_class", cls)
    except Exception:
        try:
            factory.set_editor_property("DataAssetClass", cls)
        except Exception:
            pass

    created = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, package_path, cls, factory
    )
    if not created:
        report(f"failed to create {MOD_PATH}")
        return None
    report(f"created {MOD_PATH}")
    return created


def build():
    if not unreal.EditorAssetLibrary.does_directory_exist("/Game/Data/Weapons/Mods"):
        unreal.EditorAssetLibrary.make_directory("/Game/Data/Weapons/Mods")

    asset = create_or_load()
    if not asset:
        return False

    set_prop(asset, "ModId", "StabilizedBarrel")
    set_prop(asset, "DisplayName", unreal.Text("Stabilized Barrel"))
    set_prop(asset, "Slot", resolve_enum("ProjectOrganoidWeaponModSlot", "Barrel"))
    set_prop(asset, "DamageMultiplier", 1.05)
    set_prop(asset, "FireRateMultiplier", 1.0)
    set_prop(asset, "PenetrationMultiplier", 1.0)
    set_prop(asset, "NoiseLoudnessMultiplier", 1.0)
    set_prop(asset, "NoiseRangeMultiplier", 1.0)
    set_prop(asset, "bSuppressesSoundEmission", False)
    set_prop(asset, "SOTInstallCost", 2)
    unreal.EditorAssetLibrary.save_asset(MOD_PATH)
    report("saved DA_Mod_StabilizedBarrel")
    return True


if __name__ == "__main__":
    build()
