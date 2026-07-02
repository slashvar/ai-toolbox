# ai-toolbox

A personal toolbox of Claude Code skills and AI tooling experiments.

Each subdirectory under `skills/` is self-contained: it ships an installable `skill/` payload, the `research/` that produced it, and an `install.sh` that wires it into `~/.claude/skills/`.

## Skills

| Skill | Summary |
|---|---|
| [`commit`](skills/commit/) | Safe git commits — never amend a commit that already exists on origin, stage specific files, follow the repo's commit style, sanity-check the affected package. |
| [`corner-cases`](skills/corner-cases/) | Identify and eliminate corner case programming — an anti-pattern where code is split into too many specific sub-cases that the general case already handles. |
| [`go-check`](skills/go-check/) | Go quality pipeline (lint → test) for any Go repo — scoped to changed packages, matches CI's pinned `golangci-lint`, and covers post-push Codacy/Semgrep suppression. |
| [`pr`](skills/pr/) | General-purpose PR creation — discovers the repo's PR template and conventions, runs quality gates, and writes a tight bullet-point PR body. |
| [`pr-restack`](skills/pr-restack/) | Re-align a chain of DRAFT PRs by chain-rebasing each branch onto its parent and force-push-with-lease. The rebase counterpart to `pr-sync`. |
| [`pr-self-review`](skills/pr-self-review/) | Cold-read self-review of an opened PR — a context-suppressed subagent reviews the diff, you triage findings in bulk, accepted fixes land as one commit. Auto-detects the stack (Go, Rust, JS/TS, Python, Java, Ruby) for idiom-aware review. |
| [`pr-sync`](skills/pr-sync/) | Keep a PR branch up to date with the default branch using merge (never rebase) to avoid force-pushes, with CI-status guards. The merge counterpart to `pr-restack`. |
| [`techdoc`](skills/techdoc/) | Write or clean up technical documentation against one rule set — concise prose (distilled Strunk), tables over paragraphs, depth-3 sections, short titles, no boilerplate, readable raw Markdown. |

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
