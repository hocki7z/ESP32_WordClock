# Application firmware OTA update

The application receives OTA firmware through `POST /update`. This describes the
firmware-side update flow; it does not cover packaging or bootloader configuration.

## Upload and validation

The application writes the incoming `.bin` image to the next OTA partition while
continuing to run from the current partition. Uploads larger than the destination
partition are rejected.

After upload, the running application:

1. Validates the ESP32 image with `esp_ota_end()`.
2. Searches the image for an embedded ProductInfo record and validates its
   identifier, format version, and CRC-32.
3. Rejects an invalid candidate version (`255.255.255`).
4. Requires the candidate version to be newer than the version embedded in the
   currently running firmware. Versions are compared numerically by major, minor,
   then build/patch. If the running firmware's version is invalid, a valid
   candidate is allowed as a recovery update.

The build timestamp, branch, and commit hash do not affect version acceptance.
The ProductInfo CRC detects accidental corruption; it does not authenticate the
firmware publisher.

## Restart behavior

If validation succeeds, the application sets the uploaded partition as the next
boot partition and stores a one-shot update-restart marker. It responds
**“Firmware accepted. The device is restarting now.”** and restarts after about
1.5 seconds. The response confirms acceptance, not that the new firmware boots
or operates correctly.

On startup, the marker is consumed and the firmware-update restart counter is
incremented. The application does not implement a boot-time health check or
automatic rollback policy. The previous application remains in the other OTA
slot until overwritten, but this alone does not guarantee recovery.

## Implementation references

- [OTA receiver, validation, boot selection, and restart](../src/OTAUpdate.cpp)
- [ProductInfo validation and version policy](../src/ProductInfo.cpp)
- [Update-restart marker handling](../src/main.cpp)
