# Online firmware updates

The Settings → Firmware Update panel checks a JSON manifest hosted at:

`https://raw.githubusercontent.com/hocki7z/ESP32_WordClock/fw_updates/latest.json`

Publish `latest.json` and the matching versioned OTA application image in the
`fw_updates` branch. The manifest format is shown in
[fw_updates_latest.json.example](fw_updates_latest.json.example):

- `version`: three decimal components, each from 0 to 255, e.g. `1.2.3`.
- `url`: HTTPS raw-content URL for the matching `.bin` file in this repository's
  `fw_updates` branch.

The image must be the single-file OTA application produced by PlatformIO
(`ESP32_WordClock_ota_<major>_<minor>_<build>.bin`), not the full-flash image.
The device validates the ESP32 image, embedded ProductInfo record, and version
before selecting it for boot.

The automatic-check preference is stored in the browser's local storage. Checks
run only while the web UI is open: either once five minutes after the UI starts,
or every hour on the hour using the browser's local time. No update is installed
automatically; the user must select **Start update now**.
