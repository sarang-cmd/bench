# Uncertainty, the short version

Every measurement has a last digit you had to guess. Say how much you guessed
and you are done with half the work.

## Adding and subtracting

Add the absolute uncertainties.

```
(3.2 +- 0.1) + (1.4 +- 0.2) = 4.6 +- 0.3
```

## Multiplying and dividing

Add the percentage uncertainties.

```
(3.2 +- 0.1) x (1.4 +- 0.2)
  3.2 is 3.1 percent, 1.4 is 14.3 percent
  the answer is 4.48, plus or minus 17.4 percent
  so 4.5 +- 0.8
```

## Powers

Multiply the percentage uncertainty by the power.

- squaring doubles the percentage uncertainty
- square rooting halves it

## Writing the answer down

1. Round the uncertainty to one significant figure. Two, if that figure is 1.
2. Round the value to the same decimal place as the uncertainty.
3. Never write more digits than you can defend.

> 4.48 +- 0.78 becomes 4.5 +- 0.8, not 4.48 +- 0.78.

## A warning about averages

Averaging repeated readings reduces the effect of random error, not of a
systematic one. If every reading is 0.2 too high, the average is 0.2 too high as
well, and no amount of repetition will show it.
