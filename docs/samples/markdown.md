# Bench Markdown sample

## Headings

### Third level heading

A paragraph with **bold**, *italic*, _also italic_ and `code` spans.
It also has a [link](https://example.com) that shows only its text.

- first bullet
- second bullet
  - nested bullet
    - third level

1. first number
2. second number
10. tenth number

> A quote line
> a second quote line

---

- [ ] a task that is still open
- [x] a task that is done

```c
int main(void) { return 0; } /* fenced code, never wrapped */
```

### Maths (1.3.0)

Inline maths is converted to characters the bundled font can draw:
$\pi r^2$ for a circle, $\frac{1}{2} m v^2$ for kinetic energy, and
$\sqrt{2}$ for a diagonal.  Exponents keep their caret when the raised form
does not exist: $x^{n+1}$.

$$
\Delta v = \frac{\Delta s}{\Delta t}
$$

Display maths is a `$$` line, the formula, and another `$$` line.

### Tables (1.3.0)

| Quantity | Symbol | Unit |
|----------|:------:|-----:|
| velocity | v | m/s |
| acceleration | a | m/s2 |

The alignment row decides the padding, so a numbers column lines up right.

![a picture](picture.png)

<div>html stays text</div>

A footnote reference [^1] stays text too.

[^1]: and so does the footnote body.
