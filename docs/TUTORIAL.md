# Tutorial: ten minutes with Bench

This is the same walkthrough the site's `/tutorial` page shows, in text. It
assumes the add-in is installed and you are looking at the menu.

If a key does nothing, check the tool's own help page (F5 everywhere) before
assuming the add-in is broken: every tool lists its keys in the last lines of
its help.

## 1. The home screen

The title bar says **Bench** on the left and the category on the right. The strip
under it is the dashboard: how many decks and how many notes are on the card.

| Key | What it does |
|---|---|
| F1 | Study tools: Pacer, Cards, Notes, Study log |
| F2 | Maths: Function table, Triangle, Polynomials, Root finder, Sequences, Complex numbers, Statistics |
| F3 | Science: Lab graph, Uncertainty, Sig figs, SUVAT, Moles |
| F4 | Tools: Settings, About |
| Arrows | Move between tiles |
| EXE | Open the tile under the cursor |
| F5 | Help for the tile under the cursor |

The first time you run Bench, a line under the tiles says "F1-F4 categories, F5
explains a tile, EXE opens it". It disappears for good the first time you press
F5, and the setting that remembers this is `tour_done` in
`Bench/state/settings.cfg`.

## 2. A tool is a screen, not a dialog

Every tool keeps its fields, its operation and its answer on one screen. The
pattern is the same everywhere:

- **F1 to F4** pick the operation or the mode, named in the softkeys at the
  bottom.
- **Arrows** move the cursor between fields.
- **Digits** type into the selected field; DEL deletes a character.
- **EXE** applies the operation, or moves to the next field when there is
  nothing to apply yet.
- **EXIT** goes back to the menu.

## 3. Put a note in `Bench/notes`

Connect the calculator in USB Flash mode and copy `.md` or `.txt` files into
`Bench/notes` in the **drive root** — the same place the `.g3a` went, beside
`@MainMem` and not inside it. Then open **Notes** in the Study category.

| Key | What it does |
|---|---|
| UP / DOWN | Scroll a line |
| LEFT / RIGHT | Page up / page down |
| F1 | Search: opens the on-screen keyboard |
| F2 | Next search hit |
| F3 | Heading list (UP/DOWN, EXE jumps to one) |
| F4 | Back to the top |
| EXE / EXIT | Back to the file list |

Notes draws headings, bold, italics, code, bullets, numbered lists and quotes in
the palette, and wraps long lines. The wrap width is a setting.

## 4. Run a card session

A deck is one text file in `Bench/cards`, one card per line:

```
What is the SI unit of force? | The newton, N.
```

Lines starting with `#` are comments. Open **Cards**, pick a deck, press F1.
A right answer (F1 during the session) moves the card to the next box; a wrong
one (F2) sends it back to the first. Progress is saved once, at the end of the
session, never during it.

F2 on the deck screen mixes the order inside each box, so two sessions on the
same deck are not the same sequence.

## 5. Log the minutes

The study log is a plain text file, `Bench/study/log.txt`, one session per line:

```
2026-10-31 25 pacer
```

Open **Study log** in the Study category. The screen totals the minutes, counts
the days, shows the longest run of consecutive days and a table of tools. F1
adds a session, F2 deletes the last one, LEFT and RIGHT change minutes, the tool
and the day. The file is written once, when you press EXIT.

There is no calendar in the calculator's clock, so the day is worked out from the
newest line in the file and adjusted with UP and DOWN on the day field. A line
that does not parse is skipped and counted in the footer, never guessed at.

## 6. Make it yours

**Settings** (Tools, or F6 from most screens) has five rows:

| Row | What it changes |
|---|---|
| Uncertainty figures | One figure, or two when the first digit is 1 |
| Pacer checkpoint | Marks between checkpoints, 1 to 200 |
| Note width | Fit the screen, or 50 to 150 percent |
| Angles default | The unit Triangle opens in: degrees or radians |
| First-run hint | Whether the home-screen line has been read |

Every value is one line in `Bench/state/settings.cfg`, safe to edit by hand, and
written only when it changes. A file from 1.0.0 still loads: the keys it does not
have fall back to their defaults.

## When a screen is empty

Every screen that can be empty says why instead of showing a blank page:

| Screen | What it says |
|---|---|
| Cards, no decks | "No decks yet" / "Put .txt decks in Bench/cards" |
| Notes, no notes | "No notes yet" / "Put .md or .txt in Bench/notes on the card root" |
| Notes, folder in the wrong place | "Notes are in SAVE-F" / "Move SAVE-F/Bench/notes to Bench/notes" |
| Notes, empty file | "Empty note" / "Nothing to show in this file" |
| Study log, no file | "No sessions yet." then "F1 adds one, or write Bench/study/log.txt by hand." |
| Statistics, empty list | "Type a value, then EXE to add it." |
| Statistics, nothing to show yet | "Nothing to work out yet." |
| A category with no tools | "Nothing here yet" |

If a screen says something else, that is a bug: the message includes the folder
name and the expected file shape.

## Where to go next

- `docs/plan.md` - what each tool does and why
- `docs/STATUS.md` - what is finished, what is not, and what has been checked on
  hardware
- `demo/` - notes, decks and a study log you can copy on to try all of this
