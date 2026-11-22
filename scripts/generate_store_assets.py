#!/usr/bin/env python3
"""Generate Google Play Store assets for Euterpe:
  - store/icon-512.png (512x512)
  - store/feature-graphic-1024x500.png (1024x500)
  - store/phone/01..04.png (1920x1080, 16:9)
  - store/tablet-7/01..04.png (1920x1200, 16:10)
  - store/tablet-10/01..04.png (2560x1600, 16:10)
"""
from pathlib import Path
import math
from PIL import Image, ImageDraw, ImageFont, ImageFilter

ROOT = Path(__file__).resolve().parent.parent
IMAGES = ROOT / "images"
STORE = ROOT / "playstore"

BG = (14, 17, 26)
SURFACE = (22, 27, 41)
BORDER = (43, 52, 77)
GOLD = (232, 185, 89)
IVORY = (244, 227, 178)
MUTED = (154, 164, 188)
TEXT = (241, 243, 248)
MOON_DARK = (28, 37, 65)
MOON_STROKE = (46, 59, 99)


def load_font(size: int, bold: bool = False):
    candidates = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    ]
    for path in candidates:
        if Path(path).exists():
            return ImageFont.truetype(path, size)
    return ImageFont.load_default()


def draw_euterpe_icon(size: int = 512) -> Image.Image:
    """Render the exact 4-string golden lyre + 1/4 waxing crescent moon at 4x supersampling."""
    scale = 4
    S = size * scale
    img = Image.new("RGBA", (S, S), BG + (255,))
    draw = ImageDraw.Draw(img)

    cx, cy = S / 2, S / 2
    u = S / 108.0  # matches 108x108 Android vector viewport

    # Subtle radial lunar glow behind moon
    glow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    gd = ImageDraw.Draw(glow)
    r_glow = 42 * u
    gd.ellipse([cx - r_glow, cy - r_glow, cx + r_glow, cy + r_glow], fill=(232, 185, 89, 28))
    glow = glow.filter(ImageFilter.GaussianBlur(radius=12 * u))
    img = Image.alpha_composite(img, glow)
    draw = ImageDraw.Draw(img)

    # Moon dark disc (r = 31)
    r = 31 * u
    draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=MOON_DARK, outline=MOON_STROKE, width=max(1, int(1.2 * u)))

    # 1/4 lit waxing crescent on right side (outer semicircle + inner elliptical terminator rx=16)
    crescent_pts = []
    steps = 160
    for i in range(steps + 1):
        ang = -math.pi / 2 + math.pi * (i / steps)
        crescent_pts.append((cx + r * math.cos(ang), cy + r * math.sin(ang)))
    rx = 16 * u
    for i in range(steps + 1):
        ang = math.pi / 2 - math.pi * (i / steps)
        crescent_pts.append((cx + rx * math.cos(ang), cy + r * math.sin(ang)))
    draw.polygon(crescent_pts, fill=IVORY)

    # Subtle orbital dot (Lua nod)
    dx, dy, dr = 79 * u, 29.2 * u, 3.2 * u
    draw.ellipse([dx - dr, dy - dr, dx + dr, dy + dr], fill=GOLD)

    # Lyre crossbar
    draw.line([34 * u, 37 * u, 74 * u, 37 * u], fill=GOLD, width=int(3.2 * u))
    for end_x in (34 * u, 74 * u):
        cap = 1.6 * u
        draw.ellipse([end_x - cap, 37 * u - cap, end_x + cap, 37 * u + cap], fill=GOLD)

    # Lyre curved arms (cubic bezier approximation)
    def bezier(p0, p1, p2, p3, n=48):
        pts = []
        for i in range(n + 1):
            t = i / n
            mt = 1 - t
            x = mt**3 * p0[0] + 3 * mt**2 * t * p1[0] + 3 * mt * t**2 * p2[0] + t**3 * p3[0]
            y = mt**3 * p0[1] + 3 * mt**2 * t * p1[1] + 3 * mt * t**2 * p2[1] + t**3 * p3[1]
            pts.append((x * u, y * u))
        return pts

    arm_pts = (
        bezier((39, 34), (33, 44), (33, 61), (42, 71))
        + bezier((42, 71), (46, 75), (50, 77), (54, 77))[1:]
        + bezier((54, 77), (58, 77), (62, 75), (66, 71))[1:]
        + bezier((66, 71), (75, 61), (75, 44), (69, 34))[1:]
    )
    draw.line(arm_pts, fill=GOLD, width=int(3.4 * u), joint="curve")

    # Lyre soundbox
    box_pts = [(42 * u, 68 * u), (66 * u, 68 * u)] + bezier((66, 68), (66, 75), (61, 79), (54, 79)) + bezier((54, 79), (47, 79), (42, 75), (42, 68))
    draw.polygon(box_pts, fill=(201, 148, 56), outline=(244, 211, 126))

    # Exactly 4 taut vertical strings
    for sx in (46.5, 51.5, 56.5, 61.5):
        draw.line([sx * u, 37 * u, sx * u, 68 * u], fill=(255, 245, 214), width=max(2, int(1.4 * u)))

    return img.resize((size, size), Image.Resampling.LANCZOS).convert("RGB")


def draw_feature_graphic() -> Image.Image:
    W, H = 1024, 500
    img = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(img)

    # Subtle ambient background bands
    for y in range(H):
        t = y / H
        r = int(14 + 10 * (1 - abs(t - 0.35)))
        g = int(17 + 12 * (1 - abs(t - 0.35)))
        b = int(26 + 20 * (1 - abs(t - 0.35)))
        draw.line([(0, y), (W, y)], fill=(r, g, b))

    # Place crisp icon on the left
    icon = draw_euterpe_icon(300)
    mask = Image.new("L", (300, 300), 0)
    ImageDraw.Draw(mask).rounded_rectangle([0, 0, 299, 299], radius=64, fill=255)
    img.paste(icon, (68, 100), mask)

    # Typography on the right
    f_eyebrow = load_font(18, bold=True)
    f_title = load_font(76, bold=True)
    f_sub = load_font(26, bold=False)
    f_tags = load_font(19, bold=True)

    tx = 415
    draw.text((tx, 118), "OPEN-SOURCE ANDROID INSTRUMENT", fill=GOLD, font=f_eyebrow)
    draw.text((tx, 148), "EUTERPE", fill=IVORY, font=f_title)
    draw.text((tx, 242), "Cross-Cultural Musical Idea Generator", fill=TEXT, font=f_sub)
    draw.text((tx, 292), "Maqams · Greek Dromoi · Ragas · Gamelan", fill=MUTED, font=f_tags)
    draw.text((tx, 322), "Techno · Metal · Pop · Polivoks · TB-303", fill=MUTED, font=f_tags)
    draw.text((tx, 368), "8-Layer Looper · World Percussion · Lua 5.4", fill=GOLD, font=load_font(17, bold=True))

    return img


def make_framed_shot(src_path: Path, target_size: tuple[int, int], title: str, subtitle: str) -> Image.Image:
    W, H = target_size
    canvas = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(canvas)

    # Top header band
    header_h = int(H * 0.13)
    f_title = load_font(int(H * 0.042), bold=True)
    f_sub = load_font(int(H * 0.023), bold=False)

    draw.text((int(W * 0.04), int(H * 0.026)), title, fill=IVORY, font=f_title)
    draw.text((int(W * 0.04), int(H * 0.080)), subtitle, fill=MUTED, font=f_sub)

    # Fit live screenshot cleanly below header with subtle gold/slate frame
    shot = Image.open(src_path).convert("RGB")
    pad_x = int(W * 0.03)
    pad_b = int(H * 0.03)
    avail_w = W - pad_x * 2
    avail_h = H - header_h - pad_b
    ratio = min(avail_w / shot.width, avail_h / shot.height)
    nw, nh = int(shot.width * ratio), int(shot.height * ratio)
    resized = shot.resize((nw, nh), Image.Resampling.LANCZOS)

    ox = (W - nw) // 2
    oy = header_h + (avail_h - nh) // 2

    draw.rounded_rectangle(
        [ox - 3, oy - 3, ox + nw + 3, oy + nh + 3],
        radius=16,
        outline=BORDER,
        width=3,
    )
    canvas.paste(resized, (ox, oy))
    return canvas


def main():
    (STORE / "phone").mkdir(parents=True, exist_ok=True)
    (STORE / "tablet-7").mkdir(parents=True, exist_ok=True)
    (STORE / "tablet-10").mkdir(parents=True, exist_ok=True)

    draw_euterpe_icon(512).save(STORE / "euterpe-icon-512x512.png")
    draw_feature_graphic().save(STORE / "euterpe-feature-graphic-1024x500.png")

    shots = [
        (
            "01-performance-surface.png",
            IMAGES / "screenshot-main.png",
            "Play Across Musical Cultures",
            "2-octave microtonal keyboard · 8 selectable layers · Maqams, Greek Dromoi, Ragas, Gamelan, Techno, Metal & Pop",
        ),
        (
            "02-mix-percussion-fusion.png",
            IMAGES / "screenshot-lab.png",
            "Swappable World Percussion & Exotic Fusion",
            "20 synthesized percussion voices · 6 swappable kits · asymmetric meters (7/8, 9/8, 10/8, 11/8, 13/8, 16-beat Tala)",
        ),
        (
            "03-insert-master-fx.png",
            IMAGES / "screenshot-fx.png",
            "Insert & Master FX Chain",
            "Tube Drive, Stereo Chorus & BPM-synced Ping-Pong Delay per layer · Master Reverb & Glue Compressor",
        ),
        (
            "04-sound-macros-shaping.png",
            IMAGES / "screenshot-sound.png",
            "21 YAWN DSP Presets & 1-Tap Sound Macros",
            "Karplus-Strong, FM, Electric Piano, Wavetable, Subtractive TB-303 & Polivoks 1982 · WARM / ACID / GRIT / DREAM",
        ),
    ]

    for fname, src, title, sub in shots:
        make_framed_shot(src, (1920, 1080), title, sub).save(STORE / "phone" / f"euterpe-phone-1920x1080-{fname}")
        make_framed_shot(src, (1920, 1200), title, sub).save(STORE / "tablet-7" / f"euterpe-tablet7-1920x1200-{fname}")
        make_framed_shot(src, (2560, 1600), title, sub).save(STORE / "tablet-10" / f"euterpe-tablet10-2560x1600-{fname}")

    print("Generated all Play Store assets under store/")


if __name__ == "__main__":
    main()
