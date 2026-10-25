# ADR 0182 – M8 SelectionModel Component Lifecycle and Reference Coverage

**Status:** Accepted
**Date:** 2026-10-25

## English

### Context

[ADR 0150](0150-m5-model-view-architecture-review.md) keeps List, Table and Tree model
and selection contracts explicit. That decision remains valid: row, cell and path identities do not
form a universal selection contract. M8 UI Description is now a concrete consumer for lifecycle,
ownership and identity of the three SelectionModel associations left open by
[ADR 0181](0181-m8-built-in-component-cross-reference-edge-snapshot-foundation.md).
A view with no selection model behaves differently from one with a selection model, and multiple
views can deliberately share that same selection object. Current selected values are a separate
transient concern under [ADR 0177](0177-m8-serializable-ui-description-state-coverage-policy.md).

### Decision

`ListSelectionModel`, `TableSelectionModel` and `TreeSelectionModel` remain separate final public
types and now derive directly from `Component`. The common base supplies only ordinary non-visual
ownership/lifecycle; it introduces no generic SelectionModel, selection value or presentation API.
Existing reference/subscription, callback, mutation, normalization and focus-separation semantics
remain unchanged. Ordinary Container adoption owns them without increasing visual child count.

The immutable catalog grows from 26 to 29 types with stable IDs `sasd.ui.ListSelectionModel`,
`sasd.ui.TableSelectionModel` and `sasd.ui.TreeSelectionModel`, each with direct base
`sasd.ui.Component` and kind `component`. Existing IDs are unchanged. Exact runtime identification
uses the existing catalog contract; unknown derived Components still fail closed. Each selection
model has a valid exact-type property snapshot with no catalogued scalar properties.

Ownership capture uses the existing [ADR 0180](0180-m8-description-node-identity-ordered-ownership-snapshot.md)
ComponentDescriptionNodeId and ordered Component traversal. There is no second identity system,
external selection ID, UUID, pointer identity or name identity. Shared selection targets resolve to
one owned node; separate view edges preserve the shared relation without cloning selection objects.

The closed reference-kind set grows from five to eleven. Source nodes remain in node-ID order;
within each source the fixed declaration order is:

1. `button_command`
2. `radio_button_group`
3. `list_view_model`
4. `table_view_model`
5. `tree_view_model`
6. `list_view_selection_model`
7. `table_view_selection_model`
8. `tree_view_selection_model`
9. `list_selection_model_model`
10. `table_selection_model_model`
11. `tree_selection_model_model`

View -> Model and SelectionModel -> Model are independent public runtime bindings. Both are
captured as explicit edges: the latter determines positional identity and normalization even
without a view. Capture neither derives one binding from the other nor imposes an equality
invariant. Mismatched internal models are described exactly as the public API permits; future
design-time validation is outside this capture slice.

ADR 0181's self-contained policy applies to all six new relations. A live non-null target outside
the same ownership snapshot rejects the whole capture, including a SelectionModel's external model.
Null and expired safe getters produce no edge, sentinel or tombstone. No automatic adoption occurs.

`selectedRow`, `selectedCell`, `selectedNode`, previous selection, model revision, subscriptions,
observers, viewport and expansion remain transient/derived runtime state. Reconstruction will create
the normal empty initial selection and restore association wiring, not current user interaction.
Model contents remain Application Data. This slice adds no model payloads, collection/compound
coverage, encoding, serializer, deserializer, factory, reconstruction or reference resolver.

### Consequences and validation

Ordinary Component lifetime and description identity close the selection-association coverage gap
without weakening ADR 0150 or introducing a parallel object graph. Property snapshots intentionally
contain no fake/default selection properties. Tests cover exact catalog/runtime types, non-visual
ownership order, selected-but-empty property snapshots, all three complete wirings, sharing,
independent model bindings, deterministic edge order, external rejection, expired targets and owned
snapshot values after runtime destruction. Collection/compound coverage, encoding and reconstruction
remain open; the ownership/reference graph coverage item is complete for the current baseline.

## Deutsch

### Kontext

[ADR 0150](0150-m5-model-view-architecture-review.md) bleibt gültig: List-, Table- und
Tree-Verträge bleiben eigenständig; Zeilen-, Zellen- und Pfadidentität ergeben keinen universellen
Selection-Vertrag. M8 UI Description ist nun ein konkreter Consumer für Lifecycle, Ownership und
Identität der bisher offenen SelectionModel-Assoziationen aus
[ADR 0181](0181-m8-built-in-component-cross-reference-edge-snapshot-foundation.md).
Ein View ohne SelectionModel unterscheidet sich semantisch von einem View mit SelectionModel;
mehrere Views können absichtlich dasselbe Objekt teilen. Aktuelle Selection bleibt gemäß
[ADR 0177](0177-m8-serializable-ui-description-state-coverage-policy.md) transient.

### Entscheidung

`ListSelectionModel`, `TableSelectionModel` und `TreeSelectionModel` bleiben getrennte finale
öffentliche Typen und erben unmittelbar von `Component`. Die gemeinsame Basis liefert ausschließlich
non-visuelle Ownership und Lifecycle, keinen generischen Selection-Vertrag oder Selection-Wert und
keine Presentation-API. Reference-/Subscription-, Callback-, Mutations-, Normalisierungs- und
Focus-Trennungssemantik bleiben erhalten. Normale Container-Adoption erhöht componentCount,
aber nicht childCount.

Der unveränderliche Katalog wächst von 26 auf 29 Typen. Die stabilen IDs lauten
`sasd.ui.ListSelectionModel`, `sasd.ui.TableSelectionModel` und `sasd.ui.TreeSelectionModel`, jeweils
mit direkter Basis `sasd.ui.Component` und Kind `component`. Bestehende IDs bleiben unverändert.
Runtime-Metadaten identifizieren exakte Typen; unbekannte abgeleitete Components bleiben abgelehnt.
Property-Snapshots sind mit exakter Type-ID gültig und enthalten keine katalogisierten Skalare.

Ownership verwendet die normalen Component-Nodes und ComponentDescriptionNodeId aus
[ADR 0180](0180-m8-description-node-identity-ordered-ownership-snapshot.md), ohne zweite Identitätsschicht,
externe Selection-IDs, UUIDs, Pointer- oder Namensidentität. Sharing bleibt durch mehrere Edges auf
exakt dieselbe Node-ID erhalten, ohne SelectionModel-Kopien.

Die fünf bestehenden Reference-Kinds bleiben in ihrer Reihenfolge; danach folgen die sechs neuen
Kinds in der verbindlichen Reihenfolge der englischen Liste: zunächst List/Table/Tree
View -> SelectionModel, dann List/Table/Tree SelectionModel -> Model. Insgesamt sind es elf Kinds.
Edges folgen Source-Node-ID und darin dieser festen Deklarationsreihenfolge.

View -> Model und SelectionModel -> Model sind unabhängige öffentliche Runtime-Beziehungen und
werden separat erfasst. Die SelectionModel-Beziehung bestimmt positionsbezogene Identität und
Normalisierung auch ohne View. Capture leitet keine Beziehung aus einer anderen ab und erfindet
keine Gleichheitsinvariante. Unterschiedliche interne Models werden unverändert beschrieben;
spätere Design-Time-Validation gehört nicht in diesen Slice.

Die selbständig geschlossene Policy von ADR 0181 gilt für alle sechs neuen Relationen: Lebende
non-null Ziele außerhalb desselben Ownership-Snapshots verwerfen den gesamten Capture, auch bei
SelectionModel -> externem Model. Null und abgelaufene sichere Getter erzeugen keine Edge,
Sentinels oder Tombstones. Es gibt keine automatische Adoption.

`selectedRow`, `selectedCell`, `selectedNode`, vorherige Selection, Model-Revision, Subscriptions,
Observer, Viewport und Expansion bleiben transienter/abgeleiteter Runtime-State. Spätere
Rekonstruktion erzeugt normale leere Initial-Selection und stellt Wiring wieder her, nicht aktuelle
Benutzerinteraktion. Model-Inhalte bleiben Application Data. Keine Payloads, Collection-/Compound-
Abdeckung, kein Encoding, Serializer, Deserializer, Factory, Rekonstruktionsablauf oder Resolver.

### Folgen und Validierung

Normale Component-Lebenszeit und beschreibungslokale Identität schließen die Assoziationslücke ohne
universelle Selection-Basis oder parallelen Objektgraphen. Property-Snapshots enthalten bewusst
keine erfundenen Default-Properties. Tests prüfen Katalog und exakte Runtime-Typen, non-visuelle
Ownership-Reihenfolge, leere Property-Snapshots trotz Selection, vollständiges Wiring aller drei
Familien, Sharing, unabhängige Model-Bindings, deterministische Edge-Reihenfolge, externe und
abgelaufene Ziele sowie Snapshot-Werte nach Runtime-Zerstörung. Ownership-/Reference-Abdeckung der
aktuellen Baseline ist vollständig; Collection/Compound, Encoding und Rekonstruktion bleiben offen.
