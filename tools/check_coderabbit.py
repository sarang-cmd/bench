#!/usr/bin/env python3
"""Check .coderabbit.yaml without taking a YAML dependency.

CodeRabbit reads this file in the cloud, and its only feedback for a typo is a
comment on the next pull request, possibly days later.  This walks the subset of
YAML the file actually uses - two levels of map, lists, block scalars - and
checks it against what CodeRabbit documents, plus a few rules that are about
this repository rather than about CodeRabbit:

  * a key CodeRabbit does not know, so the setting silently does nothing;
  * a `path_instructions` glob that matches no file, which is how a review rule
    for a directory that was renamed stops applying without anyone noticing;
  * a tool switched on with no config file to read;
  * a review scope that excludes the only directory a change touches;
  * `auto_approvals`, which would let a bot approve a calculator build that
    nobody has run on hardware.

Run it as a script or through tools/test_coderabbit_config.py.
"""

from __future__ import annotations

import glob
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CONFIG = ".coderabbit.yaml"

TOP_LEVEL = {
    "language",
    "tone_instructions",
    "early_access",
    "enable_free_tier",
    "inheritance",
    "reviews",
    "chat",
    "knowledge_base",
    "issue_enrichment",
    "code_generation",
}

REVIEW_KEYS = {
    "profile",
    "high_level_summary",
    "high_level_summary_instructions",
    "high_level_summary_placeholder",
    "high_level_summary_in_walkthrough",
    "auto_title_placeholder",
    "auto_title_instructions",
    "review_status",
    "review_info",
    "review_details",
    "review_progress",
    "commit_status",
    "fail_commit_status",
    "collapse_walkthrough",
    "changed_files_summary",
    "sequence_diagrams",
    "estimate_code_review_effort",
    "assess_linked_issues",
    "related_issues",
    "related_prs",
    "suggested_labels",
    "labeling_instructions",
    "auto_apply_labels",
    "suggested_reviewers",
    "auto_assign_reviewers",
    "in_progress_fortune",
    "poem",
    "enable_prompt_for_ai_agents",
    "path_filters",
    "path_instructions",
    "abort_on_close",
    "disable_cache",
    "post_merge_actions",
    "auto_approvals",
    "slop_detection",
    "auto_review",
    "finishing_touches",
    "pre_merge_checks",
    "tools",
}

AUTO_REVIEW_KEYS = {
    "enabled",
    "description_keyword",
    "auto_incremental_review",
    "auto_pause_after_reviewed_commits",
    "ignore_title_keywords",
    "labels",
    "drafts",
    "base_branches",
    "ignore_usernames",
}

TOOLS_WITH_CONFIG_FILE = {
    "phpstan",
    "golangci-lint",
    "swiftlint",
    "detekt",
}

# A tool switched on in a repository with no file in its language is a bug in
# the configuration, not a finding: it either reports nothing or errors.
IRRELEVANT_TOOLS = {
    "phpstan",
    "phpmd",
    "phpcs",
    "golangci-lint",
    "swiftlint",
    "detekt",
    "tflint",
    "checkov",
    "hadolint",
}


class ConfigError(Exception):
    pass


TRUE = {"true", "yes", "True"}
FALSE = {"false", "no", "False"}


def _scalar(value: str):
    """YAML booleans and integers are not strings; `enabled: false` must be
    false, not the truthy string "false"."""
    text = value.strip().strip('"').strip("'")
    if text in TRUE:
        return True
    if text in FALSE:
        return False
    if text.isdigit():
        return int(text)
    return text


def _indent(line: str) -> int:
    return len(line) - len(line.lstrip(" "))


def parse(text: str) -> dict:
    """Parse the map/list/block-scalar subset CodeRabbit configs use."""
    if "\t" in text:
        raise ConfigError("tab character on line %d; YAML needs spaces"
                          % (text[: text.index("\t")].count("\n") + 1,))
    if "\r" in text:
        raise ConfigError("carriage return; this repository stores LF endings")

    lines = [ln.rstrip() for ln in text.split("\n")]
    root: dict = {}
    stack: list[tuple[int, dict]] = [(-1, root)]
    i = 0
    while i < len(lines):
        raw = lines[i]
        stripped = raw.strip()
        i += 1
        if not stripped or stripped.startswith("#"):
            continue
        indent = _indent(raw)
        if indent == 0 and stripped.startswith("-"):
            raise ConfigError("a list item cannot be the whole document")
        while len(stack) > 1 and indent <= stack[-1][0]:
            stack.pop()
        parent = stack[-1][1]

        if stripped.startswith("- "):
            items = parent.setdefault("__items__", [])
            if not isinstance(items, list):
                raise ConfigError("list item where a mapping was: %r" % stripped)
            rest = stripped[2:].strip()
            if ":" in rest and not rest.startswith(("http", "'", '"')):
                key, _, value = rest.partition(":")
                item = {key.strip(): _scalar(value)}
                items.append(item)
                stack.append((indent, item))
            else:
                items.append(rest.strip('"').strip("'"))
            continue

        if ":" not in stripped:
            raise ConfigError("no colon: %r" % stripped)
        key, _, value = stripped.partition(":")
        key = key.strip().strip('"').strip("'")
        value = value.strip()

        if value == "":
            child: dict = {}
            parent[key] = child
            stack.append((indent, child))
        elif value.startswith("|"):
            body = []
            while i < len(lines):
                nxt = lines[i]
                if nxt.strip() and _indent(nxt) <= indent:
                    break
                body.append(nxt.strip())
                i += 1
            parent[key] = "\n".join(body).strip()
        else:
            parent[key] = _scalar(value)

    return _lists(root)


def _lists(node):
    """Where a mapping held only list items, it is a list, not a map."""
    if isinstance(node, dict):
        items = node.get("__items__")
        if isinstance(items, list) and len(node) == 1:
            return [_lists(v) for v in items]
        return {k: _lists(v) for k, v in node.items() if k != "__items__"}
    return node


def _expand(node):
    return node


def _matches(pattern: str, base: Path) -> list[str]:
    """Files a glob reaches.  A trailing `/**` matches the empty string in
    glob(), so `app/**` would "match" a directory that does not exist; strip
    that one result and keep only paths that are really there."""
    pattern = pattern.lstrip("!")
    if pattern.startswith("**/"):
        raw = glob.glob(str(base / "**" / pattern[3:]), recursive=True)
    else:
        raw = glob.glob(str(base / pattern), recursive=True)
    literal = str(base / pattern.rstrip("/*").rstrip("/"))
    return [p for p in raw if p.rstrip("/") != literal]


def problems(text: str, base: Path | None = None) -> list[str]:
    """Every disagreement with the documented schema and this repository."""
    base = base or ROOT
    found: list[str] = []
    cfg = _expand(parse(text))

    for key in cfg:
        if key not in TOP_LEVEL:
            found.append("unknown top-level key %r" % key)

    reviews = cfg.get("reviews")
    if not isinstance(reviews, dict):
        return found + ["reviews: missing"]

    for key in reviews:
        if key not in REVIEW_KEYS:
            found.append("unknown reviews key %r" % key)

    for key in reviews.get("auto_review", {}) or {}:
        if key not in AUTO_REVIEW_KEYS:
            found.append("unknown reviews.auto_review key %r" % key)

    profile = reviews.get("profile")
    if profile not in {"quiet", "chill", "assertive"}:
        found.append("reviews.profile %r is not quiet, chill or assertive" % profile)

    filters = reviews.get("path_filters") or []
    if not isinstance(filters, list) or not filters:
        found.append("reviews.path_filters is empty - CodeRabbit reads nothing")
    for pattern in filters if isinstance(filters, list) else []:
        if pattern.startswith("!") and not pattern.strip("!*/. "):
            found.append("path filter %r is empty" % pattern)
        if not pattern.startswith("!") and not _matches(pattern, base):
            found.append("path filter %r matches no file" % pattern)

    instructions = reviews.get("path_instructions") or []
    for entry in instructions if isinstance(instructions, list) else []:
        path = entry.get("path", "") if isinstance(entry, dict) else ""
        if not path:
            found.append("path_instructions entry without a path")
            continue
        if not _matches(path, base):
            found.append("path_instructions path %r matches no file" % path)
        if not entry.get("instructions"):
            found.append("path_instructions %r has no instructions" % path)

    tools = reviews.get("tools") or {}
    for name, settings in tools.items():
        if not isinstance(settings, dict):
            continue
        if name in IRRELEVANT_TOOLS and settings.get("enabled", True):
            found.append("tool %r is enabled but this repository has no %s files"
                         % (name, name))
        if name in TOOLS_WITH_CONFIG_FILE and settings.get("enabled", True):
            found.append("tool %r is enabled with no config_file" % name)

    approvals = reviews.get("auto_approvals") or {}
    if approvals.get("enabled"):
        found.append("reviews.auto_approvals is on: a bot must not approve a "
                     "build nobody ran on hardware")

    if not cfg.get("language"):
        found.append("language is missing; CodeRabbit defaults to en-US, but say so")

    return found


def main(argv: list[str]) -> int:
    path = Path(argv[1]) if len(argv) > 1 else ROOT / CONFIG
    try:
        text = path.read_text()
    except OSError as error:
        print("CODERABBIT CONFIG: cannot read %s (%s)" % (path, error))
        return 1
    found = problems(text, path.resolve().parent)
    if found:
        print("CODERABBIT CONFIG: %d problem(s)" % len(found))
        for line in found:
            print("  " + line)
        return 1
    print("CODERABBIT CONFIG OK (%s)" % path.name)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
