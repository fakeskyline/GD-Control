# GD Control

A Geometry Dash mod for Geode 6.0.0-beta.1, targeting **Geometry Dash 2.209 (Android ARM64)**.

## Features

- **No-Clip** — hooks `PlayLayer::destroyPlayer`. Normal deaths for player 1 and player 2 are suppressed while enabled. Anticheat spikes and level-completion handling are forwarded to the original game method.
- **Speedhack** — multiplies the delta time passed to `cocos2d::CCScheduler::update`, matching the established approach used by GDH and Eclipse.
- **GD-style entry button** — uses the game's `ButtonSprite` and `GJ_button_01.png` assets.

## Configuration

Open Geode -> Mods -> GD Control -> Settings.

No-Clip is disabled by default. Speedhack is controlled by the multiplier (0.25x–3.0x); 1.0x is normal speed.

## Compatibility

The package targets Geometry Dash **2.209**, not 2.2091. The previous package targeted 2.2091 while the installed game is 2.209. That mismatch is consistent with the unresolved `PlayLayer::circleWaveWillBeRemoved(CCCircleWave*)` symbol shown by the loader.

The referenced public implementations use older game targets:
- GDH: 2.2074
- xdBot: 2.2074
- Eclipse: 2.2081

Their algorithms are useful references, but their binary addresses and complete hook sets must not be copied blindly into 2.209.

## Build

GitHub Actions builds the Android64 (`arm64-v8a`) `.geode` package. Artifact: `GD-Control-Android64`.

## License

MIT. See [LICENSE](LICENSE).

GD Control is an independent community project and is not affiliated with RobTop Games or the Geode team.
