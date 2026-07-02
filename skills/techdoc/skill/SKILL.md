---
name: techdoc
description: Write or clean up technical documentation against a fixed style-and-structure rule set — concise prose (distilled Strunk), tables/bullets over paragraphs, depth-3 sections, short titles, no audience/purpose boilerplate, and readable raw Markdown. Use when the user wants to write technical documentation from an outline/notes/spec, or to review, rewrite, tighten, or make "doc-ready" an existing Markdown doc, README, design doc, runbook, or API reference — or says "/techdoc". Do NOT use for casual one-line edits ("add a note to the README"), for prose that is not technical documentation, or for generating a doc from a bare topic with no supplied substance.
---

# techdoc

Apply a fixed style-and-structure rule set to technical documentation. Two
entry points share one rule set. Review/rewrite is the primary path.

**First action, every run:** read `STYLE_RULES.md` (same directory). It is the
checklist. Do not work from memory of the rules.

## Which mode

| Signal                                                     | Mode           |
|------------------------------------------------------------|----------------|
| User supplies an existing doc / points at a file           | Review/rewrite |
| User supplies an outline, notes, spec, or code to document | Generate       |

## Review/rewrite mode

1. Read the target doc.
2. Apply every rule in `STYLE_RULES.md`.
3. Write the rewritten doc.
4. **Run the hygiene formatter** (see below) — never hand-align tables.
5. Append a **terse changelog** — what changed and where, one line per class of
   fix. Follow the style rules in the changelog too (no periphrase).

Changelog example:

```text
Changes
- §2 intro: cut 38 words, active voice
- §3: 4-item list → table
- flattened #### under §4 into a table (depth-3 rule)
- dropped "Audience" preamble
```

Do not silently rewrite content you cannot verify. If a rewrite would change
technical meaning, flag it instead of guessing.

## Generate mode

1. **Require substance.** Work from the supplied outline, notes, spec, or code.
   Never fabricate technical facts from a bare topic — if the input is too thin,
   ask for the key points.
2. Structure first: sections (max depth 3), tables/lists for sets of items.
3. Write prose against the rules.
4. **Run the hygiene formatter** (see below).
5. No changelog needed; the doc is new.

## Markdown hygiene (mechanical)

The alignment and whitespace rules are deterministic — do not hand-apply them,
you will get column widths wrong. After writing any doc to a file, run the
bundled formatter:

```bash
python3 scripts/format_md.py <file>        # aligns tables, strips trailing whitespace
```

Then **verify** — this catches what the formatter only reports, never rewrites:

```bash
python3 scripts/format_md.py --check <file>   # exits non-zero on remaining issues
```

Resolve every reported issue before returning. A "code fence has no language"
report means: add a language, using `text` for plain output, trees, or
changelogs. Re-run `--check` until it is clean.

## Control hints

The user may steer style at invocation ("terse", "for senior engineers"). Use
hints to calibrate; do not write them into the doc. Exception: include a hint
only when the user explicitly asks to surface it (e.g. "state the audience").
See the control-hints section of `STYLE_RULES.md`.

## Scope boundary

This skill owns style and structure. It does not generate diagrams or code
walkthroughs. If the user wants a pedagogical walkthrough with diagrams and
analogies, that is a separate concern — suggest they run a dedicated
code-explanation tool and feed its output back here for styling.
