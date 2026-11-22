# GD Control

A Geometry Dash mod for **GD 2.2091**, Geode **6.0.0-beta.1**, Android64.

## Features

- Floating **GDC** button opens the full GD CONTROL panel.
- Dark graphite panel with compact black feature cards and RU/EN language switching.
- No-Clip.
- Speedhack from 0.25x to 3.00x with FMOD master pitch synchronization.
- Local Unlock Icons.
- Practice Music Sync unlock.
- Auto Clicker with separate press/hold and release lengths (1–20 physics steps).
- TPS Bypass with adjustable physics rate (30–2400 TPS).
- FPS Bypass with adjustable render target (30–1000 FPS).

## Auto Clicker

The Auto Clicker simulates Player 1 jump presses. **HOLD** controls how many physics steps the input stays pressed; **RELEASE** controls how many steps it stays released. Both values are clamped to 1–20. It automatically releases the synthetic input on pause, restart, death or level exit.

## TPS and FPS

TPS Bypass changes the physics update cadence; FPS Bypass changes the Cocos render interval. They are independent controls. Actual FPS is still limited by the device display refresh rate and performance. Very high TPS values can increase CPU load, so raise them gradually on mobile.

## Practice Music

Enable **Unlock Practice Music** in the GD Control panel. Then open a level's pause menu, enter its extended settings (gear), and enable **Practice Music Sync**. This bypasses only the local unlock check; it does not change account/shop data.

## Build

GitHub Actions builds Android64 for Geometry Dash 2.2091.

## License

MIT. See [LICENSE](LICENSE).
