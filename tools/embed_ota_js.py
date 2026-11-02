"""Embed the OTA uploader JavaScript in a generated C++ header."""

from pathlib import Path

Import("env")  # Provided by PlatformIO/SCons.

project_dir = Path(env.subst("$PROJECT_DIR"))
build_dir = Path(env.subst("$BUILD_DIR"))
source_path = project_dir / "tools" / "ota_uploader.min.js"
header_path = build_dir / "OTAUpdateCustomJs.h"
delimiter = "OTAJS"

javascript = source_path.read_text(encoding="utf-8")
raw_string_end = ")" + delimiter + '"'
if raw_string_end in javascript:
    raise ValueError("OTA uploader JavaScript contains the C++ raw-string delimiter")

header = (
    "/* Auto-generated from tools/ota_uploader.min.js; do not edit. */\n"
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
print("OTA uploader: generated {}".format(header_path))
