"""Set firmware version macros from a clean Git-tagged source tree."""

import re
import subprocess

Import("env")

_VERSION_TAG = re.compile(r"^(\d{1,3})\.(\d{1,3})\.(\d{1,3})$")
_UNKNOWN_VERSION = (0xFF, 0xFF, 0xFF)
_SOURCE_PATHS = ("src", "include", "lib")


def _git(project_dir, *arguments):
    result = subprocess.run(
        ["git", "-C", project_dir, *arguments],
        check=True,
        capture_output=True,
        text=True,
    )
    return result.stdout.strip()


def _source_tree_is_clean(project_dir):
    status = _git(
        project_dir,
        "status",
        "--porcelain",
        "--untracked-files=all",
        "--",
        *_SOURCE_PATHS,
    )
    return not status


def _version_from_current_commit(project_dir):
    if not _source_tree_is_clean(project_dir):
        print("Build version: source changes detected; using 0xFF.0xFF.0xFF")
        return _UNKNOWN_VERSION

    tags = _git(project_dir, "tag", "--points-at", "HEAD").splitlines()
    for tag in sorted(tags):
        match = _VERSION_TAG.fullmatch(tag.strip())
        if match is None:
            continue

        version = tuple(int(component, 10) for component in match.groups())
        if all(component <= 0xFF for component in version):
            print("Build version: Git tag {} -> {}".format(
                tag,
                ".".join("0x{:02X}".format(component) for component in version),
            ))
            return version

    print("Build version: no matching tag at HEAD; using 0xFF.0xFF.0xFF")
    return _UNKNOWN_VERSION


_project_dir = env.subst("$PROJECT_DIR")
try:
    _version = _version_from_current_commit(_project_dir)
except (OSError, subprocess.CalledProcessError) as error:
    print("Build version: Git metadata unavailable ({}); using 0xFF.0xFF.0xFF".format(error))
    _version = _UNKNOWN_VERSION

env.Append(
    CPPDEFINES=[
        ("CONFIG_FW_VERSION_MAJOR", "0x{:02X}".format(_version[0])),
        ("CONFIG_FW_VERSION_MINOR", "0x{:02X}".format(_version[1])),
        ("CONFIG_FW_VERSION_PATCH", "0x{:02X}".format(_version[2])),
    ]
)
