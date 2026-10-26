# ESP32_WordClock
WordClock 16x16 LED matrix

## HTTPS web interface

The normal Wi-Fi (STA) interface now uses **https://wordclock.local/** on port
443, including secure WebSockets. It never silently falls back to HTTP. There
is no HTTP-to-HTTPS redirect listener in normal operation. Access-point recovery
and captive-portal provisioning deliberately use HTTP on port 80 instead. When
switching modes, ESPUI stops the old server before starting the other one; reload
the browser at the appropriate address after a switch.

### Build and first use

1. Build with PlatformIO as usual. Before compiling, the build provisions a
	unique RSA-2048/SHA-256 self-signed TLS identity using **PowerShell 7 (`pwsh`)**.
	No extra Python packages or public example keys are used.
2. The identity is reused on subsequent builds. It is valid for 730 days, has
	server-authentication usage and DNS SANs `wordclock.local` and `wordclock`.
	Keep the computer's clock correct when generating it.
3. Upload the firmware as usual. Once connected to Wi-Fi, open
	**https://wordclock.local/**. The serial log reports HTTPS startup failures.
	The client must support mDNS and be on a network which permits it.
4. A self-signed certificate is **not automatically trusted**. Verify its
	fingerprint locally and explicitly trust the device certificate in your
	browser/OS, use a browser-specific exception for development, or supply a
	certificate signed by a CA your devices already trust. No trust-store changes
	are made by this project. If mDNS is unavailable, accessing the IP address
	also causes a hostname warning unless that IP is included in the certificate.

### Generated identity and replacing it

The ignored **.https directory** contains the server certificate, unencrypted
private key, and generated C++ credentials header. Do not commit, publish or
share that directory, build caches or firmware binaries: the firmware embeds
the key. Flashing the same firmware onto several devices shares one identity;
use separate identities/hostnames if devices need independent credentials.

To use an existing identity, place its PEM certificate (leaf first, followed by
any intermediates) and matching unencrypted PEM private key in that directory,
using the generator's certificate/key filenames, before building. The private
key must use RSA PKCS#8 or PKCS#1 PEM. The generator only runs when neither file
exists; an incomplete pair is an error. The header is created by the pre-build
hook and is never maintained manually. A certificate/key mismatch is rejected
when TLS starts on the ESP32.

For renewal, key rotation or a hostname change, back up the old identity securely,
remove **both** certificate and key from the ignored directory, then rebuild
and upload. Clients must trust the replacement certificate again. The
`custom_https_hostname` PlatformIO option controls the mDNS name and generated
certificate name. Existing certificates are not changed automatically when that
option changes. The PowerShell generator also accepts an optional `IpAddress`
array if a certificate must include a stable device IP.

### Framework and memory requirements

- The selected ESP32 framework must already provide
  `CONFIG_ESP_HTTPS_SERVER_ENABLE` and `CONFIG_HTTPD_WS_SUPPORT`. A compiler error
  explicitly reports missing support; defining a macro alone cannot add absent
	libraries to a precompiled framework. Both options were verified enabled in
	the currently installed Arduino-ESP32 ESP32 QIO/QSPI SDK configuration.
- ESPUI uses a 12 KB HTTP-server task stack and at most two simultaneous TLS
  sockets. PsychicHttp estimates roughly 45 KB heap per TLS connection. Start
  testing with one browser tab, then check free/largest heap, live preview,
  reconnects and repeated AP/STA transitions on the actual hardware.
- HTTPS encrypts transport; it does not itself restrict who can control the
  clock. The application still starts without an HTTP username/password.
  Provisioning mode intentionally remains unencrypted: use it only on a trusted
  local setup network.

The shared local ESPUI variant must provide `setCertificate()` and
`beginHTTPS()`. A stock upstream ESPUI release is not a replacement for that
backend. Runtime certificate parsing, browser trust and memory limits require a
device test after building.
