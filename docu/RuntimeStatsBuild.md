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