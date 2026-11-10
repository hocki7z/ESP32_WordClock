"""PlatformIO pre-script: configure IDF dependency setup before framework loading.

No global pip settings or installed framework files are modified. Git checkouts
are verified, never reset; failures abort before CMake configuration.
"""

import os
from pathlib import Path
import shutil
import subprocess

from SCons.Script import COMMAND_LINE_TARGETS

Import("env")

LITTLEFS_COMMIT = "c3fe82d4d2794c7db7a9b5c7d9ec151e32fc001b"
LITTLEFS_CORE_COMMIT = "d01280e64934a09ba16cac60cf9d3a37e228bb66"


def prepare():
    if env.GetOption("clean") or any(
        target in ("clean", "fullclean") for target in COMMAND_LINE_TARGETS
    ):
        return

    project_dir = Path(env.subst("$PROJECT_DIR"))
    child_env = env["ENV"]

    def set_variable(name, value):
        # IDF uses both os.environ.copy() and SCons subprocess environments.
        os.environ[name] = value
        child_env[name] = value

    public_pypi = env.GetProjectOption("custom_idf_public_pypi", "false").lower()
    if public_pypi in ("true", "yes", "1"):
        set_variable("PIP_CONFIG_FILE", os.devnull)
        set_variable("PIP_INDEX_URL", "https://pypi.org/simple")
        set_variable("PIP_EXTRA_INDEX_URL", "")
        print("IDF setup: public PyPI only; TLS verification remains enabled")

    features = os.environ.get("PIP_USE_FEATURE", "").split()
    if "truststore" not in features:
        features.append("truststore")
    set_variable("PIP_USE_FEATURE", " ".join(features))

    ca_bundle = env.GetProjectOption("custom_idf_ca_bundle", "").strip()
    if ca_bundle:
        ca_path = Path(os.path.expandvars(os.path.expanduser(ca_bundle)))
        if not ca_path.is_absolute():
            ca_path = project_dir / ca_path
        if not ca_path.is_file():
            raise RuntimeError("IDF CA bundle does not exist: {}".format(ca_path))
        set_variable("PIP_CERT", str(ca_path))
        set_variable("REQUESTS_CA_BUNDLE", str(ca_path))

    git = shutil.which("git", path=child_env.get("PATH", os.environ.get("PATH", "")))
    if not git:
        portable_git = Path("C:/_DEV_TOOLS/VSCode/GitPortable/App/Git/bin/git.exe")
        if portable_git.is_file():
            git = str(portable_git)
    if not git:
        raise RuntimeError("Git is required to prepare the LittleFS component")

    def run_git(*args, capture=False):
        result = subprocess.run(
            [git, *map(str, args)], cwd=str(project_dir), env=dict(child_env),
            check=True, stdout=subprocess.PIPE if capture else None,
            text=True,
        )
        return result.stdout.strip() if capture else None

    component = project_dir / "components" / "littlefs"
    if not component.exists():
        print("IDF setup: downloading LittleFS v1.14.8 and its submodule", flush=True)
        component.parent.mkdir(parents=True, exist_ok=True)
        run_git("clone", "--branch", "v1.14.8", "--depth", "1",
                "--recurse-submodules", "--shallow-submodules",
                "https://github.com/joltwallet/esp_littlefs.git", component)

    if not (component / ".git").exists():
        raise RuntimeError("Existing LittleFS directory is not a Git checkout; left unchanged")
    head = run_git("-C", component, "rev-parse", "--verify", "HEAD", capture=True)
    if head != LITTLEFS_COMMIT:
        raise RuntimeError("LittleFS revision mismatch: expected {}, got {}; left unchanged".format(
            LITTLEFS_COMMIT, head))

    core = component / "src" / "littlefs"
    if not (core / ".git").exists():
        if core.exists() and any(core.iterdir()):
            raise RuntimeError("LittleFS submodule directory is populated but not a checkout; left unchanged")
        run_git("-C", component, "submodule", "update", "--init", "--recursive")
    core_head = run_git("-C", core, "rev-parse", "--verify", "HEAD", capture=True)
    if core_head != LITTLEFS_CORE_COMMIT:
        raise RuntimeError("LittleFS submodule revision mismatch; left unchanged")
    for required in (component / "include" / "esp_littlefs.h", core / "lfs.c"):
        if not required.is_file():
            raise RuntimeError("Missing LittleFS source: {}".format(required))
    print("IDF setup: LittleFS v1.14.8 verified; continuing with ESP-IDF")


try:
    prepare()
except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
    print("IDF setup failed: {}".format(error))
    env.Exit(1)