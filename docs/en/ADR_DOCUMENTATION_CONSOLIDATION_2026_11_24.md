# ADR Documentation Consolidation — 24 November 2026

Status: completed and merged

This is the canonical report. The [German report](../de/ADR_DOCUMENTATION_CONSOLIDATION_2026_11_24.md)
has the same technical content.

## Executive summary

On 24 November 2026, the project completed a focused consolidation of locally prepared additions to
16 Architecture Decision Records (ADRs). The additions contained useful implementation provenance,
but they also included extensive Codex session material: local build paths, temporary branches,
authentication workarounds, shell diagnostics, runner waiting times, repeated cleanup narratives and
self-references to the then-uncommitted working state. Keeping that operational transcript inside
accepted ADRs would have obscured the decisions and made long-term maintenance harder.

The work therefore separated three kinds of information:

1. **durable architectural decisions**, which remained unchanged;
2. **implementation evidence**, retained in concise and verifiable form where it helps explain how a
   decision was realized; and
3. **historical session transcripts**, removed when they did not contribute durable technical
   evidence.

The consolidation was preceded by a complete read-only review. It preserved the established meaning
and historical validity of every ADR, corrected three documentation defects, reduced the proposed
material to 364 targeted additions in 15 ADRs and left ADR 0178 identical to the versioned baseline.
The result was merged through [pull request #113](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/113)
without changing product code, tests, build configuration, public API or release state.

## Verified initial state

The reviewed working state was based on commit
[`8aabcfc9b97a535fc346fb794d0eb4fa8a14a2c1`](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/commit/8aabcfc9b97a535fc346fb794d0eb4fa8a14a2c1).
It contained:

- 16 locally modified ADR files;
- 2,647 added documentation lines and no original deletions;
- a separate untracked user file named `..Codex.txt`; and
- an external backup of the original documentation work.

No existing addition was replaced before the backup and read-only review had been verified. The
review compared the complete added text with the current C++20 implementation, tests, accepted ADRs,
Git history and bilingual project documentation. This established which material was technically
correct, which was valuable but too operational or repetitive, and which should not become part of
the permanent ADR record.

## Read-only review result

| Classification | Count | Meaning |
|---|---:|---|
| A — Retain unchanged | 1 | Correct, useful and suitable for permanent project documentation. |
| B — Revise technically | 14 | Valuable evidence, but requiring specific corrections, historical framing or removal of repetition and session detail. |
| C — Do not retain additions | 1 | The local additions were not durable ADR material; the versioned ADR remained authoritative. |
| D — Unresolved | 0 | No item lacked sufficient evidence after review. |

The assessment considered technical agreement with implementation and tests, consistency with
accepted decisions, historical accuracy, evidence for CI/commit/release claims, English/German
equivalence, duplication and long-term documentation value. Newer implementation and tests were used
as evidence without retroactively changing the meaning of decisions that were valid when originally
accepted.

## Consolidation performed

Pull request #113 changed 15 ADR files with 364 additions. ADR 0178 has no difference from the
versioned starting commit. ADR 0188 retained its complete, technically correct addition. The other
14 category-B additions were rewritten as concise bilingual provenance sections.

| ADR | Review | Treatment | Result in PR #113 |
|---|---|---|---:|
| [0178](../adr/0178-m8-numeric-property-value-kind-foundation.md) | C | Removed only the uncommitted additions; retained the previously versioned ADR unchanged. | 0 lines |
| [0179](../adr/0179-m8-radio-button-scalar-property-coverage.md) | B | Condensed RadioButton scalar-property implementation and test provenance. | 24 additions |
| [0180](../adr/0180-m8-description-node-identity-ordered-ownership-snapshot.md) | B | Condensed node identity, ordered ownership and snapshot provenance. | 20 additions |
| [0181](../adr/0181-m8-built-in-component-cross-reference-edge-snapshot-foundation.md) | B | Retained the cross-reference edge and snapshot evidence. | 21 additions |
| [0182](../adr/0182-m8-selection-model-component-lifecycle-and-reference-coverage.md) | B | Retained SelectionModel lifecycle and reference-coverage evidence. | 20 additions |
| [0183](../adr/0183-m8-combo-box-collection-and-optional-property-coverage.md) | B | Retained ComboBox collection and optional-property evidence. | 21 additions |
| [0185](../adr/0185-m8-inline-text-style-authoring-persistence-coverage.md) | B | Retained inline TextStyle authoring and persistence evidence. | 21 additions |
| [0186](../adr/0186-m8-canonical-json-ui-description-encoding-v1.md) | B | Retained canonical JSON v1 implementation and validation provenance. | 20 additions |
| [0187](../adr/0187-m8-json-v1-parser-validated-document-model.md) | B | Condensed parser/document-model evidence and corrected the root identifier wording. | 22 additions |
| [0188](../adr/0188-m8-built-in-ui-description-reconstruction.md) | A | Preserved the complete technically correct reconstruction evidence. | 42 additions |
| [0190](../adr/0190-m8-single-property-validation-normalization-metadata-foundation.md) | B | Retained validation and normalization metadata provenance. | 22 additions |
| [0192](../adr/0192-m8-built-in-reference-and-graph-validation-metadata-foundation.md) | B | Retained reference and graph-validation metadata provenance. | 21 additions |
| [0193](../adr/0193-m8-property-authoring-category-metadata-foundation.md) | B | Retained property-authoring category evidence. | 20 additions |
| [0194](../adr/0194-m8-resource-key-and-typed-text-style-resource-dictionary-foundation.md) | B | Retained resource-key and typed TextStyle dictionary evidence. | 20 additions |
| [0199](../adr/0199-m8-headless-tooling-integration-contract.md) | B | Condensed tooling-integration closure and marked the session boundary as historical. | 28 additions |
| [0202](../adr/0202-beta-and-canonical-product-repository-roles.md) | B | Separated the guarded PR #112 state from its later verified merge closure. | 42 additions |

The retained material covers durable architecture contracts, implementation provenance, important
rationale and exclusions, relevant tests, verified commit/PR references and confirmed CI results.
The removed material covered transient local directories, temporary branches, credential and shell
workarounds, intermediate command failures, hashes of protected unrelated files, repeated runner
delays, duplicate cleanup reports, repetition of existing normative ADR text and statements about the
then-current uncommitted session. Those facts can be useful during execution, but they do not explain
or constrain the architecture and would age poorly inside accepted ADRs.

## Corrected documentation problems

### ADR 0187: numeric root identifier

The German provenance now identifies the document root as numeric root ID `0`. This avoids the
ambiguous expression “Root null”, which could be read as a null value rather than an integer node
identifier. The underlying parser and document decision did not change.

### ADR 0199: historical M8.29/M8.30 boundary

The statement that M8.30 had not begun is explicitly identified as the state of that historical work
session. It is not presented as the current project state. The later completion of M8 is recorded in
[ADR 0200](../adr/0200-m8-developer-productivity-foundations-exit-decision.md) and the
[M8 exit review](M8_EXIT_REVIEW.md).

### ADR 0202: guarded state and final closure

The record now distinguishes chronologically between:

1. the guarded intermediate state, when pull request #112 was deliberately not merged because the
   available authorization named an obsolete head SHA; and
2. the later final closure, when updated owner authorization was received and pull request #112 was
   successfully merged.

This preserves the correctness of both historical observations instead of rewriting the earlier
state as though the merge had already occurred.

## Git history and provenance

The consolidation was divided into three focused commits:

| Purpose | Commit |
|---|---|
| M8 implementation provenance | [`c7dcff4abe44824d37ffb9729f0e2a9dcc9b1f18`](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/commit/c7dcff4abe44824d37ffb9729f0e2a9dcc9b1f18) |
| M8 tooling integration closure | [`7751eef77b2104606708dfe411730944ada21ae8`](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/commit/7751eef77b2104606708dfe411730944ada21ae8) |
| Final pull request #112 merge closure | [`f3397f2e872f44c55482c50653d6eb190a168029`](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/commit/f3397f2e872f44c55482c50653d6eb190a168029) |

All three commits are ancestors of the verified merge commit. Pull request
[#113](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/113) was merged into `main` on
24 November 2026 at 08:49:28 UTC by merge commit
[`8e6493ed531d2c84b04059fa79def6520a16fb46`](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/commit/8e6493ed531d2c84b04059fa79def6520a16fb46).

## Tests and quality assurance

### Local static checks before the consolidation commit

The documentation review verified:

- `git diff --check` with no whitespace errors;
- 202 unique and indexed ADRs;
- 528 relative Markdown links across 248 tracked Markdown files, with no missing targets;
- exactly the intended 15 ADR files in the consolidation pull request; and
- an unchanged public C++ API and product implementation.

No additional local C++ build or CTest run was performed for this Markdown-only consolidation. That
choice does not imply that product tests were skipped in CI; it distinguishes the local documentation
checks from the repository's automated build and test matrix.

### Pull-request CI

The exact pull-request head `f3397f2e872f44c55482c50653d6eb190a168029` passed all 11 required
checks: Linux GCC, Linux Clang, Linux Clang ASan + UBSan, Linux SDL3 adapter, Windows MSVC,
macOS AppleClang, GTK4 Native Host, macOS AppKit Native, Linux Release, Windows Release and macOS
Release. These results are attached to [pull request #113](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/113).

### Post-merge CI

The actual merge SHA `8e6493ed531d2c84b04059fa79def6520a16fb46` independently passed the same
11-job coverage:

| Workflow | Run | Successful jobs |
|---|---|---|
| Main CI | [45359491339](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/45359491339) | Linux GCC, Linux Clang, Linux Clang ASan + UBSan, Linux SDL3 adapter, Windows MSVC, macOS AppleClang, Linux Release, Windows Release, macOS Release |
| GTK Native CI | [45359491438](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/45359491438) | GTK4 Native Host |
| AppKit Native CI | [45359491500](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/45359491500) | macOS AppKit Native |

All three workflows completed with `success` and identify the verified merge SHA as their head.

## Backup and recoverability

Before editing, the original 2,647 additions were preserved outside the Git repository. The recorded
backup set contains:

- a patch of the original changes;
- complete copies of all 16 ADR files;
- a copy of the separate `..Codex.txt` user file;
- the diff statistics; and
- a SHA-256 checksum manifest.

No private absolute Windows paths, credentials or backup contents are part of the repository. The
presence, file count, diff statistics and checksum agreement for the separately copied user file were
verified. A successful replay of the serialized patch was **not** established, so this report does not
claim that the patch was tested as a restoration mechanism. The complete file copies and checksum
manifest are independent recovery evidence.

## Practical lessons for future documentation

The review supports the following working recommendations. They are lessons from this consolidation,
not a new architecture decision or a replacement for the existing ADR policy:

- ADRs should preserve durable decisions, rationale, alternatives and consequences.
- Implementation evidence should be short, precise and independently verifiable.
- Long Codex or shell execution transcripts should remain outside accepted ADRs.
- Historical intermediate states should be dated or explicitly marked as historical.
- English and German text should communicate the same technical facts and boundaries.
- Commit, pull-request and CI references should be checked before they are recorded.
- Git history remains the authoritative source for what actually changed and when.

## Final assessment and open boundaries

The ADR consolidation is complete as a documentation and quality-assurance measure. It did not add a
new architecture decision, alter the product, change a public API or certify a release.

The project status remains:

- M8 is complete; see the [M8 exit review](M8_EXIT_REVIEW.md) and
  [ADR 0200](../adr/0200-m8-developer-productivity-foundations-exit-decision.md).
- Version 0.2.0 remains `Unreleased`.
- The visible manual macOS/Cocoa M3 validation remains open; see the
  [M3 rendered desktop smoke test](M3_RENDERED_DESKTOP_SMOKE_TEST.md).
- A separate v0.2.0 release-readiness decision remains open.
- The decision to replace the current SDL3/SDL_ttf release-tag references with immutable source
  commit pins remains open; see the
  [migration-readiness review](SASDGMBH_MIGRATION_READINESS.md).
- The later move to a `SASDGMBH` repository was not performed by this work; see
  [ADR 0202](../adr/0202-beta-and-canonical-product-repository-roles.md).

No release candidate, tag, GitHub Release, repository transfer or product certification is implied.
