# GD Control

A Geometry Dash mod for **GD 2.2091**, Geode **6.0.0-beta.1**, Android64.

## In-game controls

- A floating **GD CONTROL** button is shown on the main menu.
- Drag the button to reposition it. Its position is saved between launches.
- Tap the button to open the in-game control panel.
- **No-Clip** can be enabled or disabled directly in the panel.
- **Speedhack** can be adjusted in 0.05x increments with the minus/plus buttons, from 0.25x to 3.00x. 1.00x is normal speed.

The same options remain available in Geode's mod settings as a fallback.

## Gameplay hooks

- No-Clip hooks `PlayLayer::destroyPlayer` and suppresses normal death for player 1 and player 2 while enabled. Anticheat spikes and level completion are forwarded to the original game method.
- Speedhack multiplies the delta time passed to `cocos2d::CCScheduler::update`.

## Build

GitHub Actions builds the Android64 (`arm64-v8a`) `.geode` package for Geometry Dash 2.2091.

## License

MIT. See [LICENSE](LICENSE).
