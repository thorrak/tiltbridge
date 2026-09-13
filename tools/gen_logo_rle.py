#!/usr/bin/env python3
"""Generate an RLE16-encoded logo header for the large TFT.

The large TFT logo used to ship as a raw RGB565 array (288 * 240 * 2 =
138,240 bytes of .rodata). The image is dominated by two colors -- black
(81.8% of pixels) and 0x201F (14.6%) -- so run-length encoding it losslessly
costs about 8.9 KB instead, a 93.6% reduction, while keeping every one of the
117 colors and the anti-aliased edges exactly as they were.

Encoding
--------
Two parallel arrays rather than an array of structs: a
``struct { uint8_t count; uint16_t color; }`` would be padded to 4 bytes per
run by alignment, inflating the output by a third. Parallel arrays pack to
exactly 3 bytes per run.

    counts[i]  uint8   number of consecutive pixels, 1..255
    colors[i]  uint16  RGB565 value for that run

Pixels are in row-major order, the same order pushImage() consumed. The
decoder walks the runs and calls LovyanGFX writeColor(color, count), which
writes a block of one color with no intermediate buffer.

Usage
-----
    python3 tools/gen_logo_rle.py src/img/tft_logo.h  -o src/img/tft_logo_rle.h
    python3 tools/gen_logo_rle.py logo.png            -o src/img/tft_logo_rle.h

Input may be a PNG (or anything Pillow reads) or the legacy GIMP C-source
dump. Generation always round-trips the encoding and compares it against the
input pixel-for-pixel before writing; a mismatch is a hard error.

The encoding is lossless and reversible, so the generated header is a complete
record of the artwork. To recover it as an image:

    python3 tools/gen_logo_rle.py --decode src/img/tft_logo_rle.h -o logo.png
"""

import argparse
import os
import re
import sys

MAX_RUN = 255  # counts are uint8


def load_legacy_header(path):
    """Parse the GIMP RGBA C-source dump into (width, height, pixels)."""
    src = open(path).read()
    m = re.search(r'\}\s*gimp_image\s*=\s*\{', src)
    if not m:
        raise SystemExit(f"{path}: no 'gimp_image = {{' initializer found")
    body = src[m.end():]
    # The per-line "// 0x0010 (16) pixels" comments contain hex literals too.
    body = re.sub(r'//[^\n]*', '', body)
    vals = [int(v, 16) for v in re.findall(r'0x([0-9a-fA-F]{4})', body)]
    if len(vals) < 3:
        raise SystemExit(f"{path}: initializer too short")
    # Leading width/height/bytes_per_pixel are plain decimals, not 0x literals,
    # so everything matched above is pixel data.
    m2 = re.search(r'\}\s*gimp_image\s*=\s*\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)', src)
    if not m2:
        raise SystemExit(f"{path}: could not read width/height header")
    w, h = int(m2.group(1)), int(m2.group(2))
    px = vals[:w * h]
    if len(px) != w * h:
        raise SystemExit(f"{path}: expected {w * h} pixels, parsed {len(px)}")
    return w, h, px


def load_image(path):
    """Load any Pillow-readable image and quantize to RGB565."""
    try:
        from PIL import Image
    except ImportError:
        raise SystemExit("Pillow is required to read image files: pip install Pillow")
    im = Image.open(path).convert("RGB")
    w, h = im.size
    px = []
    for r, g, b in im.getdata():
        px.append(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))
    return w, h, px


def encode(px):
    """Run-length encode, splitting runs longer than MAX_RUN."""
    counts, colors = [], []
    i = 0
    n = len(px)
    while i < n:
        j = i
        while j < n and px[j] == px[i] and (j - i) < MAX_RUN:
            j += 1
        counts.append(j - i)
        colors.append(px[i])
        i = j
    return counts, colors


def decode(counts, colors):
    out = []
    for c, v in zip(counts, colors):
        out.extend([v] * c)
    return out


def load_rle_header(path):
    """Parse a generated RLE header back into (width, height, pixels)."""
    src = open(path).read()
    def define(name):
        m = re.search(rf'#define\s+{name}\s+(\d+)', src)
        if not m:
            raise SystemExit(f"{path}: missing #define {name}")
        return int(m.group(1))
    w, h, runs = define("TFT_LOGO_WIDTH"), define("TFT_LOGO_HEIGHT"), define("TFT_LOGO_RUNS")

    def body(name):
        m = re.search(rf'{name}\[TFT_LOGO_RUNS\]\s*=\s*\{{(.*?)\}};', src, re.S)
        if not m:
            raise SystemExit(f"{path}: could not find array {name}")
        return re.sub(r'//[^\n]*', '', m.group(1))

    counts = [int(v) for v in re.findall(r'\b(\d+)\b', body("tft_logo_rle_counts"))]
    colors = [int(v, 16) for v in re.findall(r'0x([0-9a-fA-F]{4})', body("tft_logo_rle_colors"))]
    if len(counts) != runs or len(colors) != runs:
        raise SystemExit(f"{path}: expected {runs} runs, got {len(counts)}/{len(colors)}")
    return w, h, decode(counts, colors)


def write_png(path, w, h, px):
    try:
        from PIL import Image
    except ImportError:
        raise SystemExit("Pillow is required to write PNG: pip install Pillow")
    im = Image.new("RGB", (w, h))
    im.putdata([((((v >> 11) & 0x1F) * 255 // 31),
                 (((v >> 5) & 0x3F) * 255 // 63),
                 ((v & 0x1F) * 255 // 31)) for v in px])
    im.save(path)


def emit(path, w, h, counts, colors, source):
    n = len(counts)
    raw = w * h * 2
    enc = n * 3
    lines = []
    a = lines.append
    a("//")
    a("// Generated by tools/gen_logo_rle.py -- do not edit by hand.")
    a(f"// Source: {source}")
    a("//")
    a(f"// {w}x{h} RGB565, run-length encoded losslessly as {n} runs.")
    a(f"// {enc:,} bytes here vs {raw:,} bytes raw ({enc * 100.0 / raw:.1f}%).")
    a("//")
    a("// Parallel arrays, not an array of structs: a {uint8,uint16} struct")
    a("// pads to 4 bytes per run and would cost a third more.")
    a("//")
    a("")
    a("#ifdef LCD_LARGE_TFT")
    a("")
    a("#ifndef _TFT_LOGO_RLE_H")
    a("#define _TFT_LOGO_RLE_H")
    a("")
    a("#include <stdint.h>")
    a("")
    a(f"#define TFT_LOGO_WIDTH  {w}")
    a(f"#define TFT_LOGO_HEIGHT {h}")
    a(f"#define TFT_LOGO_RUNS   {n}")
    a("")
    a("// Consecutive pixel count for each run, row-major.")
    a(f"static const uint8_t tft_logo_rle_counts[TFT_LOGO_RUNS] = {{")
    for k in range(0, n, 16):
        a("    " + ", ".join(f"{c:3d}" for c in counts[k:k + 16]) + ",")
    a("};")
    a("")
    a("// RGB565 color for each run.")
    a(f"static const uint16_t tft_logo_rle_colors[TFT_LOGO_RUNS] = {{")
    for k in range(0, n, 12):
        a("    " + ", ".join(f"0x{v:04X}" for v in colors[k:k + 12]) + ",")
    a("};")
    a("")
    a("#endif // _TFT_LOGO_RLE_H")
    a("")
    a("#endif // LCD_LARGE_TFT")
    a("")
    open(path, "w").write("\n".join(lines))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input", help="PNG (or other Pillow format) or legacy GIMP C header")
    ap.add_argument("-o", "--output", required=True, help="header file to write")
    ap.add_argument("--decode", action="store_true",
                    help="reverse: read a generated RLE header and write it out as an image")
    args = ap.parse_args()

    if args.decode:
        w, h, px = load_rle_header(args.input)
        write_png(args.output, w, h, px)
        print(f"decoded {args.input}: {w}x{h}, {len(px):,} pixels -> {args.output}")
        return

    if args.input.endswith((".h", ".c")):
        w, h, px = load_legacy_header(args.input)
    else:
        w, h, px = load_image(args.input)

    counts, colors = encode(px)

    # Round-trip before writing anything. The whole point of RLE16 over a
    # 1-bit bitmap is that it is exact, so prove it.
    if decode(counts, colors) != px:
        raise SystemExit("FATAL: RLE round-trip did not reproduce the source pixels")
    if sum(counts) != w * h:
        raise SystemExit(f"FATAL: runs cover {sum(counts)} px, expected {w * h}")

    emit(args.output, w, h, counts, colors, os.path.basename(args.input))

    raw, enc = w * h * 2, len(counts) * 3
    print(f"{w}x{h}, {len(set(px))} distinct colors, {len(counts):,} runs")
    print(f"round-trip verified: {len(px):,} pixels reproduced exactly")
    print(f"{raw:,} B raw -> {enc:,} B encoded ({enc * 100.0 / raw:.1f}%, saves {raw - enc:,} B)")
    print(f"wrote {args.output}")


if __name__ == "__main__":
    main()
