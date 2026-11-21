# GD Control

A Geometry Dash mod for Geode, targeting Geometry Dash 2.2091 and Geode 6.0.0-beta.1 on Android ARM64.

## Features

- **No-Clip** — hooks `GJBaseGameLayer::destroyPlayer`, the class that owns this function in the 2.2091 reverse-engineered source.
- **Speedhack** — scales `CCScheduler::update` by the configured multiplier.
- **GD-style entry button** — uses the game's own `ButtonSprite` and `GJ_button_01.png` assets.

## Configuration

Open Geode -> Mods -> GD Control -> Settings.

No-Clip is disabled by default. Speedhack defaults to 1.0x. Because the scheduler hook scales the Cocos scheduler, it also affects scheduled UI and animation updates; this is a simple first implementation.

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
