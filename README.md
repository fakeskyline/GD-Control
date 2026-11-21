# GD Control

A Geometry Dash mod for Geode 6.0.0-beta.1, targeting **Geometry Dash 2.2091 (Android ARM64)**.

## Features

- **No-Clip** — hooks `PlayLayer::destroyPlayer`. Normal deaths for player 1 and player 2 are suppressed while enabled. Anticheat spikes and level-completion handling are forwarded to the original game method.
- **Speedhack** — multiplies the delta time passed to `cocos2d::CCScheduler::update`.
- **GD-style entry button** — uses the game's `ButtonSprite` and `GJ_button_01.png` assets.

## Configuration

Open Geode -> Mods -> GD Control -> Settings.

No-Clip is disabled by default. Speedhack is controlled by the multiplier (0.25x–3.0x); 1.0x is normal speed.

## Compatibility

This project uses Geode **6.0.0-beta.1**, whose SDK and generated Android64 bindings target Geometry Dash **2.2091**. The `gd.android` field must match that SDK target or CMake rejects the project before compiling.

This build is therefore for GD 2.2091. It is not a compatible package for a game installation that is actually GD 2.209. A true 2.209 target requires a Geode SDK release and matching bindings for that exact game version; changing only `mod.json` cannot provide that compatibility.

## Build

GitHub Actions builds the Android64 (`arm64-v8a`) `.geode` package. Artifact: `GD-Control-Android64`.

## License

MIT. See [LICENSE](LICENSE).

GD Control is an independent community project and is not affiliated with RobTop Games or the Geode team.
