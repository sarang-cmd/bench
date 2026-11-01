# Design system

Neobrutalist: flat colour, square corners, thick ink lines, one hard shadow.
Nothing here is decorative guesswork — the numbers that matter are measured by
`make test`, and a screen that breaks a rule fails the build.

## 1. Palette

Eight colours, no more. The 5-6-5 values are what the hardware stores; the hex
is what the design means. `tools/sim` checks one against the other on every run
(the `palette` check), so a hand-edited constant cannot drift.

| Name | Hex | 5-6-5 | Role |
|---|---|---|---|
| paper | `#f4f3ee` | `0xF79D` | the page, unselected tile bodies, text on ink |
| ink | `#2f2d2e` | `0x3166` | text, borders, shadows |
| red | `#d92d27` | `0xD165` | Pacer, headings, rules, alert blocks |
| yellow | `#f2c230` | `0xEE06` | Cards, the active field, the title hint |
| blue | `#2f5fd0` | `0x32F9` | Uncertainty, H2 headings, good news |
| teal | `#1f9c8a` | `0x24F1` | Notes |
| violet | `#8b5cf6` | `0x8AFE` | Number theory |
| shade | `#d9d7cd` | `0xD6B9` | panels, the softkey strip, code background |

Every colour goes through `col_rgb565()` in `src/core/color.c`. The table above
is the output of that function, not a hand conversion — an earlier version was
converted by hand and was wrong by one step in six of the eight colours.

## 2. Shape

- Flat fills only. No gradients, no rounded corners, no anti-aliasing.
- Borders: `TH_BORDER` (2 px) on panels and unselected tiles,
  `TH_BORDER_SEL` (3 px) on the selected tile.
- Hard offset shadow: `TH_SHADOW` (3 px) right and bottom, drawn outside the
  shape as an L — never inside it, or a filled tile swallows its own shadow.
  Selected tiles and modals only; a shadow on everything is noise at 384x216.
- Icons: one shape per tool, drawn twice — `edge` grown by `TH_ICON_OL`, then
  `fill`. That outline is why an icon can never disappear into its tile. The
  outline grows the shape by `TH_ICON_OL` on every side, not just horizontally,
  so an icon stays inside its nominal 20x20 box and never touches a border.

## 3. A tile

The home screen shows six tiles a page, three across and two down. Each tile is
a 20x20 icon with two lines under it, both left aligned to the same 7 px margin:

```
+-------------------+
|  [icon]           |   icon at 7,7 in the tile's foreground colour,
|                   |   outlined in the other one
|  Title            |   title: the 3:1 floor
|  subtitle         |   subtitle: the 4.5:1 floor
+-------------------+
```

- Unselected: paper fill, 2 px ink border, no shadow, icon in the tool's accent
  (ink when the accent is too close to paper - shade is 1.31:1).
- Selected: the tool's accent as the fill, 3 px ink border and the 3 px hard
  shadow, icon in whichever of ink and paper reads better on that accent,
  outlined in the other.
- Text on an accent is ink or paper, whichever measures better. Where even the
  better one misses its floor (red 4.46, teal 4.09, violet 3.79) the subtitle
  sits on a paper plate that hugs the words. That plate is the only decoration
  the selected tile gets, and it is why the selected tile reads at 12:1
  whatever the accent is.

### Paging

More than six tools in a category get pages. The marker is a bordered pill under
the grid, `1 / 2`, with a red arrow on each side that leads somewhere — only the
live arrows are drawn, because a greyed one would be shade on paper at 1.31:1
and the screen checker rejects it (correctly: at that contrast it is a smudge).
The pill only takes the extra band when the category actually pages, so a
category with one page keeps the whole grid.

The grid is centred on the rows **of the page being drawn**. Counting the whole
category is a bug that shipped once: nine Math tools meant three rows of height
inside a two-row page, a negative centring offset, and the first row drawn over
the title bar.

A row that is not full is centred under the one above it: Tools holds two
tiles, and two tiles against the left margin with a third of the display empty
beside them is what "awkward" meant. `core/layout.c` holds that arithmetic so it
can be tested without a screen. A full row lands on the margin either way.

## 4. Type

One font, one size, `TH_LINE` of leading. Hierarchy comes from colour, weight
and position, not from size. Tile titles are the largest thing on the home
screen by position, not by pixels — which is why they are held to the
large-text contrast floor and body text is not.

## 5. Contrast

Measured, not chosen. `col_contrast()` implements WCAG relative luminance with a
generated sRGB table (`lin_table.h`), because the calculator build has no `pow`.

Two floors:

| Text | Floor | Why |
|---|---|---|
| title bar, tile title | 3.0:1 | WCAG AA large text; these are the biggest, boldest labels |
| every other label | 4.5:1 | WCAG AA body text |
| an icon against its tile | 1.5:1 | enough to see two colours apart in daylight; the real rule is "never the same colour" |

The measurements that shaped the rules:

| Pair | Ratio | Verdict |
|---|---|---|
| ink on paper | 12.27 | the body text |
| ink on red | 2.75 | **not readable** |
| paper on red | 4.46 | fine for a title, short of AA for a subtitle |
| ink on teal | 4.09 | not readable |
| paper on teal | 3.00 | not readable |
| paper on violet | 3.79 | not readable |
| paper on blue | 5.30 | the one accent that carries paper text |
| ink on yellow / shade / ink | 8.02 / 9.33 / 12.27 | all fine |
| paper icon on shade | 1.31 | the Settings tile, and the trap that started this |

So the rule is not "ink, or paper on blue and violet". It is:

1. **Pick the foreground by measurement.** `col_better_on(fill, ink, paper)`
   returns whichever of the two reads better on that fill. A tool's accent is
   used for its icon only where it clears 1.5:1 against paper (shade does not;
   the Settings tile gets an ink icon).
2. **Never put body text on an accent fill.** Red, teal and violet cannot
   carry a subtitle at 4.5:1 in either ink or paper. The first version of this
   screen solved that with a paper plate under each line; it worked, and it
   looked like a form. The tile now inverts instead.
3. **Selection is an invert, not a tint.** An unselected tile is paper with ink
   text. The selected tile is ink with paper text — 12.27:1 for the title and
   9.33:1 for the subtitle, whatever the accent is — plus the 3 px hard shadow
   and the 3 px border. The tool's colour survives as the icon, outlined in
   paper so it cannot merge into the ink.
4. **Never draw a foreground in the colour behind it.** No text on a fill of the
   same colour, no icon on a tile of its own colour. `tools/sim` fails the build
   if a label's own pixels are painted over by a later fill.

That is the whole contrast story: one measured choice for the icon, one invert
for selection, and a checker that reads the finished framebuffer.

## 6. The checks

`make test` builds `tools/sim` and runs every key script in `tools/sim/tour`
through two checks. They are the acceptance test for this document.

The add-in reports each rectangle it draws (`SCR_TRACE` in
`src/platform/screen.h`); on the calculator the macro compiles to nothing, so
the `.g3a` is unchanged. The host preview then fails the run on:

| Check | Rule |
|---|---|
| labels overlap | two labels in the same pixels |
| outside the window | a label that leaves the 384x216 area |
| title bar or softkey strip | a content label drawn in either |
| softkey label wider than its cell | a softkey that does not fit 64 px |
| label painted over | a later panel covering part of an earlier label |
| label on a border | a label within 2 px of a panel, tile or key border, inside or out |
| label under the contrast floor | measured from the pixels, not the constants |
| icon and tile the same | an icon at 1.5:1 or less against its fill |
| palette | a `TH_*` constant that is not the documented hex |

The border check exists because "not overlapping" is not the same as "not
touching": the menu's first-run hint sat with its ascenders on the bottom tile
border, which reads as clipped text, and every screenshot check passed. Frames
are traced through `scr_frame`, since `scr_fill` records only panels of 8x4 or
more and a two-pixel border is exactly the size that goes wrong.

The contrast check samples the framebuffer, so it catches what the constants
say *and* what actually got drawn — the first version of this check found that
it had been sampling raw framebuffer coordinates instead of the add-in's window
coordinates, and had been "measuring" the 6,4 px OS border.

## 7. Applying the system

- Home screen: `src/apps/app_menu.c`
- Tiles and icons: `src/apps/registry.c`
- Widgets: `src/ui/widgets.c`, flat fills and `TH_BORDER`
- Shapes: `src/ui/shapes.c`, rasterised without libm
- Site: the same palette, the same hard shadows, the same square corners
- Brand files: `tools/brand/make_brand.py`
