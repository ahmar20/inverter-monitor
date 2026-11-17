# Inverter Monitor — ESP32-S3

Firmware for an ESP32-S3 that talks to a Voltronic/PI18-protocol hybrid inverter (built and tested against an InfiniSolar VII 5KW) over UART, and exposes a local web dashboard, a WiFi-manageable setup page, inverter settings/automation, and optional cloud telemetry upload.

## Features

- Non-blocking round-robin polling of the inverter's PI18 commands (GSX,   PIRI, MOD, FLAG, ACCT, ACLT) with per-command failure backoff.
- Live dashboard (`/dashboard`) and a raw debug view (`/serial`) of every parsed field plus the raw protocol frames.
- WiFi setup page (`/home`): scan-to-add profiles, BSSID-pinned connections with periodic roaming to a stronger sibling AP under the same SSID (useful with range extenders broadcasting one SSID), NVS-backed profile storage, and a fallback open setup AP if no saved network is in range.
- Settings page (`/settings`, HTTP Basic Auth) for inverter configuration (source priority, charge voltages, AC input range, etc.) plus a scheduler for firing settings changes at set times/days.
- OTA updates via ElegantOTA.
- Estimated grid-import power via an energy-balance calculation (`output + battery charge - battery discharge - PV`), gated on the
  inverter's own grid-power-direction flag and its reported output mode (to distinguish real AC bypass from ordinary grid-tied operation).
- Coulomb-counting SOC estimate, daily energy counters with midnight reset.
- Connectivity log (inverter + WiFi down/recovered events with duration).
- Optional cloud telemetry: a store-and-forward upload queue (RAM, spilling to LittleFS flash during outages) plus a boot-time restore of today's energy counters from the cloud.

## Hardware

- ESP32-S3 board with PSRAM (used for the cloud upload queue; falls back to internal heap if absent).
- Inverter UART wired to `Serial2` — default pins: TX `16`, RX `17`, `2400` baud, 8N1. Adjust in `include/config.h` if your wiring differs.
- An addressable RGB LED (NeoPixel) on pin `48` for status indication. Adjust in `include/config.h` if your board differs.

## Building

This is a PlatformIO project (Arduino framework, ESP32-S3).

1. Clone the repo.
2. Copy `include/secrets.h.example` to `include/secrets.h` and fill in your
   own values:
   - `SETTINGS_USER` / `SETTINGS_PASS` — HTTP Basic Auth for `/settings`, `/api/command`, `/api/schedules*`, and OTA.
   - `WIFI_FALLBACK_SSID_1` / `WIFI_FALLBACK_PASS_1` — a permanent compiled-in WiFi fallback, always tried last even if every saved
     profile is deleted or NVS is wiped. (Day-to-day WiFi is otherwise managed entirely from `/home` — no other credentials belong in code.)
   - `cloudUpdateUrl` / `cloudApiKey` — only needed if you want cloud telemetry; see below.
3. Build and upload with PlatformIO (`pio run -t upload`), or via the IDE.
4. On first boot with no saved WiFi profile, the device opens an open setup AP (`InverterMonitor-Setup`) — connect to it and browse to
   `XXX.XXX.XXX.XXX/home` (usually 192.168.X.X as per your local WiFi network) to add your network.

## Cloud telemetry (optional)

If you set `cloudUpdateUrl`/`cloudApiKey`, the firmware will:

- POST one telemetry sample every 15s to `{cloudUpdateUrl}` with a JSON body of `device_id`, `recorded_at`, and the instant power/energy-delta fields (grid, PV, battery charge/discharge, load).
- GET `{cloudUpdateUrl}/daily?device_id=...&from_date=...&to_date=...` once at boot to merge today's cloud-recorded totals with local counters, in case of a mid-day reboot.
- Send `Authorization: Bearer {cloudApiKey}` on both.

There's no backend included in this repo — point it at your own compatible service, or leave `secrets.h`'s cloud values as placeholders and the device will simply queue samples locally (and eventually drop the oldest ones once flash fills) without ever successfully uploading. Everything else (local dashboard, WiFi management, inverter control) works fully standalone.

## License

MIT — see [LICENSE](LICENSE).
