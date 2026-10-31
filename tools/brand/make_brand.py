#!/usr/bin/env python3
"""Draw the Bench brand assets: the two menu icons and the wordmark.

Neobrutalist rules used here, from the three reference pages:
  * flat blocks of saturated colour, no gradients
  * a heavy black outline on everything
  * a hard offset shadow instead of a soft one
  * geometric shapes only: square, circle, triangle, bars
  * one accent colour per surface, everything else paper or ink

Outputs
    assets-cg/icon-uns.png   92x64, unselected tile in the calculator menu
    assets-cg/icon-sel.png   92x64, selected tile
    docs/images/brand-wordmark.svg   the wordmark, for the README and the site
    docs/images/brand-card.png       a 1200x630 card, for release notes

    python3 tools/brand/make_brand.py

Needs Pillow.  The .svg is written directly, no library.
"""

import sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:  # pragma: no cover - optional tool
    print("Pillow is required: pip install pillow", file=sys.stderr)
    sys.exit(1)


PAPER = (244, 243, 238)
INK = (26, 24, 24)
RED = (217, 45, 39)
YELLOW = (242, 194, 48)
BLUE = (47, 95, 208)
TEAL = (31, 156, 138)
VIOLET = (139, 92, 246)

ROOT = Path(__file__).resolve().parents[2]


def draw_b(draw, x, y, w, h, bg):
    """A blocky capital B: a stem plus two solid bowls, counters left open.

    The counters are drawn in the background colour, so the same routine works
    on paper and on yellow."""
    t = max(3, h // 7)               # bar thickness
    stem = max(4, w // 4)
    mid = y + h // 2
    draw.rectangle([x, y, x + stem - 1, y + h - 1], fill=INK)
    draw.rectangle([x, y, x + w - 1, y + t - 1], fill=INK)
    draw.rectangle([x, mid - t // 2, x + w - t - 1, mid + (t - t // 2) - 1],
                   fill=INK)
    draw.rectangle([x, y + h - t, x + w - 1, y + h - 1], fill=INK)
    draw.rectangle([x + w - t, y, x + w - 1, y + h - 1], fill=INK)
    # Counters.
    draw.rectangle([x + stem, y + t, x + w - t - 1, mid - t // 2 - 1], fill=bg)
    draw.rectangle([x + stem, mid + (t - t // 2), x + w - t - 1, y + h - t - 1],
                   fill=bg)


def icon(selected: bool) -> Image.Image:
    w, h = 92, 64
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    bg = YELLOW if selected else PAPER
    inset = 2
    # Hard offset shadow first, then the face, then the 3 px outline.
    d.rectangle([inset + 4, inset + 4, w - 1, h - 1], fill=INK)
    d.rectangle([inset, inset, w - 5, h - 5], fill=bg)
    d.rectangle([inset, inset, w - 5, h - 5], outline=INK, width=3)

    # Red square: the one accent, top left, sits over the outline.
    d.rectangle([8, 8, 22, 22], fill=RED)
    d.rectangle([8, 8, 22, 22], outline=INK, width=2)

    # Bergeometric B.
    draw_b(d, 32, 12, 32, 40, bg)

    # Three bars bottom right: the same bars as the Number theory tile.
    base = 52
    for i, bh in enumerate((8, 13, 18)):
        bx = 62 + i * 5
        d.rectangle([bx, base - bh, bx + 3, base], fill=INK if i != 1 else TEAL)

    # A black rule along the bottom edge.
    d.rectangle([8, 56, w - 12, 59], fill=INK)
    return img


def card() -> Image.Image:
    """1200x630 social card: the wordmark, the five tools, one screenshot."""
    w, h = 1200, 630
    img = Image.new("RGB", (w, h), PAPER)
    d = ImageDraw.Draw(img)

    d.rectangle([0, 0, w, 96], fill=INK)
    d.rectangle([0, 96, 250, h], fill=YELLOW)
    d.rectangle([250, 96, w, 300], fill=BLUE)
    d.rectangle([250, 300, w, h], fill=PAPER)

    # Wordmark blocks: B E N C H as chunky geometry, no font.
    draw_b(d, 40, 150, 90, 110, YELLOW)
    for i, col in enumerate((RED, TEAL, VIOLET, YELLOW)):
        x = 160 + i * 90
        d.rectangle([x, 150, x + 70, 260], fill=col)
        d.rectangle([x, 150, x + 70, 260], outline=INK, width=5)

    shot = ROOT / "docs" / "images" / "01_menu.png"
    if shot.exists():
        menu = Image.open(shot).convert("RGB").resize((768, 432))
        img.paste(menu, (300, 120))
        d.rectangle([300, 120, 300 + 768, 120 + 432], outline=INK, width=6)
    return img


WORDMARK_FONTS = [
    "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
]


def wordmark() -> "Image.Image":
    """BENCH in a heavy grotesque, black, with the red square for the counter.

    Drawn from a real font (DejaVu Sans Bold) rather than assembled out of
    rectangles: the previous hand-built version was unreadable.
    """
    from PIL import ImageFont

    font = None
    for path in WORDMARK_FONTS:
        try:
            font = ImageFont.truetype(path, 132)
            break
        except OSError:
            continue
    if font is None:
        raise SystemExit("DejaVu Sans Bold not found; install fonts-dejavu")

    text = "BENCH"
    pad = 26
    left, top, right, bottom = font.getbbox(text)
    tw, th = right - left, bottom - top
    w, h = tw + pad * 2 + 54, th + pad * 2
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    # Paper plate with the hard offset shadow, the house style.
    d.rectangle([4, 4, w - 1, h - 1], fill=INK)
    d.rectangle([0, 0, w - 5, h - 5], fill=PAPER)
    d.rectangle([0, 0, w - 5, h - 5], outline=INK, width=6)

    # Red square: the one accent, centred on the middle of the cap height so
    # it reads as a mark next to the word and not as a raised apostrophe.
    mid = pad + th // 2
    half = 16
    d.rectangle([pad - 6, mid - half, pad - 6 + 2 * half, mid + half], fill=RED)
    d.rectangle([pad - 6, mid - half, pad - 6 + 2 * half, mid + half], outline=INK,
                width=4)

    d.text((pad + 46 - left, pad - top), text, font=font, fill=INK)
    return img


def wordmark_svg() -> str:
    """The same wordmark as live text: no hand-built letterforms."""
    return """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 620 190" \
role="img" aria-label="Bench">
  <title>Bench</title>
  <rect x="6" y="6" width="606" height="176" fill="#1a1818"/>
  <rect x="0" y="0" width="606" height="176" fill="#f4f3ee" stroke="#1a1818" \
stroke-width="6"/>
  <rect x="24" y="58" width="36" height="36" fill="#d92d27" stroke="#1a1818" \
stroke-width="4"/>
  <text x="78" y="132" fill="#1a1818"
        font-family="'DejaVu Sans', 'Helvetica Neue', Arial, sans-serif"
        font-size="132" font-weight="900" letter-spacing="-2">BENCH</text>
</svg>
"""


def main() -> int:
    (ROOT / "assets-cg").mkdir(exist_ok=True)
    (ROOT / "docs" / "images").mkdir(parents=True, exist_ok=True)

    icon(False).save(ROOT / "assets-cg" / "icon-uns.png")
    icon(True).save(ROOT / "assets-cg" / "icon-sel.png")
    card().save(ROOT / "docs" / "images" / "brand-card.png")
    wordmark().save(ROOT / "docs" / "images" / "brand-wordmark.png")
    (ROOT / "docs" / "images" / "brand-wordmark.svg").write_text(wordmark_svg())
    print("wrote assets-cg/icon-uns.png, assets-cg/icon-sel.png,")
    print("      docs/images/brand-card.png, brand-wordmark.png/.svg")
    return 0


if __name__ == "__main__":
    sys.exit(main())
