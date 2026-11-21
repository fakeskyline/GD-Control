# GD Control

A Geometry Dash mod menu for Geode, targeting Geometry Dash 2.2091 and Geode 6.0.0-beta.1 on Android ARM64.

## Current status

This repository is the initial development build. It includes the Geode project structure, native Geometry Dash menu entry, configurable gameplay settings, and GitHub Actions packaging. Features are being added incrementally and must be validated in-game before being considered complete.

### Initial features

- No-Clip toggle: skips the standard player-death handler while enabled. This is a no-death implementation, not yet a complete collision bypass; some collision responses may still occur.
- Speed multiplier: scales the `PlayLayer::update` delta time using a configurable multiplier.
- GD-style entry button: uses the game's own `ButtonSprite` and `GJ_button_01.png` assets.
- Icon Unlocker: planned; not implemented in this initial build. The mod will not falsely report locked icons as unlocked.

## Target versions

| Component | Target |
|---|---|
| Geometry Dash | 2.2091 |
| Geode | 6.0.0-beta.1 |
| Build target | Android64 (`arm64-v8a`) |
| Language | C++23 |

## Build

The GitHub Actions workflow builds the `.geode` package for Android64 using `geode-sdk/build-geode-mod@main`.

Push to `main` or run the workflow manually from the Actions tab. The resulting `.geode` file is uploaded as a workflow artifact.

## Configure

Open the Geode mod settings for **GD Control** to enable No-Clip and adjust the speed multiplier. The in-game GD-style button opens the feature/status panel.

## Notes

- No-Clip currently prevents the standard death handler; it does not yet remove every collision response or hazard interaction.
- Speed scaling is applied to the gameplay update delta, not the global Cocos scheduler, to avoid slowing unrelated UI and mod tasks.
- Icon unlocking is not included until the correct 2.2091 profile/statistics APIs are implemented and tested.
- This is an early development build. A successful CI compile does not replace testing on an actual Android device.

## License

MIT. See [LICENSE](LICENSE).

## Credits

- [Geode SDK](https://github.com/geode-sdk/geode)
- RobTop Games for Geometry Dash and its original UI assets.

GD Control is an independent community project and is not affiliated with RobTop Games or the Geode team.
