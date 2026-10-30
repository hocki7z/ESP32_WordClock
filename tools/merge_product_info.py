"""Create a full-size flash image with the generated product information partition."""

from pathlib import Path
import subprocess

try:
    Import("env")  # type: ignore[name-defined]  # Provided by PlatformIO/SCons.
except NameError:
    env = None


FLASH_SIZE = 0x400000
BOOTLOADER_OFFSET = 0x1000
PARTITION_TABLE_OFFSET = 0x8000
APP_OFFSET = 0x10000
PRODUCTINFO_OFFSET = 0x3FF000


def _copy_image(image, source, offset, label):
    source = Path(source)
    if not source.is_file():
        raise FileNotFoundError("Required {} image not found: {}".format(label, source))

    data = source.read_bytes()
    if offset + len(data) > len(image):
        raise ValueError("{} image exceeds the 4 MiB flash layout".format(label))
    image[offset:offset + len(data)] = data


def _make_full_flash_image(target, source, build_env):
    build_dir = Path(build_env.subst("$BUILD_DIR"))
    app_image = Path(str(target[0]))
    image = bytearray(b"\xFF" * FLASH_SIZE)

    _copy_image(image, build_dir / "bootloader.bin", BOOTLOADER_OFFSET, "bootloader")
    _copy_image(image, build_dir / "partitions.bin", PARTITION_TABLE_OFFSET, "partition table")
    _copy_image(image, app_image, APP_OFFSET, "application")
    _copy_image(image, build_dir / "productinfo.bin", PRODUCTINFO_OFFSET, "product information")

    output = build_dir / "firmware_full.bin"
    output.write_bytes(image)
    print("Product info: assembled {} with record at flash address 0x{:06X}".format(
        output, PRODUCTINFO_OFFSET))


def _upload_productinfo(source, target, build_env):
    """Program only the product information sector, preserving NVS/filesystem data."""

    productinfo_image = Path(str(source[0]))
    command = [
        build_env.subst("$PYTHONEXE"),
        build_env.subst("$UPLOADER"),
        "--chip", "esp32",
    ]
    port = build_env.subst("$UPLOAD_PORT")
    if port and port != "$UPLOAD_PORT":
        command.extend(("--port", port))

    baud = build_env.subst("$UPLOAD_SPEED")
    if baud and baud != "$UPLOAD_SPEED":
        command.extend(("--baud", baud))

    flash_mode = build_env.subst("$FLASH_MODE")
    flash_freq = build_env.subst("$FLASH_FREQ")
    flash_size = build_env.subst("$FLASH_SIZE")
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
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", _make_full_flash_image)
    env.AddCustomTarget(
        name="upload_productinfo",
        dependencies="$BUILD_DIR/productinfo.bin",
        actions=_upload_productinfo,
        title="Upload product information",
        description="Write only the fixed product information flash sector",
        always_build=True,
    )