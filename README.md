# GD Control

A Geometry Dash mod for **GD 2.2091**, Geode **6.0.0-beta.1**, Android64.

## In-game controls

- Floating **GD CONTROL** button; drag to reposition and tap to open the panel.
- **No-Clip** toggle for both players.
- **Speedhack** from 0.25x to 3.00x in 0.05x increments.
- **Unlock Icons** makes the game's icon-unlock check return true locally while enabled.

## Speedhack audio synchronization

Speedhack scales the Cocos scheduler delta and the FMOD master channel group's pitch by the same multiplier. This keeps streamed level music aligned with gameplay. FMOD pitch affects all audio routed through the master group, including sound effects.

## Unlock Icons

The mod hooks `GameManager::isIconUnlocked` and returns true while the option is enabled. It does not modify RobTop account data or permanently award achievements/unlocks. Disable the option to restore the game's normal unlock checks.

## Build

GitHub Actions builds Android64 (`arm64-v8a`) for Geometry Dash 2.2091.

## License

MIT. See [LICENSE](LICENSE).
