/* numedit.h - editing a number in a text field, shared by every tool.
 *
 * The tools used to carry their own copy of this: append a digit, refuse a
 * second dot, drop the last character on DEL.  Four copies of that rule is four
 * chances to disagree, and the moles screen accepted "1.2.3" for a while.
 *
 * A field is a NUL-terminated char buffer.  Nothing here parses; numparse.c
 * does that, and a field this module accepted is one numparse can read.
 *
 * Plain C11, no gint. */

#ifndef BENCH_NUMEDIT_H
#define BENCH_NUMEDIT_H

#include <stdbool.h>

/* Longest field any tool uses, plus the NUL. */
#define NUMEDIT_MAX 24

/* Append `ch` to `buf`, which holds at most `size` bytes including the NUL.
 * Accepts digits 0-9, '.', and 'e' follows nothing (no exponent entry yet).
 * Returns false, leaving the buffer alone, when the buffer is full, when `ch`
 * is not a digit or '.', or when a second '.' would appear.
 *
 * A single leading "0" is replaced by the digit that follows it, so typing 0
 * then 5 gives "5" and not "05". */
bool numedit_put(char *buf, int size, int ch);

/* Remove the last character.  False when the field was already empty. */
bool numedit_back(char *buf);

/* Replace the whole field.  Silently truncates to `size` - 1 characters; use it
 * for defaults and for recalling a value, never for typing. */
void numedit_set(char *buf, int size, char const *text);

/* True when the field is a number numparse can read: optional leading '-',
 * digits, at most one '.', and at least one digit. */
bool numedit_valid(char const *buf);

#endif /* BENCH_NUMEDIT_H */
