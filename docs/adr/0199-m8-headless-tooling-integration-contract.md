# ADR 0199: M8 Headless Tooling Integration Contract

- Status: Accepted
- Date: 2026-11-16

## English

### Context

M8 already provides an immutable built-in type catalog, exact runtime identification, a property
schema, typed runtime access, effective defaults, validation/normalization/cross-property metadata,
reference and graph contracts, non-localized authoring categories, resource capability metadata,
format-neutral documents, JSON v1/v2 and controlled reconstruction. These focused contracts are
public and backend-neutral, but the roadmap's “IDE/tooling integration points” had not been proven
through the installed package as one composable headless workflow.

One concrete gap remained. `ComponentTypeMetadata` describes stable type identity, direct base and
coarse kind, but does not say whether a known catalog member is valid as a concrete authored node.
The controlled reconstruction adapter supports 26 of the current 29 types. `ListModel`, `TableModel`
and `TreeModel` are intentionally known abstract schema/reference contracts and cannot be constructed
as exact nodes. An object palette must not infer this distinction from C++ RTTI or by attempting
reconstruction.

Human-facing attributes such as display names, descriptions, icons, editor choices and Property
Grid policy still lack a real consumer-defined localization, presentation and editing contract.
Adding them merely to make a roadmap checkbox appear complete would freeze speculative policy in
Core.

### Decision

Add the public backend-neutral `component_type_authoring_metadata.hpp` contract. The enum
`ComponentTypeAuthoringInstantiationKind` has exactly `constructible` and `abstract_contract`.
`ComponentTypeAuthoringMetadata` contains that classification, and
`builtInComponentTypeAuthoringMetadata(const ComponentTypeMetadata&)` returns it for an exact
built-in descriptor.

Identity is the complete descriptor value—`type_id`, `base_type_id` and `kind`—rather than pointer
address. Valid copies therefore succeed, while changed base/kind, wrong case, unknown and fabricated
descriptors return `std::nullopt`. The private mapping explicitly lists all 29 current catalog types;
there is no default for future types. Complete-catalog tests require 26 constructible classifications
and the three exact abstract contracts named above. Reconstruction coverage cross-checks the same
boundary so metadata and controlled construction cannot drift silently.

Constructibility is authoring information only. It is not general C++ abstractness, a constructor,
factory, registration table, generic object activator, reflection engine, persistence opcode or
display metadata. Runtime construction remains inside the existing phased reconstruction policy.

External tools compose the established focused APIs. They traverse `base_type_id` and append
`declaredBuiltInComponentProperties()` base-to-derived, preserving each declaring descriptor. They
then query value kinds, authoring categories, effective defaults, validation policies, reference
contracts and resource capability through their existing APIs. No duplicate flattened property
catalog or `ToolingPropertyDescriptor` mega-structure is added.

A new metadata-only example demonstrates the composition without Components or backends. The
installed package consumer proves that public installed headers and `SASD::UI` are sufficient for
complete type/property coverage, inheritance, categories, defaults, validation, references,
resource capability, format-neutral document editing and JSON v2 encode/parse. Its discovery phase
constructs no Component; separate existing runtime smoke checks may still exercise capture and
reconstruction afterward.

The installed public API is therefore the M8 IDE/tooling integration point. IDE plugins, language
servers, Property Grids, object inspectors, toolbox/palette UIs, drag-and-drop canvases, selection
handles, undo engines and code generators are separate consumers, not Core deliverables. A complete
visual RAD designer remains the sister project established by ADR 0009.

Human-facing or presentation/editor policy remains intentionally deferred: display/localized names,
description/help/tooltip text, editor kind/widget, property ordering, browsable/read-only/advanced/
favorite flags, icons, palette groups, documentation URLs, category display text and default visual
sizes. This deferral is not an open M8 blocker. Richer resource families likewise remain consumer-
driven and are not introduced here. M8.30 is the explicit exit-review boundary that will decide the
formal milestone/resource-parent status.

### Consequences and boundaries

Headless tools can now enumerate every built-in type, filter concrete nodes, build effective
property views, obtain all current machine-readable authoring constraints, edit owning documents,
use unchanged JSON v1/v2 entry points and optionally cross the separate reconstruction boundary for
a preview. Metadata discovery has no Component construction, resource lookup, event notification,
registration or backend initialization side effect.

JSON v1 and v2 schemas, version numbers, symbols, ordering and golden bytes do not change. Runtime
Component, resource materialization, presentation, events, layout, Model/View and all backends do not
change. No factory, reflection registry, generic property carrier, new resource family, IDE plugin or
designer engine is introduced.

### Implementation provenance and historical scope

Commit `612e4232414967d2e38018ae0cd11075be246777` implemented this decision in
[PR #106](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/106). Complete-catalog tests verify
29 exact types, 26 constructible description nodes, three abstract model contracts, all 28 property
descriptors and fail-closed descriptor identity. The headless demo and installed-package consumers
exercise instance-free discovery, documents and JSON v2 without initializing a backend. All eleven
PR checks and all three workflows for merge `6811f6acc6f860e8fdda1158857c7153e1a1132b`
passed.

At the end of this implementation session, M8.29 was complete and M8.30 had not yet begun. This is
a historical session boundary, not current milestone status; the later M8 completion decision is
recorded in [ADR 0200](0200-m8-developer-productivity-foundations-exit-decision.md).

## Deutsch

### Kontext

M8 stellt bereits einen unveränderlichen Built-in-Typkatalog, exakte Runtime-Identifikation, ein
Property Schema, typisierten Runtime-Zugriff, effektive Defaults, Validation-/Normalization-/Cross-
Property-Metadaten, Reference-/Graph-Verträge, nicht lokalisierte Authoring-Kategorien, Resource-
Capability-Metadaten, formatneutrale Dokumente, JSON v1/v2 und kontrollierte Reconstruction bereit.
Diese fokussierten Verträge sind öffentlich und backendneutral; der Roadmap-Punkt „IDE-/Tooling-
Schnittstellen“ war jedoch noch nicht als zusammenhängender Headless-Workflow über das installierte
Paket nachgewiesen.

Eine konkrete Lücke blieb: `ComponentTypeMetadata` beschreibt stabile Typidentität, direkte Base und
groben Kind, aber nicht, ob ein bekannter Katalogtyp als konkreter Authoring-Node zulässig ist. Der
kontrollierte Reconstruction Adapter unterstützt 26 der aktuellen 29 Typen. `ListModel`,
`TableModel` und `TreeModel` sind bewusst bekannte abstrakte Schema-/Reference-Verträge und nicht als
exakte Nodes konstruierbar. Eine Object Palette darf diesen Unterschied weder aus C++-RTTI ableiten
noch durch versuchsweise Reconstruction ermitteln.

Menschenlesbare Attribute wie Display Names, Beschreibungen, Icons, Editor-Auswahl und Property-
Grid-Policy besitzen weiterhin keinen realen Consumer-definierten Lokalisierungs-, Darstellungs- und
Editing-Vertrag. Sie nur für einen Roadmap-Haken einzuführen, würde spekulative Policy im Core
einfrieren.

### Entscheidung

Der öffentliche backendneutrale Vertrag `component_type_authoring_metadata.hpp` wird ergänzt. Das
Enum `ComponentTypeAuthoringInstantiationKind` enthält exakt `constructible` und
`abstract_contract`. `ComponentTypeAuthoringMetadata` trägt diese Klassifikation;
`builtInComponentTypeAuthoringMetadata(const ComponentTypeMetadata&)` liefert sie für einen exakten
Built-in-Deskriptor.

Identität ist der vollständige Deskriptorwert aus `type_id`, `base_type_id` und `kind`, nicht die
Pointer-Adresse. Gültige Kopien funktionieren; veränderte Base/Kind, falsche Groß-/Kleinschreibung,
unbekannte und erfundene Deskriptoren liefern `std::nullopt`. Das private Mapping listet alle 29
aktuellen Katalogtypen explizit; es gibt keinen Default für zukünftige Typen. Complete-Catalog-Tests
fordern 26 konstruierbare Klassifikationen und die drei oben genannten exakten abstrakten Verträge.
Reconstruction-Coverage prüft dieselbe Grenze, damit Metadaten und kontrollierte Construction Policy
nicht still auseinanderlaufen.

Konstruierbarkeit ist ausschließlich Authoring-Information. Sie ist weder allgemeine C++-
Abstraktheit noch Constructor, Factory, Registration Table, generischer Object Activator,
Reflection Engine, Persistenz-Opcode oder Display-Metadatum. Runtime-Konstruktion bleibt in der
bestehenden phasenbasierten Reconstruction Policy.

Externe Tools komponieren die etablierten fokussierten APIs. Sie traversieren `base_type_id`, hängen
`declaredBuiltInComponentProperties()` Base-to-Derived an und erhalten jeden deklarierenden
Deskriptor. Anschließend fragen sie Value Kinds, Authoring-Kategorien, effektive Defaults,
Validation Policies, Reference-Verträge und Resource Capability über die bestehenden APIs ab. Es
entsteht weder ein duplizierter abgeflachter Property-Katalog noch eine
`ToolingPropertyDescriptor`-Mega-Struktur.

Ein neues reines Metadatenbeispiel zeigt die Komposition ohne Components oder Backends. Der
installierte Package Consumer beweist, dass öffentliche installierte Header und `SASD::UI` für
vollständige Typ-/Property-Coverage, Vererbung, Kategorien, Defaults, Validierung, References,
Resource Capability, formatneutrales Document Editing und JSON-v2-Encode/Parse ausreichen. Seine
Discovery-Phase konstruiert keine Component; getrennte bestehende Runtime-Smoke-Checks dürfen danach
weiterhin Capture und Reconstruction ausführen.

Die installierte öffentliche API ist damit der M8-IDE-/Tooling-Integrationspunkt. IDE-Plugins,
Language Server, Property Grids, Object Inspectors, Toolbox-/Palette-UIs, Drag-and-drop-Canvas,
Selection Handles, Undo Engines und Code Generators sind separate Consumer und keine Core-
Liefergegenstände. Ein vollständiger visueller RAD-Designer bleibt das mit ADR 0009 etablierte
Schwesterprojekt.

Menschenlesbare bzw. Presentation-/Editor-Policy bleibt bewusst aufgeschoben: Display-/lokalisierte
Namen, Description-/Help-/Tooltip-Text, Editor Kind/Widget, Property-Reihenfolge, Browsable-/Read-
only-/Advanced-/Favorite-Flags, Icons, Palette Groups, Dokumentations-URLs, Category Display Text
und visuelle Default-Größen. Diese Deferral ist kein offener M8-Blocker. Weitere Resource-Familien
bleiben ebenfalls consumer-driven und werden hier nicht eingeführt. M8.30 ist die explizite Exit-
Review-Grenze für den formalen Milestone-/Resource-Parent-Status.

### Folgen und Grenzen

Headless-Tools können nun alle Built-in-Typen aufzählen, konkrete Nodes filtern, effektive Property-
Views bilden, alle heutigen maschinenlesbaren Authoring-Constraints ermitteln, besitzende Dokumente
bearbeiten, unveränderte JSON-v1-/v2-Entry-Points nutzen und optional für eine Preview die separate
Reconstruction-Grenze überschreiten. Metadata Discovery hat keine Side Effects durch Component-
Konstruktion, Resource Lookup, Event Notification, Registrierung oder Backend-Initialisierung.

JSON-v1-/v2-Schemas, Versionsnummern, Symbole, Reihenfolge und Golden Bytes ändern sich nicht.
Runtime-Component-, Resource-Materialisierungs-, Presentation-, Event-, Layout-, Model/View- und
Backend-Verhalten ändern sich nicht. Es entstehen weder Factory, Reflection Registry, generischer
Property Carrier, neue Resource-Familie, IDE-Plugin noch Designer Engine.

### Implementierungsnachweis und historischer Umfang

Commit `612e4232414967d2e38018ae0cd11075be246777` setzte diese Entscheidung in
[PR #106](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/106) um. Complete-Catalog-Tests
prüfen 29 exakte Typen, 26 konstruierbare Description-Nodes, drei abstrakte Model-Verträge, alle
28 Property-Deskriptoren und Fail-closed-Descriptor-Identität. Headless Demo und Consumer des
installierten Pakets verwenden instanzfreie Discovery, Dokumente und JSON v2 ohne Backend-
Initialisierung. Alle elf PR-Checks und alle drei Workflows für Merge
`6811f6acc6f860e8fdda1158857c7153e1a1132b` bestanden.

Am Ende dieser Implementierungssitzung war M8.29 abgeschlossen und M8.30 noch nicht begonnen. Dies
ist eine historische Sitzungsgrenze, kein aktueller Meilensteinstatus; die spätere M8-
Abschlussentscheidung steht in [ADR 0200](0200-m8-developer-productivity-foundations-exit-decision.md).
