"""Create firmware packages with the ProductInfo record embedded in the application.

This file is loaded by PlatformIO as a SCons post-build script. On every
build, after the application binary is ready, it creates a complete 4 MiB
flash image, a single-file OTA application image, and a ZIP containing the
application image and an update manifest.

The full-flash image contains 0xFF in unpopulated address ranges and is
intended for flashing at address zero, not for application-only OTA updates.

It can also be run directly after a PlatformIO build. The standalone command
uses ``--project-dir`` to locate the project and can take ``--build-dir`` and
``--app-image`` when the build artifacts are not in the default location.
"""

from pathlib import Path
import argparse
import json
import shutil
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

# ESP32 4 MiB flash layout used by the WordClock partition table.
# The bootloader and partition-table offsets follow the ESP32 flash layout;
# APP_OFFSET matches the app0 start (0x10000) in the selected min_spiffs.csv.
FLASH_SIZE              = 0x400000      # 4 MiB
BOOTLOADER_OFFSET       = 0x1000        # Bootloader load address
PARTITION_TABLE_OFFSET  = 0x8000        # Partition table load address
APP_OFFSET              = 0x10000       # Application slot start address


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


def _productinfo_version(app_path):
    """Find and validate the embedded ProductInfo record in an application image.

    The record is 139 bytes: identifier and format version at offsets 0-16,
    the three-byte version at offsets 17-19, and a little-endian CRC-32 at
    offsets 135-138. The preceding fields are covered by the CRC.

    Args:
        app_path: Path to the application binary containing the embedded record.

    Returns:
        A tuple containing the major, minor, and build version bytes.

    Raises:
        FileNotFoundError: If the application image is missing.
        ValueError: If no embedded record has the expected identifier, format
            version, and CRC.
    """

    data = _read_image(app_path, "application")
    # SCons does not define __file__; use its project directory during builds.
    # Direct CLI execution supplies __file__ normally.
    project_dir = (Path(env.subst("$PROJECT_DIR")) if env is not None
                   else Path(__file__).resolve().parents[1])
    magic, format_version = productinfo_format.load_format(project_dir)
    record_size = 139
    search_offset = 0
    while True:
        record_offset = data.find(magic, search_offset)
        if record_offset < 0:
            break
        record_end = record_offset + record_size
        if record_end <= len(data):
            record = data[record_offset:record_end]
            stored_crc = int.from_bytes(record[135:139], "little")
            if (record[16] == format_version and
                    zlib.crc32(record[:135]) == stored_crc):
                return tuple(record[17:20])
        search_offset = record_offset + 1

    raise ValueError("No valid embedded ProductInfo record found in application image")


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
        build_dir: Directory containing ``bootloader.bin`` and ``partitions.bin``.
        project_name: Name used to prefix versioned image filenames.
        app_image: Path to the application binary to package.

        Outputs in the PlatformIO build directory:
            * A project/version-named complete 4 MiB flash image containing the
                bootloader, partition table, and application. All unpopulated flash
                regions remain erased (`0xFF`).
            * A project/version-named `*_OTA_*.bin` application image for OTA updates.
            * `<project>_Update_<version>.zip` containing the application image and a
                manifest with its flash offset.

        The full-flash image is intended for flashing at address zero, not OTA. The
        ZIP contains only the application image. The ProductInfo record is embedded
        in the application and read directly by the firmware.
    """

    build_dir = Path(build_dir)
    app_image = Path(app_image)
    version = _productinfo_version(app_image)
    version_text = "_".join(str(component) for component in version)

    # Build a full-chip image from erased-flash bytes, then overlay every image
    # at its partition-table address. Unlisted regions remain 0xFF.
    full_image = bytearray(b"\xFF" * FLASH_SIZE)
    _place_image(full_image, build_dir / "bootloader.bin", BOOTLOADER_OFFSET, "bootloader")
    _place_image(full_image, build_dir / "partitions.bin", PARTITION_TABLE_OFFSET, "partition table")
    _place_image(full_image, app_image, APP_OFFSET, "application")
    full_path = build_dir / "firmware_full.bin"
    full_path.write_bytes(full_image)
    print("Product info: generated full-flash image {}".format(full_path))

    versioned_full_path = build_dir / "{}_{}.bin".format(project_name, version_text)
    # Remove old version-tagged project binaries before writing this build's copy.
    for stale_path in build_dir.glob("{}_*.bin".format(project_name)):
        stale_path.unlink()
    full_path.replace(versioned_full_path)
    print("Product info: generated versioned full-flash image {}".format(versioned_full_path))

    # The app embeds its ProductInfo record and restores the partition at boot,
    # so the application binary alone is sufficient for ordinary app OTA.
    ota_path = build_dir / "{}_OTA_{}.bin".format(project_name, version_text)
    for stale_path in build_dir.glob("{}_OTA_*.bin".format(project_name)):
        stale_path.unlink()
    (build_dir / "firmware_productinfo_OTA.bin").unlink(missing_ok=True)
    shutil.copyfile(app_image, ota_path)
    print("Product info: generated single-file OTA image {}".format(ota_path))

    # Package the application with its embedded ProductInfo record.
    manifest = {
        "format": "esp32-application-update-v1",
        "description": "Application image with embedded ProductInfo; bootloader and partition table are not included.",
        "images": [
            {"file": "firmware.bin", "flash_offset": APP_OFFSET},
        ],
    }
    package_path = build_dir / "{}_Update_{}.zip".format(project_name, version_text)
    # Keep only this build's update package and remove the old fixed-name ZIP.
    for stale_path in build_dir.glob("{}_Update_*.zip".format(project_name)):
        stale_path.unlink()
    (build_dir / "firmware_productinfo_update.zip").unlink(missing_ok=True)
    with zipfile.ZipFile(package_path, "w", compression=zipfile.ZIP_DEFLATED) as package:
        package.write(app_image, arcname="firmware.bin")
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
        description="Create full-flash and application OTA packages with embedded ProductInfo from a PlatformIO build."
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
        required_artifacts = ("bootloader.bin", "partitions.bin")
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
        ],
        _make_images,
    )
    env.AlwaysBuild(package_images)
    env.Default(package_images)
    env.Depends("buildprog", package_images)
    env.Depends("upload", package_images)
elif __name__ == "__main__":
    main()