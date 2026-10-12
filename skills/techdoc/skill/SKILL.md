---
name: techdoc
description: Write or clean up technical documentation against a fixed style-and-structure rule set — concise prose (distilled Strunk), tables/bullets over paragraphs, depth-3 sections, short titles, no audience/purpose boilerplate, and readable raw Markdown. Use when the user wants to write technical documentation from an outline/notes/spec, or to review, rewrite, tighten, or make "doc-ready" an existing Markdown doc, README, design doc, runbook, or API reference — or says "/techdoc". Do NOT use for casual one-line edits ("add a note to the README"), for prose that is not technical documentation, or for generating a doc from a bare topic with no supplied substance.
---

# techdoc

Apply one style-and-structure rule set to technical documentation. Review/rewrite
is the primary path.

**First action, every run:** read `STYLE_RULES.md` (same directory). It is the
checklist. Do not work from memory of the rules.

## Modes

| Signal                                                     | Mode           |
|------------------------------------------------------------|----------------|
| User supplies an existing doc / points at a file           | Review/rewrite |
| User supplies an outline, notes, spec, or code to document | Generate       |

### Review/rewrite

1. Read the target doc.
2. Apply every rule in `STYLE_RULES.md`.
3. Write the rewritten doc.
4. Run the formatter (see Formatter).
5. Append a terse changelog: one line per class of fix, in the same style.

```text
Changes
- §2 intro: cut 38 words, active voice
- §3: 4-item list → table
- flattened #### under §4 into a table (depth-3 rule)
- dropped "Audience" preamble
```

If a rewrite would change technical meaning, flag it instead of guessing.

### Generate

1. Work only from the supplied outline, notes, spec, or code. Never invent
   technical facts from a bare topic; ask for the key points if input is thin.
2. Structure first: sections (max depth 3), tables/lists for sets of items.
3. Write prose against the rules.
4. Run the formatter. No changelog.

## Formatter

Alignment and trailing whitespace are mechanical. Never hand-align tables. After
writing a Markdown file, run from this skill's directory:

```bash
python3 scripts/format_md.py <file>          # align tables, strip trailing whitespace
python3 scripts/format_md.py --check <file>  # report remaining issues, exit 1 if any
```

- Fenced code is never rewritten, including `|` lines inside it.
- A "code fence has no language" report means: add a language (`text` for plain
  output, trees, changelogs). Re-run `--check` until clean.
- `--check` scans the whole file. It also flags bare fences you did not write
  when you only appended to an existing doc. Fix only the findings in your own
  changes; mention the rest.

### Non-Markdown targets

For HTML sent through an API, wikis, or any target the formatter cannot read,
apply the prose and structure rules by hand. Skip table alignment; the markup
sets the layout.

## Control hints

Invocation hints ("terse", "for senior engineers") calibrate the writing and stay
out of the doc unless the user asks to surface them. See `STYLE_RULES.md`.

## Scope

Style and structure only. No diagrams or code walkthroughs; for those, suggest a
dedicated code-explanation tool and style its output here.
