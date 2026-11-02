"""Minify and embed the OTA uploader JavaScript for a PlatformIO build.

Terser compresses the readable source and mangles local names. If its local npm
installation is missing, install the pinned project dependencies before
generating ``OTAUpdateCustomJs.h`` in PlatformIO's build directory.
"""

import argparse
from pathlib import Path
import shutil
import subprocess


# PlatformIO injects ``Import`` and ``env`` while loading this pre-build script.
try:
    Import("env")  # type: ignore[name-defined]  # Provided by PlatformIO/SCons.
    _PLATFORMIO_ENV = env  # type: ignore[name-defined]  # Imported by the preceding SCons call.
except NameError:
    # Running as a normal Python script rather than a PlatformIO pre-script.
    _PLATFORMIO_ENV = None


_SOURCE_RELATIVE_PATH = Path("tools") / "ota_uploader.js"
_HEADER_NAME = "OTAUpdateCustomJs.h"
_TERSER_RELATIVE_PATH = Path("node_modules") / "terser" / "bin" / "terser"
_RAW_STRING_DELIMITER = "OTAJS"
_TERSER_OPTIONS = ("--compress", "--mangle", "--ecma", "2015")


def _install_npm_dependencies(project_dir):
    """Install the project's pinned npm dependencies when Terser is missing.

    Use ``npm ci`` when a lockfile is available. On a first setup, ``npm
    install`` also creates the lockfile for subsequent reproducible builds.
    Lifecycle scripts are disabled because this project only needs Terser's
    JavaScript package and should not run dependency-provided install scripts.
    """

    if shutil.which("npm") is None:
        raise RuntimeError(
            "Terser is missing and npm was not found on PATH. Install Node.js/npm "
            "or install the project dependencies manually."
        )

    lockfile = project_dir / "package-lock.json"
    npm_arguments = "ci" if lockfile.is_file() else "install"
    command = "npm {} --no-audit --no-fund --ignore-scripts".format(npm_arguments)
    print("OTA uploader: Terser is missing; running npm {}".format(npm_arguments))
    # shell=True lets Windows execute npm.cmd; command text is fixed, not user input.
    subprocess.run(command, cwd=project_dir, shell=True, check=True)


def _minify_javascript(project_dir, source_path):
    """Run local Terser with compression and identifier mangling enabled."""

    node_path = shutil.which("node")
    if node_path is None:
        raise RuntimeError("Node.js is required to minify the OTA JavaScript with Terser")

    terser_path = project_dir / _TERSER_RELATIVE_PATH
    if not terser_path.is_file():
        _install_npm_dependencies(project_dir)

    if not terser_path.is_file():
        raise RuntimeError(
            "npm completed, but the local Terser executable was not found at {}".format(terser_path)
        )

    try:
        result = subprocess.run(
            [node_path, str(terser_path), str(source_path), *_TERSER_OPTIONS],
            check=True,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )
    except subprocess.CalledProcessError as error:
        detail = error.stderr.strip() if error.stderr else str(error)
        raise RuntimeError("Terser failed to minify the OTA uploader: {}".format(detail)) from error

    javascript = result.stdout.strip()
    if not javascript:
        raise RuntimeError("Terser returned empty output for the OTA uploader script")
    return javascript


def _make_header(javascript):
    """Wrap minified JavaScript in a C++ raw-string header."""

    raw_string_end = ")" + _RAW_STRING_DELIMITER + '"'
    if raw_string_end in javascript:
        raise ValueError("Minified JavaScript contains the C++ raw-string delimiter")

    return (
        "/* Auto-generated from tools/ota_uploader.js using Terser; do not edit. */\n"
        "#pragma once\n\n"
        "namespace OTAUpdateAssets {\n"
        + 'inline constexpr char kCustomJavaScript[] = R"' + _RAW_STRING_DELIMITER + "(\n"
        + javascript
        + ")" + _RAW_STRING_DELIMITER + '";\n'
        + "} // namespace OTAUpdateAssets\n"
    )


def _generate_header(project_dir, build_dir, build_env=None):
    """Minify the source asset and emit its generated build-directory header.

    When ``build_env`` is supplied, add the output directory to the compiler's
    include path so source files can include the generated header.
    """

    project_dir = Path(project_dir).resolve()
    build_dir = Path(build_dir).resolve()
    source_path = project_dir / _SOURCE_RELATIVE_PATH
    header_path = build_dir / _HEADER_NAME

    if not source_path.is_file():
        raise FileNotFoundError("OTA uploader source file not found: {}".format(source_path))

    source = source_path.read_text(encoding="utf-8")
    javascript = _minify_javascript(project_dir, source_path)
    header_path.parent.mkdir(parents=True, exist_ok=True)
    header_path.write_text(_make_header(javascript), encoding="utf-8", newline="\n")

    if build_env is not None:
        # Make the generated header visible to the C++ compiler for this build.
        build_env.Append(CPPPATH=[str(build_dir)])
    print("OTA uploader: Terser minified {} to {} characters; generated {}".format(
        len(source), len(javascript), header_path
    ))


def main(argv=None, project_dir=None, build_dir=None, platformio_env=None):
    """Generate the embedded JavaScript header as a PlatformIO hook or CLI.

    PlatformIO supplies the project and build directories through its SCons
    environment. A normal Python invocation can override those paths with the
    ``--project-dir`` and ``--build-dir`` command-line arguments.
    """

    if platformio_env is None:
        platformio_env = _PLATFORMIO_ENV

    if platformio_env is not None:
        project_dir = project_dir or platformio_env.subst("$PROJECT_DIR")
        build_dir = build_dir or platformio_env.subst("$BUILD_DIR")
    else:
        parser = argparse.ArgumentParser(
            description="Minify the OTA JavaScript and generate its C++ header."
        )
        default_project_dir = Path(__file__).resolve().parents[1]
        parser.add_argument(
            "--project-dir",
            default=project_dir or default_project_dir,
            type=Path,
            help="PlatformIO project root (defaults to the parent of this script's tools directory)",
        )
        parser.add_argument(
            "--build-dir",
            default=build_dir,
            type=Path,
            help=(
                "Directory for the generated header (defaults to "
                ".pio/build/ota-js-standalone under the project root)"
            ),
        )
        arguments = parser.parse_args(argv)
        project_dir = arguments.project_dir
        build_dir = arguments.build_dir

    project_dir = Path(project_dir).resolve()
    if build_dir is None:
        build_dir = project_dir / ".pio" / "build" / "ota-js-standalone"

    _generate_header(project_dir, build_dir, platformio_env)


# PlatformIO loads this file as a pre-script; normal Python runs use the CLI.
if _PLATFORMIO_ENV is not None:
    main(platformio_env=_PLATFORMIO_ENV)
elif __name__ == "__main__":
    main()
