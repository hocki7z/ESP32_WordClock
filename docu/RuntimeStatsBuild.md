# Runtime-statistics build

Select `nodemcu-32s_runstats` in PlatformIO and use its normal Build or Upload
task. This is also the project's default environment. The `tools/build_runstats.bat` launcher is
optional; it no longer performs a separate initialization step.

The environment runs `tools/prepare_idf_build.py` before loading ESP-IDF:

- Configures public PyPI and Windows/system certificate trust for the build
  process and its children only. TLS verification stays enabled. No global pip
  configuration is changed. ESP-IDF installs any missing Python dependencies in
  its own virtual environment; the script does not maintain a second package list.
- Clones LittleFS v1.14.8 with its pinned submodule when absent, then verifies
  both revisions. Existing checkouts are never reset or overwritten. A failed
  or partial clone is reported rather than silently replaced.
- Skips dependency preparation on Clean and Full Clean.

The initial build needs network access to PyPI, GitHub and PlatformIO package
servers. With the dependencies installed, subsequent builds reuse them. Full
Clean removes build artifacts, not the local LittleFS checkout or IDF Python
environment. Only run one build/clean/upload task at a time.

## Company network configuration

Public PyPI must be permitted by company policy. Set `custom_idf_public_pypi =
false` in the runtime-statistics environment to preserve normal pip settings.
The public-PyPI mode ignores pip configuration files, including proxy/certificate
settings in those files; proxy environment variables are preserved.

For an IT-approved PEM CA bundle, set `custom_idf_ca_bundle` in the environment.
The script sets both `PIP_CERT` and `REQUESTS_CA_BUNDLE` for child processes.
Alternatively, inherit those environment variables from the launching terminal.
The optional batch launcher's first argument also accepts a CA bundle path.
No passwords, tokens or private keys belong in the project configuration.

The `truststore` feature requires pip >=22.2 and Python >=3.10, as used by the
current PlatformIO installation. It does not disable HTTPS verification.

Commit the bootstrap script and CMake configuration. If LittleFS is obtained by
the bootstrap rather than tracked as vendored source or a proper Git submodule,
do not accidentally stage it as an embedded repository. Its exact source
revisions are pinned in the bootstrap script.

## Arduino board-variant warning

With PlatformIO Espressif32 6.8.1 and Arduino Core 2.0.17, the combined build prints:

> Arduino framework as an ESP-IDF component doesn't handle the `variant` field!
> The default `esp32` variant will be used.

This is a PlatformIO limitation notice, not a compilation error. The combined
build uses Arduino's generic ESP32 pin definitions instead of the NodeMCU-32S
variant selected by an Arduino-only build.

The installed NodeMCU-32S and generic ESP32 variants have identical UART, I2C,
SPI, analog, touch and DAC pin mappings. The NodeMCU-32S variant additionally
defines `LED_BUILTIN` (GPIO 2), `BUILTIN_LED` and `KEY_BUILTIN` (GPIO 0).
The current application does not reference these board-specific aliases; its
FastLED display uses an explicit `LED_DATA_PIN`. The fallback is therefore
acceptable for the current hardware and application.

The message is printed unconditionally by this PlatformIO combined-framework
builder. Compiler warning flags and `board_build.variant` do not suppress it.
Leave it visible rather than patching installed PlatformIO files merely to hide
the notice. Recheck pin definitions if changing boards or adding code that relies
on board-specific aliases; suppressing the message would not fix a pin mismatch.