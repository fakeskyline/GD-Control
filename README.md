# GD Control

A Geometry Dash mod for Geode, targeting Geometry Dash 2.2091 and Geode 6.0.0-beta.1 on Android ARM64.

## Features

- **No-Clip** — hooks the `PlayLayer::destroyPlayer` override. Normal player deaths are suppressed while enabled; the anticheat spike and level-end path are still forwarded to the game.
- **Speedhack** — scales `GJBaseGameLayer::update` only while a `PlayLayer` is active. The global Cocos scheduler and unrelated menu/UI animations are not scaled.
- **GD-style entry button** — uses the game's own `ButtonSprite` and `GJ_button_01.png` assets.

## Configuration

Open Geode -> Mods -> GD Control -> Settings.

No-Clip is disabled by default. Speedhack defaults to 1.0x and can be set from 0.25x to 3.0x.

## Implementation notes

The 2.2091 bindings mark `GJBaseGameLayer::destroyPlayer` as inline, so Geode cannot hook that base method directly. No-Clip therefore hooks the `PlayLayer` override, following the approach used by QOLMod for the same Geode/GD version pair.

Speedhack scales the gameplay layer's update delta rather than the global scheduler, so the mod does not intentionally accelerate menu and unrelated UI scheduling.

## Target versions

| Component | Target |
|---|---|
| Geometry Dash | 2.2091 |
| Geode | 6.0.0-beta.1 |
| Build target | Android64 (`arm64-v8a`) |
| Language | C++23 |

## Build

GitHub Actions builds the `.geode` package for Android64 using `geode-sdk/build-geode-mod@main`. The artifact is named `GD-Control-Android64`.

## License

MIT. See [LICENSE](LICENSE).

GD Control is an independent community project and is not affiliated with RobTop Games or the Geode team.
