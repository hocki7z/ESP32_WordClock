"""Embed the OTA uploader JavaScript in a generated C++ header."""

from pathlib import Path

try:
    from jsmin import jsmin as jsminify
except ImportError as error:
    raise RuntimeError(
        "The OTA JavaScript build step requires jsmin. Install it with "
        "'python -m pip install -r tools/requirements-jsmin.txt' "
        "using the Python environment that runs PlatformIO."
    ) from error

Import("env")  # Provided by PlatformIO/SCons.

project_dir = Path(env.subst("$PROJECT_DIR"))
build_dir = Path(env.subst("$BUILD_DIR"))
source_path = project_dir / "tools" / "ota_uploader.js"
header_path = build_dir / "OTAUpdateCustomJs.h"
delimiter = "OTAJS"

source = source_path.read_text(encoding="utf-8")
javascript = jsminify(source)
raw_string_end = ")" + delimiter + '"'
if raw_string_end in javascript:
    raise ValueError("OTA uploader JavaScript contains the C++ raw-string delimiter")

header = (
    "/* Auto-generated from tools/ota_uploader.js using jsmin; do not edit. */\n"
    "#pragma once\n\n"
    "namespace OTAUpdateAssets {\n"
    + 'inline constexpr char kCustomJavaScript[] = R"' + delimiter + "(\n"
    + javascript
    + ")" + delimiter + '";\n'
    + "} // namespace OTAUpdateAssets\n"
)

header_path.parent.mkdir(parents=True, exist_ok=True)
header_path.write_text(header, encoding="utf-8", newline="\n")
env.Append(CPPPATH=[str(build_dir)])
print("OTA uploader: minified {} to {} characters and generated {}".format(
    len(source), len(javascript), header_path
))
