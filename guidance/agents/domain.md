# Domain Docs

How the engineering skills should consume this repo's domain documentation when exploring the codebase.

## Before exploring, read these

- **`GLOSSARY-MAP.md`** at the repo root: it points at one `guidance/<topic>/GLOSSARY.md` per topic. Read each one relevant to the work.
- **`guidance/<topic>/adr/`**: read the ADRs of every topic the work touches. There is no repo-wide `docs/adr/`; `docs/` is off-limits to agents (see `AGENTS.md`).

If a topic has no glossary or ADRs yet, **proceed silently**. Don't flag their absence; don't suggest creating them upfront. The `/domain-modeling` skill (reached via `/grill-with-docs` and `/improve-codebase-architecture`) creates them lazily when terms or decisions actually get resolved.

## File structure

```
/
├── GLOSSARY-MAP.md                    ← one row per topic
└── guidance/
    ├── agents/                        ← skills config (this file), not a topic
    └── parser-structure/
        ├── GLOSSARY.md
        └── adr/
            ├── 0001-syntax-then-separate-semantic-pass.md
            └── 0002-parser-owns-tokens.md
```

A new topic gets a new folder `guidance/<topic>/` (its `GLOSSARY.md` and ADRs in `adr/`, numbered `0001-slug.md` per topic) and a new row in `GLOSSARY-MAP.md`.

## Use the glossary's vocabulary

When your output names a domain concept (in an issue title, a refactor proposal, a hypothesis, a test name), use the term as defined in the topic's `GLOSSARY.md`. Don't drift to synonyms the glossary explicitly avoids.

If the concept you need isn't in the glossary yet, that's a signal: either you're inventing language the project doesn't use (reconsider) or there's a real gap (note it for `/domain-modeling`).

## Flag ADR conflicts

If your output contradicts an existing ADR, surface it explicitly rather than silently overriding:

> _Contradicts parser-structure ADR-0002 (parser owns tokens), but worth reopening because…_
