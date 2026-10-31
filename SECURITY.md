# Security

## What Bench does with your data

Bench reads and writes files under `fls0/Bench/` on the calculator and does
nothing else. It has no network code, no background process, no telemetry and no
configuration that could exfiltrate anything. The add-in runs entirely in RAM on
the calculator; the only persisted state is the notes, decks, settings and study
log you can see in `README.md`.

## Reporting

If you find a problem - a file that makes the add-in crash, a path that escapes
`Bench/`, anything that could corrupt other calculator memory - open an issue
with the file that triggers it and the build you were running. Please do not
attach a personal notes file; a minimal reproduction is better.

There is no bounty and no promise of a fix window. This is a personal project.

## Scope

Out of scope: anything that requires flashing the OS, the bootloader or
Examination Mode. Bench does not touch those and pull requests that do will be
closed.
