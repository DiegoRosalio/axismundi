#!/usr/bin/env python3
"""Draws the axismundi icon in the GNOME style (no image libraries needed) and writes:
  data/icons/hicolor/scalable/apps/<APP_ID>.svg            the app icon (128-unit grid)
  data/icons/hicolor/symbolic/apps/<APP_ID>-symbolic.svg   one color, 16 px, for panels and menus
  data/icons/hicolor/<N>x<N>/apps/<APP_ID>.png             PNG sizes for places without SVG
  src/ui/generated/icon.inc                                PNG bytes embedded in the app (window icon, top bar)

The picture follows the GNOME app icon conventions: an object seen slightly from above (a light tile with a
darker bottom edge), GNOME palette colors, simple shapes. On the tile: the world axis (axis mundi) as a blue
vertical arrow through an orbit ring (the back half behind the axes, the front half in front), the red x axis
toward the viewer and the green y axis going back, as in the app's 3D view. Run: python3 tools/icon.py
"""
import math
import os
import struct
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

APP_ID = "com.bisonanon.axismundi"

# Geometry on GNOME's 128 x 128 grid
TILE = (8, 8, 120, 110)       # the tile's top face (x0, y0, x1, y1)
TILE_EDGE = 6                 # its darker bottom edge
CORNER = 16
TOP_LIGHT, TOP_DARK = (255, 255, 255), (246, 245, 244)   # GNOME Light 1 -> Light 2
EDGE = (192, 191, 188)                                   # Light 4
RING_C, RING_RX, RING_RY = (64, 64), 42, 13
ORIGIN = (64, 64)
Z_TIP, Z_TAIL = (64, 17), (64, 97)
X_TIP = (31, 83)    # toward the viewer, left
Y_TIP = (99, 52)    # going back, right
BLUE, RED, GREEN = (53, 132, 228), (237, 51, 59), (46, 194, 126)   # Blue 3, Red 2, Green 4
RING_BACK, RING_FRONT = (154, 153, 150), (119, 118, 123)          # Light 5, Dark 1
ORIGIN_DOT = (61, 56, 70)                                          # Dark 3


def clamp(v, a, b):
    return a if v < a else b if v > b else v


def seg_dist(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    t = clamp(((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy), 0.0, 1.0)
    return math.hypot(px - ax - t * dx, py - ay - t * dy)


def in_triangle(px, py, a, b, c):
    def side(p, q, r):
        return (p[0] - r[0]) * (q[1] - r[1]) - (q[0] - r[0]) * (p[1] - r[1])
    d1, d2, d3 = side((px, py), a, b), side((px, py), b, c), side((px, py), c, a)
    neg = d1 < 0 or d2 < 0 or d3 < 0
    pos = d1 > 0 or d2 > 0 or d3 > 0
    return not (neg and pos)


def arrow_parts(tail, tip, head_len, head_w):
    """Shaft end and the arrowhead triangle for an arrow from tail to tip"""
    dx, dy = tip[0] - tail[0], tip[1] - tail[1]
    n = math.hypot(dx, dy)
    ux, uy = dx / n, dy / n
    base = (tip[0] - ux * head_len, tip[1] - uy * head_len)
    left = (base[0] - uy * head_w, base[1] + ux * head_w)
    right = (base[0] + uy * head_w, base[1] - ux * head_w)
    return base, (tip, left, right)


def rounded_rect_inside(px, py, x0, y0, x1, y1, r):
    cx, cy = clamp(px, x0 + r, x1 - r), clamp(py, y0 + r, y1 - r)
    return math.hypot(px - cx, py - cy) <= r and x0 <= px <= x1 and y0 <= py <= y1


def render(size):
    """RGBA rows for the icon at `size` px (4x4 supersampling)"""
    ss = 4
    n = size * ss
    k = n / 128.0  # grid units -> supersampled pixels

    def stroke(w, min_px):  # never thinner than min_px at the final size, so small icons stay readable
        return max(w * k, min_px * ss)

    axis_w, side_w = stroke(5, 1.6), stroke(4.2, 1.3)
    head_len, head_w = stroke(15, 3.5), stroke(8, 2.2)
    ring_w = stroke(2.6, 1.0)

    def P(p):
        return (p[0] * k, p[1] * k)

    zbase, zhead = arrow_parts(P(Z_TAIL), P(Z_TIP), head_len, head_w)
    xbase, xhead = arrow_parts(P(ORIGIN), P(X_TIP), head_len * 0.8, head_w * 0.8)
    ybase, yhead = arrow_parts(P(ORIGIN), P(Y_TIP), head_len * 0.8, head_w * 0.8)
    rc, rrx, rry = P(RING_C), RING_RX * k, RING_RY * k
    x0, y0, x1, y1 = (v * k for v in TILE)
    corner, edge = CORNER * k, TILE_EDGE * k

    def on_ring(px, py):
        # Distance to the ellipse = |F| / |grad F| with F = (x/a)^2 + (y/b)^2 - 1
        ex, ey = px - rc[0], py - rc[1]
        f = (ex / rrx) ** 2 + (ey / rry) ** 2 - 1.0
        g = math.hypot(2 * ex / rrx ** 2, 2 * ey / rry ** 2)
        return g > 0 and abs(f) / g < ring_w / 2

    pixels = bytearray(size * size * 4)
    for y in range(size):
        for x in range(size):
            acc = [0.0, 0.0, 0.0, 0.0]
            for sy in range(ss):
                for sx in range(ss):
                    px, py = x * ss + sx + 0.5, y * ss + sy + 0.5
                    top = rounded_rect_inside(px, py, x0, y0, x1, y1, corner)
                    if not top:
                        # The darker bottom edge: the same tile shifted down
                        if rounded_rect_inside(px, py, x0, y0 + edge, x1, y1 + edge, corner):
                            for i in range(3):
                                acc[i] += EDGE[i]
                            acc[3] += 255.0
                        continue
                    t = (py - y0) / (y1 - y0)
                    col = [TOP_LIGHT[i] + (TOP_DARK[i] - TOP_LIGHT[i]) * t for i in range(3)]

                    def over(c, a):
                        for i in range(3):
                            col[i] = col[i] * (1 - a) + c[i] * a

                    front = py > rc[1]
                    if not front and on_ring(px, py):
                        over(RING_BACK, 0.7)  # the back half of the ring, behind the axes
                    for base, head, c in ((xbase, xhead, RED), (ybase, yhead, GREEN)):
                        if seg_dist(px, py, *P(ORIGIN), *base) < side_w / 2 or in_triangle(px, py, *head):
                            over(c, 1.0)
                    if seg_dist(px, py, *P(Z_TAIL), *zbase) < axis_w / 2 or in_triangle(px, py, *zhead):
                        over(BLUE, 1.0)
                    if math.hypot(px - P(ORIGIN)[0], py - P(ORIGIN)[1]) < stroke(4, 1.4):
                        over(ORIGIN_DOT, 1.0)
                    if front and on_ring(px, py):
                        over(RING_FRONT, 0.95)  # the front half, over the axes
                    for i in range(3):
                        acc[i] += col[i]
                    acc[3] += 255.0
            m = ss * ss
            o = (y * size + x) * 4
            if acc[3] > 0:
                cov = acc[3] / 255.0
                for i in range(3):
                    pixels[o + i] = int(clamp(acc[i] / cov + 0.5, 0, 255))
                pixels[o + 3] = int(acc[3] / m + 0.5)
    return pixels


def png_bytes(size, rgba):
    raw = b"".join(b"\x00" + bytes(rgba[y * size * 4:(y + 1) * size * 4]) for y in range(size))
    def chunk(tag, data):
        c = tag + data
        return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def rgb(c):
    return "#%02x%02x%02x" % c


def svg():
    def arrow(tail, tip, color, w, hl, hw):
        base, (t, l, r) = arrow_parts(tail, tip, hl, hw)
        return (f'<path d="M {tail[0]} {tail[1]} L {base[0]:.2f} {base[1]:.2f}" stroke="{rgb(color)}" '
                f'stroke-width="{w}" stroke-linecap="round" fill="none"/>\n'
                f'<path d="M {t[0]:.2f} {t[1]:.2f} L {l[0]:.2f} {l[1]:.2f} L {r[0]:.2f} {r[1]:.2f} Z" fill="{rgb(color)}"/>')
    x0, y0, x1, y1 = TILE
    cx, cy = RING_C
    back = f"M {cx - RING_RX} {cy} A {RING_RX} {RING_RY} 0 0 1 {cx + RING_RX} {cy}"
    front = f"M {cx - RING_RX} {cy} A {RING_RX} {RING_RY} 0 0 0 {cx + RING_RX} {cy}"
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="128" height="128" viewBox="0 0 128 128">
<defs><linearGradient id="top" x1="0" y1="{y0}" x2="0" y2="{y1}" gradientUnits="userSpaceOnUse">
<stop offset="0" stop-color="{rgb(TOP_LIGHT)}"/><stop offset="1" stop-color="{rgb(TOP_DARK)}"/></linearGradient></defs>
<rect x="{x0}" y="{y0 + TILE_EDGE}" width="{x1 - x0}" height="{y1 - y0}" rx="{CORNER}" fill="{rgb(EDGE)}"/>
<rect x="{x0}" y="{y0}" width="{x1 - x0}" height="{y1 - y0}" rx="{CORNER}" fill="url(#top)"/>
<path d="{back}" stroke="{rgb(RING_BACK)}" stroke-opacity="0.7" stroke-width="2.6" fill="none"/>
{arrow(ORIGIN, X_TIP, RED, 4.2, 12, 6.4)}
{arrow(ORIGIN, Y_TIP, GREEN, 4.2, 12, 6.4)}
{arrow(Z_TAIL, Z_TIP, BLUE, 5, 15, 8)}
<circle cx="{ORIGIN[0]}" cy="{ORIGIN[1]}" r="4" fill="{rgb(ORIGIN_DOT)}"/>
<path d="{front}" stroke="{rgb(RING_FRONT)}" stroke-opacity="0.95" stroke-width="2.6" fill="none"/>
</svg>
'''


def symbolic():
    """One color (GTK recolors it with the theme), 16 x 16, the same picture simplified"""
    c = "#2e3436"
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16">
<path d="M 8 4.5 L 8 15" stroke="{c}" stroke-width="1.5" stroke-linecap="round" fill="none"/>
<path d="M 8 1 L 10.75 5 L 5.25 5 Z" fill="{c}"/>
<ellipse cx="8" cy="9.5" rx="6.5" ry="2.3" stroke="{c}" stroke-width="1.2" fill="none"/>
<path d="M 8 9.5 L 3.5 12 M 8 9.5 L 12.5 7.5" stroke="{c}" stroke-width="1.3" stroke-linecap="round" fill="none"/>
</svg>
'''


def main():
    icons = os.path.join(ROOT, "data", "icons", "hicolor")
    for d in ("scalable", "symbolic"):
        os.makedirs(os.path.join(icons, d, "apps"), exist_ok=True)
    with open(os.path.join(icons, "scalable", "apps", APP_ID + ".svg"), "w") as f:
        f.write(svg())
    with open(os.path.join(icons, "symbolic", "apps", APP_ID + "-symbolic.svg"), "w") as f:
        f.write(symbolic())
    embedded = {}
    for size in (16, 32, 48, 64, 128, 256, 512):
        data = png_bytes(size, render(size))
        folder = os.path.join(icons, f"{size}x{size}", "apps")
        os.makedirs(folder, exist_ok=True)
        with open(os.path.join(folder, APP_ID + ".png"), "wb") as f:
            f.write(data)
        if size <= 128:
            embedded[size] = data
        print(f"{size}x{size}  {len(data)} bytes")
    lines = ["// Generated by tools/icon.py: the app icon as PNG bytes (do not edit)", "#pragma once", ""]
    for size, data in embedded.items():
        lines.append(f"static const unsigned char kIconPng{size}[] = {{")
        for i in range(0, len(data), 20):
            lines.append("    " + ", ".join(str(b) for b in data[i:i + 20]) + ",")
        lines.append("};")
        lines.append(f"static const int kIconPng{size}Size = {len(data)};")
        lines.append("")
    with open(os.path.join(ROOT, "src", "ui", "generated", "icon.inc"), "w") as f:
        f.write("\n".join(lines))
    print("src/ui/generated/icon.inc written")


if __name__ == "__main__":
    main()
