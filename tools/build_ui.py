"""
PlatformIO pre-action: builds the Vue/Vite web UI in tiltbridge_web_ui/, copies
the artifacts into data/, and generates the sources that embed them into the
firmware image.

Why this runs at import time
----------------------------
The UI is compiled into firmware.bin, not just written to the LittleFS image
(see tools/gen_embedded_ui.py for why, and for how). The generated sources land
in src/generated/, and src/CMakeLists.txt picks them up with a
FILE(GLOB_RECURSE src/*.*) that is evaluated when CMake configures the project.
PlatformIO configures CMake while loading the framework builder, which happens
after `pre:` extra scripts are imported and before any SCons action runs. So the
generation has to happen here, at module scope -- an AddPreAction would fire too
late and the glob would have already missed the files.

The Vite build is skipped when data/ is newer than everything it is built from,
so the usual edit-firmware-and-rebuild loop does not pay for it. A firmware
build does still need Node.js + npm the first time, or after a UI change.

data/ layout
------------
The entire contents of data/ are replaced with the Vite dist/ output, except for
data/wifiui/, which is owned by the esp_wifi_config library.

data/ is a staging area for the embedding step, not the LittleFS image. The
image is built from fs_image/ (platformio.ini `data_dir`), because the OTA
partition layout leaves 192 KiB for a filesystem and the built UIs are ~325 KB.
"""

Import("env")  # noqa: F821  (provided by PlatformIO)

import os
import shutil
import subprocess
import sys

from SCons.Script import COMMAND_LINE_TARGETS

PROJECT_DIR = env["PROJECT_DIR"]  # noqa: F821
UI_DIR = os.path.join(PROJECT_DIR, "tiltbridge_web_ui")
DATA_DIR = os.path.join(PROJECT_DIR, "data")
DIST_DIR = os.path.join(UI_DIR, "dist")
GEN_SCRIPT = os.path.join(PROJECT_DIR, "tools", "gen_embedded_ui.py")

# Subdirectories of data/ that are NOT produced by the UI build and must be
# preserved across rebuilds. Owned by the esp_wifi_config library.
PRESERVE_ENTRIES = ("wifiui",)

# Inputs the built UI depends on. Anything newer than data/ means a rebuild.
UI_SOURCES = ("src", "public", "index.html", "package.json", "package-lock.json",
              "vite.config.js", "vite.config.ts")

# Targets that only inspect or tear down the build tree. Building the UI for
# these would be a slow surprise, and none of them read data/.
SKIP_TARGETS = ("clean", "cleanall", "fullclean", "monitor", "device",
                "erase", "envdump", "idedata", "compiledb", "sysenv")

# The output that tells us the UI has been copied into data/ at all.
DATA_SENTINEL = os.path.join(DATA_DIR, "index.html.gz")

BANNER = "=" * 72


def _banner(msg):
    print("")
    print(BANNER)
    print(msg)
    print(BANNER)


def _abort(msg):
    print("")
    print("ERROR: " + msg)
    print("")
    print("The firmware image bundles a Vue/Vite web UI that must be built before")
    print("the firmware can be assembled. Install Node.js (LTS) from")
    print("https://nodejs.org/ -- this provides the `npm` command -- then retry.")
    print("")
    sys.exit(1)


def _newest_mtime(paths):
    newest = 0.0
    for path in paths:
        if os.path.isfile(path):
            newest = max(newest, os.path.getmtime(path))
        elif os.path.isdir(path):
            for root, _dirs, files in os.walk(path):
                for name in files:
                    newest = max(newest, os.path.getmtime(os.path.join(root, name)))
    return newest


def _ui_is_current():
    """True when data/ already holds a build newer than every UI source."""
    if not os.path.isfile(DATA_SENTINEL):
        return False
    sources = [os.path.join(UI_DIR, entry) for entry in UI_SOURCES]
    return _newest_mtime(sources) <= os.path.getmtime(DATA_SENTINEL)


def _clear_data_dir():
    """Remove everything in data/ except PRESERVE_ENTRIES."""
    if not os.path.isdir(DATA_DIR):
        return
    for name in os.listdir(DATA_DIR):
        if name in PRESERVE_ENTRIES:
            continue
        path = os.path.join(DATA_DIR, name)
        if os.path.isdir(path) and not os.path.islink(path):
            shutil.rmtree(path)
        else:
            os.remove(path)


def _copy_tree(src, dst):
    """Recursively copy src into dst, preserving PRESERVE_ENTRIES at the top level."""
    for entry in os.listdir(src):
        if entry in PRESERVE_ENTRIES:
            # Extremely defensive: the Vue build shouldn't produce anything
            # named wifiui, but if it does, skip it to avoid clobbering.
            print("  ! skipping '{}' from dist (reserved for esp_wifi_config)".format(entry))
            continue
        s = os.path.join(src, entry)
        d = os.path.join(dst, entry)
        if os.path.isdir(s):
            shutil.copytree(s, d)
        else:
            shutil.copy2(s, d)
        print("  + " + entry)


def _run_vite_build():
    _banner("Building web UI (tiltbridge_web_ui/ -> data/)")

    if not os.path.isdir(UI_DIR):
        _abort("tiltbridge_web_ui/ directory not found at {}".format(UI_DIR))

    npm = shutil.which("npm")
    if npm is None:
        _abort("`npm` not found on PATH.")

    node_modules = os.path.join(UI_DIR, "node_modules")
    if not os.path.isdir(node_modules):
        print("First-time setup: installing UI dependencies (npm ci)...")
        print("This can take a minute. Subsequent builds will be fast.")
        subprocess.check_call([npm, "ci"], cwd=UI_DIR)

    print("Running `npm run build` in tiltbridge_web_ui/ ...")
    subprocess.check_call([npm, "run", "build"], cwd=UI_DIR)

    if not os.path.isdir(DIST_DIR):
        _abort("vite build did not produce tiltbridge_web_ui/dist/")

    os.makedirs(DATA_DIR, exist_ok=True)
    print("Clearing data/ (preserving: {})".format(", ".join(PRESERVE_ENTRIES)))
    _clear_data_dir()

    print("Copying dist/ into data/:")
    _copy_tree(DIST_DIR, DATA_DIR)

    # The sentinel drives the staleness check above, and shutil.copy2 preserved
    # dist/'s mtimes -- which can predate a source file edited during the build.
    os.utime(DATA_SENTINEL, None)
    print("UI build complete.")
    print("")


def _generate_embedded_sources():
    """Regenerate src/generated/embedded_ui.{S,cpp} from the contents of data/."""
    subprocess.check_call([sys.executable, GEN_SCRIPT])


def prepare_web_ui():
    if _ui_is_current():
        print("Web UI in data/ is up to date; skipping Vite build.")
    else:
        _run_vite_build()
    _generate_embedded_sources()


# Runs while this script is imported, which is before PlatformIO configures
# CMake -- see the module docstring. Skipped for targets that never read data/.
if not any(t in SKIP_TARGETS for t in COMMAND_LINE_TARGETS):
    prepare_web_ui()
