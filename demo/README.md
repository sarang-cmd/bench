# Demo content

Copy these files onto the calculator to have something to look at before you
write your own notes:

```
demo/notes/  ->  fls0/Bench/notes/
demo/cards/  ->  fls0/Bench/cards/
demo/study/  ->  fls0/Bench/study/
```

The add-in creates those three folders on its first run, so run it once, connect
in USB Flash mode, and drop the files in.

## Needs human proofreading

**Everything in this folder was written for the demo and has not been checked by
a teacher.** The physics, chemistry and biology are meant to be right, and they
are written in my own words rather than copied from a booklet or a past paper,
but they are revision notes, not a specification. Read them before you trust a
number in an exam.

## What is here

| File | What it is |
|---|---|
| `notes/photosynthesis.md` | Headings, bold, a list, a quote, a fenced block |
| `notes/constant-acceleration.md` | Lists, code, a numbered list, a quote |
| `notes/uncertainty-rules.md` | Worked arithmetic for absolute and percentage uncertainty |
| `cards/mechanics.txt` | Ten cards, `question \| answer` per line |
| `cards/moles-and-concentration.txt` | Eight cards on amount of substance |
| `cards/quadratics.txt` | Eight cards on quadratic graphs |
| `study/log.txt` | Sixteen sessions across a fortnight, `YYYY-MM-DD minutes tool` |

`make demo-check` parses every file with the same code the add-in uses and fails
if a line would be skipped, so the demo cannot rot into something the calculator
refuses to read.
