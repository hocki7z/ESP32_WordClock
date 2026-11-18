"""
PlatformIO build middleware to shorten compiler command-line length on Windows.

Windows CreateProcess enforces a hard 32,767-character limit on command-line
strings. Compiling ESP32 projects with large libraries (e.g., FastLED) often
exceeds this limit due to hundreds of long `-I` ESP-IDF SDK include paths.

This script collapses all include paths inside the ESP32 SDK include folder
into a single `-iprefix` base path paired with shorter relative `-iwithprefix/`
flags for both C/C++ (CCFLAGS) and Assembly (ASFLAGS) compilation.

Supports both:
- Arduino-ESP32 Core 2.x (`framework-arduinoespressif32/tools/sdk/<mcu>/include`)
- Arduino-ESP32 Core 3.x (`framework-arduinoespressif32-libs/<mcu>/include`)

Reference: https://github.com
"""

import os
from platformio import fs

# Import the active SCons construction environment provided by PlatformIO
Import("env")

# Detect the target MCU architecture (e.g., "esp32", "esp32s3", "esp32c3")
platform = env.PioPlatform()
target = env.BoardConfig().get("build.mcu", "esp32")

# 1. Check for Arduino Core 3.x separate libs package first
if "framework-arduinoespressif32-libs" in platform.packages:
    libs_pkg_dir = platform.get_package_dir("framework-arduinoespressif32-libs") or ""
    sdk_dir = os.path.join(libs_pkg_dir, target, "include")
elif "framework-arduinoespressif32" in platform.packages:
    # 2. Fall back to Arduino Core 2.x bundled SDK path
    framework_pkg_dir = platform.get_package_dir("framework-arduinoespressif32") or ""
    sdk_dir = os.path.join(framework_pkg_dir, "tools", "sdk", target, "include")
else:
    sdk_dir = ""

# Build the normalized path to the precompiled ESP-IDF SDK headers inside the framework
FRAMEWORK_SDK_DIR = fs.to_unix_path(sdk_dir) if os.path.isdir(sdk_dir) else ""

# Check if PlatformIO is only dumping project metadata for IDE IntelliSense
IS_INTEGRATION_DUMP = env.IsIntegrationDump()


def is_framework_subfolder(potential_subfolder):
    """Safely verify whether a path resides inside FRAMEWORK_SDK_DIR."""
    if not FRAMEWORK_SDK_DIR or not os.path.isabs(potential_subfolder):
        return False
    # Ensure both paths are on the same Windows drive letter before comparing
    if (
        os.path.splitdrive(FRAMEWORK_SDK_DIR)[0]
        != os.path.splitdrive(potential_subfolder)[0]
    ):
        return False
    return os.path.commonpath([FRAMEWORK_SDK_DIR]) == os.path.commonpath(
        [FRAMEWORK_SDK_DIR, potential_subfolder]
    )


def shorten_includes(env, node):
    """Rewrite SDK include paths on each C/C++ and Assembly object node."""
    # Leave include paths untouched during IDE IntelliSense indexing so VS Code
    # can resolve all header locations normally
    if IS_INTEGRATION_DUMP or not FRAMEWORK_SDK_DIR:
        return node

    includes = [fs.to_unix_path(inc) for inc in env.get("CPPPATH", [])]
    shortened_includes = []
    generic_includes = []

    for inc in includes:
        if is_framework_subfolder(inc):
            # Convert full SDK paths into relative paths prefixed by -iwithprefix/
            shortened_includes.append(
                "-iwithprefix/"
                + fs.to_unix_path(os.path.relpath(inc, FRAMEWORK_SDK_DIR))
            )
        else:
            # Keep project, Arduino core, and external library paths unchanged
            generic_includes.append(inc)

    # Skip modifying flags if no SDK paths matched
    if not shortened_includes:
        return node

    # Rebuild the target object with shortened prefix flags for both C/C++ and ASM
    return env.Object(
        node,
        CPPPATH=generic_includes,
        CCFLAGS=env["CCFLAGS"]
        + ["-iprefix", FRAMEWORK_SDK_DIR]
        + shortened_includes,
        ASFLAGS=env["ASFLAGS"]
        + ["-iprefix", FRAMEWORK_SDK_DIR]
        + shortened_includes,
    )


# Register the middleware to intercept object compilation steps
env.AddBuildMiddleware(shorten_includes)