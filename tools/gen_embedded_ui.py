#!/usr/bin/env python3
"""Generate the C/assembly sources that bake the web UIs into the firmware.

Why this exists
---------------
The Vue UI used to live only in the LittleFS partition, which meant an OTA
firmware update shipped new C++ against an old UI until the user separately
flashed a filesystem image. Embedding the UI in the application image makes
the two update atomically.

ESP-IDF has a mechanism for exactly this -- `EMBED_FILES` in
idf_component_register() -- and it is unusable under PlatformIO. IDF
implements it with an add_custom_command() that turns each file into a
generated .S, but PlatformIO does not run CMake's build; it reads the CMake
file-API codemodel, reproduces the source list in SCons, and replays exactly
one custom command (sections.ld). The generated .S is therefore listed as a
source that nothing ever creates:

    *** [.../embedtest.bin.S.o] Source `.../embedtest.bin.S' not found

So we generate the same thing ourselves, ahead of the build, into src/, where
the FILE(GLOB_RECURSE src/*.*) in src/CMakeLists.txt picks it up as an
ordinary source. No CMake changes, no custom commands.

Two tables, two consumers
-------------------------
tb_embedded_assets   the TiltBridge Vue UI, from data/. Looked up by
                     idf_static_serve_file(), which is handed paths with no
                     leading slash ("index.html").

tb_wifiui_assets     the esp_wifi_config provisioning UI, from data/wifiui/.
                     Handed to the library through
                     wifi_cfg_webui_set_asset_provider(), which is called with
                     paths that do have a leading slash ("/index.html").

Kept apart rather than merged because both UIs have an index.html. They do not
collide today only because the library registers its handlers for the duration
of provisioning and unregisters them afterwards; separate tables keep that true
no matter what either UI is built to contain. The differing leading-slash
convention is not cosmetic -- it is what each consumer actually passes in.

Output
------
src/generated/embedded_ui.S     .incbin of each asset, one symbol pair each
src/generated/embedded_ui.cpp   the lookup tables over those symbols

.incbin keeps the generated assembly small (a few hundred lines rather than
the ~2 MB of `.byte 0x..,` text that spelling the bytes out would produce).
The cost is that the assembler reads the asset files at build time without
SCons knowing, so a changed asset would not by itself retrigger anything --
the .S text is identical. A content hash of every input is therefore written
into a comment at the top of the .S, which makes any asset change a text
change, which makes SCons rebuild and relink. Do not remove it.

Usage
-----
    python3 tools/gen_embedded_ui.py                 # data/ -> src/generated/
    python3 tools/gen_embedded_ui.py --list          # report, write nothing
"""

import argparse
import hashlib
import os
import sys

# Subdirectories of data/ that the main table does not cover.
#
#   wifiui/  is the esp_wifi_config provisioning UI. Embedded too, but into its
#            own table -- see the module docstring.
#   conf/    is runtime configuration, which has to stay writable on LittleFS.
MAIN_EXCLUDE_DIRS = ("wifiui", "conf")

# Editor and OS droppings that should never reach the device.
EXCLUDE_NAMES = (".DS_Store", "Thumbs.db")

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA_DIR = os.path.join(PROJECT_DIR, "data")
OUT_DIR = os.path.join(PROJECT_DIR, "src", "generated")


class Asset:
    def __init__(self, abspath, relpath, uri_prefix):
        self.abspath = abspath
        self.relpath = relpath          # e.g. "index.js.gz", relative to its root
        self.gzipped = relpath.endswith(".gz")
        # The path the consumer asks for: index.js.gz on disk is served as
        # index.js with Content-Encoding: gzip, matching what the LittleFS path
        # in idf_static_serve_file() has always done.
        stem = relpath[:-3] if self.gzipped else relpath
        self.uri = uri_prefix + stem
        self.size = os.path.getsize(abspath)


class AssetSet:
    """One table's worth of assets: where they come from and what to call them."""

    def __init__(self, name, symbol_prefix, table, root, uri_prefix,
                 exclude_dirs=(), index_uri=None, description=""):
        self.name = name
        self.symbol_prefix = symbol_prefix
        self.table = table
        self.root = root
        self.uri_prefix = uri_prefix
        self.exclude_dirs = exclude_dirs
        self.index_uri = index_uri
        self.description = description
        self.assets = []

    def collect(self):
        for dirpath, dirnames, filenames in os.walk(self.root):
            rel_root = os.path.relpath(dirpath, self.root)
            if rel_root == ".":
                rel_root = ""
                dirnames[:] = [d for d in dirnames if d not in self.exclude_dirs]
            for name in sorted(filenames):
                if name in EXCLUDE_NAMES:
                    continue
                relpath = os.path.join(rel_root, name) if rel_root else name
                self.assets.append(
                    Asset(os.path.join(dirpath, name),
                          relpath.replace(os.sep, "/"),
                          self.uri_prefix))
        self.assets.sort(key=lambda a: a.uri)

    @property
    def total(self):
        return sum(a.size for a in self.assets)

    def symbol(self, i):
        return "%s_%d" % (self.symbol_prefix, i)


def build_sets(data_dir):
    return [
        AssetSet(
            name="TiltBridge web UI",
            symbol_prefix="tb_asset",
            table="tb_embedded_assets",
            root=data_dir,
            uri_prefix="",
            exclude_dirs=MAIN_EXCLUDE_DIRS,
            index_uri="index.html",
            description="served by idf_static_serve_file(); paths have no leading slash",
        ),
        AssetSet(
            name="esp_wifi_config provisioning UI",
            symbol_prefix="tb_wifiui_asset",
            table="tb_wifiui_assets",
            root=os.path.join(data_dir, "wifiui"),
            uri_prefix="/",
            index_uri="/index.html",
            description="handed to the library via wifi_cfg_webui_set_asset_provider()",
        ),
    ]


def content_hash(sets):
    """Hash every input so an asset edit shows up as a change to the .S text."""
    h = hashlib.sha256()
    for s in sets:
        h.update(s.table.encode())
        h.update(b"\0")
        for a in s.assets:
            h.update(a.relpath.encode())
            h.update(b"\0")
            with open(a.abspath, "rb") as fp:
                h.update(fp.read())
    return h.hexdigest()


def emit_asm(sets, digest):
    lines = [
        "/*",
        " * Generated by tools/gen_embedded_ui.py -- do not edit, not tracked in git.",
        " *",
        " * Both web UIs, baked into the application image so that an OTA update",
        " * replaces the firmware and the pages it serves in one step.",
        " *",
        " * The hash below covers the contents of every embedded file. .incbin",
        " * pulls those bytes in behind SCons' back, so without it a changed asset",
        " * would leave this file byte-identical and nothing would rebuild.",
        " *",
        " * content-hash: " + digest,
        " */",
        "",
        "\t.section .rodata.embedded",
        "",
    ]
    for s in sets:
        lines += [
            "\t/* ===== %s: %d files, %s bytes ===== */" % (
                s.name, len(s.assets), format(s.total, ",")),
            "",
        ]
        for i, a in enumerate(s.assets):
            sym = s.symbol(i)
            lines += [
                "\t/* %s -> %s (%s, %d bytes) */" % (
                    a.relpath, a.uri, "gzip" if a.gzipped else "identity", a.size),
                "\t.balign 4",
                "\t.global %s_start" % sym,
                "%s_start:" % sym,
                '\t.incbin "%s"' % a.abspath,
                "\t.global %s_end" % sym,
                "%s_end:" % sym,
                "",
            ]
    return "\n".join(lines) + "\n"


def emit_table(sets, digest):
    lines = [
        "//",
        "// Generated by tools/gen_embedded_ui.py -- do not edit, not tracked in git.",
        "//",
        "// Lookup tables over the symbols defined in embedded_ui.S.",
        "//",
    ]
    for s in sets:
        lines.append("//   %-20s %2d files, %9s bytes -- %s"
                     % (s.table, len(s.assets), format(s.total, ","), s.description))
    lines += [
        "//",
        "// content-hash: " + digest,
        "//",
        "",
        '#include "embedded_ui.h"',
        "",
        'extern "C" {',
    ]
    for s in sets:
        for i in range(len(s.assets)):
            lines.append("extern const uint8_t %s_start[];" % s.symbol(i))
            lines.append("extern const uint8_t %s_end[];" % s.symbol(i))
    lines.append("}")

    for s in sets:
        width = max(len(a.uri) for a in s.assets) + 3
        lines += [
            "",
            "// %s -- %s" % (s.name, s.description),
            "const embedded_asset_t %s[] = {" % s.table,
        ]
        for i, a in enumerate(s.assets):
            sym = s.symbol(i)
            lines.append("    { %-*s %s_start, %s_end, %-5s }," % (
                width, '"%s",' % a.uri, sym, sym,
                "true" if a.gzipped else "false"))
        lines += [
            "};",
            "",
            "const size_t %s_count = sizeof(%s) / sizeof(%s[0]);"
            % (s.table, s.table, s.table),
        ]
    return "\n".join(lines) + "\n"


def write_if_changed(path, text):
    """Write only when the content differs, so mtimes stay put.

    PlatformIO imports the pre-scripts once per environment, so `pio run` with
    no -e regenerates these files six times. Rewriting them unconditionally
    bumped the mtime each time and forced six needless reassembles and relinks
    of ~325 KB of assets. Skipping the write is safe precisely because the
    content hash covers every input: identical text means identical assets.
    """
    if os.path.isfile(path):
        with open(path) as fp:
            if fp.read() == text:
                return False
    with open(path, "w") as fp:
        fp.write(text)
    return True


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--data-dir", default=DATA_DIR)
    ap.add_argument("--out-dir", default=OUT_DIR)
    ap.add_argument("--list", action="store_true",
                    help="report what would be embedded and exit")
    ap.add_argument("-q", "--quiet", action="store_true")
    args = ap.parse_args()

    if not os.path.isdir(args.data_dir):
        raise SystemExit(
            "gen_embedded_ui: %s does not exist.\n"
            "The web UI is a build artifact. Run `pio run -e <env> --target buildfs`,\n"
            "or `npm run build` in tiltbridge_web_ui/, to produce it."
            % args.data_dir)

    sets = build_sets(args.data_dir)
    for s in sets:
        if not os.path.isdir(s.root):
            raise SystemExit(
                "gen_embedded_ui: %s does not exist, so the %s cannot be embedded.\n"
                "It is the only source for that UI now -- the library serves it\n"
                "through our asset provider and reads no filesystem."
                % (s.root, s.name))
        s.collect()
        if not s.assets:
            raise SystemExit(
                "gen_embedded_ui: no files to embed for the %s (%s).\n"
                "The firmware serves it from flash, so building like this would\n"
                "produce a device that answers every one of its pages with a 404."
                % (s.name, s.root))
        if s.index_uri and not any(a.uri == s.index_uri for a in s.assets):
            raise SystemExit(
                "gen_embedded_ui: the %s has no %s.\n"
                "That is its entry point, so this is almost certainly a\n"
                "half-written data/ rather than an intentional build."
                % (s.name, s.index_uri))

    if args.list:
        for s in sets:
            print("%s (%s):" % (s.name, s.table))
            for a in s.assets:
                print("  %-40s %8d  %s" % (a.uri, a.size, "gzip" if a.gzipped else ""))
            print("  %d files, %s bytes" % (len(s.assets), format(s.total, ",")))
        print("total: %s bytes" % format(sum(s.total for s in sets), ","))
        return

    digest = content_hash(sets)
    os.makedirs(args.out_dir, exist_ok=True)
    changed = (write_if_changed(os.path.join(args.out_dir, "embedded_ui.S"),
                                emit_asm(sets, digest)) |
               write_if_changed(os.path.join(args.out_dir, "embedded_ui.cpp"),
                                emit_table(sets, digest)))

    if not args.quiet:
        print("Embedding web UIs: %s -> %s%s" % (
            ", ".join("%s %d files/%s B" % (s.table, len(s.assets), format(s.total, ","))
                      for s in sets),
            os.path.relpath(args.out_dir, PROJECT_DIR),
            "" if changed else " (unchanged)"))


if __name__ == "__main__":
    main()
