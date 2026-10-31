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


def wordmark_svg() -> str:
    return """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 520 120" \
role="img" aria-label="Bench">
  <title>Bench</title>
  <rect x="6" y="6" width="508" height="108" fill="#f4f3ee" stroke="#1a1818" \
stroke-width="6"/>
  <rect x="18" y="18" width="26" height="26" fill="#d92d27" stroke="#1a1818" \
stroke-width="3"/>
  <g fill="#1a1818">
    <rect x="58" y="30" width="14" height="60"/>
    <rect x="70" y="30" width="26" height="10"/>
    <rect x="92" y="30" width="10" height="22"/>
    <rect x="70" y="50" width="24" height="10"/>
    <rect x="70" y="58" width="28" height="10"/>
    <rect x="94" y="66" width="10" height="24"/>
    <rect x="70" y="80" width="26" height="10"/>
  </g>
  <g fill="#1a1818">
    <rect x="130" y="30" width="40" height="10"/>
    <rect x="130" y="30" width="10" height="60"/>
    <rect x="130" y="55" width="32" height="10"/>
    <rect x="130" y="80" width="40" height="10"/>
  </g>
  <g fill="#1a1818">
    <rect x="190" y="30" width="10" height="60"/>
    <rect x="218" y="30" width="10" height="60"/>
    <rect x="190" y="30" width="38" height="10"/>
    <rect x="190" y="80" width="38" height="10"/>
  </g>
  <g fill="#1a1818">
    <rect x="248" y="30" width="10" height="60"/>
    <rect x="276" y="30" width="10" height="60"/>
    <rect x="248" y="30" width="38" height="10"/>
  </g>
  <g fill="#1a1818">
    <rect x="306" y="30" width="10" height="60"/>
    <rect x="334" y="30" width="10" height="60"/>
    <rect x="306" y="30" width="38" height="10"/>
    <rect x="306" y="55" width="30" height="10"/>
  </g>
  <rect x="360" y="30" width="10" height="60" fill="#d92d27"/>
  <rect x="380" y="30" width="46" height="10" fill="#2f5fd0"/>
  <rect x="380" y="55" width="34" height="10" fill="#1f9c8a"/>
  <rect x="380" y="80" width="52" height="10" fill="#8b5cf6"/>
</svg>
"""


def main() -> int:
    (ROOT / "assets-cg").mkdir(exist_ok=True)
    (ROOT / "docs" / "images").mkdir(parents=True, exist_ok=True)

    icon(False).save(ROOT / "assets-cg" / "icon-uns.png")
    icon(True).save(ROOT / "assets-cg" / "icon-sel.png")
    card().save(ROOT / "docs" / "images" / "brand-card.png")
    (ROOT / "docs" / "images" / "brand-wordmark.svg").write_text(wordmark_svg())
    print("wrote assets-cg/icon-uns.png, assets-cg/icon-sel.png,")
    print("      docs/images/brand-card.png, docs/images/brand-wordmark.svg")
    return 0


if __name__ == "__main__":
    sys.exit(main())
