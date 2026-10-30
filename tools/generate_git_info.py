"""Generate src/AutoGitInfo.h immediately before a PlatformIO build."""

from datetime import datetime, timezone
import argparse
import re
import subprocess
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

# Sentinel version bytes for when no valid tag is reachable from HEAD.
_UNKNOWN_VERSION_STR    = "255.255.255"
_UNKNOWN_VERSION        = (0xFF, 0xFF, 0xFF)

# Constants for header generation.
_GENERATED_HEADER       = "AutoGitInfo.h"     # Path("src") / "AutoGitInfo.h"
_DEFINE_VALUE_COLUMN    = 49    # Column at which to align the value of each ``#define`` in the generated header.


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
    """List tags formatted as three decimal byte-sized version components.

    Tags that do not match ``MAJOR.MINOR.PATCH`` or contain components above
    255 are ignored.
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


def _closest_tag(project_dir):
    """Return the nearest valid version tag that is an ancestor of ``HEAD``.

    The commit distance selects the nearest tag. If none is reachable, use the
    documented sentinel tag and all-0xFF version bytes.
    """

    nearest = None

    # Iterate over all valid version tags and find the nearest ancestor of HEAD.
    for tag, version in _version_tags(project_dir):
        # Ignore tags on other lines of development that are not ancestors of HEAD.
        ancestor = subprocess.run(
            ["git", "-C", str(project_dir), "merge-base", "--is-ancestor", tag, "HEAD"],
            capture_output=True,
        )
        if ancestor.returncode != 0:
            continue

        distance = int(_git(project_dir, "rev-list", "--count", "{}..HEAD".format(tag)))
        candidate = (distance, tag, version)
        if nearest is None or candidate < nearest:
            nearest = candidate

    # If no valid tag is reachable from HEAD, return the sentinel tag and version.
    if nearest is None:
        return _UNKNOWN_VERSION_STR, _UNKNOWN_VERSION

    _, tag, version = nearest

    return tag, version


def _working_tree_modified(project_dir):
    """Report changes to tracked files, excluding the generated header itself."""

    status = _git(
        project_dir,
        "status",
        "--porcelain",
        "--untracked-files=no",     # intentionally excludes new/untracked files.
        "--",
        ".",
        ":(exclude)src/AutoGitInfo.h",
    )

    return bool(status)


def _make_header(project_dir):
    """Collect Git/build metadata and render it as a C header string."""

    # Compute the current date/time once so all generated fields are consistent.
    now_local = datetime.now().astimezone()
    now_utc   = datetime.now(timezone.utc)

    # Compute values once so generated fields all describe the same build state.
    modified = _working_tree_modified(project_dir)
    tag, tag_version = _closest_tag(project_dir)
    software_version = tag_version

    # Compute the Git version string once so it is consistent across all generated fields.
    git_version = _git_version()
    commit_count = int(_git(project_dir, "rev-list", "--count", "HEAD"))
    branch = _git(project_dir, "branch", "--show-current") or "HEAD"
    full_hash = _git(project_dir, "rev-parse", "HEAD")
    commit_hash_bytes = bytes.fromhex(full_hash)

    # Keep this mapping and ``comments`` in sync; insertion order controls header layout.
    values = {
        "AUTO_GIT_INFO_GIT_VERSION": _c_string(git_version),
        "AUTO_GIT_INFO_GEN_DATE": _c_string(
            "{}.{}.{}, {}".format(now_local.day, now_local.month, now_local.year, now_local.strftime("%H:%M:%S"))
        ),
        "AUTO_GIT_INFO_GEN_DATE_ISO_8601": _c_string(now_utc.isoformat(timespec="milliseconds").replace("+00:00", "Z")),
        "AUTO_GIT_INFO_COMMITS_NUMBER": str(commit_count),
        "AUTO_GIT_INFO_WORKING_TREE_MODIFIED": "1" if modified else "0",
        "AUTO_GIT_INFO_GIT_BRANCH": _c_string(branch),
        "AUTO_GIT_INFO_COMMIT_FULL_HASH": _c_string(full_hash),
        "AUTO_GIT_INFO_COMMIT_SHORT_HASH": _c_string(full_hash[:8]),
        "AUTO_GIT_INFO_COMMIT_HASH_BYTES": ",".join("0x{:02X}".format(part) for part in commit_hash_bytes),
        "AUTO_GIT_INFO_COMMIT_TAG": _c_string(tag),
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
        "AUTO_GIT_INFO_WORKING_TREE_MODIFIED": "1 if the working tree has changes, otherwise 0",
        "AUTO_GIT_INFO_GIT_BRANCH": "Current Git branch",
        "AUTO_GIT_INFO_COMMIT_FULL_HASH": "Full Git hash",
        "AUTO_GIT_INFO_COMMIT_SHORT_HASH": "Short Git hash",
        "AUTO_GIT_INFO_COMMIT_HASH_BYTES": "Full Git hash as 20 binary bytes",
        "AUTO_GIT_INFO_COMMIT_TAG": "Nearest reachable XX.YY.ZZ tag, or 255.255.255 if no valid tag",
        "AUTO_GIT_INFO_SW_VERSION": "Software version",
        "AUTO_GIT_INFO_SW_VERSION_MAJOR": "Software version major byte",
        "AUTO_GIT_INFO_SW_VERSION_MINOR": "Software version minor byte",
        "AUTO_GIT_INFO_SW_VERSION_BUILD": "Software version build byte",
    }

    # Generate the header lines.
    lines = ["/*", " * Auto-generated by tools/generate_git_info.py; do not edit.", " */", "#pragma once", ""]
    for name, value in values.items():
        lines.extend(("/*  {}  */".format(comments[name]), _format_define(name, value), ""))

    return "\n".join(lines)


def _generate_git_info(project_dir):
    """Generate the metadata header, using explicit defaults if Git fails.

    Args:
        project_dir: Root directory of the PlatformIO project/repository.
    """

    # Resolve the project directory and determine the output header path.
    project_dir = Path(project_dir).resolve()
    header_path = project_dir / _GENERATED_HEADER

    try:
        content = _make_header(project_dir)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        # If Git is unavailable or the project is not a Git repository, generate a fallback header.

        # The error may have a "stderr" attribute (from subprocess.CalledProcessError)
        # or not (from OSError/ValueError).
        detail = getattr(error, "stderr", None)
        detail = detail.strip() if detail else str(error)
        print("Git info: unable to read Git metadata: {}; generating fallback header".format(detail))

        # Compute the current date/time once so all generated fields are consistent.
        now_local = datetime.now().astimezone()
        now_utc   = datetime.now(timezone.utc)

        # Keep the header usable by firmware builds even outside a Git checkout.
        fallback = {
            "AUTO_GIT_INFO_GIT_VERSION": '"n/a"',
            "AUTO_GIT_INFO_GEN_DATE": _c_string(
                "{}.{}.{}, {}".format(now_local.day, now_local.month, now_local.year, now_local.strftime("%H:%M:%S"))
            ),
            "AUTO_GIT_INFO_GEN_DATE_ISO_8601": _c_string(
                now_utc.isoformat(timespec="milliseconds").replace("+00:00", "Z")
            ),
            "AUTO_GIT_INFO_COMMITS_NUMBER": "0",
            "AUTO_GIT_INFO_WORKING_TREE_MODIFIED": "0",
            "AUTO_GIT_INFO_GIT_BRANCH": '"n/a"',
            "AUTO_GIT_INFO_COMMIT_FULL_HASH": '"n/a"',
            "AUTO_GIT_INFO_COMMIT_SHORT_HASH": '"n/a"',
            "AUTO_GIT_INFO_COMMIT_HASH_BYTES": ",".join("0x00" for _ in range(20)),
            "AUTO_GIT_INFO_COMMIT_TAG": '"255.255.255"',
            "AUTO_GIT_INFO_SW_VERSION": "0xFF,0xFF,0xFF",
            "AUTO_GIT_INFO_SW_VERSION_MAJOR": "0xFF",
            "AUTO_GIT_INFO_SW_VERSION_MINOR": "0xFF",
            "AUTO_GIT_INFO_SW_VERSION_BUILD": "0xFF",
        }

        # Generate a simple header with the fallback values, without comments.
        lines = ["/* Auto-generated by tools/generate_git_info.py; Git metadata unavailable. */", "#pragma once", ""]
        lines.extend(_format_define(name, value) for name, value in fallback.items())
        content = "\n".join(lines) + "\n"

    # Write the generated header to disk, creating the parent directory if necessary.
    header_path.parent.mkdir(parents=True, exist_ok=True)
    header_path.write_text(content, encoding="utf-8", newline="\n")
    print("Git info: generated {}".format(header_path))

    pass    # end of _generate_git_info


def main(argv=None, project_dir=None):
    """Run generation either as a PlatformIO pre-script or as a CLI command.

    PlatformIO supplies ``project_dir`` from its build environment. For a
    standalone run, parse the optional project-root argument from ``argv``.
    """

    if project_dir is None and _PLATFORMIO_ENV is not None:
        project_dir = _PLATFORMIO_ENV.subst("$PROJECT_DIR")

    if project_dir is None:
        # Standalone invocation: default to the project containing this script.
        parser = argparse.ArgumentParser(description="Generate src/AutoGitInfo.h from Git metadata.")
        parser.add_argument(
            "--project-dir",
            default=Path(__file__).resolve().parents[1],
            type=Path,
            help="PlatformIO project root (defaults to the parent of this script's tools directory)",
        )
        arguments = parser.parse_args(argv)
        project_dir = arguments.project_dir

    # Generate the header file in the specified project directory.
    _generate_git_info(project_dir)

    pass    # end of main


# PlatformIO loads this file as a pre-script; normal Python runs use the CLI.
if _PLATFORMIO_ENV is not None:
    main(project_dir=_PLATFORMIO_ENV.subst("$PROJECT_DIR"))
elif __name__ == "__main__":
    main()
