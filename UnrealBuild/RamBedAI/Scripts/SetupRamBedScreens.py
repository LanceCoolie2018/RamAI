"""
RamBed level cleanup for C++ auto-spawn (Option C).

Run from Unreal Editor:
  Tools -> Execute Python Script -> Scripts/SetupRamBedScreens.py

Or from project root:
  SetupRamBedAutoSpawn.bat
"""

import unreal

MAP_PATH = "/Game/RamBed"
LEGACY_HA_CLASS_NAMES = {
    "BP_HA_AnchoredDisplay_C",
    "BP_HA_AnchoredDisplay",
}
CPP_HA_CLASS_NAME = "HaAnchoredDisplayActor"


def _actor_class_name(actor) -> str:
    return actor.get_class().get_name()


def remove_legacy_ha_displays():
    removed = []
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        class_name = _actor_class_name(actor)
        if class_name in LEGACY_HA_CLASS_NAMES:
            label = actor.get_actor_label()
            unreal.EditorLevelLibrary.destroy_actor(actor)
            removed.append(label)
            unreal.log_warning(
                f"Removed legacy HA display '{label}' ({class_name}). "
                "C++ auto-spawn will create HA Anchored Display at runtime."
            )

    if not removed:
        unreal.log("No legacy BP_HA_AnchoredDisplay actors found in level.")
    return removed


def report_cpp_ha_displays():
    displays = [
        actor
        for actor in unreal.EditorLevelLibrary.get_all_level_actors()
        if _actor_class_name(actor) == CPP_HA_CLASS_NAME
    ]

    if displays:
        for display in displays:
            unreal.log(
                f"Found C++ HA display '{display.get_actor_label()}' at {display.get_actor_location()}"
            )
        unreal.log_warning(
            "C++ HA Anchored Display actor(s) are in the level. "
            "Auto-spawn will reposition them; you can delete them too if you want spawn-only."
        )
    else:
        unreal.log(
            "No placed HA dashboard actors. bAutoSpawnHaDashboard=True will spawn one at runtime."
        )


def main():
    unreal.EditorLevelLibrary.load_level(MAP_PATH)

    removed = remove_legacy_ha_displays()
    report_cpp_ha_displays()

    unreal.EditorLevelLibrary.save_current_level()
    unreal.log(
        "RamBed Option C complete. "
        f"Removed {len(removed)} legacy HA actor(s). "
        "Save confirmed. Deploy to Quest and look for: "
        "'RamBed setup: spawned HA dashboard' and 'HA Dashboard: loading'."
    )


if __name__ == "__main__":
    main()