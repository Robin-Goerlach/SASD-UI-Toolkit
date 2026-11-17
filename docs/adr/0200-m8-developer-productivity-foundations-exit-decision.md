# ADR 0200: M8 Developer Productivity Foundations Exit Decision

- Status: Accepted
- Date: 2026-11-17
- Predecessors: [0194](0194-m8-resource-key-and-typed-text-style-resource-dictionary-foundation.md),
  [0195](0195-m8-resource-reference-metadata-and-authoring-contract-foundation.md),
  [0196](0196-m8-explicit-resource-resolution-and-scoping-foundation.md),
  [0197](0197-m8-resource-aware-ui-description-json-v2.md),
  [0198](0198-m8-resource-aware-reconstruction-and-materialization.md),
  [0199](0199-m8-headless-tooling-integration-contract.md)

## English

### Context

M8 built the backend-neutral developer-productivity foundations in 29 implementation slices. Before
closing the milestone, the repository needed one explicit review of type/property coverage,
validation, persistence, reconstruction, resources, tooling, packaging and portability. The review
also needed to decide whether the resource-system parent remained incomplete merely because no real
consumer yet required a second resource family.

The review used source and tests at `6811f6acc6f860e8fdda1158857c7153e1a1132b`, accepted ADRs,
installed-package consumers and successful cross-platform CI as its baseline. The detailed matrix is
recorded in the [English M8 exit review](../en/M8_EXIT_REVIEW.md) and
[German M8 exit review](../de/M8_EXIT_REVIEW.md).

### Decision

M8 Developer Productivity Foundations is complete. The current resource-system scope is complete
because one concrete `TextStyle` family proves the entire architectural pipeline: stable key,
explicit dictionary ownership, typed reference and property capability, caller-ordered resolution
and scoping, strict JSON v2 persistence, validation, reconstruction and one-time materialization.
Another resource family is not needed to prove those boundaries.

Richer resource families are deferred beyond M8 until a real backend or tooling consumer supplies
concrete requirements. Font, image, icon, brush, theme, binary, animation and localization resource
contracts are not inferred speculatively.

The installed public SASD UI Toolkit API is the completed M8 IDE/tooling integration point. It
supports instance-free type/property discovery, defaults, validation, references, resource
capability and owning document editing. Human-facing display/localization/editor policy remains
consumer-owned and deferred. A visual RAD designer, IDE extension, Property Inspector or object
palette remains a separate consumer or sister project, not Toolkit Core.

No M9 is defined by this decision. Future milestones require concrete product or consumer needs.

The visible manual macOS/Cocoa M3 gate remains open and external to M8. Automated AppleClang and
AppKit CI do not close it. This ADR does not prepare or authorize an RC, tag or release; v0.2.0
release readiness remains a separate decision and the changelog remains Unreleased.

### Consequences

- All required M8 roadmap parents can be marked complete without an artificial open future child.
- Additional resource families and human-facing tooling metadata require real consumers and new,
  separately reviewed contracts.
- JSON v1, JSON v2, reconstruction, runtime, backend and package semantics remain unchanged.
- The next boundary is the visible macOS/Cocoa M3 validation followed by a separate v0.2.0
  release-readiness review.

## Deutsch

### Kontext

M8 entwickelte die backendneutralen Grundlagen für Entwicklerkomfort in 29 Implementierungs-Slices.
Vor dem Abschluss des Meilensteins benötigte das Repository einen expliziten Review von Typ-/Property-
Abdeckung, Validierung, Persistenz, Rekonstruktion, Resources, Tooling, Paketierung und
Portabilität. Der Review musste außerdem entscheiden, ob der Resource-System-Parent nur deshalb
unvollständig blieb, weil noch kein realer Consumer eine zweite Resource-Familie benötigte.

Als Ausgangsstand verwendete der Review Source und Tests bei
`6811f6acc6f860e8fdda1158857c7153e1a1132b`, angenommene ADRs, installierte Package Consumer und
erfolgreiche plattformübergreifende CI. Die detaillierte Matrix steht im
[englischen M8 Exit Review](../en/M8_EXIT_REVIEW.md) und
[deutschen M8 Exit Review](../de/M8_EXIT_REVIEW.md).

### Entscheidung

M8 Grundlagen für Entwicklerkomfort ist abgeschlossen. Der heutige Resource-System-Umfang ist
vollständig, weil eine konkrete `TextStyle`-Familie die gesamte Architektur-Pipeline beweist:
stabiler Key, explizites Dictionary-Ownership, typisierte Reference und Property-Capability,
Caller-geordnete Resolution und Scopes, strikte JSON-v2-Persistenz, Validierung, Rekonstruktion und
einmalige Materialisierung. Eine weitere Resource-Familie ist zum Nachweis dieser Grenzen nicht
erforderlich.

Reichere Resource-Familien bleiben jenseits von M8 zurückgestellt, bis ein realer Backend- oder
Tooling-Consumer konkrete Anforderungen liefert. Font-, Image-, Icon-, Brush-, Theme-, Binary-,
Animation- und Localization-Resource-Verträge werden nicht spekulativ erfunden.

Die installierte öffentliche API des SASD UI Toolkit ist der abgeschlossene M8-IDE-/Tooling-
Integrationspunkt. Sie unterstützt instanzfreie Typ-/Property-Discovery, Defaults, Validierung,
References, Resource-Capability und besitzende Dokumentbearbeitung. Menschenlesbare Display-,
Localization- und Editor-Policy bleibt consumer-owned und zurückgestellt. Ein visueller RAD
Designer, eine IDE-Erweiterung, ein Property Inspector oder eine Object Palette bleibt separater
Consumer oder Schwesterprojekt und gehört nicht zum Toolkit Core.

Diese Entscheidung definiert kein M9. Weitere Meilensteine benötigen konkrete Produkt- oder
Consumer-Anforderungen.

Der sichtbare manuelle macOS-/Cocoa-M3-Gate bleibt offen und liegt außerhalb von M8. Automatisierte
AppleClang- und AppKit-CI schließen ihn nicht. Dieser ADR bereitet oder autorisiert weder RC noch Tag
noch Release; die v0.2.0-Release-Readiness bleibt eine separate Entscheidung und das Changelog bleibt
Unreleased.

### Folgen

- Alle erforderlichen M8-Roadmap-Parents können abgeschlossen werden, ohne ein künstlich offenes
  Future-Child zu behalten.
- Zusätzliche Resource-Familien und menschenlesbare Tooling-Metadaten benötigen reale Consumer und
  neue, getrennt geprüfte Verträge.
- JSON v1, JSON v2, Rekonstruktion, Runtime-, Backend- und Package-Semantik bleiben unverändert.
- Die nächste Grenze ist die sichtbare macOS-/Cocoa-M3-Validierung, gefolgt von einem separaten
  v0.2.0-Release-Readiness-Review.
