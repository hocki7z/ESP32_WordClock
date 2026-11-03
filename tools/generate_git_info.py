"""Generate build metadata and AutoGitInfo.h before compiling.

Version tags are selected from reachable semantic-version tags using the
configured ``highest`` (default) or ``nearest`` strategy.
"""

from datetime import datetime, timezone
import argparse
import re
import subprocess
import zlib
from pathlib import Path

# PlatformIO pre-script support: import the build environment if available.
try:
    Import("env")           # type: ignore[name-defined]  # Provided by PlatformIO/SCons.
    _PLATFORMIO_ENV = env   # type: ignore[name-defined]  # Imported by the preceding SCons call.
except NameError:
    # Running as a normal Python script rather than a PlatformIO pre-script.
    _PLATFORMIO_ENV = None

# Regex pattern for tags that look like semantic version numbers, e.g. 1.2.3.
_VERSION_PATTERN        = re.compile(r"^(\d{1,3})\.(\d{1,3})\.(\d{1,3})$")

# Sentinel version bytes for when no suitable semantic-version tag exists.
_UNKNOWN_VERSION_STR    = "255.255.255"
_UNKNOWN_VERSION        = (0xFF, 0xFF, 0xFF)

# Constants for header generation.
_GENERATED_HEADER       = "AutoGitInfo.h"     # Generated in the active build directory.
_DEFINE_VALUE_COLUMN    = 49    # Column at which to align the value of each ``#define`` in the generated header.

# Constants for flash partition generation.
_PRODUCTINFO_PARTITION_SIZE = 0x1000


def _format_define(name, value):
    """Format one C preprocessor definition with its value at the target column.

    Args:
        name: Macro identifier, without the ``#define`` keyword.
        value: Text to place after the macro name.

    Returns:
        A formatted ``#define`` line.
    """

    prefix = "#define " + name
    return prefix.ljust(_DEFINE_VALUE_COLUMN - 1) + value

def _git(project_dir, *arguments, check=True):
    """Run a Git command in the project directory and return trimmed stdout."""

    result = subprocess.run(
        ["git", "-C", str(project_dir), *arguments],
        check=check,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )

    return result.stdout.strip()


def _git_version():
    """Return the version string reported by the installed Git executable."""

    result = subprocess.run(
        ["git", "--version"],
        check=True,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )

    return result.stdout.strip()


def _c_string(value):
    """Quote and escape text so it is safe to emit as a C string literal."""

    return '"{}"'.format(
        value.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
    )


def _version_tags(project_dir):
    """List all local tags that follow the supported software-version format.

    Tags that do not match ``MAJOR.MINOR.PATCH`` or contain components above
    255 are ignored. Reachability from ``HEAD`` is checked separately by
    :func:`_reachable_version_tags`.
    """

    candidates = []

    for tag in _git(project_dir, "tag", "--list").splitlines():
        match = _VERSION_PATTERN.fullmatch(tag.strip())
        if match is None:
            continue
        version = tuple(int(part, 10) for part in match.groups())
        if all(part <= 0xFF for part in version):
            candidates.append((tag, version))

    return candidates

def _reachable_version_tags(project_dir):
    """Return valid ancestor tags as ``(tag, version, commit_distance)`` tuples."""

    candidates = []
    for tag, version in _version_tags(project_dir):
        ancestor = subprocess.run(
            ["git", "-C", str(project_dir), "merge-base", "--is-ancestor", tag, "HEAD"],
            capture_output=True,
        )
        if ancestor.returncode == 1:
            continue
        if ancestor.returncode != 0:
            raise subprocess.CalledProcessError(
                ancestor.returncode, ancestor.args, ancestor.stdout, ancestor.stderr
            )

        distance = int(_git(project_dir, "rev-list", "--count", "{}..HEAD".format(tag)))
        candidates.append((tag, version, distance))

    return candidates


def _closest_tag(project_dir):
    """Return the nearest valid version tag that is an ancestor of ``HEAD``.

    The commit distance selects the nearest tag. If none is reachable, use the
    documented sentinel tag and all-0xFF version bytes.
    """

    candidates = _reachable_version_tags(project_dir)
    if not candidates:
        return _UNKNOWN_VERSION_STR, _UNKNOWN_VERSION

    # Prefer the nearest tag; at equal distance, prefer the higher version/tag.
    tag, version, _ = max(candidates, key=lambda candidate: (
        -candidate[2], candidate[1], candidate[0]
    ))
    return tag, version

def _highest_version_tag(project_dir):
    """Return the highest valid software-version tag reachable from ``HEAD``.

    When the same version appears more than once, prefer the nearest tag, then
    the lexicographically greatest tag name. If no valid ancestor tag exists,
    return the documented sentinel tag and all-0xFF version bytes.
    """

    candidates = _reachable_version_tags(project_dir)
    if not candidates:
        return _UNKNOWN_VERSION_STR, _UNKNOWN_VERSION

    tag, version, _ = max(candidates, key=lambda candidate: (
        candidate[1], -candidate[2], candidate[0]
    ))
    return tag, version


def _select_version_tag(project_dir, strategy):
    """Select a version tag using the configured ``highest`` or ``nearest`` strategy."""

    if strategy == "highest":
        return _highest_version_tag(project_dir)
    if strategy == "nearest":
        return _closest_tag(project_dir)
    raise ValueError(
        "Unsupported version-tag strategy {!r}; expected 'highest' or 'nearest'.".format(strategy)
    )


def _working_tree_modified(project_dir):
    """Report whether tracked project files have uncommitted changes."""

    status = _git(
        project_dir,
        "status",
        "--porcelain",
        "--untracked-files=no",     # intentionally excludes new/untracked files.
        "--",
        ".",
    )

    return bool(status)


def _collect_build_info(project_dir, tag_strategy="highest"):
    """Collect one Git metadata/timestamp snapshot using the requested tag policy."""

    now_local = datetime.now().astimezone()
    now_utc = now_local.astimezone(timezone.utc)
    tag, software_version = _select_version_tag(project_dir, tag_strategy)
    print("Git info: selected version tag {} using '{}' strategy".format(tag, tag_strategy))
    full_hash = _git(project_dir, "rev-parse", "HEAD")

    return {
        "git_version": _git_version(),
        "generated_local": "{}.{}.{}, {}".format(
            now_local.day, now_local.month, now_local.year, now_local.strftime("%H:%M:%S")
        ),
        "generated_utc": now_utc.isoformat(timespec="milliseconds").replace("+00:00", "Z"),
        "commit_count": int(_git(project_dir, "rev-list", "--count", "HEAD")),
        "working_tree_modified": _working_tree_modified(project_dir),
        "branch": _git(project_dir, "branch", "--show-current") or "HEAD",
        "full_hash": full_hash,
        "tag": tag,
        "software_version": software_version,
    }


def _fallback_build_info():
    """Return fallback metadata when Git information cannot be collected."""

    now_local = datetime.now().astimezone()
    now_utc = now_local.astimezone(timezone.utc)
    return {
        "git_version": "",
        "generated_local": "{}.{}.{}, {}".format(
            now_local.day, now_local.month, now_local.year, now_local.strftime("%H:%M:%S")
        ),
        "generated_utc": now_utc.isoformat(timespec="milliseconds").replace("+00:00", "Z"),
        "commit_count": 0,
        "working_tree_modified": False,
        "branch": "",
        "full_hash": "",
        "tag": _UNKNOWN_VERSION_STR,
        "software_version": _UNKNOWN_VERSION,
    }


def _generate_flash_record(build_info):
    """Generate the fixed-layout partition image from the shared metadata snapshot."""

    version = build_info["software_version"]
    build_time_bytes = build_info["generated_utc"].encode("ascii")
    commit_hash_short = build_info["full_hash"][:8].encode("ascii")
    branch_short = build_info["branch"][:20].encode("utf-8")

    build_time_bytes = build_time_bytes[:24].ljust(25, b"\0")
    commit_hash_short = commit_hash_short[:8].ljust(9, b"\0")
    branch_short = branch_short[:80].ljust(81, b"\0")
    record = (
        b"WCPRODIN"                         # 0..7: record magic
        + bytes((2,))                       # 8: record format version
        + bytes(version)                    # 9..11: software version
        + build_time_bytes                  # 12..36: NUL-terminated UTC ISO-8601 date/time
        + branch_short                      # 37..117: UTF-8 branch, max 20 chars, NUL-terminated
        + commit_hash_short                 # 118..126: NUL-terminated short commit hash
    )
    record += zlib.crc32(record).to_bytes(4, "little")  # 127..130: CRC-32
    if len(record) != 131:
        raise ValueError("Product info record must be exactly 131 bytes")

    image = bytearray(b"\xFF" * _PRODUCTINFO_PARTITION_SIZE)
    image[:len(record)] = record
    build_dir = Path(_PLATFORMIO_ENV.subst("$BUILD_DIR"))
    image_path = build_dir / "productinfo.bin"
    image_path.parent.mkdir(parents=True, exist_ok=True)
    image_path.write_bytes(image)
    print("Product info: generated {} (record at partition offset 0x0)".format(image_path))

    # Embed only the record in the application. The remaining partition bytes
    # are erased to 0xFF when the record is synchronized on first boot.
    header_lines = [
        "/* Auto-generated from productinfo.bin; do not edit. */",
        "#pragma once",
        "#include <cstdint>",
        "namespace ProductInfoNS { namespace FirmwareImage {",
        "alignas(4) static constexpr uint8_t mcData[] = {",
    ]
    for offset in range(0, len(record), 16):
        chunk = record[offset:offset + 16]
        header_lines.append("    " + ", ".join("0x{:02X}".format(value) for value in chunk) + ",")
    header_lines.extend(("};", "} } // namespace ProductInfoNS::FirmwareImage", ""))
    image_header_path = build_dir / "ProductInfoImage.h"
    image_header_path.write_text("\n".join(header_lines), encoding="ascii", newline="\n")
    _PLATFORMIO_ENV.Append(CPPPATH=[str(build_dir)])
    print("Product info: embedded {}-byte record generated {}".format(len(record), image_header_path))

    pass    # end of _generate_flash_record


def _make_header(build_info):
    """Render AutoGitInfo.h from a previously collected metadata snapshot."""

    software_version = build_info["software_version"]
    branch = build_info["branch"]
    full_hash = build_info["full_hash"]

    # Keep this mapping and ``comments`` in sync; insertion order controls header layout.
    values = {
        "AUTO_GIT_INFO_GIT_VERSION": _c_string(build_info["git_version"]),
        "AUTO_GIT_INFO_GEN_DATE": _c_string(build_info["generated_local"]),
        "AUTO_GIT_INFO_GEN_DATE_ISO_8601": _c_string(build_info["generated_utc"]),
        "AUTO_GIT_INFO_COMMITS_NUMBER": str(build_info["commit_count"]),
        "AUTO_GIT_INFO_WORKING_TREE_MODIFIED": "1" if build_info["working_tree_modified"] else "0",
        "AUTO_GIT_INFO_GIT_BRANCH": _c_string(branch),
        "AUTO_GIT_INFO_GIT_BRANCH_SHORT": _c_string(branch[:20]),
        "AUTO_GIT_INFO_COMMIT_HASH": _c_string(full_hash),
        "AUTO_GIT_INFO_COMMIT_HASH_SHORT": _c_string(full_hash[:8]),
        "AUTO_GIT_INFO_COMMIT_TAG": _c_string(build_info["tag"]),
        "AUTO_GIT_INFO_SW_VERSION": ",".join("0x{:02X}".format(part) for part in software_version),
        "AUTO_GIT_INFO_SW_VERSION_MAJOR": "0x{:02X}".format(software_version[0]),
        "AUTO_GIT_INFO_SW_VERSION_MINOR": "0x{:02X}".format(software_version[1]),
        "AUTO_GIT_INFO_SW_VERSION_BUILD": "0x{:02X}".format(software_version[2]),
    }

    comments = {
        "AUTO_GIT_INFO_GIT_VERSION": "Git version",
        "AUTO_GIT_INFO_GEN_DATE": "Local generation date",
        "AUTO_GIT_INFO_GEN_DATE_ISO_8601": "Local generation date in ISO format (UTC)",
        "AUTO_GIT_INFO_COMMITS_NUMBER": "Number of commits reachable from HEAD",
        "AUTO_GIT_INFO_WORKING_TREE_MODIFIED": "1 if tracked files are modified, otherwise 0",
        "AUTO_GIT_INFO_GIT_BRANCH": "Current Git branch",
        "AUTO_GIT_INFO_GIT_BRANCH_SHORT": "Current Git branch, limited to 20 characters",
        "AUTO_GIT_INFO_COMMIT_HASH": "Full commit hash",
        "AUTO_GIT_INFO_COMMIT_HASH_SHORT": "Short commit hash, limited to 8 hexadecimal characters",
        "AUTO_GIT_INFO_COMMIT_TAG": "Selected reachable XX.YY.ZZ tag, or 255.255.255 if none is valid",
        "AUTO_GIT_INFO_SW_VERSION": "Software version",
        "AUTO_GIT_INFO_SW_VERSION_MAJOR": "Software version major byte",
        "AUTO_GIT_INFO_SW_VERSION_MINOR": "Software version minor byte",
        "AUTO_GIT_INFO_SW_VERSION_BUILD": "Software version build byte",
    }

    # Generate the header lines.
    lines = [
        "/*",
        " * Auto-generated by tools/generate_git_info.py; do not edit.",
        " */",
        "#pragma once",
        "",
    ]
    for name, value in values.items():
        lines.extend(("/*  {}  */".format(comments[name]), _format_define(name, value), ""))

    return "\n".join(lines)


def _generate_git_info(project_dir, build_dir, build_env=None, tag_strategy="highest"):
    """Generate the build metadata header.

    Args:
        project_dir: Root directory of the PlatformIO project/repository.
        build_dir: Output directory for the generated header.
        build_env: Optional PlatformIO environment used to update the include path.
        tag_strategy: ``highest`` or ``nearest`` reachable version tag selection.

    Git command failures use explicit fallback metadata. An unsupported tag
    strategy is rejected rather than silently falling back.
    """

    # Resolve the project directory and determine the output header path.
    project_dir = Path(project_dir).resolve()
    header_path = Path(build_dir).resolve() / _GENERATED_HEADER
    if tag_strategy not in ("highest", "nearest"):
        raise ValueError(
            "Unsupported version-tag strategy {!r}; expected 'highest' or 'nearest'.".format(
                tag_strategy
            )
        )

    try:
        build_info = _collect_build_info(project_dir, tag_strategy)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        # If Git is unavailable, use one fallback snapshot for both outputs.
        detail = getattr(error, "stderr", None)
        detail = detail.strip() if detail else str(error)
        print("Git info: unable to read Git metadata: {}; using fallback metadata".format(detail))
        build_info = _fallback_build_info()

    content = _make_header(build_info)

    # Write the generated header to disk, creating the parent directory if necessary.
    header_path.parent.mkdir(parents=True, exist_ok=True)
    header_path.write_text(content, encoding="utf-8", newline="\n")
    print("Git info: generated {}".format(header_path))

    if build_env is not None:
        # Allow application sources to include the generated build-specific metadata.
        build_env.Append(CPPPATH=[str(header_path.parent)])

    if _PLATFORMIO_ENV is not None:
        _generate_flash_record(build_info)
        # PlatformIO's ESP32 uploader appends these images to its normal
        # esptool write_flash command; this makes regular Upload flash ProductInfo.
        _PLATFORMIO_ENV.Append(
            FLASH_EXTRA_IMAGES=[("0x3FF000", "$BUILD_DIR/productinfo.bin")]
        )

    pass    # end of _generate_git_info


def main(argv=None, project_dir=None, build_dir=None, tag_strategy=None):
    """Run generation either as a PlatformIO pre-script or as a CLI command.

    PlatformIO supplies project/build directories and the tag strategy from its
    build environment. A normal Python invocation can override these settings
    with ``--project-dir``, ``--build-dir`` and ``--tag-strategy``.
    """

    if project_dir is None and _PLATFORMIO_ENV is not None:
        project_dir = _PLATFORMIO_ENV.subst("$PROJECT_DIR")
    if build_dir is None and _PLATFORMIO_ENV is not None:
        build_dir = _PLATFORMIO_ENV.subst("$BUILD_DIR")
    if tag_strategy is None and _PLATFORMIO_ENV is not None:
        tag_strategy = _PLATFORMIO_ENV.GetProjectOption("custom_git_tag_strategy") or "highest"

    if project_dir is None:
        # Standalone invocation: default to the project containing this script.
        parser = argparse.ArgumentParser(description="Generate AutoGitInfo.h from Git metadata.")
        parser.add_argument(
            "--project-dir",
            default=Path(__file__).resolve().parents[1],
            type=Path,
            help="PlatformIO project root (defaults to the parent of this script's tools directory)",
        )
        parser.add_argument(
            "--build-dir",
            default=build_dir,
            type=Path,
            help="Header output directory (defaults to .pio/build/git-info-standalone under the project root)",
        )
        parser.add_argument(
            "--tag-strategy",
            choices=("highest", "nearest"),
            default=tag_strategy or "highest",
            help="Select the highest version tag (default) or nearest reachable tag",
        )
        arguments = parser.parse_args(argv)
        project_dir = arguments.project_dir
        build_dir = arguments.build_dir
        tag_strategy = arguments.tag_strategy

    project_dir = Path(project_dir).resolve()
    if build_dir is None:
        build_dir = project_dir / ".pio" / "build" / "git-info-standalone"
    tag_strategy = tag_strategy or "highest"

    # Generate the header file in the specified build directory.
    _generate_git_info(project_dir, build_dir, _PLATFORMIO_ENV, tag_strategy)

    pass    # end of main


# PlatformIO loads this file as a pre-script; normal Python runs use the CLI.
if _PLATFORMIO_ENV is not None:
    main(
        project_dir=_PLATFORMIO_ENV.subst("$PROJECT_DIR"),
        build_dir=_PLATFORMIO_ENV.subst("$BUILD_DIR"),
    )
elif __name__ == "__main__":
    main()
