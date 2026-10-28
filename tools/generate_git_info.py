"""Generate src/AutoGitInfo.h immediately before a PlatformIO build."""

from datetime import datetime, timezone
import argparse
import re
import subprocess
from pathlib import Path

try:
    Import("env")
    _platformio_env = env
except NameError:
    # Running as a normal Python script rather than a PlatformIO pre-script.
    _platformio_env = None

_VERSION_PATTERN = re.compile(r"^(\d{1,3})\.(\d{1,3})\.(\d{1,3})$")
_UNKNOWN_VERSION = (0xFF, 0xFF, 0xFF)
_UNKNOWN_TICKS = 0xFFFFFFFF
_GENERATED_HEADER = Path("src") / "AutoGitInfo.h"


def _git(project_dir, *arguments, check=True):
    return subprocess.run(
        ["git", "-C", str(project_dir), *arguments],
        check=check,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    ).stdout.strip()


def _git_version():
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
    return '"{}"'.format(
        value.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
    )


def _version_tags(project_dir):
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
    """Return the closest reachable valid version tag and commits since it."""
    nearest = None
    for tag, version in _version_tags(project_dir):
        ancestor = subprocess.run(
            ["git", "-C", str(project_dir), "merge-base", "--is-ancestor", tag, "HEAD"],
            capture_output=True,
        )
        if ancestor.returncode != 0:
            continue

        ticks = int(_git(project_dir, "rev-list", "--count", "{}..HEAD".format(tag)))
        candidate = (ticks, tag, version)
        if nearest is None or candidate < nearest:
            nearest = candidate

    if nearest is None:
        return "", _UNKNOWN_VERSION, _UNKNOWN_TICKS

    ticks, tag, version = nearest
    return tag, version, ticks


def _working_tree_modified(project_dir):
    # Exclude this generated file so regenerating it does not mark its own build dirty.
    status = _git(
        project_dir,
        "status",
        "--porcelain",
        "--untracked-files=all",
        "--",
        ".",
        ":(exclude)src/AutoGitInfo.h",
    )
    return bool(status)


def _make_header(project_dir):
    now_local = datetime.now().astimezone()
    now_utc = datetime.now(timezone.utc)
    modified = _working_tree_modified(project_dir)
    tag, tag_version, ticks = _closest_tag(project_dir)
    software_version = tag_version

    git_version = _git_version()
    commit_count = int(_git(project_dir, "rev-list", "--count", "HEAD"))
    branch = _git(project_dir, "branch", "--show-current") or "HEAD"
    full_hash = _git(project_dir, "rev-parse", "HEAD")

    values = {
        "AUTO_GIT_INFO_GIT_VERSION": _c_string(git_version),
        "AUTO_GIT_INFO_COMMITS_NUMBER": str(commit_count),
        "AUTO_GIT_INFO_ISO_8601_DATE": _c_string(now_utc.isoformat(timespec="milliseconds").replace("+00:00", "Z")),
        "AUTO_GIT_INFO_LOCALE_DATE": _c_string(
            "{}.{}.{}, {}".format(now_local.day, now_local.month, now_local.year, now_local.strftime("%H:%M:%S"))
        ),
        "AUTO_GIT_INFO_GIT_BRANCH": _c_string(branch),
        "AUTO_GIT_INFO_TAG": _c_string(tag),
        "AUTO_GIT_INFO_SW_VERSION": ",".join("0x{:02X}".format(part) for part in software_version),
        "AUTO_GIT_INFO_SW_VERSION_MAJOR": "0x{:02X}".format(software_version[0]),
        "AUTO_GIT_INFO_SW_VERSION_MINOR": "0x{:02X}".format(software_version[1]),
        "AUTO_GIT_INFO_SW_VERSION_BUILD": "0x{:02X}".format(software_version[2]),
        "AUTO_GIT_INFO_TICKS": str(ticks),
        "AUTO_GIT_INFO_WC_MODIFIED": "1" if modified else "0",
        "AUTO_GIT_INFO_FULL_HASH": _c_string(full_hash),
        "AUTO_GIT_INFO_SHORT_HASH": _c_string(full_hash[:8]),
    }

    comments = {
        "AUTO_GIT_INFO_GIT_VERSION": "Git version",
        "AUTO_GIT_INFO_COMMITS_NUMBER": "Number of commits",
        "AUTO_GIT_INFO_ISO_8601_DATE": "ISO formatted generation date (UTC)",
        "AUTO_GIT_INFO_LOCALE_DATE": "Local generation date",
        "AUTO_GIT_INFO_GIT_BRANCH": "Current Git branch",
        "AUTO_GIT_INFO_TAG": "Nearest reachable tag matching XX.YY.ZZ",
        "AUTO_GIT_INFO_SW_VERSION": "Software version split from AUTO_GIT_INFO_TAG (0xFF bytes if no valid tag)",
        "AUTO_GIT_INFO_SW_VERSION_MAJOR": "Software version major byte",
        "AUTO_GIT_INFO_SW_VERSION_MINOR": "Software version minor byte",
        "AUTO_GIT_INFO_SW_VERSION_BUILD": "Software version build byte",
        "AUTO_GIT_INFO_TICKS": "Commits between the tag and HEAD (0xFFFFFFFF if no valid tag)",
        "AUTO_GIT_INFO_WC_MODIFIED": "1 if the working tree has changes, otherwise 0",
        "AUTO_GIT_INFO_FULL_HASH": "Full Git hash",
        "AUTO_GIT_INFO_SHORT_HASH": "Short Git hash",
    }

    lines = ["/*", " * Auto-generated by tools/generate_git_info.py; do not edit.", " */", "#pragma once", ""]
    for name, value in values.items():
        lines.extend(("/*  {}  */".format(comments[name]), "#define {:<30} {}".format(name, value), ""))
    return "\n".join(lines)


def _generate_git_info(project_dir):
    project_dir = Path(project_dir).resolve()
    header_path = project_dir / _GENERATED_HEADER
    try:
        content = _make_header(project_dir)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        detail = getattr(error, "stderr", None)
        detail = detail.strip() if detail else str(error)
        print("Git info: unable to read Git metadata: {}; generating fallback header".format(detail))
        fallback = dict.fromkeys(
            (
                "AUTO_GIT_INFO_GIT_VERSION", "AUTO_GIT_INFO_ISO_8601_DATE", "AUTO_GIT_INFO_LOCALE_DATE",
                "AUTO_GIT_INFO_GIT_BRANCH", "AUTO_GIT_INFO_TAG", "AUTO_GIT_INFO_FULL_HASH",
                "AUTO_GIT_INFO_SHORT_HASH",
            ),
            '"unknown"',
        )
        fallback.update(
            AUTO_GIT_INFO_COMMITS_NUMBER="0",
            AUTO_GIT_INFO_SW_VERSION="0xFF,0xFF,0xFF",
            AUTO_GIT_INFO_SW_VERSION_MAJOR="0xFF",
            AUTO_GIT_INFO_SW_VERSION_MINOR="0xFF",
            AUTO_GIT_INFO_SW_VERSION_BUILD="0xFF",
            AUTO_GIT_INFO_TICKS=str(_UNKNOWN_TICKS),
            AUTO_GIT_INFO_WC_MODIFIED="1",
        )
        lines = ["/* Auto-generated by tools/generate_git_info.py; Git metadata unavailable. */", "#pragma once", ""]
        lines.extend("#define {:<30} {}".format(name, value) for name, value in fallback.items())
        content = "\n".join(lines) + "\n"

    header_path.parent.mkdir(parents=True, exist_ok=True)
    header_path.write_text(content, encoding="utf-8", newline="\n")
    print("Git info: generated {}".format(header_path))

if _platformio_env is not None:
    _generate_git_info(_platformio_env.subst("$PROJECT_DIR"))
else:
    parser = argparse.ArgumentParser(description="Generate src/AutoGitInfo.h from Git metadata.")
    parser.add_argument(
        "--project-dir",
        default=Path(__file__).resolve().parents[1],
        type=Path,
        help="PlatformIO project root (defaults to the parent of this script's tools directory)",
    )
    _arguments = parser.parse_args()
    _generate_git_info(_arguments.project_dir)
