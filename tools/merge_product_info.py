"""Generate full-flash and application-plus-ProductInfo images."""

from pathlib import Path
import json
import subprocess
import zipfile

try:
    Import("env")  # type: ignore[name-defined]  # Provided by PlatformIO/SCons.
except NameError:
    env = None


FLASH_SIZE = 0x400000
BOOTLOADER_OFFSET = 0x1000
PARTITION_TABLE_OFFSET = 0x8000
APP_OFFSET = 0x10000
PRODUCTINFO_OFFSET = 0x3FF000


def _read_image(path, label):
    path = Path(path)
    if not path.is_file():
        raise FileNotFoundError("Required {} image not found: {}".format(label, path))
    return path.read_bytes()


def _place_image(image, path, offset, label):
    data = _read_image(path, label)
    if offset + len(data) > len(image):
        raise ValueError("{} image exceeds the 4 MiB flash layout".format(label))
    image[offset:offset + len(data)] = data


def _make_images(target, source, env):
    """Create full-flash and app+ProductInfo raw images after firmware.bin builds."""

    build_dir = Path(env.subst("$BUILD_DIR"))
    app_image = Path(str(target[0]))
    productinfo_image = build_dir / "productinfo.bin"

    # Full device image: bootloader, partition table, app and ProductInfo.
    full_image = bytearray(b"\xFF" * FLASH_SIZE)
    _place_image(full_image, build_dir / "bootloader.bin", BOOTLOADER_OFFSET, "bootloader")
    _place_image(full_image, build_dir / "partitions.bin", PARTITION_TABLE_OFFSET, "partition table")
    _place_image(full_image, app_image, APP_OFFSET, "application")
    _place_image(full_image, productinfo_image, PRODUCTINFO_OFFSET, "product information")
    full_path = build_dir / "firmware_full.bin"
    full_path.write_bytes(full_image)
    print("Product info: generated full-flash image {}".format(full_path))

    # App update image: same flash-address layout, but only app/ProductInfo bytes
    # are populated. Holes are 0xFF; this must not be blindly flashed as a full chip image.
    update_image = bytearray(b"\xFF" * (PRODUCTINFO_OFFSET + productinfo_image.stat().st_size))
    _place_image(update_image, app_image, APP_OFFSET, "application")
    _place_image(update_image, productinfo_image, PRODUCTINFO_OFFSET, "product information")
    update_path = build_dir / "firmware_productinfo_update.bin"
    update_path.write_bytes(update_image)
    print("Product info: generated app+ProductInfo update image {}".format(update_path))

    # Also provide a self-describing package with individual files/offsets for OTA or tooling.
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


def _upload_productinfo(target, source, env):
    """Program only the product information sector, preserving NVS/filesystem data."""

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
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", _make_images)
    env.AddCustomTarget(
        name="upload_productinfo",
        dependencies="$BUILD_DIR/productinfo.bin",
        actions=_upload_productinfo,
        title="Upload product information",
        description="Write only the fixed product information flash sector",
        always_build=True,
    )