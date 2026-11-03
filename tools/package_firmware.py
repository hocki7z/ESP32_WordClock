"""Create firmware packages that include the fixed ProductInfo flash sector.

This file is loaded by PlatformIO as a SCons post-build script. On every
build, after the application binary is ready, it creates a complete 4 MiB flash image, an
offset-addressed app-plus-ProductInfo image, a single-file OTA app image, and
a ZIP package with the component images and their flash offsets. It also adds
an ``upload_productinfo`` target that programs only the ProductInfo sector.

The generated raw offset images contain 0xFF in unpopulated address ranges;
they must be flashed using the intended offsets and must not be mistaken for
a generic application-only OTA image.

It can also be run directly after a PlatformIO build. The standalone command
uses ``--project-dir`` to locate the project and can take ``--build-dir`` and
``--app-image`` when the build artifacts are not in the default location.
"""

from pathlib import Path
import argparse
import json
import shutil
import subprocess
import sys
import zipfile
import zlib

try:
    Import("env")  # type: ignore[name-defined]  # Provided by PlatformIO/SCons.
except NameError:
    env = None

# SCons does not add the extra-script directory to Python's import path.
# Standalone execution already includes it.
if env is not None:
    tools_dir = str(Path(env.subst("$PROJECT_DIR")) / "tools")
    if tools_dir not in sys.path:
        sys.path.insert(0, tools_dir)

import productinfo_format

# ESP32 4 MiB flash layout used by the WordClock custom partition table.
# Offsets are absolute flash addresses; PRODUCTINFO_OFFSET reserves the final
# 4 KiB sector for the padded ProductInfo partition image.
FLASH_SIZE              = 0x400000      # 4 MiB
BOOTLOADER_OFFSET       = 0x1000        # Bootloader load address
PARTITION_TABLE_OFFSET  = 0x8000        # Partition table load address
APP_OFFSET              = 0x10000       # Application slot start address
PRODUCTINFO_OFFSET      = 0x3FF000      # Final 4 KiB ProductInfo partition start


def _read_image(path, label):
    """Read a required build artifact or raise an informative missing-file error.

    Args:
        path: Filesystem path of the binary image to read.
        label: Human-readable image name used in any error message.

    Returns:
        The complete image contents as bytes.

    Raises:
        FileNotFoundError: If the required image does not exist.
    """
    path = Path(path)
    if not path.is_file():
        raise FileNotFoundError("Required {} image not found: {}".format(label, path))
    return path.read_bytes()


def _place_image(image, path, offset, label):
    """Copy an image into a flash-sized byte array at its absolute offset.

    The destination is modified in place. Unoccupied locations retain their
    existing contents (the output builders initialize them to erased-flash
    value 0xFF).

    Args:
        image: Mutable byte array representing the output flash image.
        path: Source binary image path.
        offset: Absolute flash address at which to place the source image.
        label: Human-readable name used if the image does not fit.

    Raises:
        FileNotFoundError: If the source image is missing.
        ValueError: If the image would extend beyond the destination buffer.
    """
    data = _read_image(path, label)
    if offset + len(data) > len(image):
        raise ValueError("{} image exceeds the 4 MiB flash layout".format(label))
    image[offset:offset + len(data)] = data

    pass    # end of _place_image


def _productinfo_version(path):
    """Validate the ProductInfo v3 record and return its version tuple.

    The record is 139 bytes: identifier and format version at offsets 0-16,
    the three-byte version at offsets 17-19, and a little-endian CRC-32 at
    offsets 135-138. The preceding fields are covered by the CRC.

    Args:
        path: Path to the generated ProductInfo partition image.

    Returns:
        A tuple containing the major, minor, and build version bytes.

    Raises:
        FileNotFoundError: If the ProductInfo image is missing.
        ValueError: If the record is truncated, has an unsupported signature
            or format version, or fails CRC validation.
    """

    data = _read_image(path, "product information")
    # SCons does not define __file__; use its project directory during builds.
    # Direct CLI execution supplies __file__ normally.
    project_dir = (Path(env.subst("$PROJECT_DIR")) if env is not None
                   else Path(__file__).resolve().parents[1])
    magic, format_version = productinfo_format.load_format(project_dir)
    record_size = 139
    if len(data) < record_size or data[:16] != magic or data[16] != format_version:
        raise ValueError("ProductInfo image has an unsupported or invalid record")

    record = data[:record_size]
    stored_crc = int.from_bytes(record[135:139], "little")
    if zlib.crc32(record[:135]) != stored_crc:
        raise ValueError("ProductInfo image CRC validation failed")

    return tuple(record[17:20])


def _make_images(target, source, env):
    """Generate packages after the packaging target's build dependencies are ready."""

    build_dir = Path(env.subst("$BUILD_DIR"))
    project_name = Path(env.subst("$PROJECT_DIR")).name
    app_image = Path(env.subst("$BUILD_DIR/${PROGNAME}.bin"))
    _generate_images(build_dir, project_name, app_image)

    pass    # end of _make_images


def _generate_images(build_dir, project_name, app_image):
    """Generate firmware image variants and a ZIP package from build artifacts.

    Args:
        build_dir: Directory containing ``bootloader.bin``, ``partitions.bin``,
            and ``productinfo.bin``.
        project_name: Name used to prefix versioned image filenames.
        app_image: Path to the application binary to package.

    Outputs in the PlatformIO build directory:
      * ``firmware_full.bin`` and a project/version-named copy: complete 4 MiB
        flash image with bootloader, partition table, application, and ProductInfo.
      * ``firmware_productinfo_update.bin``: offset-addressed application and
        ProductInfo data only, padded with 0xFF between their flash addresses.
      * A project/version-named ``*_ota_*.bin``: application image for ordinary
        application OTA; ProductInfo is synchronized by firmware at boot.
      * ``firmware_productinfo_update.zip``: component binaries and a manifest
        describing their flash offsets.

    The offset-addressed update image is not a full-flash image and should not
    be blindly uploaded as one. Use the ZIP manifest or the matching upload
    method to preserve the intended flash addresses.
    """

    build_dir = Path(build_dir)
    app_image = Path(app_image)
    productinfo_image = build_dir / "productinfo.bin"
    version = _productinfo_version(productinfo_image)
    version_text = "_".join(str(component) for component in version)

    # Build a full-chip image from erased-flash bytes, then overlay every image
    # at its partition-table address. Unlisted regions remain 0xFF.
    full_image = bytearray(b"\xFF" * FLASH_SIZE)
    _place_image(full_image, build_dir / "bootloader.bin", BOOTLOADER_OFFSET, "bootloader")
    _place_image(full_image, build_dir / "partitions.bin", PARTITION_TABLE_OFFSET, "partition table")
    _place_image(full_image, app_image, APP_OFFSET, "application")
    _place_image(full_image, productinfo_image, PRODUCTINFO_OFFSET, "product information")
    full_path = build_dir / "firmware_full.bin"
    full_path.write_bytes(full_image)
    print("Product info: generated full-flash image {}".format(full_path))

    versioned_full_path = build_dir / "{}_{}.bin".format(project_name, version_text)
    # Remove old version-tagged project binaries before writing this build's copy.
    for stale_path in build_dir.glob("{}_*.bin".format(project_name)):
        stale_path.unlink()
    shutil.copyfile(full_path, versioned_full_path)
    print("Product info: generated versioned full-flash image {}".format(versioned_full_path))

    # Keep absolute flash offsets for the application and ProductInfo data. The
    # gap remains 0xFF; this is an offset update artifact, not a full-chip image.
    update_image = bytearray(b"\xFF" * (PRODUCTINFO_OFFSET + productinfo_image.stat().st_size))
    _place_image(update_image, app_image, APP_OFFSET, "application")
    _place_image(update_image, productinfo_image, PRODUCTINFO_OFFSET, "product information")
    update_path = build_dir / "firmware_productinfo_update.bin"
    update_path.write_bytes(update_image)
    print("Product info: generated app+ProductInfo update image {}".format(update_path))

    # The app embeds its ProductInfo record and restores the partition at boot,
    # so the application binary alone is sufficient for ordinary app OTA.
    ota_path = build_dir / "{}_ota_{}.bin".format(project_name, version_text)
    for stale_path in build_dir.glob("{}_ota_*.bin".format(project_name)):
        stale_path.unlink()
    (build_dir / "firmware_productinfo_ota.bin").unlink(missing_ok=True)
    shutil.copyfile(app_image, ota_path)
    print("Product info: generated single-file OTA image {}".format(ota_path))

    # Package individual binaries with explicit offsets for tools that support
    # multi-image flash updates. The manifest prevents callers guessing offsets.
    manifest = {
        "format": "esp32-offset-update-v1",
        "description": "Application and ProductInfo images only; bootloader and partition table are not included.",
        "images": [
            {"file": "firmware.bin", "flash_offset": APP_OFFSET},
            {"file": "productinfo.bin", "flash_offset": PRODUCTINFO_OFFSET},
        ],
    }
    package_path = build_dir / "firmware_productinfo_update.zip"
    with zipfile.ZipFile(package_path, "w", compression=zipfile.ZIP_DEFLATED) as package:
        package.write(app_image, arcname="firmware.bin")
        package.write(productinfo_image, arcname="productinfo.bin")
        package.writestr("manifest.json", json.dumps(manifest, indent=2) + "\n")
    print("Product info: generated update package {}".format(package_path))

    pass    # end of _generate_images


def main(argv=None):
    """Build the firmware packages from an existing PlatformIO build.

    When ``--build-dir`` is omitted, the script looks for exactly one build
    directory under ``<project>/.pio/build`` containing the required partition
    artifacts. Use ``--app-image`` to select an application binary with a
    non-default name or location.
    """

    project_default = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(
        description="Create full-flash, OTA, and ProductInfo update packages from a PlatformIO build."
    )
    parser.add_argument(
        "--project-dir",
        type=Path,
        default=project_default,
        help="PlatformIO project root (defaults to the parent of this script's tools directory)",
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        help="PlatformIO environment build directory (auto-detected when exactly one is available)",
    )
    parser.add_argument(
        "--app-image",
        type=Path,
        help="Application binary path (defaults to firmware.bin inside the build directory)",
    )
    arguments = parser.parse_args(argv)

    project_dir = arguments.project_dir.resolve()
    build_dir = arguments.build_dir
    if build_dir is None:
        platformio_build_dir = project_dir / ".pio" / "build"
        required_artifacts = ("bootloader.bin", "partitions.bin", "productinfo.bin")
        candidates = [
            candidate
            for candidate in (platformio_build_dir, *platformio_build_dir.glob("*"))
            if candidate.is_dir() and all((candidate / name).is_file() for name in required_artifacts)
        ]
        if len(candidates) != 1:
            parser.error(
                "could not uniquely identify a PlatformIO build directory; "
                "pass --build-dir (found {} candidates)".format(len(candidates))
            )
        build_dir = candidates[0]
    else:
        build_dir = build_dir.resolve()

    app_image = arguments.app_image or (build_dir / "firmware.bin")
    _generate_images(build_dir, project_dir.name, app_image)

    pass    # end of main


def _upload_productinfo(target, source, env):
    """Flash only the ProductInfo partition using the configured esptool.

    The source is the padded partition binary, so this target does not touch
    the application, NVS, SPIFFS, bootloader, or partition table. Port, baud,
    and flash settings come from the active PlatformIO environment, with
    sensible ESP32 defaults if a setting is unavailable.

    Args:
        target: SCons target list (unused; required by the action interface).
        source: SCons source list; its first entry is ``productinfo.bin``.
        env: Active PlatformIO/SCons environment used to resolve tool settings.

    Returns:
        The esptool process exit code (zero indicates success).
    """

    productinfo_image = Path(str(source[0]))
    command = [
        env.subst("$PYTHONEXE"),
        env.subst("$UPLOADER"),
        "--chip", "esp32",
    ]
    port = env.subst("$UPLOAD_PORT")
    if port and port != "$UPLOAD_PORT":
        command.extend(("--port", port))

    baud = env.subst("$UPLOAD_SPEED")
    if baud and baud != "$UPLOAD_SPEED":
        command.extend(("--baud", baud))

    flash_mode = env.subst("$FLASH_MODE")
    flash_freq = env.subst("$FLASH_FREQ")
    flash_size = env.subst("$FLASH_SIZE")
    command.extend((
        "--before", "default_reset", "--after", "hard_reset", "write_flash", "-z",
        "--flash_mode", flash_mode if flash_mode != "$FLASH_MODE" else "qio",
        "--flash_freq", flash_freq if flash_freq != "$FLASH_FREQ" else "40m",
        "--flash_size", flash_size if flash_size != "$FLASH_SIZE" else "4MB",
        hex(PRODUCTINFO_OFFSET), str(productinfo_image),
    ))
    print("Product info: writing only flash sector at 0x{:06X}".format(PRODUCTINFO_OFFSET))
    return subprocess.call(command)


if env is not None:
    # A binary post-action runs only when SCons rebuilds that binary. Use an
    # always-built alias so incremental builds also refresh the packages,
    # without forcing the application or other input images to rebuild.
    package_images = env.Alias(
        "package_productinfo",
        [
            "$BUILD_DIR/${PROGNAME}.bin",
            "$BUILD_DIR/bootloader.bin",
            "$BUILD_DIR/partitions.bin",
            "$BUILD_DIR/productinfo.bin",
        ],
        _make_images,
    )
    env.AlwaysBuild(package_images)
    env.Default(package_images)
    env.Depends("buildprog", package_images)
    env.Depends("upload", package_images)
    # Expose an explicit PlatformIO target for updating metadata without
    # reflashing firmware or disturbing user settings and filesystem contents.
    env.AddCustomTarget(
        name="upload_productinfo",
        dependencies="$BUILD_DIR/productinfo.bin",
        actions=_upload_productinfo,
        title="Upload product information",
        description="Write only the fixed product information flash sector",
        always_build=True,
    )
elif __name__ == "__main__":
    main()