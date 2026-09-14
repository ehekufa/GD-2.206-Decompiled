#!/usr/bin/env python3
"""Разбор сырого снимка экрана Android (`adb exec-out screencap`, без -p).

Формат буфера: uint32 width, height, pixel_format, на Android 9+ ещё uint32
colorspace, дальше сырые пиксели (RGBA_8888 / RGBX_8888 / BGRA_8888).

Скрипт нужен CI, чтобы отличить настоящий кадр игры от чёрного прямоугольника:
если EGL/GLES не поднялись, процесс всё равно живой, а на экране заливка одним
цветом. Печатает key=value и мини-карту яркости, код возврата 0 — кадр живой.

    python3 tools/screenstat.py screen.raw --ascii
"""
import struct
import sys
from collections import Counter

# сколько точек берём по осям (полный разбор 1080p в чистом Python не нужен)
GRID_W, GRID_H = 160, 90
ART_W, ART_H = 48, 16
RAMP = " .:-=+*#%@"

# критерии «на экране что-то нарисовано»
MIN_COLORS = 8        # уникальных цветов в сетке
MAX_TOP_SHARE = 0.97  # доля самого частого цвета


def load(path):
    with open(path, "rb") as f:
        buf = f.read()
    if len(buf) < 16:
        sys.exit("screencap: пустой буфер (%d байт)" % len(buf))
    w, h, fmt = struct.unpack("<III", buf[:12])
    for off in (12, 16):                       # с colorspace и без него
        if w > 0 and h > 0 and off + w * h * 4 == len(buf):
            return w, h, fmt, off, buf
    sys.exit("screencap: неожиданный формат (w=%d h=%d fmt=%d bytes=%d)"
             % (w, h, fmt, len(buf)))


def sample(w, h, off, buf):
    """Сетка GRID_W x GRID_H пикселей: [(r, g, b), ...] построчно."""
    px = []
    for gy in range(GRID_H):
        y = min(h - 1, gy * h // GRID_H)
        row = off + y * w * 4
        for gx in range(GRID_W):
            x = min(w - 1, gx * w // GRID_W)
            i = row + x * 4
            px.append((buf[i], buf[i + 1], buf[i + 2]))
    return px


def ascii_art(px):
    lines = []
    for ay in range(ART_H):
        line = []
        for ax in range(ART_W):
            gx = ax * GRID_W // ART_W
            gy = ay * GRID_H // ART_H
            r, g, b = px[gy * GRID_W + gx]
            lum = (r * 299 + g * 587 + b * 114) // 1000
            line.append(RAMP[min(len(RAMP) - 1, lum * len(RAMP) // 256)])
        lines.append("".join(line))
    return lines


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    if not args:
        sys.exit("usage: screenstat.py screen.raw [--ascii]")
    w, h, fmt, off, buf = load(args[0])
    px = sample(w, h, off, buf)

    cnt = Counter(px)
    top, top_n = cnt.most_common(1)[0]
    share = top_n / float(len(px))
    lum = [(r * 299 + g * 587 + b * 114) / 1000.0 for r, g, b in px]
    mean = sum(lum) / len(lum)

    print("size=%dx%d fmt=%d colors=%d top=#%02x%02x%02x share=%.2f "
          "mean_lum=%.1f min_lum=%.0f max_lum=%.0f"
          % (w, h, fmt, len(cnt), top[0], top[1], top[2], share,
             mean, min(lum), max(lum)))

    if "--ascii" in sys.argv[1:]:
        print("--- экран (карта яркости) ---")
        for line in ascii_art(px):
            print("|%s|" % line)

    ok = len(cnt) >= MIN_COLORS and share <= MAX_TOP_SHARE
    print("verdict=%s" % ("frame" if ok else "blank"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
