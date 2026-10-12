# techdoc

A Claude Code skill that writes and cleans up technical documentation against
one rule set: concise prose (distilled from Strunk's *Elements of Style*),
tables and bullets over paragraphs, section depth capped at 3, short titles, no
audience/purpose boilerplate, and Markdown that reads cleanly without a renderer.

| Mode           | Input                         | Output                             |
|----------------|-------------------------------|------------------------------------|
| Review/rewrite | An existing doc               | The rewrite plus a terse changelog |
| Generate       | Outline, notes, spec, or code | A new doc; no facts invented       |

Format-agnostic (README, design doc, runbook, API reference) and standalone.
Needs `python3` (standard library only) for the bundled formatter.

## Install

```sh
./install.sh     # symlink skill/ into ~/.claude/skills/techdoc
rm ~/.claude/skills/techdoc   # uninstall; removes only the link
```

Edits in this repo are live at once. The installer refuses to overwrite an
existing regular directory at the target.

## Layout

```text
techdoc/
├── README.md
├── install.sh           # symlinks skill/ into ~/.claude/skills/techdoc
├── research/
│   └── RATIONALE.md     # why these rules, what was cut
└── skill/
    ├── SKILL.md         # entry point loaded by Claude Code
    ├── STYLE_RULES.md   # the rule set, read on every run
    ├── scripts/         # format_md.py and its tests
    └── evals/           # fixtures + evals.json
```

## Test

```sh
cd skill/scripts && python3 -m unittest
```
