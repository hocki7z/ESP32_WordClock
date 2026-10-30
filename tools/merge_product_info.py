"""Package the application and generated ProductInfo partition for updates."""

from pathlib import Path
import json
import subprocess
import zipfile

try:
    Import("env")  # type: ignore[name-defined]  # Provided by PlatformIO/SCons.
except NameError:
    env = None


APP_OFFSET = 0x10000
PRODUCTINFO_OFFSET = 0x3FF000


def _make_update_package(target, source, env):
    """Bundle only the app and ProductInfo partition images with their offsets."""

    build_dir = Path(env.subst("$BUILD_DIR"))
    app_image = Path(str(target[0]))
    productinfo_image = build_dir / "productinfo.bin"
    for label, image_path in (("application", app_image), ("product information", productinfo_image)):
        if not image_path.is_file():
            raise FileNotFoundError("Required {} image not found: {}".format(label, image_path))

    manifest = {
        "format": "esp32-offset-update-v1",
        "description": "Application and ProductInfo images only; bootloader and partition table are not included.",
        "images": [
            {"file": "firmware.bin", "flash_offset": APP_OFFSET},
            {"file": "productinfo.bin", "flash_offset": PRODUCTINFO_OFFSET},
        ],
    }
    output = build_dir / "firmware_productinfo_update.zip"
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED) as package:
        package.write(app_image, arcname="firmware.bin")
        package.write(productinfo_image, arcname="productinfo.bin")
        package.writestr("manifest.json", json.dumps(manifest, indent=2) + "\n")

    print("Product info: packaged application and ProductInfo only in {}".format(output))


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
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", _make_update_package)
    env.AddCustomTarget(
        name="upload_productinfo",
        dependencies="$BUILD_DIR/productinfo.bin",
        actions=_upload_productinfo,
        title="Upload product information",
        description="Write only the fixed product information flash sector",
        always_build=True,
    )