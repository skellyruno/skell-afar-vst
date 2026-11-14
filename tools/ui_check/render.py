#!/usr/bin/env python3
"""Rasterise the draw calls recorded by preview (see preview.cpp) with PIL. Usage: render.py in.txt out.png [w h] [assets_dir]"""
import sys, math
import numpy as np
from PIL import Image, ImageDraw, ImageFont

SS = 2
W = int(sys.argv[3]) if len(sys.argv) > 3 else 1100
H = int(sys.argv[4]) if len(sys.argv) > 4 else 700
ASSETS = sys.argv[5] if len(sys.argv) > 5 else "Assets"
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans%s.ttf"
canvas = Image.new("RGBA", (W * SS, H * SS), (6, 9, 10, 255))
stack = [(0.0, 0.0)]
_fonts = {}
WARN = []

def font(size, bold):
    k = (round(size * 4), bold)
    if k not in _fonts:
        _fonts[k] = ImageFont.truetype(FONT % ("-Bold" if bold else ""), max(6, int(round(size * 0.80 * SS))))
    return _fonts[k]

def off():
    return sum(p[0] for p in stack), sum(p[1] for p in stack)

def paste_layer(mask, box, fill):
    """mask: L image covering box (canvas px). fill: (r,g,b,a) or gradient dict."""
    x0, y0, x1, y1 = box
    w, h = x1 - x0, y1 - y0
    if isinstance(fill, tuple):
        layer = Image.new("RGBA", (w, h), fill)
    else:
        c1, c2, gx1, gy1, gx2, gy2, radial = fill["c1"], fill["c2"], fill["x1"], fill["y1"], fill["x2"], fill["y2"], fill["radial"]
        ys, xs = np.mgrid[y0:y1, x0:x1]
        if radial:
            r = math.hypot(gx2 - gx1, gy2 - gy1) or 1.0
            t = np.clip(np.hypot(xs - gx1, ys - gy1) / r, 0, 1)
        else:
            dx, dy = gx2 - gx1, gy2 - gy1
            den = dx * dx + dy * dy or 1.0
            t = np.clip(((xs - gx1) * dx + (ys - gy1) * dy) / den, 0, 1)
        arr = np.zeros((h, w, 4), np.float32)
        for i in range(4):
            arr[..., i] = c1[i] + (c2[i] - c1[i]) * t
        layer = Image.fromarray(np.clip(arr * 255, 0, 255).astype(np.uint8), "RGBA")
    a = np.asarray(layer.getchannel("A"), np.float32) * np.asarray(mask, np.float32) / 255.0
    layer.putalpha(Image.fromarray(a.astype(np.uint8), "L"))
    canvas.alpha_composite(layer, (x0, y0))

def run(bbox, draw_fn, fill):
    x0, y0, x1, y1 = [int(v) for v in bbox]
    x0 = max(0, x0 - 2); y0 = max(0, y0 - 2); x1 = min(W * SS, x1 + 3); y1 = min(H * SS, y1 + 3)
    if x1 <= x0 or y1 <= y0:
        return
    m = Image.new("L", (x1 - x0, y1 - y0), 0)
    draw_fn(ImageDraw.Draw(m), -x0, -y0)
    paste_layer(m, (x0, y0, x1, y1), fill)

def parse_fill(t):
    r, g, b, a = map(float, t[:4])
    if t[4] == "N":
        return (int(r * 255), int(g * 255), int(b * 255), int(a * 255)), t[5:]
    v = list(map(float, t[5:18]))
    ox, oy = off()
    g_ = dict(c1=v[0:4], c2=v[4:8], x1=(v[8] + ox) * SS, y1=(v[9] + oy) * SS, x2=(v[10] + ox) * SS, y2=(v[11] + oy) * SS, radial=bool(v[12]))
    return g_, t[18:]

for line in sys.stdin if len(sys.argv) < 2 or sys.argv[1] == "-" else open(sys.argv[1]):
    t = line.rstrip("\n").split(" ")
    k = t[0]
    ox, oy = off()
    S = lambda v: v * SS
    if k == "PUSH":
        stack.append((float(t[1]), float(t[2]))); continue
    if k == "POP":
        stack.pop(); continue
    if k == "I":
        name = t[1]; dx, dy, dw, dh, sx, sy, sw, sh = map(float, t[2:10])
        try:
            im = Image.open(f"{ASSETS}/{name}.png").convert("RGBA")
        except Exception:
            continue
        im = im.crop((int(sx), int(sy), int(sx + sw), int(sy + sh))).resize((max(1, int(dw * SS)), max(1, int(dh * SS))), Image.LANCZOS)
        px, py = int((dx + ox) * SS), int((dy + oy) * SS)
        # clip to canvas
        canvas.alpha_composite(im, (max(px, 0), max(py, 0)), (max(-px, 0), max(-py, 0), im.width, im.height)) if px < W * SS and py < H * SS else None
        continue
    if k == "T":
        maxlines = int(t[1]); r, g, b, a = map(float, t[2:6]); x, y, w, h, size = map(float, t[6:11]); bold = int(t[11]); just = int(t[12]); text = " ".join(t[13:])
        f = font(size, bold)
        layer = Image.new("RGBA", canvas.size, (0, 0, 0, 0)); d = ImageDraw.Draw(layer)
        X, Y, Wd, Ht = S(x + ox), S(y + oy), S(w), S(h)
        # DejaVu is ~8% wider than the UI fonts: judge overflow with that allowance
        fits = lambda s_: d.textlength(s_, font=f) / 1.08 <= Wd + 1
        if maxlines <= 1:
            lines = [text]
            if not fits(text):
                WARN.append(f"text wider than its box ({d.textlength(text, font=f)/1.08/SS:.0f}px > {w:.0f}px): '{text[:50]}' at ({x+ox:.0f},{y+oy:.0f})")
        else:
            def wrap(ok):
                out, cur = [], ""
                for word in text.split(" "):
                    trial = (cur + " " + word).strip()
                    if ok(trial) or not cur: cur = trial
                    else: out.append(cur); cur = word
                return out + [cur]
            lines = wrap(lambda s_: d.textlength(s_, font=f) <= Wd + 1)           # what the preview draws (strict)
            needed = wrap(fits)                                                     # with the real-font allowance
            if len(needed) > maxlines:
                WARN.append(f"text needs {len(needed)} lines but allows {maxlines}: '{text[:50]}...' at ({x+ox:.0f},{y+oy:.0f})")
            if len(needed) * f.size * 1.25 > Ht + 2:
                WARN.append(f"wrapped text taller than its box: '{text[:50]}...' at ({x+ox:.0f},{y+oy:.0f})")
        lh = f.size * 1.25
        top = Y if (just == 4 or len(lines) > 1) else Y + (Ht - f.size) / 2 - f.size * 0.12
        for i, ln in enumerate(lines):
            tw = d.textlength(ln, font=f)
            tx = X + (Wd - tw) / 2 if just == 1 else (X + Wd - tw if just in (3, 6) else X)
            d.text((tx, top + i * lh), ln, font=f, fill=(int(r * 255), int(g * 255), int(b * 255), int(a * 255)))
        canvas.alpha_composite(layer); continue
    fill, rest = parse_fill(t[1:])
    v = list(map(float, rest))
    if k == "L":
        x0, y0, x1, y1, th = v
        pts = [S(x0 + ox), S(y0 + oy), S(x1 + ox), S(y1 + oy)]
        run((min(pts[0], pts[2]) - th * SS, min(pts[1], pts[3]) - th * SS, max(pts[0], pts[2]) + th * SS, max(pts[1], pts[3]) + th * SS),
            lambda d, ax, ay: d.line([(pts[0] + ax, pts[1] + ay), (pts[2] + ax, pts[3] + ay)], fill=255, width=max(1, int(th * SS))), fill)
    elif k in ("FE", "DE"):
        x, y, w, h = v[:4]; th = v[4] if k == "DE" else 0
        b = [S(x + ox), S(y + oy), S(x + w + ox), S(y + h + oy)]
        if k == "FE": run(b, lambda d, ax, ay: d.ellipse([b[0] + ax, b[1] + ay, b[2] + ax, b[3] + ay], fill=255), fill)
        else: run(b, lambda d, ax, ay: d.ellipse([b[0] + ax, b[1] + ay, b[2] + ax, b[3] + ay], outline=255, width=max(1, int(th * SS))), fill)
    elif k in ("FR", "DR"):
        x, y, w, h, rad = v[:5]; th = v[5] if k == "DR" else 0
        b = [S(x + ox), S(y + oy), S(x + w + ox), S(y + h + oy)]
        if k == "FR": run(b, lambda d, ax, ay: d.rounded_rectangle([b[0] + ax, b[1] + ay, b[2] + ax, b[3] + ay], radius=S(rad), fill=255), fill)
        else: run(b, lambda d, ax, ay: d.rounded_rectangle([b[0] + ax, b[1] + ay, b[2] + ax, b[3] + ay], radius=S(rad), outline=255, width=max(1, int(th * SS))), fill)
    elif k == "SP":
        th = v[0]; pts = [(S(v[i] + ox), S(v[i + 1] + oy)) for i in range(1, len(v) - 1, 2)]
        if len(pts) >= 2:
            xs = [p[0] for p in pts]; ys = [p[1] for p in pts]
            run((min(xs) - th * SS, min(ys) - th * SS, max(xs) + th * SS, max(ys) + th * SS),
                lambda d, ax, ay: d.line([(p[0] + ax, p[1] + ay) for p in pts], fill=255, width=max(1, int(th * SS)), joint="curve"), fill)
    elif k == "FP":
        pts = [(S(v[i] + ox), S(v[i + 1] + oy)) for i in range(0, len(v) - 1, 2)]
        if len(pts) >= 3:
            xs = [p[0] for p in pts]; ys = [p[1] for p in pts]
            run((min(xs), min(ys), max(xs), max(ys)), lambda d, ax, ay: d.polygon([(p[0] + ax, p[1] + ay) for p in pts], fill=255), fill)

out = canvas.resize((W, H), Image.LANCZOS).convert("RGB")
out.save(sys.argv[2])
for w_ in dict.fromkeys(WARN): print("LAYOUT WARNING:", w_)
