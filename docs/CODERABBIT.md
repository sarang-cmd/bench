# CodeRabbit

[CodeRabbit](https://coderabbit.ai) reviews pull requests: a summary, then
line comments with the reason for each one. Nothing is merged by it, nothing is
changed by it, and it runs outside this repository.

The configuration is [`.coderabbit.yaml`](../.coderabbit.yaml), checked in like
any other file. It is written for this project, not copied from a template: it
tells the reviewer what `src/core` is allowed to include, which nine colours a
screen may use, and that a regenerated table is not a code change.

## The one step that is not in a commit

**CodeRabbit only starts working once the GitHub App is installed on the
repository.** That needs a browser session with admin rights on
`sarang-cmd/bench`, so it cannot be done from a commit or from CI:

1. Open <https://github.com/apps/coderabbitai> and choose **Install**.
2. Pick the account that owns `sarang-cmd/bench`, then **Only select
   repositories** → `bench` → **Install**.
3. Open any pull request. Within a minute the `coderabbitai[bot]` account posts
   a walkthrough comment, and a `CodeRabbit` check appears next to the build.

If the app is not installed, nothing here fails: the build job stays green and
pull requests simply have no review. That is why this file exists separately
from the workflow - the workflow checks the configuration, it cannot check that
the app is installed.

## What it is told to look at

| Path | What the review is asked to catch |
|---|---|
| `src/core/**` | a gint include or call in core, libm, allocation, a new function with no test |
| `src/apps/**` | a gint call that does not exist in 2.10, a colour outside the nine, a wrong border or shadow width, an overlapping screen, a key with no help row, writing flash outside a session end |
| `tests/**` | an assertion without a concrete vector, a missing boundary case |
| `tools/**` | a dependency, a generator without a staleness check |
| `docs/**` | a hand-edited generated block, a number that disagrees with its source, a hardware claim with no record |
| `site/**` | palette drift, a layout that breaks under 320 px, a download that points at GitHub |

`AGENTS.md` is also read as a code guideline, so the rules that apply to
everyone apply to the bot too.

## What it is told to ignore

Generated files (`**/*.generated.json`, `docs/tools.json`, `spike/spike.diff`),
images, binaries in `dist/`, and the site lockfile. Reviewing a regenerated
table produces comments that are always dismissed, and a reviewer whose comments
are always dismissed gets skipped, including the one that mattered.

Poems and sequence diagrams are off. Automatic approvals are off: this project's
merge decision includes "has anyone run this on a calculator", which no bot can
answer, and the hardware claims are marked unverified on purpose.

## Checking the configuration

```sh
make coderabbit-check
```

`tools/check_coderabbit.py` reads the YAML with a small parser of its own -
this repository takes no new tooling dependencies - and rejects:

- a key CodeRabbit does not document, which would silently do nothing (a
  misspelt `pome`, or `reviewz` for `reviews`);
- a `path_instructions` glob or a non-negated `path_filter` that matches no file,
  which is how a rule for a renamed directory stops applying without anyone
  noticing;
- a tool switched on with no config file to read, or a tool for a language this
  repository does not contain (`phpstan`, `tflint` and friends);
- `auto_approvals: enabled: true`;
- tabs and CRLF endings, which YAML reads differently from what they look like.

`make py-test` runs the check and 28 tests over it. CI runs both, so a broken
review configuration fails a build.

## Changing a review rule

Edit `.coderabbit.yaml`, run `make coderabbit-check`, and say in the pull
request what the new rule catches. CodeRabbit re-reads the file on the next pull
request; there is nothing to redeploy.

Asking the bot to look again is a comment on the pull request: `@coderabbitai
review` for a full pass after a push, `@coderabbitai resolve` to close the
threads it opened, `@coderabbitai help` for the list. Its comments support
"apply suggestion" like any other review.
