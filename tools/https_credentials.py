"""PlatformIO pre-build: provision a local TLS identity, without logging secrets."""

from pathlib import Path
import re
import shutil
import subprocess

from SCons.Script import Import

Import("env")


def configure_https(build_env):
    project = Path(build_env.subst("$PROJECT_DIR"))
    directory = project / ".https"
    hostname = build_env.GetProjectOption("custom_https_hostname", "wordclock")
    if not re.fullmatch(r"[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?", hostname):
        raise RuntimeError("custom_https_hostname must be a single valid DNS label")
    directory.mkdir(exist_ok=True)
    certificate = directory / "server.crt"
    private_key = directory / "server.key"
    if certificate.exists() != private_key.exists():
        raise RuntimeError("Incomplete TLS identity: supply both .https/server.crt and server.key")
    if not certificate.exists():
        # PowerShell 7 supplies .NET CertificateRequest and PKCS#8 export on
        # Windows. No Python packages, public sample keys or certificate store
        # changes are required. The key is never passed through stdout.
        powershell = shutil.which("pwsh")
        if not powershell:
            raise RuntimeError(
                "TLS provisioning needs PowerShell 7 (pwsh), or an existing PEM "
                "certificate/key pair in .https/server.crt and .https/server.key"
            )
        subprocess.run(
            [powershell, "-NoProfile", "-NonInteractive", "-File",
             str(project / "tools" / "Generate-HttpsCertificate.ps1"),
             "-OutputDirectory", str(directory), "-Hostname", hostname],
            check=True,
        )

    # Normalize locally, retain terminating NULs via C++ string literals.
    cert = certificate.read_text(encoding="ascii").replace("\r\n", "\n")
    key = private_key.read_text(encoding="ascii").replace("\r\n", "\n")
    if "-----BEGIN CERTIFICATE-----" not in cert or not any(
        marker in key for marker in ("-----BEGIN PRIVATE KEY-----", "-----BEGIN RSA PRIVATE KEY-----")
    ):
        raise RuntimeError("TLS identity must contain a PEM certificate and unencrypted PEM private key")
    if ')tls"' in cert or ')tls"' in key:
        raise RuntimeError("Invalid PEM delimiter")
    content = (
        "// Generated locally; contains a PRIVATE KEY. Do not commit or share.\n"
        "#pragma once\nnamespace WordClockTls {\n"
        f'inline constexpr char Hostname[] = "{hostname}";\n'
        f'inline constexpr char Certificate[] = R"tls({cert})tls";\n'
        f'inline constexpr char PrivateKey[] = R"tls({key})tls";\n'
        "}\n"
    )
    header = directory / "WordClockTlsCredentials.h"
    if not header.exists() or header.read_text(encoding="ascii") != content:
        header.write_text(content, encoding="ascii", newline="\n")
    build_env.AppendUnique(CPPPATH=[str(directory)])


configure_https(env)  # Provided by the PlatformIO/SCons build environment.