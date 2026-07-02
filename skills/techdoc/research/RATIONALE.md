# techdoc — rationale

Why these rules, and what was left out.

## Prose rules: which Strunk rules, and why

Strunk's *Elements of Style* has ~22 rules across usage, composition, and form.
Most are 1918 literary-prose concerns (colloquialisms, "shall vs will",
citation of quotations) that do not serve technical documentation. The skill
keeps the eight that map directly onto clear technical writing.

| Kept | Strunk origin | Why it survives for tech docs |
|------|---------------|-------------------------------|
| Omit needless words | "Omit needless words" | The core rule. Kills periphrase |
| Active voice | "Use the active voice" | Names the actor — matters in procedures |
| Positive form | "Put statements in positive form" | Positive assertions are easier to verify |
| Concrete/specific | "Use definite, specific, concrete language" | Numbers and names over vague hedges |
| Related words together | "Keep related words together" | Reduces ambiguity in dense sentences |
| One topic per paragraph | "Make the paragraph the unit of composition" | Scannability |
| Parallel form | "Express coordinate ideas in similar form" | Lists and tables read faster when parallel |
| Drop qualifiers | "Avoid the use of qualifiers" | "very/rather/quite" weaken precision |

Dropped: rules on quotation, punctuation minutiae, participial-phrase grammar,
and archaic usage. They add length without changing how a technical doc reads.

## Structural rules: not from Strunk

Strunk predates Markdown and structured technical docs. These are the author's
own conventions, and they are the skill's real differentiator:

- Tables/bullets over paragraphs.
- Section depth capped at 3.
- No lone subsection.
- Short titles; no repeated section purpose.
- No audience/tech-level/purpose boilerplate (opt-in override).

## Design choices

| Choice | Reason |
|--------|--------|
| Distilled rules, not verbatim Strunk | 1918 examples are literary; verbatim text bloats the skill |
| No per-doc-type templates | One rule set is format-agnostic; templates drift toward boilerplate |
| No external Markdown formatter | Zero-dependency keeps the skill portable when distributed |
| Standalone, no skill composition | Pedagogical tools (diagrams/analogies) conflict with "no periphrase" |
| Rules in a separate file, loaded every run | The rules *are* the job; a lean SKILL.md holds only the workflow |

## Source

Strunk, William Jr. *The Elements of Style* (1918). Public domain. Rules
adapted, not quoted.
