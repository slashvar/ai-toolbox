# techdoc style rules

The checklist. Apply every rule on every run.

## Prose

Distilled from Strunk's *Elements of Style*, adapted for technical writing.

| Rule                        | Do                                         | Instead of                                         |
|-----------------------------|--------------------------------------------|----------------------------------------------------|
| Omit needless words         | "to configure X"                           | "in order to configure X"                          |
| Active voice                | "the server rejects the request"           | "the request is rejected by the server"            |
| Positive form               | "the list stays empty"                     | "the list does not get any items"                  |
| Concrete and specific       | "retries 3 times, 2s apart"                | "retries a few times"                              |
| Keep related words together | "pass `--force` to skip the check"         | "pass, to skip the check, `--force`"               |
| One topic per paragraph     | split when the subject changes             | one paragraph covering setup + errors + tuning     |
| Parallel form in lists      | all items start with a verb (or all nouns) | mixed "Installing…", "you configure…", "the cache" |
| Drop weak qualifiers        | "the call blocks"                          | "the call is rather likely to block"               |

Extra tech-doc prose rules:

- **No periphrase.** State the fact, not the approach to the fact. Cut
  "it should be noted that", "as mentioned above", "in this section we will".
- **Simple sentence shape.** Prefer subject–verb–complement. Avoid nested
  clauses and multi-layer sentences; split them.
- **Define before use.** Introduce an acronym or term once, then reuse it.

## Structure

| Rule                           | Detail                                                                        |
|--------------------------------|-------------------------------------------------------------------------------|
| Tables/bullets over paragraphs | If content is a set of items, parameters, or steps, make it a table or list   |
| Max depth 3                    | Headings go no deeper than `###`. Restructure if you need `####`              |
| No lone subsection             | Never a single `##`/`###` under a parent. Zero or two-plus, never one         |
| Short titles                   | Name the thing. "Retries", not "How retries are handled in this system"       |
| No repeated purpose            | Don't restate a section's job inside it ("This section explains…")            |
| No meta-boilerplate            | No "Audience:", "Tech level:", "Purpose:" lines — unless explicitly requested |

## Markdown hygiene

The raw source must read cleanly without a renderer.

- **Align table separators.** Pad cells so `|` columns line up in source.
- **One blank line** between blocks (heading, paragraph, list, table, fence).
- **Language on fences.** Tag every fence — `bash`, `go`, or `text` for plain
  output, trees, and changelogs. Never a bare ` ``` `.
- **Inline links.** Use link references only when a URL repeats.
- **No trailing whitespace**, no hard-wrapped tables.

`scripts/format_md.py` enforces alignment and whitespace and flags bare fences
(see Formatter in `SKILL.md`).

## Control hints (invocation-time)

The user may steer style at invocation ("terse", "for experts", "assume no
prior context"). Use hints to calibrate tone and depth. Do not write them into
the document, unless the user explicitly asks to surface one ("state the
audience"). That request overrides the no-meta-boilerplate rule.
