# GD Control

A Geometry Dash mod for Geode, targeting Geometry Dash 2.2091 and Geode 6.0.0-beta.1 on Android ARM64.

## Current status

This is a compatibility/stability build. It adds a native Geometry Dash-style **CONTROL** button to the main menu and confirms that the mod loads.

Gameplay hooks are temporarily disabled. The previous Android build failed at runtime because the PlayLayer modification referenced a missing Geometry Dash symbol (`PlayLayer::circleWaveWillBeRemoved(CCCircleWave*)`). The CI compiler and linker did not detect this runtime ABI issue.

### Features

- GD-style entry button using the game's `ButtonSprite` and `GJ_button_01.png` assets.
- No-Clip: temporarily disabled while a compatible hook is developed.
- Speed multiplier: temporarily disabled while a compatible hook is developed.
- Icon Unlocker: planned, not implemented.

## Target versions

| Component | Target |
|---|---|
| Geometry Dash | 2.2091 |
| Geode | 6.0.0-beta.1 |
| Build target | Android64 (`arm64-v8a`) |
| Language | C++23 |

## Build

GitHub Actions builds the `.geode` package for Android64 using `geode-sdk/build-geode-mod@main`.

The resulting package is uploaded as the `GD-Control-Android64` workflow artifact.

## Notes

A successful CI build does not guarantee runtime compatibility. Test the package on the target Android/Geometry Dash version before enabling gameplay hooks.

## License

MIT. See [LICENSE](LICENSE).

## Credits

- [Geode SDK](https://github.com/geode-sdk/geode)
- RobTop Games for Geometry Dash and its original UI assets.

GD Control is an independent community project and is not affiliated with RobTop Games or the Geode team.
