#!/usr/bin/env python3
"""Convert the simulator's PPM screenshots to 2x PNGs.

    python3 tools/sim/ppm2png.py docs/images

Every .ppm in the folder gets a .png next to it, scaled 2x with
nearest-neighbour so a 396x224 frame reads comfortably on a normal screen.
Needs Pillow (`pip install pillow`); without it the PPMs are left as they are.
"""

import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:  # pragma: no cover - optional tool
    print("Pillow is not installed; PNG conversion skipped")
    sys.exit(1)


def main(argv):
    folder = Path(argv[1] if len(argv) > 1 else "docs/images")
    count = 0
    for ppm in sorted(folder.glob("*.ppm")):
        image = Image.open(ppm).convert("RGB")
        big = image.resize((image.width * 2, image.height * 2), Image.NEAREST)
        # The screens use a handful of flat colours, so a 16-colour palette is
        # lossless here and keeps the repository small.
        big.quantize(colors=16, method=Image.MEDIANCUT, dither=Image.NONE).save(
            ppm.with_suffix(".png"), optimize=True
        )
        # The PPM is a scratch frame: the PNG is the artefact that is kept.
        ppm.unlink()
        count += 1
    print(f"{count} screenshot(s) converted in {folder}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
