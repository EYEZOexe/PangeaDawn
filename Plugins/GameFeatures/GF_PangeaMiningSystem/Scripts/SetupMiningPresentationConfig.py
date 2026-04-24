import unreal


PACKAGE_PATH = "/GF_PangeaMiningSystem/Mining/Test"
CONFIG_ASSET_NAME = "DA_MiningSitePresentation_Test"
SITE_DEFINITION_PATH = "/GF_PangeaMiningSystem/Mining/Test/DA_MiningSite_Iron_Test"
SITE_MENU_WIDGET_CLASS_PATH = "/Script/MiningSystemUI.MiningSiteMenuWidget"


def make_station(primary_name: str, secondary_name: str) -> unreal.MiningPresentationStation:
    station = unreal.MiningPresentationStation()
    station.set_editor_property("primary_marker_name", primary_name)
    station.set_editor_property("secondary_marker_name", secondary_name)
    return station


def make_role(
    role,
    smart_object_asset: str,
    interaction_duration: float,
    interaction_duration_variance: float,
    travel_speed: float,
    stations: list[tuple[str, str]],
) -> unreal.MiningPresentationRoleConfig:
    role_config = unreal.MiningPresentationRoleConfig()
    role_config.set_editor_property("role", role)
    role_config.set_editor_property("smart_object_definition", unreal.EditorAssetLibrary.load_asset(smart_object_asset))
    role_config.set_editor_property("interaction_duration", interaction_duration)
    role_config.set_editor_property("interaction_duration_variance", interaction_duration_variance)
    role_config.set_editor_property("travel_speed", travel_speed)
    role_config.set_editor_property("stations", [make_station(primary, secondary) for primary, secondary in stations])
    return role_config


def main() -> None:
    if not hasattr(unreal, "MiningSitePresentationConfig"):
        raise RuntimeError("MiningSitePresentationConfig is not reflected yet. Compile the module first.")

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_lib = unreal.EditorAssetLibrary

    config_asset_path = f"{PACKAGE_PATH}/{CONFIG_ASSET_NAME}"
    config_asset = editor_lib.load_asset(config_asset_path)
    if config_asset is None and editor_lib.does_asset_exist(config_asset_path):
        editor_lib.delete_asset(config_asset_path)

    if not config_asset:
        config_asset = asset_tools.create_asset(
            CONFIG_ASSET_NAME,
            PACKAGE_PATH,
            unreal.MiningSitePresentationConfig,
            unreal.DataAssetFactory(),
        )

    worker_role = make_role(
        unreal.MiningPresentationRole.WORKER,
        "/GF_PangeaMiningSystem/Mining/Test/SO_MiningWorker_Test.SO_MiningWorker_Test",
        2.0,
        0.35,
        220.0,
        [
            ("WorkerRouteA_1", "WorkerRouteB_1"),
            ("WorkerRouteA_2", "WorkerRouteB_2"),
        ],
    )
    guard_role = make_role(
        unreal.MiningPresentationRole.GUARD,
        "/GF_PangeaMiningSystem/Mining/Test/SO_MiningGuard_Test.SO_MiningGuard_Test",
        2.25,
        0.2,
        250.0,
        [
            ("GuardRouteA_1", "GuardRouteB_1"),
            ("GuardRouteA_2", "GuardRouteB_2"),
        ],
    )
    courier_role = make_role(
        unreal.MiningPresentationRole.COURIER,
        "/GF_PangeaMiningSystem/Mining/Test/SO_MiningCourier_Test.SO_MiningCourier_Test",
        1.0,
        0.1,
        320.0,
        [
            ("CourierRouteA", "CourierRouteB"),
        ],
    )

    config_asset.set_editor_property("worker_role", worker_role)
    config_asset.set_editor_property("guard_role", guard_role)
    config_asset.set_editor_property("courier_role", courier_role)
    editor_lib.save_loaded_asset(config_asset)

    site_definition = editor_lib.load_asset(SITE_DEFINITION_PATH)
    if not site_definition:
        raise RuntimeError(f"Could not load site definition: {SITE_DEFINITION_PATH}")

    site_definition.set_editor_property("presentation_config", config_asset)
    menu_widget_class = unreal.load_class(None, SITE_MENU_WIDGET_CLASS_PATH)
    if menu_widget_class:
        site_definition.set_editor_property("site_menu_widget_class", menu_widget_class)
    editor_lib.save_loaded_asset(site_definition)
    unreal.log("Mining presentation config created and assigned successfully.")


if __name__ == "__main__":
    main()
