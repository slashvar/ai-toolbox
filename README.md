# ai-toolbox

A personal toolbox of Claude Code skills and AI tooling experiments.

Each subdirectory under `skills/` is self-contained: it ships an installable `skill/` payload, the `research/` that produced it, and an `install.sh` that wires it into `~/.claude/skills/`.

## Skills

| Skill | Summary |
|---|---|
| [`corner-cases`](skills/corner-cases/) | Identify and eliminate corner case programming — an anti-pattern where code is split into too many specific sub-cases that the general case already handles. |
| [`pr-self-review`](skills/pr-self-review/) | Cold-read self-review of an opened PR — a context-suppressed subagent reviews the diff, you triage findings in bulk, accepted fixes land as one commit. Auto-detects the stack (Go, Rust, JS/TS, Python, Java, Ruby) for idiom-aware review. |

## Layout convention

```
skills/<name>/
├── README.md        # overview + install/uninstall
├── install.sh       # symlinks skill/ into ~/.claude/skills/<name>
├── skill/           # the installable payload (SKILL.md, references/, evals/)
└── research/        # exercises, learnings, and patterns that produced the skill
```

Future skills follow the same structure.

## License

BSD 2-Clause — see [`LICENSE`](LICENSE).
