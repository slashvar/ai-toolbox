# techdoc

A Claude Code skill for **writing and cleaning up technical documentation**. It
applies one fixed rule set: concise prose (distilled from Strunk's *Elements of
Style*), tables and bullets over paragraphs, section depth capped at 3, short
titles, no audience/purpose boilerplate, and Markdown source that reads cleanly
without a renderer.

Two entry points, one rule set:

- **Review/rewrite** an existing doc — returns the rewrite plus a terse
  changelog of what was fixed and where.
- **Generate** a new doc from an outline, notes, spec, or code — never
  fabricates facts from a bare topic.

It is format-agnostic (README, design doc, runbook, API reference) and
standalone — no dependencies on other skills or external formatters.

## Install

```sh
./install.sh
```

This symlinks `skill/` into `~/.claude/skills/techdoc`. Because it's a symlink,
any edit in this repo is immediately live in Claude Code — no re-install needed.

If `~/.claude/skills/techdoc` already exists as a regular directory, the
installer refuses to overwrite it. Remove or move it first.

## Uninstall

```sh
rm ~/.claude/skills/techdoc
```

Safe because the install is a symlink — only the link is removed, the repo is
untouched.

## Layout

```
techdoc/
├── README.md              # this file
├── install.sh             # symlinks skill/ into ~/.claude/skills/techdoc
├── research/
│   └── RATIONALE.md        # why these rules, what was cut
└── skill/
    ├── SKILL.md            # entry point loaded by Claude Code
    ├── STYLE_RULES.md      # the rule set, read on every run
    └── evals/              # before/after fixtures + evals.json
```

## Requirements

None. Zero external dependencies by design, so the skill stays portable.
