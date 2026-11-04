# Application firmware update flow

This describes the OTA update behavior implemented in the WordClock application.
It does not cover build scripts, firmware packaging, or bootloader configuration.

## 1. Select and upload the firmware

In **Settings → Firmware Update**, select the application OTA `.bin` file and
choose **Upload and install**. Do not select a full-flash image or update ZIP.

The browser sends the file to `POST /update` and displays upload progress.
Reaching 100% means the file has been transferred, not that it has been accepted
or that the new application has started.

## 2. Write to the inactive application partition

The currently running firmware selects the next OTA partition using
`esp_ota_get_next_update_partition(nullptr)` and prepares it with `esp_ota_begin()`.
Incoming chunks are written using `esp_ota_write()`, with a check that they fit
within the destination partition.

With the project's two application slots:

| Running application | Update destination |
| --- | --- |
| `app0` | `app1` |
| `app1` | `app0` |

The current application continues running throughout the upload and validation.
Its application partition is not overwritten. The shared ProductInfo partition
is not updated at this stage.

## 3. Validate the uploaded image

After the final chunk, the **currently running application** performs these checks
in order:

1. **ESP32 image validity:** `esp_ota_end()` must accept the application image.
2. **Embedded ProductInfo:** `FindProductInfoInOtaImage()` scans the uploaded
   bytes for the configured 16-byte identifier, then checks the record's format
   version and CRC-32. Reads overlap so an identifier crossing a buffer boundary
   is not missed. The record need not be at a fixed offset within the application.
3. **Candidate version:** `255.255.255` is rejected as an invalid version.
4. **Version policy:** `IsNewVersionAllowed()` compares the candidate against the
   version stored in the shared ProductInfo partition.

Versions are compared numerically in major, minor, then build/patch order.

| Installed ProductInfo | Candidate | Decision |
| --- | --- | --- |
| Valid version | Strictly newer valid version | Accept |
| Valid version | Equal or older version | Reject |
| Missing, unreadable, invalid record, or invalid installed version | Valid version | Allow recovery update |
| Any state | `255.255.255` | Reject |

The timestamp, branch, commit hash, and filename do not determine whether a
version is newer. A valid identifier and CRC identify a supported record and
detect corruption; they do **not** authenticate the firmware's publisher.

## 4. Reject or select the new application

If writing or validation fails, the server reports an error. Normal rejection
paths do not select the new boot partition or schedule a restart. The current
application continues running; rejected or partial data can remain in the
inactive slot and be overwritten by a later update. This is **rejection before
boot**, not rollback.

If all checks pass, `esp_ota_set_boot_partition()` selects the uploaded partition
for the next boot. It does not start the new application immediately.

## 5. Respond and restart

After successful boot selection, the server responds:

> Firmware accepted. The device is restarting now.

It schedules a restart after approximately 1.5 seconds, giving the HTTP response
time to reach the browser. The browser does not perform a post-reboot health check;
a successful response confirms acceptance, not successful operation of the new
firmware. Reconnect or reload the Web UI after the device restarts.

## 6. Synchronize ProductInfo on startup

The bootloader starts the selected application. During `InitApplication()`, before
application tasks are created, `SyncFlashPartitionWithFirmware()` compares that
application's embedded ProductInfo with the dedicated partition at `0x3FF000`.

- If the records match, no erase/write is needed.
- If they differ, the ProductInfo partition is erased and the embedded record is
  written to it.
- The stored record is then read and validated.
- A synchronization failure is logged; application initialization continues.

The uploaded record is carried **inside the application image**. Startup
synchronization copies it to the shared ProductInfo partition; OTA does not place
it at `0x3FF000` during the upload. The stored record supplies product information
and the installed version used by subsequent update checks.

## 7. Subsequent updates and rollback limits

After switching from `app0` to `app1`, the next update targets `app0`, and vice
versa. The previous application remains in the other slot until a later update
overwrites it.

The application does not implement a boot-time self-test, health confirmation,
or explicit automatic rollback policy. Two application slots alone do not
guarantee recovery from firmware that boots but fails to operate correctly.
Bootloader rollback configuration is outside this application's update flow.

## Implementation references

- [OTA receiver, validation, boot selection, and restart](../src/OTAUpdate.cpp)
- [ProductInfo scanning, version policy, and synchronization](../src/ProductInfo.cpp)
- [Startup initialization](../src/main.cpp)
- [Browser uploader](../tools/ota_uploader.js)